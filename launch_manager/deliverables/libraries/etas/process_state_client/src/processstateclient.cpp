/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
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
// (c) 2025 ETAS GmbH. All rights reserved.

#include <etas/vrte/lcm/log.hpp>
#include <etas/vrte/lcm/process_state_client/processstateclient.hpp>

namespace etas
{

    namespace vrte
    {

        namespace lcm
        {
            ProcessStateClient::ProcessStateClient() noexcept : m_LCM_PHM_socket{}
            {
            }
            ProcessStateClient::~ProcessStateClient() noexcept
            {
                static_cast<void>(m_LCM_PHM_socket.close());
            }
            score::Result<std::monostate> ProcessStateClient::init() noexcept
            {
                if (m_LCM_PHM_socket.connect("ProcessState") != ipc_dropin::ReturnCode::kOk)
                {
                    return score::Result<std::monostate>{score::MakeUnexpected(score::lcm::ExecErrc::kCommunicationError)};
                }
                else
                {
                    return score::Result<std::monostate>{};
                }
            }
            score::Result<std::optional<PosixProcess>> ProcessStateClient::getNextChangedPosixProcess() noexcept
            {
                etas::vrte::lcm::PosixProcess changedProcess;
                if (m_LCM_PHM_socket.getOverflowFlag())
                {
                    LM_LOG_ERROR()
                        << "ProcessStateClient::getNextChangedPosixProcess: Overflow occurred, will be reported as kCommunicationError";
                    return score::Result<std::optional<etas::vrte::lcm::PosixProcess>>{score::MakeUnexpected(
                        score::lcm::ExecErrc::kCommunicationError)};
                }
                auto res = m_LCM_PHM_socket.tryReceive(changedProcess);
                switch (res)
                {
                case ipc_dropin::ReturnCode::kOk:
                    return score::Result<std::optional<etas::vrte::lcm::PosixProcess>>{changedProcess};
                case ipc_dropin::ReturnCode::kQueueStateCorrupt:
                    return score::Result<std::optional<etas::vrte::lcm::PosixProcess>>{score::MakeUnexpected(
                        score::lcm::ExecErrc::kCommunicationError)};
                case ipc_dropin::ReturnCode::kQueueEmpty:
                    return score::Result<std::optional<etas::vrte::lcm::PosixProcess>>{
                        std::nullopt};
                default:
                    return score::Result<std::optional<etas::vrte::lcm::PosixProcess>>{score::MakeUnexpected(
                        score::lcm::ExecErrc::kGeneralError)};
                }
            }
        } // namespace lcm
    } // namespace vrte

} // namespace etas
