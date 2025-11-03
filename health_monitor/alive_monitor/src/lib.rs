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
pub mod alive_api;
pub mod alive_api_base;
#[cfg(target_os = "linux")]
pub mod alive_api_linux;
#[cfg(target_os = "nto")]
pub mod alive_api_qnx;
pub mod alive_monitor;
pub mod ffi;
