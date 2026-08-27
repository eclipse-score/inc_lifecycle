<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Sanitizer

Runtime sanitizer builds for launch_manager and health_monitor, using the
shared runtime infrastructure (wrapper, suppression files, env templates) from
`@score_cpp_policies//sanitizers`. This directory only holds the repo-specific
Bazel configuration in `sanitizer.bazelrc`, which is imported from the root
`.bazelrc`.

## Usage

The following commands run all unit and integration tests with sanitizer enabled.

ASan + UBSan + LSan (recommended):

```bash
bazel test --config=asan_ubsan_lsan --config=x86_64-linux //score/... //tests/...
```

ThreadSanitizer (cannot be combined with ASan/LSan):

```bash
bazel test --config=tsan --config=x86_64-linux //score/... //tests/...
```

The `asan`, `ubsan` and `lsan` configs are convenience aliases that all resolve
to `asan_ubsan_lsan`.

## Notes

- Tests can opt out of a sanitizer with the tags `no-asan`, `no-ubsan`,
  `no-lsan` (ASan/UBSan/LSan run) or `no-tsan` (TSan run).
- Sanitizer builds enable minimal debug info (`-g1`) and disable stripping so
  that stack traces resolve to source locations.
