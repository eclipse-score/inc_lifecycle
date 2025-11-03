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
use crate::alive_monitor::AliveMonitor;
use std::time::Duration;

pub type AliveMonitorFfi = AliveMonitor;

#[unsafe(no_mangle)]
pub extern "C" fn create_alive_monitor(heartbeat_interval: u64) -> *mut AliveMonitorFfi {
    let handle = Box::new(AliveMonitorFfi::new(Duration::from_millis(
        heartbeat_interval,
    )));
    Box::into_raw(handle)
}

#[unsafe(no_mangle)]
pub extern "C" fn alive_monitor_free(handle: *mut AliveMonitorFfi) {
    if handle.is_null() {
        return;
    }
    unsafe { drop(Box::from_raw(handle)) };
}

#[unsafe(no_mangle)]
pub extern "C" fn configure_minimum_time(handle: *mut AliveMonitorFfi, minimum_time_ms: u64) {
    if handle.is_null() {
        return;
    }
    let alive_monitor = unsafe { &mut *handle };
    alive_monitor.configure_minimum_time(minimum_time_ms);
}

#[unsafe(no_mangle)]
pub extern "C" fn keep_alive(handle: *mut AliveMonitorFfi) {
    if handle.is_null() {
        return;
    }
    let alive_monitor = unsafe { &mut *handle };
    alive_monitor.keep_alive();
}
