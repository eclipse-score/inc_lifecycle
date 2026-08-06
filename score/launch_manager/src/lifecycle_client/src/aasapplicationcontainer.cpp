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

#include "score/mw/lifecycle/aasapplicationcontainer.h"

#include "score/mw/lifecycle/lifecyclemanager.h"
#include <score/jthread.hpp>

namespace score
{
namespace mw
{
namespace lifecycle
{

AasApplicationContainer::AasApplicationContainer(
    const std::int32_t argc,
    const char* const argv[],
    const std::size_t count_expected_applications) noexcept
    : Application{}, context_{argc, argv}, applications_{}, count_expected_applications_{count_expected_applications}
{
    applications_.reserve(count_expected_applications_);
}

AasApplicationContainer::~AasApplicationContainer() noexcept = default;

std::int32_t AasApplicationContainer::Initialize(const ApplicationContext& context)
{
    std::vector<std::int32_t> thread_results(applications_.size());
    std::vector<score::cpp::jthread> threads;
    threads.reserve(applications_.size());
    auto result_iterator = thread_results.begin();

    for (auto& app : applications_)
    {
        threads.emplace_back(
            [](decltype(result_iterator) thread_result,
               Application& app_ref,
               const ApplicationContext& app_context) mutable {
                *thread_result = app_ref.Initialize(app_context);
            },
            result_iterator,
            std::ref(*app),
            context);
        result_iterator++;
    }

    for (auto& thread : threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    auto nonzero = std::find_if(thread_results.begin(), thread_results.end(), [](const auto& value) {
        return value != 0;
    });
    if (nonzero != thread_results.end())
    {
        return *nonzero;
    }
    return 0;
}

std::int32_t AasApplicationContainer::Run(const score::cpp::stop_token& token)
{
    std::int32_t result = 0;

    if (!applications_.empty())
    {
        // Take the last application out so that we can reuse the main thread to run it later, see below.
        auto last_application = std::move(applications_.back());
        applications_.pop_back();

        std::vector<std::int32_t> thread_results(applications_.size(), 0);
        std::vector<score::cpp::jthread> threads;
        threads.reserve(applications_.size());
        auto result_iterator = thread_results.begin();
        for (auto& application : applications_)
        {
            threads.emplace_back(
                [](decltype(result_iterator) thread_result,
                   std::unique_ptr<Application> app,
                   const score::cpp::stop_token& stop_token) {
                    *thread_result = app->Run(stop_token);
                },
                result_iterator,
                std::move(application),
                token);
            result_iterator++;
        }
        applications_.clear();

        result = last_application->Run(token);

        for (auto& thread : threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }

        if (result == 0)
        {
            auto nonzero = std::find_if(thread_results.begin(), thread_results.end(), [](const auto& value) {
                return value != 0;
            });
            if (nonzero != thread_results.end())
            {
                result = *nonzero;
            }
        }
    }

    return result;
}

std::int32_t AasApplicationContainer::Launch()
{
    return lifecycle_manager.run(*this, context_);
}

}  // namespace lifecycle
}  // namespace mw
}  // namespace score
