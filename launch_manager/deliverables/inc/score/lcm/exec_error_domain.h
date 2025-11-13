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
// (c) 2025 ETAS GmbH. All rights reserved.

/// @file

#ifndef SCORE_LCM_ERROR_DOMAIN_H_
#define SCORE_LCM_ERROR_DOMAIN_H_

#include "score/result/result.h"

namespace score
{

   namespace lcm
   {

      enum class ExecErrc : score::result::ErrorCode
      {
         kGeneralError = 1,                       ///< Some unspecified error occurred
         kInvalidArguments = 2,                   ///< Invalid argument was passed
         kCommunicationError = 3,                 ///< Communication error occurred
         kMetaModelError = 4,                     ///< Wrong meta model identifier passed to a function
         kCancelled = 5,                          ///< Transition to the requested Process Group state was cancelled by a newer request
         kFailed = 6,                             ///< Requested operation could not be performed
         kFailedUnexpectedTerminationOnExit = 7,  ///< Unexpected Termination during transition in Process of previous Process Group State happened
         kFailedUnexpectedTerminationOnEnter = 8, ///< Unexpected Termination during transition in Process of target Process Group State happened
         kInvalidTransition = 9,                  ///< Transition invalid (e.g. report kRunning when already in Running Process State)
         kAlreadyInState = 10,                    ///< Transition to the requested Process Group state failed because it is already in requested state
         kInTransitionToSameState = 11,           ///< Transition to the requested Process Group state failed because transition to requested state is already in progress
         kNoTimeStamp = 12,                       ///< DeterministicClient time stamp information is not available
         kCycleOverrun = 13                       ///< Deterministic activation cycle time exceeded
      };

      class ExecErrorDomain final : public score::result::ErrorDomain
      {

         [[nodiscard]] std::string_view MessageFor(const score::result::ErrorCode &code) const noexcept override
         {
            switch (static_cast<ExecErrc>(code))
            {
            case ExecErrc::kGeneralError:
               return "Some unspecified error occurred";
            case ExecErrc::kInvalidArguments:
               return "An invalid argument was passed";
            case ExecErrc::kCommunicationError:
               return "A communication error occurred";
            case ExecErrc::kMetaModelError:
               return "Wrong meta model identifier passed to a function";
            case ExecErrc::kCancelled:
               return "Transition to the requested Process Group state was cancelled by a newer request";
            case ExecErrc::kFailed:
               return "Requested operation could not be performed";
            case ExecErrc::kFailedUnexpectedTerminationOnExit:
               return "Unexpected Termination during transition in Process of previous Process Group State happened";
            case ExecErrc::kFailedUnexpectedTerminationOnEnter:
               return "Unexpected Termination during transition in Process of target Process Group State happened";
            case ExecErrc::kInvalidTransition:
               return "Transition invalid (e.g. report kRunning when already in Running Process State)";
            case ExecErrc::kAlreadyInState:
               return "Transition to the requested Process Group state failed because it is already in requested state";
            case ExecErrc::kInTransitionToSameState:
               return "Transition to the requested Process Group state failed because transition to requested state is already in progress";
            case ExecErrc::kNoTimeStamp:
               return "DeterministicClient time stamp information is not available";
            case ExecErrc::kCycleOverrun:
               return "Deterministic activation cycle time exceeded";
            default:
               return "Unknown error";
            }
         }
      };

      constexpr ExecErrorDomain g_ExecErrorDomain{};

      constexpr score::result::Error MakeError(ExecErrc code, const std::string_view user_message = "") noexcept
      {
         return score::result::Error{static_cast<score::result::ErrorCode>(code), g_ExecErrorDomain, user_message};
         ;
      }

   } // namespace lcm

} // namespace score

#endif // SCORE_LCM_ERROR_DOMAIN_H_
