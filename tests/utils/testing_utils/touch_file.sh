#!/bin/sh
# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
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

usage() {
    cat <<'EOF'
Usage: touch_file.sh [--sleep <seconds>] [--text <text>] <file>

Test helper emulating a self-terminating component: touches <file> and exits.
With --text it writes <text> to the file instead of just touching it, and with
--sleep it waits the given number of seconds beforehand.

Options:
  --sleep <seconds>  Sleep this many seconds before writing the file.
  --text <text>      Write <text> to the file instead of only touching it.
  --help             Show this help and exit.
EOF
}

sleep_seconds=0
text=
write_text=false
file=

while [ $# -gt 0 ]; do
    case "$1" in
        --sleep) sleep_seconds="$2"; shift 2 ;;
        --text) text="$2"; write_text=true; shift 2 ;;
        --help) usage; exit 0 ;;
        *) file="$1"; shift ;;
    esac
done

if [ -z "$file" ]; then
    usage >&2
    exit 1
fi

sleep "$sleep_seconds"

if [ "$write_text" = true ]; then
    echo "$text" > "$file"
else
    touch "$file"
fi

exit 0
