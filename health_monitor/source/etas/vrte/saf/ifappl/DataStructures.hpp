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
#ifndef DATASTRUCTURES_HPP_INCLUDED
#define DATASTRUCTURES_HPP_INCLUDED

#include <cstdint>

#include "etas/vrte/saf/ipc/IpcServer.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ifappl
{

/// Maximum number of Checkpoints to be stored in IPC channel
/// @todo Implement logic to determine the number of checkpoint entries
/// that a Monitor instance can report between two cycles
/// of PHM daemon.
// coverity[autosar_cpp14_a0_1_1_violation] value is referenced in multiple files, but depending on build package.
constexpr uint16_t k_maxCheckpointBufferElements{512U};

/// Variable data exchange buffer: For every report of checkpoint,
/// one new instance of the below structure is created and stored
/// in the shared memory
/* RULECHECKER_comment(0,17, check_member_function_in_struct, "Member fuctions \
required for Vector and IPC APIs", true_no_defect) */
struct CheckpointBufferElement final
{
    etas::vrte::saf::timers::NanoSecondType timestamp{0U};  ///< Timestamp
    uint32_t checkpointId{0U};                               ///< Checkpoint ID

    /// @brief Default constructor needed for storage in vector
    CheckpointBufferElement() = default;

    /// @brief Constructor for usage with emplace
    /// @param [in] f_timestamp The checkpoint timestamp
    /// @param [in] f_checkpointId  The checkpoint id
    CheckpointBufferElement(etas::vrte::saf::timers::NanoSecondType f_timestamp,
                            uint32_t f_checkpointId) noexcept(true) :
        timestamp(f_timestamp), checkpointId(f_checkpointId)
    {
    }
};

/// @brief IPC server type instantiation with maximum checkpoint buffer size
using CheckpointIpcServer = ipc::IpcServer<CheckpointBufferElement, k_maxCheckpointBufferElements>;

}  // namespace ifappl
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
