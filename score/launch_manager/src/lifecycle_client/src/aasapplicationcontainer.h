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

#ifndef SCORE_MW_MW_LIFECYCLE_AASAPPLICATIONCONTAINER_H
#define SCORE_MW_MW_LIFECYCLE_AASAPPLICATIONCONTAINER_H

#include "score/mw/lifecycle/application.h"
#include "score/mw/lifecycle/lifecyclemanager.h"

#include <memory>
#include <vector>

namespace score::mw::lifecycle
{

/**
 * @brief AasApplicationContainer is a container for multiple applications.
 * This class is responsible for initializing and running the applications.
 * It provides API's to run the applications in parallel and to manage their lifecycle.
 */
class AasApplicationContainer : public Application
{
  public:
    /**
     * @brief Constructor for AasApplicationContainer.
     * This constructor initializes the container with the command line arguments and
     * the expected number of applications.
     *
     * @param argc The number of command line arguments.
     * @param argv The command line arguments.
     * @param count_expected_applications The expected number of applications.
     */
    AasApplicationContainer(
        const std::int32_t argc,
        const char* const argv[],
        const std::size_t count_expected_applications) noexcept;

    AasApplicationContainer(const AasApplicationContainer&) = delete;
    AasApplicationContainer& operator=(const AasApplicationContainer&) = delete;

    AasApplicationContainer(AasApplicationContainer&&) = delete;
    AasApplicationContainer& operator=(AasApplicationContainer&&) = delete;

    /**
     * @brief Destructor for AasApplicationContainer.
     */
    ~AasApplicationContainer() noexcept override;

    /**
     * @brief Add an application to the container.
     * This method creates an instance of the application and adds it to the container.
     * The application is expected to be a subclass of Application.
     *
     * @tparam App The type of the application to add.
     * @tparam Args The types of the arguments to pass to the application constructor.
     * @param args The arguments to pass to the application constructor.
     * @return A reference to this container.
     */
    template <typename App, typename... Args>
    AasApplicationContainer& With(Args&&... args)
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            applications_.size() + 1 <= count_expected_applications_, "Passed more Applications than expected");
        applications_.push_back(std::make_unique<App>(std::forward<Args>(args)...));
        return *this;
    }

    /**
     * @brief Launch the application container.
     * This method initializes and runs the applications in the container.
     *
     * @return 0 on success, non-zero on failure.
     */
    std::int32_t Launch();

    /**
     * @brief Initialize the application container.
     * This method initializes all applications in the container in parallel.
     * It creates a thread for each application and waits for all threads to finish.
     *
     * @param context The application context.
     * @return 0 on success, non-zero value as a result from any failed application.
     */
    std::int32_t Initialize(const ApplicationContext& context) override;

    /**
     * @brief Run the application container.
     * This method can be used to run the applications in the container in parallel.
     *
     * @param token The stop token used for synchronization.
     * @return 0 on success, non-zero value as a result from any failed application.
     */
    std::int32_t Run(const score::cpp::stop_token& token) override;

  private:
    score::mw::lifecycle::ApplicationContext context_;
    std::vector<std::unique_ptr<Application>> applications_;
    std::size_t count_expected_applications_;
    LifeCycleManager lifecycle_manager;
};

}  // namespace score::mw::lifecycle

#endif  // #ifndef SCORE_MW_LIFECYCLE_AASAPPLICATIONCONTAINER_H
