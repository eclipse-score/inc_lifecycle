<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

## Running the integration tests

To run all tests, simply run `bazel test //tests/integration/... --config=x86_64-linux`

## Config

Currently the following configs are supported:
- `host`
- `x86_64-linux`

## Debugging

Using `config=host`, `--sandbox_add_mount_pair=/tmp`, and `--compilation_mode=dbg` tests and their binaries will be written to `/tmp/tests/`. From the test name directory, launch manager can be started with gdb using the following command: `sudo gdb --args ./launch_manager -c etc/<config-name>.bin`. This should load debug symbols and allow breakpoints to be set. If `layout src` fails to load source files, use `dir <path-to-repo>` to point gdb to the correct location