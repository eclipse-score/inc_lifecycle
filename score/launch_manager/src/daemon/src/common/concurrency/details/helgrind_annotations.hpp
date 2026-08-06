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

#ifndef HELGRIND_ANNOTATIONS_HPP_INCLUDED
#define HELGRIND_ANNOTATIONS_HPP_INCLUDED

// When built with --config=host the system compiler finds valgrind/helgrind.h
// in its default include paths and the real annotations are active.  Under any
// other config the header is absent and the macros expand to no-ops.
#if __has_include(<valgrind/helgrind.h>)
#include <valgrind/helgrind.h>
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ANNOTATE_HAPPENS_BEFORE(obj) \
    do                               \
    {                                \
    } while (false)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ANNOTATE_HAPPENS_AFTER(obj) \
    do                              \
    {                               \
    } while (false)
#endif

#endif  // HELGRIND_ANNOTATIONS_HPP_INCLUDED
