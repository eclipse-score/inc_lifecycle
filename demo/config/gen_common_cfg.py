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
def get_process_index_range(process_count: int, process_group_index: int):
    # Every ProcessGroup gets the same number of processes
    # The Process Index is a globally unique increasing number
    return range(
        process_group_index * process_count,
        (process_group_index * process_count) + process_count,
    )
