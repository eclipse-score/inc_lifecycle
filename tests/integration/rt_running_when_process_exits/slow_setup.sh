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

# This script stands in for a slow, self-terminating filesystem setup step whose
# component has ready condition "Terminated" and NO dependent component. It waits
# for a moment before writing its marker file so that a run target depending
# directly on it can be observed reporting success *before* the process has
# actually terminated (i.e. before the marker file exists) when the deferral of
# graph accounting until termination is not implemented.

sleep 1

echo "slow setup done" > slow_setup_output.txt

exit 0
