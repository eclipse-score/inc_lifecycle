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

#ifndef SCORE_MW_LIFECYCLE_RUNAPPLICATION_H
#define SCORE_MW_LIFECYCLE_RUNAPPLICATION_H

#include "score/mw/lifecycle/applicationcontext.h"
#include "score/mw/lifecycle/lifecyclemanager.h"

#include <cstdint>

namespace score
{
namespace mw
{
namespace lifecycle
{

template <typename ApplicationType>
class Run final
{
  public:
    Run(const std::int32_t argc,
        const char* const argv[]) /* NOLINT(modernize-avoid-c-arrays): array tolerated for command line arguments */
        : context_{argc, argv}
    {
    }

    template <typename... Args>
    std::int32_t AsPosixProcess(Args&&... args) const
    {
        score::mw::lifecycle::LifeCycleManager lifecycle_manager{};
        /* Branching in below line is due to hidden exception handling */
        return InstantiateAndRunApplication(lifecycle_manager, std::forward<Args>(args)...);  // LCOV_EXCL_BR_LINE
    }

  private:
    template <typename... Args>
    std::int32_t InstantiateAndRunApplication(score::mw::lifecycle::LifeCycleManager& lifecycle_manager, Args&&... args)
        const
    {
        /* KW_SUPPRESS_START:MISRA.VAR.NEEDS.CONST:False positive:app is passed as const reference to run(). */
        ApplicationType app{std::forward<Args>(args)...};
        /* KW_SUPPRESS_END:MISRA.VAR.NEEDS.CONST */
        return lifecycle_manager.run(app, context_);  // LCOV_EXCL_BR_LINE
    }

    const score::mw::lifecycle::ApplicationContext context_;
};

/**
 * \brief Abstracts initialization and running of an application with LifeCycleManager.
 */
template <typename ApplicationType, typename... Args>

/* NOLINTNEXTLINE(modernize-avoid-c-arrays): array tolerated for command line arguments */
std::int32_t run_application(const std::int32_t argc, const char* const argv[], Args&&... args)
{
    score::mw::lifecycle::Run<ApplicationType> runner(argc, argv);
    return runner.AsPosixProcess(std::forward<Args>(args)...);
}

}  // namespace lifecycle
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LIFECYCLE_RUNAPPLICATION_H
