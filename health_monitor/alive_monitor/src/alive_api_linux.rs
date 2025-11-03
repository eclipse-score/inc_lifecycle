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
#[cfg(target_os = "linux")]
mod linux_impl {
    use crate::alive_api_base::AliveApiBase;
    use sd_notify;
    pub struct AliveApiLinux;

    impl AliveApiBase for AliveApiLinux {
        fn configure_minimum_time(&self, minimum_time_ms: u64) {
            println!("Linux: configuring watchdog with {} ms", minimum_time_ms);
        }

        fn keep_alive(&self) {
            match sd_notify::notify(false, &[sd_notify::NotifyState::Watchdog]) {
                Ok(_) => println!("WATCHDOG ping OK"),
                Err(error) => eprintln!("WATCHDOG ping failed: {:?}", error),
            }
        }
    }

    pub fn make_alive_api() -> AliveApiLinux {
        AliveApiLinux
    }
}

#[cfg(target_os = "linux")]
pub use self::linux_impl::*;
