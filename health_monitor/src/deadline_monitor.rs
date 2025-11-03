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
use iceoryx2_bb_lock_free::mpmc::container::*;
use std::{sync::{Arc, Mutex, MutexGuard}, time::{Duration, Instant}};
use crate::common::{Status, Error};

pub trait Hook: Send + Sync {
    fn on_status_change(&self, from: Status, to: Status);
}

pub struct DeadlineMonitorBuilder {
    hooks: Vec<Box<dyn Hook>>,
}

impl DeadlineMonitorBuilder {
    pub fn new() -> Self {
        DeadlineMonitorBuilder {
            hooks: Vec::new()
        }
    }

    // TODO: This API is annoying to use because of the &mut self, but for now it's necessary to implement the FFI API.

    // TODO: In the future, may want to register deadlines here too.

    pub fn add_hook(&mut self, hook: Box<dyn Hook>) -> &mut Self {
        self.hooks.push(hook);

        self
    }

    pub fn build(self) -> Result<DeadlineMonitor, Error> {
        Ok(DeadlineMonitor {
            inner: Arc::new(Inner {
                container: FixedSizeContainer::new(),
                status: Mutex::new(Status::Running),
                hooks: self.hooks,
            })
        })
    }
}

#[derive(Clone)]
pub struct DeadlineMonitor {
    inner: Arc<Inner>
}

impl DeadlineMonitor {
    pub fn create_deadline(&self, min: Duration, max: Duration) -> Result<Deadline, Error> {
        // Not checking if status is already Failed, because it locks a mutex and is thus heavy at runtime.

        // TODO: Should there be a smallest possible step?
        if max < min {
            return Err(Error::BadParameter);
        }

        Ok(Deadline {
            inner: Arc::clone(&self.inner),
            min,
            max,
            handle: None,
            // TODO: I think this is heavy? It's a system call. Anyway shouldn't be necessary if the collection returned the removed value.
            earliest: Instant::now(),
            latest: Instant::now(),
        })
    }

    pub fn create_deadline_guard(&self, min: Duration, max: Duration) -> Result<DeadlineGuard, Error> {
        let mut deadline = self.create_deadline(min, max)?;
        deadline.start()?;

        Ok(DeadlineGuard(deadline))
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

    pub(crate) fn step(&self) -> Status {
        // TODO: Right now checking deadlines no matter the status, and only failing if running.
        //       This does run the check even if disabled or already failed unnecessarily, so maybe could check the status first.

        let now = Instant::now();
        let mut failed = false;

        // Check for expired deadlines.
        self.inner.container.get_state().for_each(|_, deadline: &ActiveDeadline| {
            if now > deadline.latest {
                println!("Deadline failed: earliest={:?}, latest={:?}, now={:?}, since_earliest={:?}", deadline.earliest, deadline.latest, now, now - deadline.earliest);

                failed = true;

                CallbackProgression::Stop
            } else {
                CallbackProgression::Continue
            }
        });

        let mut status = self.inner.status.lock().unwrap();

        if failed && *status == Status::Running {
            self.inner.update_locked_status_and_notify(&mut status, Status::Failed);
        }

        *status
    }
}

// TODO: This should be at least Send? Is it by default?
pub struct Deadline {
    inner: Arc<Inner>,
    min: Duration,
    max: Duration,

    handle: Option<ContainerHandle>, // TODO: Option shouldn't be necessary, but there's no public API to create a default ContainerHandle.
    
    // TODO: Would've been nicer if remove returned ActiveDeadline, then wouldn't have to keep this stored.
    earliest: Instant,
    latest: Instant,
}

impl Deadline {
    pub fn start(&mut self) -> Result<(), Error> {
        // Not checking if status is already Failed, because it locks a mutex and is thus heavy at runtime.

        if self.handle.is_none() {
            let now = Instant::now();

            self.earliest = now + self.min; 
            self.latest = now + self.max;

            let res = unsafe { self.inner.container.add(ActiveDeadline {
                earliest: self.earliest,
                latest: self.latest,
            }) };

            match res {
                Ok(handle) => {
                    self.handle = Some(handle);
                    Ok(())
                },
                Err(_) => Err(Error::OutOfMemory)
            }
        } else {
            Err(Error::NotAllowed)
        }
    }

    pub fn stop(&mut self) -> Result<(), Error> {
        // Not checking if status is already Failed, because it locks a mutex and is thus heavy at runtime.

        if let Some(handle) = self.handle.take() {
            unsafe { self.inner.container.remove(handle, ReleaseMode::Default) };

            let now = Instant::now();
            if now < self.earliest || now > self.latest {
                let mut status = self.inner.status.lock().unwrap();
                if *status == Status::Running {
                    self.inner.update_locked_status_and_notify(&mut status, Status::Failed);
                    // TODO: Better error name.
                    return Err(Error::Generic);
                }

                // TODO: Report AlreadyFailed or something if *status == Status::Failed?
            }

            Ok(())
        } else {
            Err(Error::NotAllowed)
        }
    }

    pub fn min(&self) -> Duration {
        self.min
    }

    pub fn max(&self) -> Duration {
        self.max
    }
}

impl Drop for Deadline {
    fn drop(&mut self) {
        debug_assert!(self.handle.is_none(), "Deadline dropped while started.");
    }
}

pub struct DeadlineGuard(Deadline);

impl Drop for DeadlineGuard {
    fn drop(&mut self) {
        // TODO: Error handling? panic?
        let _ = self.0.stop();
    }
}

struct Inner {
    container: FixedSizeContainer<ActiveDeadline, 256_usize>,
    status: Mutex<Status>,
    hooks: Vec<Box<dyn Hook>>, // Safety: This is, and should remain, read-only.
}

// Safety: All fields except for hooks are Send + Sync. hooks is read-only, and Hook is Send + Sync.
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
}

#[derive(Clone, Copy, Debug)]
struct ActiveDeadline {
    earliest: Instant,
    latest: Instant,
}

#[cfg(test)]
mod tests {
    use std::thread;
    use super::*;

    struct LoggerCondition;

    impl LoggerCondition {
        fn new() -> Self {
            Self {}
        }
    }

    impl Hook for LoggerCondition {
        fn on_status_change(&self, from: Status, to: Status) {
            println!("Status changed from {:?} to {:?}", from, to);
        }
    }
    
    #[test]
    fn disable_enable() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        
        assert_eq!(deadline_monitor.status(), Status::Running);
        assert_eq!(deadline_monitor.disable(), Ok(()));
        assert_eq!(deadline_monitor.status(), Status::Disabled);
        assert_eq!(deadline_monitor.enable(), Ok(()));
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn enable_while_enabled() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        
        assert_eq!(deadline_monitor.status(), Status::Running);
        assert_eq!(deadline_monitor.enable(), Err(Error::NotAllowed));
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn disable_while_disabled() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        
        assert_eq!(deadline_monitor.status(), Status::Running);
        assert_eq!(deadline_monitor.disable(), Ok(()));
        assert_eq!(deadline_monitor.status(), Status::Disabled);
        assert_eq!(deadline_monitor.disable(), Err(Error::NotAllowed));
        assert_eq!(deadline_monitor.status(), Status::Disabled);
        assert_eq!(deadline_monitor.enable(), Ok(()));
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn min_larger_than_max() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");

        let create_err = deadline_monitor.create_deadline(Duration::from_millis(100), Duration::from_millis(99)).err();
        assert!(create_err.is_some());
        assert_eq!(create_err.unwrap(), Error::BadParameter);
    }

    #[test]
    fn deadline_met() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .create_deadline(Duration::from_millis(10), Duration::from_millis(1000))
                .unwrap();

            deadline.start().expect("Failed to start.");
            thread::sleep(Duration::from_millis(250));
            deadline.stop().expect("Failed to stop.");
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn deadline_too_slow() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .create_deadline(Duration::from_millis(10), Duration::from_millis(1000))
                .unwrap();

            deadline.start().expect("Failed to start.");
            thread::sleep(Duration::from_millis(2000));
            assert_eq!(deadline.stop(), Err(Error::Generic));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }
    
    #[test]
    fn deadline_too_fast() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .create_deadline(Duration::from_millis(100), Duration::from_millis(1000))
                .unwrap();

            deadline.start().expect("Failed to start.");
            assert_eq!(deadline.stop(), Err(Error::Generic));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }

    #[test]
    fn deadline_too_slow_while_disabled() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        deadline_monitor.disable().expect("Failed to disable.");
        assert_eq!(deadline_monitor.status(), Status::Disabled);

        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .create_deadline(Duration::from_millis(10), Duration::from_millis(1000))
                .unwrap();

            deadline.start().expect("Failed to start.");
            thread::sleep(Duration::from_millis(2000));
            deadline.stop().expect("Failed to stop even though disabled.");
        });

        let _ = t1.join();

        deadline_monitor.enable().expect("Failed to enable.");
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn deadline_too_fast_while_disabled() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        deadline_monitor.disable().expect("Failed to disable.");
        assert_eq!(deadline_monitor.status(), Status::Disabled);

        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .create_deadline(Duration::from_millis(100), Duration::from_millis(1000))
                .unwrap();

            deadline.start().expect("Failed to start.");
            deadline.stop().expect("Failed to stop even though disabled.");
        });

        let _ = t1.join();

        deadline_monitor.enable().expect("Failed to enable.");
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn deadline_guard_met() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let _guard = deadline_monitor_clone
                .create_deadline_guard(Duration::from_millis(10), Duration::from_millis(1000))
                .unwrap();

            thread::sleep(Duration::from_millis(250));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn deadline_guard_too_slow() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let _guard = deadline_monitor_clone
                .create_deadline_guard(Duration::from_millis(10), Duration::from_millis(1000))
                .unwrap();

            thread::sleep(Duration::from_millis(2000));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }

    #[test]
    fn deadline_guard_too_fast() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let _guard = deadline_monitor_clone
                .create_deadline_guard(Duration::from_millis(100), Duration::from_millis(1000))
                .unwrap();
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }

    #[test]
    fn stop_new_deadline() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        let mut deadline = deadline_monitor
                .create_deadline(Duration::ZERO, Duration::from_millis(1000))
                .unwrap();

        assert_eq!(deadline.stop(), Err(Error::NotAllowed));
    }

    #[test]
    fn start_started_deadline() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_hook(Box::new(LoggerCondition::new()));
        let deadline_monitor = deadline_monitor_builder.build().expect("Failed to build the monitor.");
        let mut deadline = deadline_monitor
                .create_deadline(Duration::ZERO, Duration::from_millis(1000))
                .unwrap();

        assert_eq!(deadline.start(), Ok(()));
        assert_eq!(deadline.start(), Err(Error::NotAllowed));
        assert_eq!(deadline.stop(), Ok(()));
    }
}
