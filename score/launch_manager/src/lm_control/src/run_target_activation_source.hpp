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

#ifndef SCORE_MW_LIFECYCLE_RUN_TARGET_ACTIVATION_SOURCE_HPP
#define SCORE_MW_LIFECYCLE_RUN_TARGET_ACTIVATION_SOURCE_HPP

namespace score::mw::lifecycle
{

/// @brief Describes what caused a Run Target activation to occur.
///
/// Passed to the activation completion callback registered via
/// `ILmControl::register_run_target_activation_callback()`.
///
/// @note Activation cannot fail and always resolves into some
///       Run Target. This enum describes *why* it resolved,
///       not whether it succeeded.
enum class RunTargetActivationSource
{
    /// @brief Activation was explicitly requested by a State Manager.
    kStateManagerRequest = 0,

    /// @brief Activation happened automatically as part of a recovery action,
    ///        without an explicit State Manager request.
    kRecoveryAction = 1
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_MW_LIFECYCLE_RUN_TARGET_ACTIVATION_SOURCE_HPP
