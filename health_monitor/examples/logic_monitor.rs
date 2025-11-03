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
use health_monitor::{common::*, logic_monitor::*};

struct DebugHook {}

impl Hook for DebugHook {
    fn on_status_change(&self, from: Status, to: Status) {
        println!("on_status_change {:?} {:?}", from, to);
    }

    fn on_state_change(&self, from: State, to: State) {
        println!("on_state_change {:?} {:?}", from, to);
    }
}

fn main() {
    let mut builder = LogicMonitorBuilder::new("Init".into());
    builder
        .add_transition("Init".into(), "Running".into())
        .add_transition("Running".into(), "Paused".into())
        .add_transition("Paused".into(), "Running".into())
        .add_transition("Running".into(), "Stopped".into())
        .add_hook(Box::new(DebugHook{}));
    let monitor = builder.build().expect("Failed to build the monitor");

    let monitor1 = monitor.clone();
    let t1 = thread::spawn(move || {
        thread::sleep(Duration::from_millis(100));
        assert_eq!(monitor1.transition("Running".into()), Ok(()));
    });

    let monitor2 = monitor.clone();
    let t2 = thread::spawn(move || {
        thread::sleep(Duration::from_millis(500));
        assert_eq!(monitor2.transition("Paused".into()), Ok(()));
        thread::sleep(Duration::from_millis(100));
        assert_eq!(monitor2.transition("Running".into()), Ok(()));
    });

    let monitor3 = monitor.clone();
    let t3 = thread::spawn(move || {
        thread::sleep(Duration::from_millis(1000));
        assert_eq!(monitor3.transition("Stopped".into()), Ok(()));
    });

    let _ = t1.join();
    let _ = t2.join();
    let _ = t3.join();

    assert_eq!(monitor.status(), Status::Running);

    // Try to transition into an invalid state.
    assert_eq!(monitor.transition("Running".into()), Err(Error::NotAllowed));
    assert_eq!(monitor.status(), Status::Failed);
}
