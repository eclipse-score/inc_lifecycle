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

#include "score/mw/lifecycle/lifecyclemanager.h"

#include "score/os/errno.h"
#include <cstdlib>
#include "score/mw/lifecycle/report_running.h"
#include "score/mw/log/logging.h"
#include "score/os/stdlib_impl.h"

#include <score/utility.hpp>
#include <thread>

namespace
{
static constexpr auto kFailedBlockSig = "Failed to block all signals.";
static constexpr auto kFailedCreateSig = "Failed to create signal set.";
static constexpr auto kFailedAddSigterm = "Failed to add SIGTERM to set.";
}  // namespace

score::mw::lifecycle::LifeCycleManager::LifeCycleManager(std::unique_ptr<score::os::Signal> signal_interface) noexcept
    : m_stop_source{}, m_app{nullptr}, m_signal_set{}, m_signal_handler_thread{}, signal_(std::move(signal_interface))  // NOLINT(cppcoreguidelines-prefer-member-initializer)
{
    mw::log::LogInfo() << "Initializing LifeCycleManager";
    if (!initialize_internal())
    {
        mw::log::LogError() << "Signal handler thread creating failed, application will exit with status EXIT_FAILURE";
        /* KW_SUPPRESS_START:MISRA.STDLIB.ABORT,MISRA.USE.EXPANSION: */
        /* (1) Exit call tolerated as we need to return an exit code. (2) Macro tolerated as as failure value is
         * implementation defined. */
        score::os::Stdlib::instance().exit(EXIT_FAILURE);
        /* KW_SUPPRESS_END:MISRA.STDLIB.ABORT,MISRA.USE.EXPANSION */
    }
}

score::mw::lifecycle::LifeCycleManager::~LifeCycleManager() noexcept
{
    at_exit();
}

void score::mw::lifecycle::LifeCycleManager::at_exit() noexcept
{
    if (m_signal_handler_thread.joinable())
    {
        const auto result =
            signal_->SendSelfSigterm(); /* NOLINT(score-banned-function) using SendSelfSigterm by desing */

        if (!result.has_value())
        {
            const auto error = result.error();
            mw::log::LogError() << "Error during sending self-SIGTERM:" << error.ToString();
        }

        m_signal_handler_thread.join();
    }
}

std::int32_t score::mw::lifecycle::LifeCycleManager::run(Application& app, const ApplicationContext& context)
{
    m_app.store(&app, std::memory_order_release);
    mw::log::LogInfo() << "LifeCycleManager started";
    /* Branching in below line is due to hidden exception handling */
    const auto init_status = m_app.load(std::memory_order_relaxed)->Initialize(context);  // LCOV_EXCL_BR_LINE

    std::string application_name{"None"};
    const auto& args = context.get_arguments();
    if (!args.empty())
    {
        application_name = args[0];  // LCOV_EXCL_BR_LINE
    }

    if (init_status != 0)
    {
        mw::log::LogError() << "Error occurred during Initialize";
        mw::log::LogError() << "Application" << application_name << "initialize finished with" << init_status;
        at_exit();
        return init_status;
    }

    mw::log::LogInfo() << "Running Application";
    report_running();
    /* Branching in below line is due to hidden exception handling */
    const auto run_status = m_app.load(std::memory_order_relaxed)->Run(m_stop_source.get_token());  // LCOV_EXCL_BR_LINE
    if (run_status != 0)
    {
        mw::log::LogError() << "Error occurred during Run";
    }
    mw::log::LogInfo() << "Shutting down Application";
    mw::log::LogInfo() << "Application" << application_name << "run finished with" << run_status;
    at_exit();
    return run_status;
}

bool score::mw::lifecycle::LifeCycleManager::initialize_internal()
{
    /* KW_SUPPRESS_START:UNREACH.GEN: False positive: Code is reachable. */
    const auto empty_set_status = signal_->SigEmptySet(m_signal_set);
    if (empty_set_status.has_value() == false)
    {
        mw::log::LogError() << score::mw::log::LogString{kFailedCreateSig, sizeof(kFailedCreateSig)}
                            << empty_set_status.error().ToString();
        return false;
    }
    /* KW_SUPPRESS_END:UNREACH.GEN */
    const auto add_termination_status = signal_->AddTerminationSignal(m_signal_set);
    if (add_termination_status.has_value() == false)
    {
        mw::log::LogError() << score::mw::log::LogString{kFailedAddSigterm, sizeof(kFailedAddSigterm)}
                            << add_termination_status.error().ToString();
        return false;
    }
    const auto pthread_sig_mask_status =
        signal_->PthreadSigMask(m_signal_set); /* NOLINT(score-banned-function) using PthreadSigMask by desing */
    if (pthread_sig_mask_status.has_value() == false)
    {
        mw::log::LogError() << score::mw::log::LogString{kFailedBlockSig, sizeof(kFailedBlockSig)}
                            << pthread_sig_mask_status.error().ToString();
        return false;
    }
    // only start thread if everything was ok
    m_signal_handler_thread = std::thread(&LifeCycleManager::handle_signal, this);  // LCOV_EXCL_BR_LINE
    return true;
}

void score::mw::lifecycle::LifeCycleManager::handle_signal()
{
    std::int32_t sig(0);
    mw::log::LogInfo() << "Signal handler started";
    const auto sigwait_status = signal_->SigWait(m_signal_set, sig);  // LCOV_EXCL_BR_LINE
    if ((m_app.load(std::memory_order_acquire) != nullptr) && (sigwait_status.has_value() == true))
    {
        mw::log::LogInfo() << "Signal SIGTERM received, requesting to stop the app";
        score::cpp::ignore = m_stop_source.request_stop();
        report_shutdown();
    }
    else
    {
        if ((sigwait_status.has_value() == false) ||
            ((sigwait_status.has_value() == true) && (sigwait_status.value() != 0)))
        {
            mw::log::LogError() << "Waiting for SIGTERM failed!";
        }
        else
        {
            mw::log::LogError() << "SIGTERM received before Application instance is created!";
        }

        mw::log::LogError() << "Application will exit with status EXIT_FAILURE!";
        /* quick_exit() is used instead of exit() to avoid undefined behavior when trying to finish execution even if it
          is still possible that initialization is ongoing and using global resources i.e. logging.
         KW_SUPPRESS_START:MISRA.STDLIB.ABORT,MISRA.USE.EXPANSION:
         (1) Exit call tolerated as we need to return an exit code. (2) Macro tolerated as failure value is
         implementation defined. */
        score::os::Stdlib::instance().quick_exit(EXIT_FAILURE);
        /* KW_SUPPRESS_END:MISRA.STDLIB.ABORT,MISRA.USE.EXPANSION */
    }
}

void score::mw::lifecycle::LifeCycleManager::report_running() noexcept
{
    mw::log::LogInfo() << "Reporting kRunning to Launch Manager";
    score::mw::lifecycle::report_running();
}

void score::mw::lifecycle::LifeCycleManager::report_shutdown() noexcept {}
