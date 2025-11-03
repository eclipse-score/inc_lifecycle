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
use std::sync::{
    Arc,
    atomic::{AtomicBool, Ordering},
};
use std::{thread, time::Duration};
use utils;

fn main() {
    let app_name = utils::read_app_name();
    println!("Starting application {app_name}...");
    let heartbeat_interval = utils::read_heartbeat_interval();
    println!("{app_name} heartbeat interval in ms: {heartbeat_interval}");

    utils::notify_ready();
    println!("{app_name} is READY");

    let is_error = Arc::new(AtomicBool::new(false));
    let is_shutdown_requested = Arc::new(AtomicBool::new(false));

    {
        let shutdown_flag = Arc::clone(&is_shutdown_requested);
        thread::spawn(move || utils::signal_handler_loop(shutdown_flag))
    };

    {
        let shutdown_flag = Arc::clone(&is_shutdown_requested);
        let error_flag = Arc::clone(&is_error);
        thread::spawn(move || utils::keep_alive_loop(heartbeat_interval, shutdown_flag, error_flag))
    };

    let mut iteration: i32 = 1;
    const MAX_ITERATIONS: i32 = 10;
    while !is_shutdown_requested.load(Ordering::Acquire) {
        iteration += 1;
        if iteration >= MAX_ITERATIONS {
            // simulate a crash
            std::process::exit(-1);
        }
        thread::sleep(Duration::from_secs(1));
    }
}
