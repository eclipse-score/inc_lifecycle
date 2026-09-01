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

#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"

namespace score::mw::lifecycle::internal::saf::ifexm
{

ObservableEvent::ObservableEvent(const IdentifierHash& process_id) noexcept(false) : Observable<ObservableEvent>()
{
    event.id = process_id;
}

void ObservableEvent::pushData(void) noexcept
{
    pushResultToObservers();
}

}  // namespace score::mw::lifecycle::internal::saf::ifexm
