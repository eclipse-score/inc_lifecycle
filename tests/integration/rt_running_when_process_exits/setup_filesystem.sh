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

# This script stands in for a real filesystem setup step (e.g. mounting
# partitions). Instead of mounting anything it simply writes a marker file that
# a dependent component reads later. It is launched by the Launch Manager as a
# self-terminating component whose ready condition is "Terminated": the Launch
# Manager only considers it ready once this script has exited successfully.

echo "filesystem is ready" > setup_filesystem_output.txt

exit 0
