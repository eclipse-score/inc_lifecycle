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

#ifndef SCORE_MW_LIFECYCLE_APPLICATION_H
#define SCORE_MW_LIFECYCLE_APPLICATION_H

#include "score/mw/lifecycle/applicationcontext.h"

#include <score/stop_token.hpp>

#include <cstdint>

namespace score
{
namespace mw
{
namespace lifecycle
{

/**
 * \brief Class that represents an Application
 *
 * The main idea is to unify API that applications should implement,
 * in order to be started and controlled by LifeCycleManager.
 */
class Application
{
  public:
    /**
     * \brief Method dedicated for initialization of application
     *
     * \details Intention is to perform all the resources and services
     *          initialization within this method.
     *
     * \param context Abstracted cmd line arguments of an application
     *
     * \return 0 on successful initialization, non-zero otherwise.
     *
     * \post If non-zero returned, application will exit with exit
     *       code returned from Initialize and Run method won't be called.
     *       NOTE: In case non-zero is returned, the whole group where this
     *       application belongs to will not leave the machine state Init and
     *       thus, all the applications of such group will never be promoted to
     *       the machine state Running. This could lead to the whole ECU not being
     *       able to startup at all which will result in continuous general rejects.
     */
    virtual std::int32_t Initialize(const ApplicationContext& context) = 0;

    /**
     * \brief Method dedicated for doing business logic of application
     *
     * \details This method blocks the calling thread until the application is
     *          requested to stop via the provided stop_token. All cyclic work,
     *          event loops, or blocking waits shall run inside this method.
     *          Use score::cpp utilities (e.g. stop_token::stop_requested()) to
     *          cooperate with the stop signal and return promptly when triggered.
     *
     * \param token Stop token that signals the application to shut down.
     *              Run() shall return as soon as the token is triggered.
     *
     * \return 0 on successful execution, non-zero on error.
     *
     * \post If non-zero returned, the application will exit with the exit
     *       code returned from Run.
     */
    virtual std::int32_t Run(const score::cpp::stop_token&) = 0;

    Application() noexcept = default;
    virtual ~Application() noexcept = default;

  protected:
    Application(const Application&) = default;
    Application& operator=(const Application&) = default;

    Application(Application&&) = default;
    Application& operator=(Application&&) = default;
};

}  // namespace lifecycle
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LIFECYCLE_APPLICATION_H
