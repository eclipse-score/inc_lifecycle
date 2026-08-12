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

#ifndef IPROCESSSTATE_NOTIFIER_MOCK_HPP_INCLUDED
#define IPROCESSSTATE_NOTIFIER_MOCK_HPP_INCLUDED

#include "score/mw/launch_manager/process_state_client/iprocess_state_notifier.hpp"

#include <gmock/gmock.h>

#include <memory>

namespace score
{
namespace lcm
{

/// @brief Reusable gmock mock for IProcessStateNotifier, for use by tests of components that notify PHM of process
/// state changes.
class MockProcessStateNotifier : public IProcessStateNotifier
{
  public:
    MOCK_METHOD(std::unique_ptr<score::lcm::IProcessStateReceiver>, constructReceiver, (), (override));
    MOCK_METHOD(bool, queuePosixProcess, (const score::lcm::PosixProcess& f_posixProcess), (noexcept, override));
};

}  // namespace lcm
}  // namespace score

#endif  // IPROCESSSTATE_NOTIFIER_MOCK_HPP_INCLUDED
