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

#ifndef SCORE_MW_LIFECYCLE_APPLICATIONCONTEXT_H
#define SCORE_MW_LIFECYCLE_APPLICATIONCONTEXT_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace score
{
namespace mw
{
namespace lifecycle
{

/**
 * \brief Represents cmd line arguments of an Application.
 */
class ApplicationContext
{
  private:
    std::vector<std::string> m_args;
    std::string m_app_path;

  public:
    /* NOLINTNEXTLINE(modernize-avoid-c-arrays): array tolerated for command line arguments */
    ApplicationContext(const std::int32_t argc, const char* const argv[]);

    /**
     * \brief Utility function.
     *
     * \return list of cmd line arguments represented as vector.
     */
    const std::vector<std::string>& get_arguments() const noexcept;
    /**
     * \brief Utility function.
     *
     * \return string argument if it exists, otherwise empty string.
     */
    std::string get_argument(const std::string_view flag) const noexcept;
};

}  // namespace lifecycle
}  // namespace mw
}  // namespace score

#endif  // PSCORE_MW_LIFECYCLE_APPLICATIONCONTEXT_H
