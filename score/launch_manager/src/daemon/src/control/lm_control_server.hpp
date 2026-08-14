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
#ifndef SCORE_MW_LAUNCH_MANAGER_CONTROL_LM_CONTROL_SERVER_HPP
#define SCORE_MW_LAUNCH_MANAGER_CONTROL_LM_CONTROL_SERVER_HPP

#include <cstdint>
#include <optional>
#include <string_view>

#include "score/mw/lifecycle/lm_control_service.h"

namespace score::mw::launch_manager::control
{

/// @brief Internal request to activate a Run Target, correlated to its
///        completion via requestId.
struct RunTargetRequest
{
    lifecycle::RunTargetName runTargetName;
    bool                     force;
    std::int32_t             requestId;
};

/// @brief Abstraction over the process graph that LmControlServer delegates to.
class IGraph
{
  public:
    virtual ~IGraph() noexcept = default;

    /// @return the currently active Run Target, or nullopt while an
    ///         activation is in progress.
    virtual std::optional<std::string_view> getActiveRunTarget() = 0;

    /// @return true if the request was accepted/enqueued, false if rejected.
    virtual bool enqueueRunTargetActivation(RunTargetRequest request) = 0;

  protected:
    IGraph()                         = default;
    IGraph(const IGraph&)            = default;
    IGraph& operator=(const IGraph&) = default;
    IGraph(IGraph&&)                 = default;
    IGraph& operator=(IGraph&&)      = default;
};

/// @brief Hosts the mw::com LmControl skeleton on the Launch Manager side.
///
/// Offers the LmControlService over mw::com so that State Managers can call
/// activate_run_target and get_active_run_target via the proxy. Delegates
/// the actual Run Target querying and activation to an injected IGraph.
class LmControlServer final
{
  public:
    /// @param graph               The graph to delegate Run Target queries and
    ///                            activations to. Must outlive this instance.
    /// @param instance_specifier  The mw::com instance specifier for the
    ///                            skeleton. Reads SCORE_LCM_SKELETON_INSTANCE_SPECIFIER
    ///                            from the environment if empty.
    explicit LmControlServer(IGraph& graph, std::string_view instance_specifier = {});

    ~LmControlServer() noexcept;

    LmControlServer(const LmControlServer&)            = delete;
    LmControlServer& operator=(const LmControlServer&) = delete;
    LmControlServer(LmControlServer&&)                 = delete;
    LmControlServer& operator=(LmControlServer&&)      = delete;

    /// @brief Create the skeleton, register handlers, and offer the service.
    /// @return true on success, false if skeleton creation or OfferService failed.
    bool Initialize();

    /// @brief Stop offering the service and destroy the skeleton.
    void Shutdown();

    /// @brief Notify that a Run Target activation has settled. Sends the
    ///        activation_result event to all subscribed proxies.
    /// @param runTarget  The Run Target the graph settled on.
    /// @param requestId  The id of the request that triggered this activation,
    ///                   or nullopt if the activation was not tied to a
    ///                   specific request (e.g. a recovery action).
    void activationCompleted(std::string_view runTarget, std::optional<std::int32_t> requestId);

  private:
    lifecycle::ActivateRunTargetResponse OnActivateRunTarget(
            const lifecycle::ActivateRunTargetRequest& request);

    lifecycle::GetActiveRunTargetResponse OnGetActiveRunTarget();

    IGraph&                                      graph_;
    std::string                                  instance_specifier_;
    std::optional<lifecycle::LmControlSkeleton>  skeleton_;
    std::int32_t                                 next_request_id_{0};
};

}  // namespace score::mw::launch_manager::control

#endif  // SCORE_MW_LAUNCH_MANAGER_CONTROL_LM_CONTROL_SERVER_HPP
