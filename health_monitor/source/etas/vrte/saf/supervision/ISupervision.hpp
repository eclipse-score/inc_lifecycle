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
#ifndef ISUPERVISION_HPP_INCLUDED
#define ISUPERVISION_HPP_INCLUDED

#include <cstdint>

#include <string>
#include <vector>
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace supervision
{

/// @brief ISupervision
/// @details The Interface Supervision class declares/defines methods, which are common for the different
/// supervision types which are: alive-, deadline-, logical-, local-, global-supervision
class ISupervision
{
public:
    /// @brief No default constructor
    ISupervision() = delete;

    /// @brief Constructor
    /// @param [in] f_supervisionConfigName_p       Unique name set by configuration
    /// @warning    Constructor may throw std::exceptions
    explicit ISupervision(const char* const f_supervisionConfigName_p) noexcept(false);

    /// @brief Default destructor
    /* RULECHECKER_comment(0, 3, check_min_instructions, "Default destructor is not provided\
       a function body", true_no_defect) */
    virtual ~ISupervision() = default;

    /// @brief Trigger evaluation
    /// @details Cyclic evaluation trigger for the supervision.
    /// This method tells the supervision that all supervision interfaces were queried for new data
    /// and the collected data (checkpoints) is now ready for evaluation.
    /// @param [in] f_syncTimestamp   Timestamp for cyclic synchronization
    virtual void evaluate(const timers::NanoSecondType f_syncTimestamp) = 0;

    /// @brief Get the name of the configuration element for the corresponding supervision container
    /// @return std::string   Constant string containing the name of the
    ///                             corresponding supervision configuration container
    const char* getConfigName(void) const;

protected:
    /// @brief Default Move Constructor
    /* RULECHECKER_comment(0, 7, check_min_instructions, "Default constructor is not provided\
       a function body", true_no_defect) */
    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /* RULECHECKER_comment(0, 3, check_copy_in_move_constructor, "Default constructor is not provided\
       the member initializer", false) */
    ISupervision(ISupervision&&) = default;
    /// @brief No Copy Constructor
    ISupervision(ISupervision&) = delete;
    /// @brief No Move assignment
    ISupervision& operator=(const ISupervision&&) = delete;
    /// @brief No Copy Assignment
    ISupervision& operator=(const ISupervision&) = delete;

private:
    /// Unique name set by configuration
    const std::string k_cfgName;
};

}  // namespace supervision
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
