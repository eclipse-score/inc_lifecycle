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
#[cfg(target_os = "nto")]
mod qnx_impl {
    use crate::alive_api_base::AliveApiBase;
    pub struct AliveApiQnx;

    impl AliveApiBase for AliveApiQnx {
        fn configure_minimum_time(&self, minimum_time_ms: u64) {
            println!("QNX: configuring watchdog with {} ms", minimum_time_ms);
        }

        fn keep_alive(&self) {
            println!("QNX: health OK");
        }
    }

    pub fn make_alive_api() -> AliveApiQnx {
        AliveApiQnx
    }
}

#[cfg(target_os = "nto")]
pub use self::qnx_impl::*;
