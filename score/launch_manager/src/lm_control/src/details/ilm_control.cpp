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

#include "score/mw/lifecycle/ilm_control.hpp"
#include "score/mw/lifecycle/details/lm_control_impl.hpp"

#include <exception>

namespace score::mw::lifecycle
{

score::Result<std::unique_ptr<ILmControl>> ILmControl::Create(std::string_view instance_specifier)
{
    try
    {
        auto instance = std::make_unique<internal::LmControlImpl>();
        const auto init_result = instance->init(instance_specifier);
        if (!init_result.has_value())
        {
            return score::MakeUnexpected<std::unique_ptr<ILmControl>>(init_result.error());
        }
        return instance;
    }
    catch (const std::exception& ex)
    {
        LM_LOG_ERROR() << "LmControl: Create failed with exception:" << ex.what();
        return score::MakeUnexpected<std::unique_ptr<ILmControl>>(ExecErrc::kGeneralError);
    }
    catch (...)
    {
        LM_LOG_ERROR() << "LmControl: Create failed with an unknown exception";
        return score::MakeUnexpected<std::unique_ptr<ILmControl>>(ExecErrc::kGeneralError);
    }
}

}  // namespace score::mw::lifecycle
