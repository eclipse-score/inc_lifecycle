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
#include <ffi.h>

#include <cassert>
#include <cstdlib>
#include <iostream>

void on_status_changed(void *data, hm_Status from, hm_Status to)
{
    std::cout << __FUNCTION__ << " from: " << (int32_t)from << " to: " << (int32_t)to << std::endl;
}

void on_state_changed(void *data, hm_LogicMonitorState from, hm_LogicMonitorState to)
{
    std::cout << __FUNCTION__ << " from: " << from.hash << " to: " << to.hash << std::endl;
}

int main()
{
    auto *builder = hm_lmb_new(hm_lm_state_from_str("Initial"));
    hm_lmb_add_transition(builder, hm_lm_state_from_str("Initial"),
                          hm_lm_state_from_str("Running"));
    hm_lmb_add_transition(builder, hm_lm_state_from_str("Running"), hm_lm_state_from_str("Paused"));
    hm_lmb_add_transition(builder, hm_lm_state_from_str("Paused"), hm_lm_state_from_str("Running"));
    hm_lmb_add_transition(builder, hm_lm_state_from_str("Running"),
                          hm_lm_state_from_str("Stopped"));
    hm_lmb_add_hook(builder, on_status_changed, nullptr, on_state_changed, nullptr);
    auto *monitor = hm_lmb_build(&builder);
    assert(builder == nullptr);
    assert(hm_lm_status(monitor) == hm_Status::Running);

    hm_lm_disable(monitor);
    assert(hm_lm_status(monitor) == hm_Status::Disabled);

    // Disable while disabled. Does nothing.
    hm_lm_disable(monitor);  // TODO: Should report error and the error should be handled.
    assert(hm_lm_status(monitor) == hm_Status::Disabled);

    hm_lm_enable(monitor);
    assert(hm_lm_status(monitor) == hm_Status::Running);

    // Enable while enabled. Does nothing.
    hm_lm_enable(monitor);  // TODO: Should report error and the error should be handled.
    assert(hm_lm_status(monitor) == hm_Status::Running);

    // Valid transition.
    hm_lm_transition(monitor, hm_lm_state_from_str("Running"));
    assert(hm_lm_state(monitor) == hm_lm_state_from_str("Running"));
    assert(hm_lm_status(monitor) == hm_Status::Running);

    // Valid transition.
    hm_lm_transition(monitor, hm_lm_state_from_str("Paused"));
    assert(hm_lm_state(monitor) == hm_lm_state_from_str("Paused"));
    assert(hm_lm_status(monitor) == hm_Status::Running);

    hm_lm_disable(monitor);
    assert(hm_lm_status(monitor) == hm_Status::Disabled);

    // Try a valid transition while disabled.
    hm_lm_transition(monitor, hm_lm_state_from_str("Running"));  // TODO: Should report NotAllowed.
    assert(hm_lm_state(monitor) == hm_lm_state_from_str("Paused"));
    assert(hm_lm_status(monitor) == hm_Status::Disabled);

    // Try an invalid transition while disabled.
    hm_lm_transition(monitor, hm_lm_state_from_str("Stopped"));  // TODO: Should report NotAllowed.
    assert(hm_lm_state(monitor) == hm_lm_state_from_str("Paused"));
    assert(hm_lm_status(monitor) == hm_Status::Disabled);

    hm_lm_enable(monitor);
    assert(hm_lm_status(monitor) == hm_Status::Running);

    // Try an invalid transition while enabled.
    hm_lm_transition(monitor, hm_lm_state_from_str("Stopped"));  // TODO: Should report NotAllowed.
    assert(hm_lm_state(monitor) == hm_lm_state_from_str("Paused"));
    assert(hm_lm_status(monitor) == hm_Status::Failed);

    // Try to transition while failed.
    std::cout << "Trying to transition while failed" << std::endl;
    // No further state transitions should be printed.
    hm_lm_transition(monitor, hm_lm_state_from_str("Stopped"));  // TODO: Should report Generic.
    assert(hm_lm_status(monitor) == hm_Status::Failed);

    hm_lm_delete(&monitor);
    assert(monitor == nullptr);

    return EXIT_SUCCESS;
}
