<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Coverage

Unified C++ + Rust code coverage via the shared LLVM source-based setup from
`@score_tooling//coverage`. The code itself lives in score_tooling; this
directory only holds the repo-specific pieces.

## Usage

Run the coverage build over the in-scope targets, then generate the HTML report:

```bash
# 1. Collect coverage
#    Note: Targets with "no-coverage" tag are skipped
bazel coverage --config=llvm_cov //score/... --build_tests_only

# 2. Generate the HTML report
bazel run @score_tooling//coverage:generate_coverage_html -- \
  --yaml quality/coverage/coverage_justifications.yaml \
  --archive-dir coverage_artifacts

# 3. Open it.
xdg-open coverage_artifacts/coverage_linux/index.html
```

## Notes

- Untested files that are in scope show up at 0% rather than vanishing.
- To widen the scope, add the component's top-level targets to
  `score_coverage_scope` in `BUILD`; the scope aspect walks their transitive
  deps to bring every production source file below them into scope.
