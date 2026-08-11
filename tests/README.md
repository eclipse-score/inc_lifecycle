<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Test Options

## Bazel Args

| Arg                            | What It Does                                                                |
|:-------------------------------|:-----------------------------------------------------------------------------|
|`--sandbox_add_mount_pair=/tmp` | Uses the users real `/tmp` dir for testing (only for `--config=host` builds |
|`--test_output=streamed`        | logs apear while running                                                    |
|`--test_output=all`             | outputs all logs after test                                                 |
|`--nocache_test_results`        | Reruns tests even if they have previously passed                            |
|`--runs_per_test=N`             | Reruns the tests N times                                                    |
|`--compilation_mode=dbg`        | Builds the binaries in debug mode (debug symbols & `NDEBUG` **not** defined)|


## Pytest Args

Note the arguments are written so that you can add them to the end of the bazel
test command.

| Arg                            | What It Does                                  | Example Command                                 |
|:-------------------------------|:----------------------------------------------|:------------------------------------------------|
|`--test_arg=--no-local-cleanup` | Integration tests don't cleanup after running |`bazel test //... --test_arg=--no-local-cleanup` |
|`--test_arg=-s`                 | More logging from the python test framework   |`bazel test //... --test_arg=-s`                 |
