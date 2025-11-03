#!/usr/bin/env bash
set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SYSTEMD_DIR="/etc/systemd/system"
EXAMPLES_DIR="${ROOT_DIR}/examples"

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "Usage: $0 <app_crash|report_error> [rust|ffi]"
    exit 1
fi

USE_CASE="$1"
MODE="${2:-rust}"

if [[ "$USE_CASE" != "app_crash" && "$USE_CASE" != "report_error" ]]; then
    echo "Error: Use-case must be 'app_crash' or 'report_error'"
    exit 1
fi

if [[ "$MODE" != "rust" && "$MODE" != "ffi" ]]; then
    echo "Error: Mode must be 'rust' or 'ffi'"
    exit 1
fi

BUILD_TYPE="${BUILD_TYPE:-debug}"
if [[ "$BUILD_TYPE" == "release" ]]; then
    BUILD_SUBDIR="release"
    BUILD_FLAG="--release"
else
    BUILD_SUBDIR="debug"
    BUILD_FLAG=""
fi

TARGET_DIR="${ROOT_DIR}/../target/${BUILD_SUBDIR}"

echo "Building workspace (${BUILD_SUBDIR})..."
(cd "$ROOT_DIR/.." && cargo build $BUILD_FLAG)

declare -A APPS
APPS["appA"]="${EXAMPLES_DIR}/common/appA"
APPS["appB"]="${EXAMPLES_DIR}/common/appB"
APPS["appC"]="${EXAMPLES_DIR}/${USE_CASE}"

declare ASIL_QM="scoreuser:scoregroup_asil_b"
declare ASIL_B="scoreuser:scoregroup_asil_b"

declare -A APP_OWNERS
APP_OWNERS["appA"]="$ASIL_QM"
APP_OWNERS["appB"]="$ASIL_B"
APP_OWNERS["appC"]="$ASIL_B"

check_add_user_group() {
    local user="$1"
    local group="$2"

    if [[ -z "$user" || -z "$group" ]]; then
        echo "Usage: check_add_user_group <user> <group>"
        return 1
    fi

    if ! id "$user" &>/dev/null; then
        echo "User '$user' does not exist. Creating..."
        sudo useradd -m -s /bin/bash "$user" || {
            echo "Failed to create user '$user'"
            return 1
        }
    fi

    if ! getent group "$group" &>/dev/null; then
        echo "Group '$group' does not exist. Creating..."
        sudo groupadd "$group" || {
            echo "Failed to create group '$group'"
            return 1
        }
    fi

    if ! id -nG "$user" | grep -qw "$group"; then
        echo "Adding user '$user' to group '$group'..."
        sudo usermod -aG "$group" "$user" || {
            echo "Failed to add user '$user' to group '$group'"
            return 1
        }
    else
        echo "User '$user' already in group '$group'"
    fi
}

build_cpp_app() 
{
    local name="$1"
    local src_path
    local out_bin
    if [[ "$name" == "appC" ]]; then
        src_path="${EXAMPLES_DIR}/${USE_CASE}/src/cpp/main.cpp"
        out_bin="${TARGET_DIR}/${USE_CASE}"
    else
        src_path="${APPS[$name]}/src/cpp/main.cpp"
        out_bin="${TARGET_DIR}/${name}"
    fi

    alive_monitor_path=$(realpath ${ROOT_DIR}/../health_monitor/alive_monitor/src/cpp/alive_monitor.cpp)

    echo "Compiling ${name}..."
    g++ -std=c++23 \
        "$src_path" \
        "$alive_monitor_path" \
        -I"${EXAMPLES_DIR}/common/utils/include/" \
        -I"${ROOT_DIR}/../health_monitor/alive_monitor/include" \
        -I"${ROOT_DIR}/../health_monitor/src" \
        "${TARGET_DIR}/libutils.a" \
        "${TARGET_DIR}/libalive_monitor.a" \
        "${TARGET_DIR}/libhealth_monitor.a" \
        -lpthread -ldl -lrt \
        -o "$out_bin"
}

deploy_app() {
    local name="$1"
    local path="$2"
    local dest_dir="/opt/${name}/bin"

    local bin_src
    local bin_name
    if [[ "$name" == "appC" ]]; then
        bin_src="${TARGET_DIR}/${USE_CASE}"
        bin_dest="${dest_dir}/appC"
    else
        bin_src="${TARGET_DIR}/$(basename "$path")"
        bin_dest="${dest_dir}/$(basename "$path")"
    fi

    if [[ ! -f "$bin_src" ]]; then
        echo "Error: binary not found: $bin_src"
        exit 1
    fi

    # Create bin directory
    sudo mkdir -p "$dest_dir"

    # Chown the directory
    IFS=':' read -r user group <<< "${APP_OWNERS[$name]}"

    check_add_user_group "$user" "$group"

    sudo chown "$user:$group" "$dest_dir"

    # Copy binary
    echo "Installing binary $bin_src -> $bin_dest"
    sudo cp -f "$bin_src" "$bin_dest"
    sudo chmod 755 "$bin_dest"
    sudo chown "$user:$group" "$bin_dest"

    # Deploy systemd service file and drop-ins
    if [[ -d "$path/etc" ]]; then
        echo "Installing systemd service files from $path/etc"
        sudo cp -r "$path/etc/"* "$SYSTEMD_DIR/"
    fi
}

if [[ "$MODE" == "ffi" ]]; then
    for app in appC appB appA; do
        build_cpp_app "$app"
    done
fi

# --- Deploy binaries ---
for app in appC appB appA; do
    deploy_app "$app" "${APPS[$app]}"
done

echo "Reloading systemctl daemon..."
sudo systemctl daemon-reload

for app in appC appB appA; do
    echo "Starting service: $app"
    sudo systemctl start "$app.service"
done

echo "Waiting 20 seconds..."
sleep 20

FIRST_SERVICE="appC.service"
echo "Stopping last service: $FIRST_SERVICE"
sudo systemctl stop "$FIRST_SERVICE"

echo "Deployment complete."
