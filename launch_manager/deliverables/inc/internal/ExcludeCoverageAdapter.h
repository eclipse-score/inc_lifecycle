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

#ifndef EXCLUDE_COVERAGE_ADAPTER_H_INCLUDED
#define EXCLUDE_COVERAGE_ADAPTER_H_INCLUDED

#ifdef __CTC__
#define EXCLUDE_COVERAGE_START(justification) _Pragma("CTC ANNOTATION justification") _Pragma("CTC SKIP")
#else
#define EXCLUDE_COVERAGE_START(justification)
#endif

#ifdef __CTC__
#define EXCLUDE_COVERAGE_END _Pragma("CTC ENDSKIP")
#else
#define EXCLUDE_COVERAGE_END
#endif

#endif