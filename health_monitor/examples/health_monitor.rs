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
use health_monitor::{common::*, health_monitor::*, deadline_monitor::*, logic_monitor::*};
use alive_monitor::alive_monitor::*;

struct DeadlineMonitorDebugHook;

impl health_monitor::deadline_monitor::Hook for DeadlineMonitorDebugHook {
    fn on_status_change(&self, from: Status, to: Status) {
        println!("Status changed from {:?} to {:?}", from, to);
    }
}

struct LogicMonitorDebugHook {}

impl health_monitor::logic_monitor::Hook for LogicMonitorDebugHook {
    fn on_status_change(&self, from: Status, to: Status) {
        println!("on_status_change {:?} {:?}", from, to);
    }

    fn on_state_change(&self, from: State, to: State) {
        println!("on_state_change {:?} {:?}", from, to);
    }
}

fn main() {
    let heartbeat_interval = Duration::from_millis(500);

    let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
    deadline_monitor_builder.add_hook(Box::new(DeadlineMonitorDebugHook{}));
    let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");

    let mut logic_monitor_builder = LogicMonitorBuilder::new("Init".into());
    logic_monitor_builder
        .add_transition("Init".into(), "Running".into())
        .add_transition("Running".into(), "Paused".into())
        .add_transition("Paused".into(), "Running".into())
        .add_transition("Running".into(), "Stopped".into())
        .add_hook(Box::new(LogicMonitorDebugHook{}));
    let logic_monitor = logic_monitor_builder.build().expect("Failed to build the monitor");

    let alive_monitor = AliveMonitor::new(heartbeat_interval);

    let health_monitor = HealthMonitor::new(&deadline_monitor, &logic_monitor, &alive_monitor, heartbeat_interval / 2);

    let deadline_monitor_clone_1 = deadline_monitor.clone();
    let t1 = thread::spawn(move || {
        let mut deadline_1 = deadline_monitor_clone_1.create_deadline(Duration::from_millis(10), Duration::from_millis(1000)).unwrap();
        let mut deadline_2 = deadline_monitor_clone_1.create_deadline(Duration::from_millis(50), Duration::from_millis(250)).unwrap();

        // Run task 1.
        deadline_1.start().expect("Failed to start.");
        thread::sleep(Duration::from_millis(250));
        let _ = deadline_1.stop();

        // Run task 2.
        deadline_2.start().expect("Failed to start.");
        thread::sleep(Duration::from_millis(100));
        let _ = deadline_2.stop();

        // Run task 1 again.
        deadline_1.start().expect("Failed to start.");
        thread::sleep(Duration::from_millis(250));
        let _ = deadline_1.stop();
    });

    let deadline_monitor_clone_2 = deadline_monitor.clone();
    let t2 = thread::spawn(move || {
        let mut deadline = deadline_monitor_clone_2.create_deadline(Duration::from_millis(10), Duration::from_millis(1000)).unwrap();

        deadline.start().expect("Failed to start.");
        thread::sleep(Duration::from_millis(250));
        let _ = deadline.stop();
    });


    let _ = t1.join();
    let _ = t2.join();

    assert_eq!(deadline_monitor.status(), Status::Running);

    let deadline_monitor_clone_3 = deadline_monitor.clone();
    let t3 = thread::spawn(move || {
        let mut deadline = deadline_monitor_clone_3.create_deadline(Duration::from_millis(0), Duration::from_millis(100)).unwrap();

        // This task is too long.
        deadline.start().expect("Failed to start.");
        thread::sleep(Duration::from_millis(1000));
        // TODO: Check stop result. The HealthMonitor should've failed the DeadlineMonitor at this point, and thus stop will at the moment return Ok(()).
        let _ = deadline.stop();
    });

    let _ = t3.join();

    assert_eq!(deadline_monitor.status(), Status::Failed);
}
