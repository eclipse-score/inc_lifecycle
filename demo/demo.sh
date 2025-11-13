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
#!/bin/bash
COLOR='\033[0;32m' # Green
NC='\033[0m' # No Color

read -p "$(echo -e ${COLOR}Next: Show running processes${NC})"
echo "$> ps -a | head -n 40"
ps -a | head -n 40
echo "$> ps -a | wc -l"
ps -a | wc -l

read -p "$(echo -e  ${COLOR}Next: Turning on ProcessGroup1/Startup${NC})"
echo "$> lmcontrol ProcessGroup1/Startup"
lmcontrol ProcessGroup1/Startup

read -p "$(echo -e  ${COLOR}Next: Show running processes${NC})"
echo "$> ps -a | wc -l"
ps -a | wc -l

read -p "$(echo -e  ${COLOR}Next: Show CPU utilization${NC})"
echo "$> top"
top

read -p "$(echo -e  ${COLOR}Next: Turning off ProcessGroup1/Startup${NC})"
echo "$> lmcontrol ProcessGroup1/Off"
lmcontrol ProcessGroup1/Off

read -p "$(echo -e  ${COLOR}Next: Show running processes${NC})"
echo "$> ps -a | wc -l"
ps -a | wc -l

read -p "$(echo -e  ${COLOR}Next: Killing an application process${NC})"
echo "$> pkill -9 MainPG_lc0"
pkill -9 MainPG_lc0
read -p "$(echo -e  ${COLOR}Next: Show running processes${NC})"
echo "$> ps -a"
ps -a
echo "$> ps -a | wc -l"
ps -a | wc -l

read -p "$(echo -e  ${COLOR}Next: Moving back to MainPG/Startup${NC})"
echo "$> lmcontrol MainPG/Startup"
lmcontrol MainPG/Startup

read -p "$(echo -e  ${COLOR}Next: Trigger supervision failure${NC})"
echo "$> fail $(pgrep MainPG_app0)"
kill -s SIGUSR1 $(pgrep MainPG_app0)

read -p "$(echo -e  ${COLOR}Next: Show running processes${NC})"
echo "$> ps -a"
ps -a
echo "$> ps -a | wc -l"
ps -a | wc -l