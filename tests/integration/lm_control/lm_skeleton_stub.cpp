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
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "score/mw/com/runtime.h"
#include "score/mw/com/runtime_configuration.h"
#include "score/mw/launch_manager/control/lm_control_server.hpp"

namespace
{
using score::mw::launch_manager::control::IGraph;
using score::mw::launch_manager::control::LmControlServer;
using score::mw::launch_manager::control::RunTargetRequest;

std::atomic<bool> g_stop{false};

void on_signal(int) noexcept
{
    g_stop = true;
}

/// @brief Minimal IGraph that echoes the requested Run Target back as
///        settled, once processRequests() is called from the main loop.
class StubGraph final : public IGraph
{
  public:
    void setServer(LmControlServer* server) noexcept
    {
        server_ = server;
    }

    std::optional<std::string_view> getActiveRunTarget() override
    {
        return "Running";
    }

    bool enqueueRunTargetActivation(RunTargetRequest request) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.push_back(request);
        return true;
    }

    /// @brief Settle all queued activations. Must be called from the main loop.
    ///
    /// Simulates a recovery action: activating "FailRunning" is treated as a
    /// failure that the graph recovers from by settling on "Recovery" instead,
    /// reported with no requestId so activationCompleted attributes it to
    /// RunTargetActivationSource::kRecoveryAction.
    void processRequests()
    {
        std::vector<RunTargetRequest> requests;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            requests.swap(pending_);
        }

        for (const auto& request : requests)
        {
            if (server_ == nullptr)
            {
                continue;
            }

            if (request.runTargetName.as_string_view() == "FailRunning")
            {
                server_->activationCompleted("Recovery", std::nullopt);
            }
            else
            {
                server_->activationCompleted(request.runTargetName.as_string_view(), request.requestId);
            }
        }
    }

  private:
    LmControlServer* server_{nullptr};
    std::mutex mutex_;
    std::vector<RunTargetRequest> pending_;
};
}  // namespace

/// @brief Minimal binary that starts the LmControlSkeleton (stub handlers) and
///        waits for SIGTERM. Used as the Launch Manager side in IPC integration tests.
///
/// Usage: lm_skeleton_stub <path-to-lm-mw-com-config.json>
int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: lm_skeleton_stub <mw_com_config.json>\n";
        return 1;
    }

    std::signal(SIGTERM, on_signal);
    std::signal(SIGINT, on_signal);

    score::mw::com::runtime::InitializeRuntime(argc, argv);

    StubGraph graph;
    LmControlServer server{graph};
    graph.setServer(&server);

    if (!server.Initialize())
    {
        std::cerr << "lm_skeleton_stub: LmControlServer::Initialize failed\n";
        return 1;
    }

    std::cout << "lm_skeleton_stub: skeleton ready" << std::endl;

    while (!g_stop)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        graph.processRequests();
    }

    server.Shutdown();
    return 0;
}
