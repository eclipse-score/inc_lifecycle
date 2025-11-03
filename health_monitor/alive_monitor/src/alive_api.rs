// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
//
use crate::alive_api_base::AliveApiBase;

pub fn create_alive_api() -> Box<dyn AliveApiBase> {
    #[cfg(target_os = "linux")]
    {
        Box::new(crate::alive_api_linux::make_alive_api())
    }
    #[cfg(target_os = "nto")]
    {
        Box::new(crate::alive_api_qnx::make_alive_api())
    }
}
