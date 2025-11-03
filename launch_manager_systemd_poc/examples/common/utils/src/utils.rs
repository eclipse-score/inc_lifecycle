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
use alive_monitor::alive_api;
use signal_hook::consts::TERM_SIGNALS;
use signal_hook::consts::signal::{SIGINT, SIGTERM};
use signal_hook::iterator::Signals;
use std::sync::{
    Arc,
    atomic::{AtomicBool, Ordering},
};
use std::{env, thread, time::Duration};

pub fn keep_alive_loop(
    sleep_interval_ms: u64,
    is_shutdown_requested: Arc<AtomicBool>,
    is_error: Arc<AtomicBool>,
) {
    let alive_api = alive_api::create_alive_api();
    alive_api.configure_minimum_time(sleep_interval_ms);
    println!("Starting keep alive thread...");
    while !is_shutdown_requested.load(Ordering::Acquire) {
        if !is_error.load(Ordering::Acquire) {
            alive_api.keep_alive();
            thread::sleep(Duration::from_millis(sleep_interval_ms));
        } else {
            println!("An error was encountered, not sending heartbeat...");
            thread::sleep(Duration::from_millis(sleep_interval_ms));
        }
    }
    println!("Alive service shutting down...");
}

pub fn signal_handler_loop(is_shutdown_requested: Arc<AtomicBool>) {
    println!("Starting signal handler thread...");
    let mut signals = Signals::new(TERM_SIGNALS).expect("Unable to register signal handler");
    let signals_handle = signals.handle();
    while !is_shutdown_requested.load(Ordering::Acquire) {
        match signals.pending().next() {
            Some(sig) => match sig {
                SIGTERM | SIGINT => {
                    println!("Received termination signal!");
                    is_shutdown_requested.store(true, Ordering::Release);
                    break;
                }
                _ => println!("Unhandled signal: {:?}", sig),
            },
            None => {
                thread::sleep(Duration::from_millis(50));
            }
        }
    }
    println!("Signal handler shutting down...");
    signals_handle.close();
}

pub fn read_app_name() -> String {
    return std::env::var("APP_NAME").unwrap_or_else(|_| "default".to_string());
}

pub fn read_heartbeat_interval() -> u64 {
    return env::var("APP_HEARTBEAT_INTERVAL")
        .expect("{app_name} Heartbeat interval is not set")
        .parse()
        .expect("{app_name} Heartbeat interval is not a valid unsigned number of seconds");
}

pub fn notify_ready() {
    sd_notify::notify(false, &[sd_notify::NotifyState::Ready])
        .unwrap_or_else(|e| eprintln!("Failed to send ready notification: {e}"));
}

pub fn run_app() {
    let app_name = read_app_name();
    println!("Starting application {app_name}...");

    let heartbeat_interval = read_heartbeat_interval();
    println!("{app_name} heartbeat interval in ms: {heartbeat_interval}");

    notify_ready();
    println!("{app_name} is READY");

    let is_error = Arc::new(AtomicBool::new(false));
    let is_shutdown_requested = Arc::new(AtomicBool::new(false));

    let signal_thread = {
        let shutdown_flag = Arc::clone(&is_shutdown_requested);
        thread::spawn(move || signal_handler_loop(shutdown_flag))
    };

    let keep_alive_thread = {
        let shutdown_flag = Arc::clone(&is_shutdown_requested);
        let error_flag = Arc::clone(&is_error);
        thread::spawn(move || keep_alive_loop(heartbeat_interval, shutdown_flag, error_flag))
    };

    while !is_shutdown_requested.load(Ordering::Acquire) {
        thread::sleep(Duration::from_millis(50));
    }
    println!("{app_name} Shutdown complete.");
    signal_thread.join().unwrap();
    keep_alive_thread.join().unwrap();
}
