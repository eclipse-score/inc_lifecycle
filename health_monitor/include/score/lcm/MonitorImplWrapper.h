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

#ifndef SCORE_LCM_MonitorImplWrapper_H_
#define SCORE_LCM_MonitorImplWrapper_H_

#include <cstdint>
#include <memory>
#include <string_view>

namespace score
{
namespace lcm
{

/// @brief Represents a Checkpoint
using Checkpoint = std::uint32_t;

/// @brief Forward Declaration for them Implementation class for Monitor
class MonitorImpl;

/// @brief Forward declaration of enumeration of local supervision status
enum class LocalSupervisionStatus : std::uint32_t;

/// @brief Wrapper of implementation class for score::lcm::Monitor class
///        This class is just a wrapper and forwards the calls from score::lcm::Monitor class
///        to the actual implementation class, i.e., score::lcm::MonitorImpl
class MonitorImplWrapper
{
public:
    /// @brief Non-parametric constructor is not supported
    MonitorImplWrapper() = delete;

    /// @brief Constructor of MonitorImplWrapper class
    /// @param [in] f_instanceSpecifier_r  Instance specifier object with the metamodel path of
    ///                                    the Monitor
    /// @throws std::runtime_error in case of an error loading the process-specific configuration
    /// @throws std::bad_alloc in case of insufficient memory
    explicit MonitorImplWrapper(const std::string_view& f_instanceSpecifier_r) noexcept(false);

    /// @brief The copy constructor for MonitorImplWrapper is not supported.
    MonitorImplWrapper(const MonitorImplWrapper&) = delete;

    /* RULECHECKER_comment(0, 6, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /// @brief Default move constructor for MonitorImplWrapper
    /// @param [in,out] MonitorImplWrapper&&  rvalue reference of the MonitorImplWrapper object
    ///                                                which shall be moved
    MonitorImplWrapper(MonitorImplWrapper&&) noexcept = default;

    /// @brief The copy assignment operator for MonitorImplWrapper is not supported.
    MonitorImplWrapper& operator=(const MonitorImplWrapper&) & = delete;

    /// @brief Default move assignment operator for MonitorImplWrapper
    /// @param [in,out] MonitorImplWrapper&&  rvalue reference of the MonitorImplWrapper object
    ///                                                which shall be moved
    /// @return Reference of the moved MonitorImplWrapper object
    MonitorImplWrapper& operator=(MonitorImplWrapper&&) & noexcept = default;

    /// @brief Destructor of the class
    virtual ~MonitorImplWrapper() noexcept(true);

    /// @brief Reports an occurrence of a Checkpoint
    /// @param [in] f_checkpointId   Checkpoint identifier.
    void ReportCheckpoint(score::lcm::Checkpoint f_checkpointId) const noexcept(true);

private:
    /// @brief Unique pointer to the implementation class of Monitor
    std::unique_ptr<MonitorImpl> monitorImplPtr;
};

}  // namespace lcm
}  // namespace score

#endif
