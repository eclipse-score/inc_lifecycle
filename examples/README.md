<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Demo Setup

## Running the Demo Verification

Execute `bazel test //examples/... --config=<...> --test_output=all --test_arg=-s` to build all dependencies and run the automated demo verification.

The extra flags make the `_step()` markers from `test_examples.py` visible in the Bazel output instead of leaving them under pytest capture.

The test verifies the following scenarios end-to-end:

- Launch manager starts and enters `Startup` run target
- Transition to `Running` — all demo apps start
- Transition back to `Startup` — all demo apps stop
- Transition to `Running` again
- Process crash: `cpp_supervised_app` is killed with SIGKILL — recovery to `fallback_run_target` is triggered
- Transition to `Running` again
- Supervision failure: `cpp_supervised_app` receives SIGUSR1 to misreport checkpoints — recovery to `fallback_run_target` is triggered
- Launch manager is stopped gracefully with SIGTERM

## Example Applications

| Application | Description |
|---|---|
| `cpp_supervised_app` | C++ app with alive supervision and crash recovery |
| `rust_supervised_app` | Rust app with alive supervision and crash recovery |
| `cpp_lifecycle_app` | C++ app demonstrating basic lifecycle management |
| `control_daemon` | State Manager app for requesting RunTarget transitions |

## Launch Manager Configuration

See the [Launch Manager Configuration](demo_verification/lifecycle_demo_test.json) for the demo scenario.