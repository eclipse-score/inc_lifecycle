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
use crate::alive_api::create_alive_api;
use crate::alive_api_base::AliveApiBase;
use std::sync::Arc;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::{Duration, Instant};

struct AtomicInstant {
    nanos: AtomicU64,
    epoch: Instant,
}

impl AtomicInstant {
    fn new() -> Self {
        Self {
            nanos: AtomicU64::new(0),
            epoch: Instant::now(),
        }
    }

    fn store(&self, instant: Instant) {
        let nanos = instant.duration_since(self.epoch).as_nanos() as u64;
        self.nanos.store(nanos, Ordering::Release);
    }

    fn load(&self) -> Instant {
        return self.epoch + Duration::from_nanos(self.nanos.load(Ordering::Acquire));
    }
}

struct AliveMonitorInner {
    last_heartbeat: AtomicInstant,
    heartbeat_interval_ms: AtomicU64,
    alive_api: Box<dyn AliveApiBase>,
}

#[derive(Clone)]
pub struct AliveMonitor {
    inner: Arc<AliveMonitorInner>,
}

impl AliveMonitor {
    pub fn new(heartbeat_interval: Duration) -> Self {
        let this = Self {
            inner: Arc::new(AliveMonitorInner {
                last_heartbeat: AtomicInstant::new(),
                heartbeat_interval_ms: AtomicU64::new((heartbeat_interval.as_millis() as u64) / 2),
                alive_api: create_alive_api(),
            }),
        };
        this.configure_minimum_time(this.get_heartbeat_interval());
        this
    }

    pub fn keep_alive(&self) {
        self.inner.alive_api.keep_alive();
        self.inner.last_heartbeat.store(Instant::now());
    }

    pub fn configure_minimum_time(&self, minimum_time_ms: u64) {
        self.inner.alive_api.configure_minimum_time(minimum_time_ms);
    }

    pub fn get_last_heartbeat(&self) -> Instant {
        return self.inner.last_heartbeat.load();
    }

    pub fn get_heartbeat_interval(&self) -> u64 {
        return self.inner.heartbeat_interval_ms.load(Ordering::Acquire);
    }
}
