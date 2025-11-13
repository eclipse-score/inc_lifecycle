# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
#! /bin/bash
set -e

file_exists() {
    local path=$1
    if test -f "$path"; then
        echo "$path exists."
    else
        echo "$path does not exist, you first need to build the project with bazel."
        exit 1
    fi
}


LM_BINARY="$PWD/../bazel-bin/launch_manager/launch_manager"
HM_BINARY="$PWD/../bazel-bin/health_monitor/health_monitor"
DEMO_APP_BINARY="$PWD/../bazel-bin/demo/cpp_supervised_app/cpp_supervised_app"
DEMO_APP_WO_HM_BINARY="$PWD/../bazel-bin/demo/cpp_lifecycle_app/cpp_lifecycle_app"
RUST_APP_BINARY="$PWD/../bazel-bin/demo/rust_supervised_app/rust_supervised_app"
CONTROL_APP_BINARY="$PWD/../bazel-bin/demo/control_application/control_daemon"
CONTROL_CLI_BINARY="$PWD/../bazel-bin/demo/control_application/lmcontrol"

file_exists $LM_BINARY
file_exists $HM_BINARY
file_exists $DEMO_APP_BINARY
file_exists $DEMO_APP_WO_HM_BINARY
file_exists $RUST_APP_BINARY
file_exists $CONTROL_APP_BINARY
file_exists $CONTROL_CLI_BINARY

NUMBER_OF_CPP_PROCESSES_PER_PROCESS_GROUP=25
NUMBER_OF_RUST_PROCESSES_PER_PROCESS_GROUP=25
NUMBER_OF_NON_SUPERVISED_CPP_PROCESSES_PER_PROCESS_GROUP=50
PROCESS_GROUPS="--process_groups MainPG ProcessGroup1 ProcessGroup2 ProcessGroup3"

rm -rf tmp
rm -rf config/tmp
mkdir config/tmp
python3 config/gen_health_monitor_process_cfg.py -c "$NUMBER_OF_CPP_PROCESSES_PER_PROCESS_GROUP" -r "$NUMBER_OF_RUST_PROCESSES_PER_PROCESS_GROUP"  $PROCESS_GROUPS -o config/tmp/
../bazel-bin/external/flatbuffers+/flatc --binary -o config/tmp ../health_monitor/source/config/hm_flatcfg.fbs config/tmp/health_monitor_process_cfg_*.json

python3 config/gen_health_monitor_cfg.py -c "$NUMBER_OF_CPP_PROCESSES_PER_PROCESS_GROUP" -r "$NUMBER_OF_RUST_PROCESSES_PER_PROCESS_GROUP" $PROCESS_GROUPS -o config/tmp/
../bazel-bin/external/flatbuffers+/flatc --binary -o config/tmp ../health_monitor/source/config/hm_flatcfg.fbs config/tmp/hm_demo.json

python3 config/gen_launch_manager_cfg.py -c "$NUMBER_OF_CPP_PROCESSES_PER_PROCESS_GROUP" -r "$NUMBER_OF_RUST_PROCESSES_PER_PROCESS_GROUP" -n "$NUMBER_OF_NON_SUPERVISED_CPP_PROCESSES_PER_PROCESS_GROUP" $PROCESS_GROUPS -o config/tmp/
../bazel-bin/external/flatbuffers+/flatc --binary -o config/tmp ../launch_manager/deliverables/config/lm_flatcfg.fbs config/tmp/lm_demo.json

../bazel-bin/external/flatbuffers+/flatc --binary -o config/tmp ../health_monitor/source/config/hmcore_flatcfg.fbs config/hmcore.json

mkdir -p tmp/launch_manager/etc
cp $LM_BINARY tmp/launch_manager/launch_manager
cp config/tmp/lm_demo.bin tmp/launch_manager/etc/
cp config/lm_logging.json tmp/launch_manager/etc/logging.json

mkdir -p tmp/health_monitor/etc
cp $HM_BINARY tmp/health_monitor/health_monitor
cp config/tmp/hm_demo.bin tmp/health_monitor/etc/
cp config/tmp/hmcore.bin tmp/health_monitor/etc/
cp config/hm_logging.json tmp/health_monitor/etc/logging.json 

mkdir -p tmp/supervision_demo/etc
cp $DEMO_APP_BINARY tmp/supervision_demo/
cp config/tmp/health_monitor_process_cfg_*.bin tmp/supervision_demo/etc/

mkdir -p tmp/cpp_lifecycle_app/etc
cp $DEMO_APP_WO_HM_BINARY tmp/cpp_lifecycle_app/

cp $RUST_APP_BINARY tmp/supervision_demo/
cp config/tmp/health_monitor_process_cfg_*.bin tmp/supervision_demo/etc/

mkdir -p tmp/control_app
cp $CONTROL_APP_BINARY tmp/control_app/
cp $CONTROL_CLI_BINARY tmp/control_app/

mkdir -p tmp/lib
cp $PWD/../bazel-bin/launch_manager/libxaap-processstate-client.so tmp/lib/
cp $PWD/../bazel-bin/launch_manager/liblifecycle_client.so tmp/lib/
cp $PWD/../bazel-bin/launch_manager/libcontrol_client.so tmp/lib/

docker build . -t demo

if test "$1" = "tmux"
then
    docker container stop demo_app_container > /dev/null 2>&1 || true
    docker container rm demo_app_container > /dev/null 2>&1 || true
    sleep 1
    tmux split-window -h
    tmux select-pane -t 0
    tmux send 'docker run --name demo_app_container -it demo' ENTER;
    tmux select-pane -t 1
    tmux resize-pane -R 20
    tmux send 'sleep 1 && docker exec -it demo_app_container /bin/bash' ENTER;
    tmux send '/opt/demo.sh' ENTER;
else
    docker run -it demo
fi
