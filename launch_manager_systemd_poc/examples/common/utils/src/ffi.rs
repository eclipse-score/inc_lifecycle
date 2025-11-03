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
use std::ffi::CString;
use std::os::raw::c_char;
use std::slice;
use std::sync::{Arc, atomic::AtomicBool};

#[unsafe(no_mangle)]
pub extern "C" fn read_heartbeat_interval_c() -> u64 {
    crate::utils::read_heartbeat_interval()
}

#[unsafe(no_mangle)]
pub extern "C" fn read_app_name_c(buf: *mut c_char, buf_len: u16) {
    if buf.is_null() || buf_len == 0 {
        return;
    }

    let name = crate::utils::read_app_name();
    let cstr = CString::new(name).unwrap();
    let bytes = cstr.as_bytes_with_nul();
    let copy_len = std::cmp::min(bytes.len(), buf_len as usize);

    unsafe {
        let dest_slice = slice::from_raw_parts_mut(buf as *mut u8, copy_len);
        dest_slice[..copy_len].copy_from_slice(&bytes[..copy_len]);
        if copy_len < buf_len as usize {
            dest_slice[copy_len - 1] = 0;
        }
    }
}

pub struct SignalHandleData {
    is_shutdown_requested: Arc<AtomicBool>,
}

#[unsafe(no_mangle)]
pub extern "C" fn signal_handle_data_create() -> *mut SignalHandleData {
    let data = SignalHandleData {
        is_shutdown_requested: Arc::new(AtomicBool::new(false)),
    };
    Box::into_raw(Box::new(data))
}

#[unsafe(no_mangle)]
pub extern "C" fn signal_handle_data_free(ptr: *mut SignalHandleData) {
    if ptr.is_null() {
        return;
    }
    unsafe {
        drop(Box::from_raw(ptr));
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn signal_handle_data_set_shutdown(ptr: *mut SignalHandleData, value: bool) {
    if ptr.is_null() {
        return;
    }
    let data = unsafe { &*ptr };
    data.is_shutdown_requested
        .store(value, std::sync::atomic::Ordering::Release);
}

#[unsafe(no_mangle)]
pub extern "C" fn signal_handle_data_is_shutdown_requested(ptr: *mut SignalHandleData) -> bool {
    if ptr.is_null() {
        return false;
    }
    let data = unsafe { &*ptr };
    return data
        .is_shutdown_requested
        .load(std::sync::atomic::Ordering::Acquire);
}

#[unsafe(no_mangle)]
pub extern "C" fn signal_handler_loop_c(ptr: *mut SignalHandleData) {
    if ptr.is_null() {
        return;
    }
    let data = unsafe { &*ptr };
    crate::utils::signal_handler_loop(data.is_shutdown_requested.clone());
}

#[unsafe(no_mangle)]
pub extern "C" fn notify_ready_c() {
    crate::utils::notify_ready();
}

#[unsafe(no_mangle)]
pub extern "C" fn run_app_c() {
    crate::utils::run_app();
}
