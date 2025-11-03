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
use std::{thread, time::Duration};
use health_monitor::{common::*, deadline_monitor::*};

struct DebugHook;

impl Hook for DebugHook {
    fn on_status_change(&self, from: Status, to: Status) {
        println!("Status changed from {:?} to {:?}", from, to);
    }
}

fn main() {
    let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
    deadline_monitor_builder.add_hook(Box::new(DebugHook{}));
    let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");

    let deadline_monitor_clone_1 = deadline_monitor.clone();
    let t_1 = thread::spawn(move || {
        let mut deadline_1 = deadline_monitor_clone_1.create_deadline(Duration::from_millis(10), Duration::from_millis(1000)).unwrap();
        let mut deadline_2 = deadline_monitor_clone_1.create_deadline(Duration::from_millis(50), Duration::from_millis(250)).unwrap();

        // Run task 1.
        deadline_1.start().expect("Failed to start.");
        thread::sleep(Duration::from_millis(250));
        deadline_1.stop().expect("Failed to stop.");

        // Run task 2.
        deadline_2.start().expect("Failed to start.");
        thread::sleep(Duration::from_millis(100));
        deadline_2.stop().expect("Failed to stop.");

        // Run task 1 again.
        deadline_1.start().expect("Failed to start.");
        thread::sleep(Duration::from_millis(250));
        deadline_1.stop().expect("Failed to stop.");
    });

    let deadline_monitor_clone_2 = deadline_monitor.clone();
    let t_2 = thread::spawn(move || {
        let mut deadline = deadline_monitor_clone_2.create_deadline(Duration::from_millis(10), Duration::from_millis(1000)).unwrap();

        deadline.start().expect("Failed to start.");
        thread::sleep(Duration::from_millis(250));
        deadline.stop().expect("Failed to stop.");
    });


    let _ = t_1.join();
    let _ = t_2.join();

    assert_eq!(deadline_monitor.status(), Status::Running);

    let deadline_monitor_clone_3 = deadline_monitor.clone();
    let t_3 = thread::spawn(move || {
        let mut deadline = deadline_monitor_clone_3.create_deadline(Duration::from_millis(0), Duration::from_millis(100)).unwrap();

        // This task is too long.
        deadline.start().expect("Failed to start.");
        thread::sleep(Duration::from_millis(250));
        deadline.stop().expect_err("Stop should have failed.");
    });

    let _ = t_3.join();

    assert_eq!(deadline_monitor.status(), Status::Failed);
}
