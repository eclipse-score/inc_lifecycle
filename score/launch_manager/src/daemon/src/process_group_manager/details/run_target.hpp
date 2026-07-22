/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#ifndef SCORE_LCM_RUN_TARGET_HPP_INCLUDED
#define SCORE_LCM_RUN_TARGET_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"

#include <atomic>

namespace score::lcm::internal
{

/// @brief A virtual node in the dependency graph corresponding to a ProcessGroupState.
/// A RunTarget does not correspond to any OS process; it is immediately active once all
/// of its dependencies are active, and immediately inactive once deactivated.
class RunTarget final : public IComponent
{
  public:
    explicit RunTarget(uint32_t index) : index_(index) {}

    RunTarget(RunTarget&& other) noexcept : index_(other.index_), active_(other.active_.load()) {}
    RunTarget(const RunTarget&) = delete;
    RunTarget& operator=(const RunTarget&) = delete;
    RunTarget& operator=(RunTarget&&) = delete;
    ~RunTarget() override = default;

    RequestResult activate(score::cpp::stop_token) override
    {
        active_ = true;
        return RequestState::kSuccess;
    }

    RequestResult deactivate(score::cpp::stop_token) override
    {
        active_ = false;
        return RequestState::kSuccess;
    }

    RequestResult tryHandleTermination(int32_t) override
    {
        return RequestState::kSuccess;
    }

    uint32_t getIndex() const override
    {
        return index_;
    }

    bool active() const override
    {
        return active_;
    }

  private:
    uint32_t index_;
    std::atomic<bool> active_{false};
};

}  // namespace score::lcm::internal

#endif  // SCORE_LCM_RUN_TARGET_HPP_INCLUDED
