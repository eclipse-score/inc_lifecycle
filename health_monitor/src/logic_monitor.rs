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
use std::{collections::{HashMap, HashSet}, hash::{DefaultHasher, Hash, Hasher}, sync::{Arc, Mutex, MutexGuard}};
use crate::common::{Status, Error};

pub trait Hook: Send + Sync {
    fn on_status_change(&self, from: Status, to: Status);
    fn on_state_change(&self, from: State, to: State);
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
#[repr(C)]
pub struct State {
    pub(crate) hash: u64
}

impl State {
    pub fn from_str(name: &str) -> Self {
        let mut hasher = DefaultHasher::new();
        name.hash(&mut hasher);

        Self {
            hash: hasher.finish()
        }
    }
}

impl From<&str> for State {
    fn from(value: &str) -> Self {
        Self::from_str(value)
    }
}

impl Hash for State {
    fn hash<H: Hasher>(&self, state: &mut H) {
        state.write_u64(self.hash);
    }
}

pub struct LogicMonitorBuilder {
    // TODO: If this is to use iceoryx, then probably easiest to use Vec and search linearly. How many states can a program have? Can't be that many.
    //       Write a Set wrapper around Vec.
    graph: HashMap<State, HashSet<State>>,
    hooks: Vec<Box<dyn Hook>>,
    initial_state: State,
}

impl LogicMonitorBuilder {
    pub fn new(initial_state: State) -> Self {
        Self {
            graph: HashMap::new(),
            hooks: Vec::new(),
            initial_state
        }
    }

    pub fn add_transition(&mut self, from: State, to: State) -> &mut Self {
        // TODO: Can from and to be the same? Can you loop "State" into "State"?

        if let Some(allowed_to_transitions) = self.graph.get_mut(&from) {
            allowed_to_transitions.insert(to);
        } else {
            let mut set = HashSet::new();
            set.insert(to);
            self.graph.insert(from, set);
        }

        self
    }

    pub fn add_hook(&mut self, hook: Box<dyn Hook>) -> &mut Self {
        self.hooks.push(hook);

        self
    }

    pub fn build(self) -> Result<LogicMonitor, Error> {
        let mut all_to_states = HashSet::<State>::new();
        for (_, to_states) in &self.graph {
            all_to_states.extend(to_states);
        }

        // Verify that the graph is connected, meaning all states except for the initial state are reachable.
        // TODO: Is looping a state possible? Can "State" transition into "State"? If so, then this check isn't good enough.
        for from_state in self.graph.keys() {
            if from_state != &self.initial_state && !all_to_states.contains(from_state) {
                return Err(Error::BadParameter);
            }
        }

        Ok(LogicMonitor {
            inner: Arc::new(Inner {
                graph: self.graph,
                hooks: self.hooks,
                status: Mutex::new(Status::Running),
                state: Mutex::new(self.initial_state),
            }),
        })
    }
}

#[derive(Clone)]
pub struct LogicMonitor {
    inner: Arc<Inner>,
}

impl LogicMonitor {
    pub fn transition(&self, to: State) -> Result<(), Error> {
        // Locking the status here to prevent enabling and disabling while transitioning.
        let mut status = self.inner.status.lock().unwrap();

        if *status == Status::Failed {
            return Err(Error::Generic);
        }

        if *status == Status::Disabled {
            return Err(Error::NotAllowed);
        }

        {
            let mut state = self.inner.state.lock().unwrap();

            if let Some(allowed_to_states) = self.inner.graph.get(&*state) {
                if allowed_to_states.contains(&to) {
                    self.inner.update_locked_state_and_notify(&mut state, to);
                    return Ok(());
                }
            }
        }

        // This code is reached if state change failed.
        self.inner.update_locked_status_and_notify(&mut status, Status::Failed);

        Err(Error::NotAllowed)
    }

    pub fn enable(&self) -> Result<(), Error> {
        let mut status = self.inner.status.lock().unwrap();
        if *status == Status::Disabled {
            self.inner.update_locked_status_and_notify(&mut status, Status::Running);
            Ok(())
        } else {
            Err(Error::NotAllowed)
        }
    }

    pub fn disable(&self) -> Result<(), Error> {
        let mut status = self.inner.status.lock().unwrap();
        if *status == Status::Running {
            self.inner.update_locked_status_and_notify(&mut status, Status::Disabled);
            Ok(())
        } else {
            Err(Error::NotAllowed)
        }
    }

    pub fn status(&self) -> Status {
        let status = self.inner.status.lock().unwrap();
        *status
    }

    pub fn state(&self) -> State {
        let state = self.inner.state.lock().unwrap();
        *state
    }
}

struct Inner {
    graph: HashMap<State, HashSet<State>>, // Safety: This is, and should remain, read-only.
    hooks: Vec<Box<dyn Hook>>, // Safety: This is, and should remain, read-only.
    status: Mutex<Status>,
    state: Mutex<State>,
}

// Safety: graph is read-only, hooks is read-only and each hook is Send + Sync, status and state are protected by locks.
unsafe impl Send for Inner {}
unsafe impl Sync for Inner {}

impl Inner {
    fn update_locked_status_and_notify(&self, locked_status: &mut MutexGuard<'_, Status>, to: Status) {
        let from = **locked_status;
        **locked_status = to;
        // This is done under a lock to guarantee the order of reported status changes.
        // TODO: This calls into user code and is thus potentially heavy while holding a lock.
        for hook in &self.hooks {
            hook.on_status_change(from, to);
        }
    }

    fn update_locked_state_and_notify(&self, locked_state: &mut MutexGuard<'_, State>, to: State) {
        let from = **locked_state;
        **locked_state = to;
        // This is done under a lock to guarantee the order of reported state changes.
        // TODO: This calls into user code and is thus potentially heavy while holding a lock.
        for hook in &self.hooks {
            hook.on_state_change(from, to);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    struct DebugHook {}

    impl Hook for DebugHook {
        fn on_status_change(&self, from: Status, to: Status) {
            println!("on_status_change {:?} {:?}", from, to);
        }

        fn on_state_change(&self, from: State, to: State) {
            println!("on_state_change {:?} {:?}", from, to);
        }
    }

    #[test]
    fn valid_graph() {
        let mut builder = LogicMonitorBuilder::new("Init".into());
        builder
            .add_transition("Init".into(), "Running".into())
            .add_transition("Running".into(), "Stopped".into());
        let monitor = builder.build().expect("Failed to build the monitor");

        assert_eq!(monitor.state(), "Init".into());
        assert_eq!(monitor.status(), Status::Running);
        assert_eq!(monitor.transition("Running".into()), Ok(()));
        assert_eq!(monitor.status(), Status::Running);
        assert_eq!(monitor.transition("Stopped".into()), Ok(()));
        assert_eq!(monitor.status(), Status::Running);
    }

    #[test]
    fn invalid_graph() {
        let mut builder = LogicMonitorBuilder::new("Init".into());
        builder
            .add_transition("Init".into(), "Running".into())
            .add_transition("Running".into(), "Stopped".into())
            .add_transition("Paused".into(), "Running".into())
            .add_transition("Running".into(), "Stopped".into());

        // Can't use assert_eq! because PartialEq is not implemented for LogicMonitor.
        assert!(builder.build().is_err_and(|e| e == Error::BadParameter));
    }

    #[test]
    fn failure_on_unsuppported_transition() {
        let mut builder = LogicMonitorBuilder::new("Init".into());
        builder
            .add_transition("Init".into(), "Running".into())
            .add_transition("Running".into(), "Paused".into())
            .add_transition("Paused".into(), "Running".into())
            .add_transition("Running".into(), "Stopped".into())
            .add_hook(Box::new(DebugHook{}));
        let monitor = builder.build().expect("Failed to build the monitor");

        assert_eq!(monitor.state(), "Init".into());
        assert_eq!(monitor.status(), Status::Running);

        let _ = monitor.disable();
        let _ = monitor.enable();

        assert_eq!(monitor.transition("Running".into()), Ok(()));
        assert_eq!(monitor.state(), "Running".into());
        assert_eq!(monitor.status(), Status::Running);

        assert_eq!(monitor.transition("Paused".into()), Ok(()));
        assert_eq!(monitor.state(), "Paused".into());
        assert_eq!(monitor.status(), Status::Running);

        // Can't transition from Paused to Stopped.
        assert_eq!(monitor.transition("Stopped".into()), Err(Error::NotAllowed));
        assert_eq!(monitor.state(), "Paused".into());
        assert_eq!(monitor.status(), Status::Failed);
    }
}
