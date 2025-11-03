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
use crate::deadline_monitor::*;
use crate::logic_monitor::*;
use crate::health_monitor::*;
use crate::common::{Status, Error};
use std::{ffi::{c_char, CStr}, os::raw::c_void, ptr::null_mut, time::Duration};
use alive_monitor::ffi::*;

//
// DeadlineMonitorBuilder
//

#[unsafe(no_mangle)]
extern fn hm_dmb_new() -> *mut DeadlineMonitorBuilder {
    Box::into_raw(Box::new(DeadlineMonitorBuilder::new()))
}

#[unsafe(no_mangle)]
extern fn hm_dmb_delete(builder_ptr: *mut *mut DeadlineMonitorBuilder) {
    let _builder = unsafe { Box::from_raw(*builder_ptr) };
    unsafe { *builder_ptr = null_mut(); }
}

type DeadlineMonitorOnStatusChanged = unsafe extern fn(data: *mut c_void, from: i32, to: i32);

struct DeadlineMonitorHooks {
    on_status_changed: DeadlineMonitorOnStatusChanged,
    on_status_changed_data: *mut c_void,
}

// Safety: Right now it's up to the FFI user to ensure the hooks are safe to call.
unsafe impl Send for DeadlineMonitorHooks {}
unsafe impl Sync for DeadlineMonitorHooks {}

impl crate::deadline_monitor::Hook for DeadlineMonitorHooks {
    fn on_status_change(&self, from: Status, to: Status) {
        unsafe { (self.on_status_changed)(self.on_status_changed_data, from as i32, to as i32) };
    }
}

#[unsafe(no_mangle)]
extern fn hm_dmb_add_hook(builder: *mut DeadlineMonitorBuilder, on_status_changed: DeadlineMonitorOnStatusChanged, on_status_changed_data: *mut c_void) {
    unsafe {
        (*builder).add_hook(Box::new(DeadlineMonitorHooks {
            on_status_changed,
            on_status_changed_data
        }));
    }
}

#[unsafe(no_mangle)]
extern fn hm_dmb_build(builder_ptr: *mut *mut DeadlineMonitorBuilder) -> *mut DeadlineMonitor {
    let builder = unsafe { Box::from_raw(*builder_ptr) };
    unsafe { *builder_ptr = null_mut(); }

    // TODO: Propagate error.
    match builder.build() {
        Ok(monitor) => Box::into_raw(Box::new(monitor)),
        Err(_err) => todo!("Report error"),
    }
}

//
// DeadlineMonitor
//

#[unsafe(no_mangle)]
extern fn hm_dm_delete(monitor: *mut *mut DeadlineMonitor) {
    let _monitor = unsafe { Box::from_raw(*monitor) };
    unsafe { *monitor = null_mut(); }
}

#[unsafe(no_mangle)]
extern fn hm_dm_new_deadline(monitor: *mut DeadlineMonitor, min_ms: u64, max_ms: u64) -> *mut Deadline {
    let result = unsafe { (*monitor).create_deadline(Duration::from_millis(min_ms), Duration::from_millis(max_ms)) };

    match result {
        Ok(deadline) => Box::into_raw(Box::new(deadline)),
        Err(_err) => todo!("Report error"),
    }
}

#[unsafe(no_mangle)]
extern fn hm_dm_enable(monitor: *mut DeadlineMonitor) {
    // TODO: Propagate error.
    let _ = unsafe { (*monitor).enable() };
}

#[unsafe(no_mangle)]
extern fn hm_dm_disable(monitor: *mut DeadlineMonitor) {
    // TODO: Propagate error.
    let _ = unsafe { (*monitor).disable() };
}

#[unsafe(no_mangle)]
extern fn hm_dm_status(monitor: *const DeadlineMonitor) -> i32 {
    unsafe { (*monitor).status() as i32 }
}

//
// Deadline
//

#[unsafe(no_mangle)]
extern fn hm_dl_delete(deadline: *mut *mut Deadline) {
    let _deadline = unsafe { Box::from_raw(*deadline) };
    unsafe { *deadline = null_mut(); }
}

#[unsafe(no_mangle)]
extern fn hm_dl_start(deadline: *mut Deadline) {
    // TODO: Propagate error.
    let _ = unsafe { (*deadline).start() };
}

#[unsafe(no_mangle)]
extern fn hm_dl_stop(deadline: *mut Deadline) {
    // TODO: Propagate error.
    let _ = unsafe { (*deadline).stop() };
}

#[unsafe(no_mangle)]
extern fn hm_dl_min_ms(deadline: *const Deadline) -> u64 {
    unsafe { (*deadline).min().as_millis() as u64 }
}

#[unsafe(no_mangle)]
extern fn hm_dl_max_ms(deadline: *const Deadline) -> u64 {
    unsafe { (*deadline).max().as_millis() as u64 }
}

//
// LogicMonitorBuilder
//

#[unsafe(no_mangle)]
extern fn hm_lm_state_from_str(name: *const c_char) -> State {
    let name = unsafe { CStr::from_ptr(name) };

    match name.to_str() {
        Ok(name) => State::from_str(name),
        Err(_err) => todo!("Report error"),
    }
}

#[unsafe(no_mangle)]
extern fn hm_lmb_new(initial_state: State) -> *mut LogicMonitorBuilder {
    Box::into_raw(Box::new(LogicMonitorBuilder::new(initial_state)))
}

#[unsafe(no_mangle)]
extern fn hm_lmb_delete(builder_ptr: *mut *mut LogicMonitorBuilder) {
    let _builder = unsafe { Box::from_raw(*builder_ptr) };
    unsafe { *builder_ptr = null_mut(); }
}

#[unsafe(no_mangle)]
extern fn hm_lmb_add_transition(builder: *mut LogicMonitorBuilder, from: State, to: State) {
    unsafe { (*builder).add_transition(from, to) };
}

type LogicMonitorOnStatusChanged = unsafe extern fn(data: *mut c_void, from: i32, to: i32);
type LogicMonitorOnStateChanged = unsafe extern fn(data: *mut c_void, from: State, to: State);

struct LogicMonitorHooks {
    on_status_changed: LogicMonitorOnStatusChanged,
    on_status_changed_data: *mut c_void,
    on_state_changed: LogicMonitorOnStateChanged,
    on_state_changed_data: *mut c_void,
}

// Safety: Right now it's up to the FFI user to ensure the hooks are safe to call.
unsafe impl Send for LogicMonitorHooks {}
unsafe impl Sync for LogicMonitorHooks {}

impl crate::logic_monitor::Hook for LogicMonitorHooks {
    fn on_status_change(&self, from: Status, to: Status) {
        unsafe { (self.on_status_changed)(self.on_status_changed_data, from as i32, to as i32) };
    }

    fn on_state_change(&self, from: State, to: State) {
        unsafe { (self.on_state_changed)(self.on_state_changed_data, from, to) };
    }
}

#[unsafe(no_mangle)]
extern fn hm_lmb_add_hook(
    builder: *mut LogicMonitorBuilder,
    on_status_changed: LogicMonitorOnStatusChanged,
    on_status_changed_data: *mut c_void,
    on_state_changed: LogicMonitorOnStateChanged,
    on_state_changed_data: *mut c_void)
{
    unsafe {
        (*builder).add_hook(Box::new(LogicMonitorHooks {
            on_status_changed,
            on_status_changed_data,
            on_state_changed,
            on_state_changed_data,
        }));
    }
}

#[unsafe(no_mangle)]
extern fn hm_lmb_build(builder_ptr: *mut *mut LogicMonitorBuilder) -> *mut LogicMonitor {
    let builder = unsafe { Box::from_raw(*builder_ptr) };
    unsafe { *builder_ptr = null_mut(); }

    // TODO: Propagate error.
    match builder.build() {
        Ok(monitor) => Box::into_raw(Box::new(monitor)),
        Err(_err) => todo!("Report error"),
    }
}

//
// LogicMonitor
//

#[unsafe(no_mangle)]
extern fn hm_lm_delete(monitor: *mut *mut LogicMonitor) {
    let _monitor = unsafe { Box::from_raw(*monitor) };
    unsafe { *monitor = null_mut(); }
}

#[unsafe(no_mangle)]
extern fn hm_lm_transition(monitor: *mut LogicMonitor, to: State) {
    // TODO: Propagate error.
    let _ = unsafe { (*monitor).transition(to) };
}

#[unsafe(no_mangle)]
extern fn hm_lm_enable(monitor: *mut LogicMonitor) {
    // TODO: Propagate error.
    let _ = unsafe { (*monitor).enable() };
}

#[unsafe(no_mangle)]
extern fn hm_lm_disable(monitor: *mut LogicMonitor) {
    // TODO: Propagate error.
    let _ = unsafe { (*monitor).disable() };
}

#[unsafe(no_mangle)]
extern fn hm_lm_status(monitor: *const LogicMonitor) -> i32 {
    unsafe { (*monitor).status() as i32 }
}

#[unsafe(no_mangle)]
extern fn hm_lm_state(monitor: *const LogicMonitor) -> State {
    unsafe { (*monitor).state() }
}

//
// HealthMonitor
//

#[unsafe(no_mangle)]
extern fn hm_new(
    deadline_monitor: *const DeadlineMonitor,
    logic_monitor: *const LogicMonitor,
    alive_monitor: *const AliveMonitorFfi,
    report_interval_ms: u64) -> *mut HealthMonitor
{
    let (deadline_monitor, logic_monitor, alive_monitor) = unsafe { (&*deadline_monitor, &*logic_monitor, &*alive_monitor) };

    Box::into_raw(Box::new(HealthMonitor::new(
        deadline_monitor,
        logic_monitor,
        alive_monitor,
        Duration::from_millis(report_interval_ms)
    )))
}

#[unsafe(no_mangle)]
extern fn hm_delete(monitor: *mut *mut HealthMonitor) {
    let _monitor = unsafe { Box::from_raw(*monitor) };
    unsafe { *monitor = null_mut(); }
}

#[unsafe(no_mangle)]
extern fn hm_status(monitor: *const HealthMonitor) -> i32 {
    unsafe { (*monitor).status() as i32 }
}
