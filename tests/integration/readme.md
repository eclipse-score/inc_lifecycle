<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Local integration testing

## Running the integration tests

To run all tests, simply run `bazel test //tests/integration/... --config=x86_64-linux`

## Config

Currently the following configs are supported:
- `host`
- `x86_64-linux`

## Crash dumps (core dumps)

Core-dump capture is **opt-in**: add `--config=core_dump` to include the support. **Attention: This influences the kernel `core_pattern` value of your host system!**


How it works:
- `--config=core_dump` forwards `SCORE_ENABLE_CORE_DUMP=1` into the test
  environment and sets the `//config:core_dump` build flag (see `.bazelrc`); the
  shared pytest plugin keys off the env var, individual tests need no adaptions.
- The build flag selects a **debug image variant** (`score_itf_examples_debug`,
  the normal image plus `gdb`) so cores can be analysed inside the container.
  Normal runs keep the slim `score_itf_examples` image, unchanged.
- The sandbox container runs privileged with an unlimited core-file `ulimit` and
  a read-write bind-mount of the workspace root.
- A shared fixture sets the kernel `core_pattern` to a sandbox-local path
  (`/tmp/score_cores/core.%e.%p.%s.%t`). On teardown it symbolizes each core
  **inside the container** (where the binary and matching libraries live) into a
  `<core>.bt.txt` backtrace, copies the cores and backtraces into the Bazel test
  outputs, and restores the original `core_pattern`.
- Before changing `core_pattern`, the fixture mirrors the original value to
  `.original_core_pattern` in the workspace root. The sandboxed test process sees
  the source tree read-only, so this file is written from inside the privileged
  container via the workspace bind-mount (hence it is root-owned). It is removed
  again once the value is restored, so it exists only if a run is force-killed.

Further technical limitations are described in [Important: the `core_pattern` is a global kernel setting](#important-the-core_pattern-is-a-global-kernel-setting)

### Getting a crash dump

Run the (crashing) test with `--config=core_dump`, disabling the cache so it
actually executes:
```
bazel test //tests/integration/<test> --config=x86_64-linux --config=core_dump --nocache_test_results
```

If a crash dump was created, a `CRASH DUMP` section is printed right under the
pytest `FAILURES` section at the end of the run (the `x86_64-linux` config
enables `--test_output=errors`, so the failing log is shown automatically):
```
=================================== FAILURES ===================================
...
================================== CRASH DUMP ==================================
CRASH DUMP HAS BEEN CREATED! See <.../test.outputs/cores> for details.

core.launch_manager.42.6.1787209649:
    Program terminated with signal SIGABRT, Aborted.
    #0  0x... in ?? () from /lib/x86_64-linux-gnu/libc.so.6
    #1  0x... in raise () from /lib/x86_64-linux-gnu/libc.so.6
    #2  0x... in abort () from /lib/x86_64-linux-gnu/libc.so.6
    #3  0x... in <your crashing frame> at <file>:<line>
    ...
    Full backtrace (all threads): <.../cores/core.launch_manager.*.bt.txt>
    Reopen in gdb inside the debug image:
        docker run --rm -it -v <.../launch_manager>:/tmp/.../launch_manager:ro -v <.../cores>:/cores:ro score_itf_examples_debug:latest gdb /tmp/.../launch_manager /cores/core.launch_manager.*
=========================== short test summary info ============================
```
The crashing thread's stack is printed **inline** (symbolized inside the
container, so libraries match). The printed paths are absolute and
copy-pasteable. Core files are named `core.<exe>.<pid>.<signal>.<time>` (signal
`11` = `SIGSEGV`, `6` = `SIGABRT`); they are only produced on an actual crash and
can be large (hundreds of MB).

### Analysing a crash dump

The crashing thread is already in the banner above. For the **full all-threads**
dump, open the auto-captured backtrace file the fixture wrote next to the core:
```
cat "$(bazel info bazel-testlogs)/tests/integration/<test>/<test>/test.outputs/cores/"*.bt.txt
```
`--config=core_dump` builds with `-c dbg` automatically, so the backtrace carries
file/line information — no extra flag needed.

The banner already prints a ready-to-run `docker run … gdb …` command per core.
It must run **inside** the debug image (tagged `score_itf_examples_debug:latest`,
which ships `gdb`), not on the host: the core references the container's
libraries, so a host `gdb` unwind walks garbage. The command mounts the host
binary at its original in-container path (so gdb matches the core without
warnings) and the cores directory, then drops you into an interactive session:
```
CORES="$(bazel info bazel-testlogs)/tests/integration/<test>/<test>/test.outputs/cores"
BIN="$(bazel info bazel-bin)/<path-to-crashing-binary>"
docker run --rm -it -v "$BIN:/tmp/<remote>/<binary>:ro" -v "$CORES:/cores:ro" \
    score_itf_examples_debug:latest \
    gdb /tmp/<remote>/<binary> /cores/core.*
```
Running host `gdb` against the core is a last resort; if you must, point it at an
exported container rootfs via `set sysroot` / `solib-search-path`.

## Important: the `core_pattern` is a global kernel setting

`core_pattern` is **not** namespaced per container - it is shared with the
host. The fixture therefore changes it globally at the start of a test and
restores the original value on teardown. Two consequences:

- **Run one crashing target at a time when investigating.** Concurrent tests
  race on the shared `core_pattern`, so a parallel run can miss dumps or leave
  a stale value.
- **If a run is force-killed before teardown**, the restore may not run and
  `core_pattern` is left pointing at `/tmp/score_cores/...`. The original value
  was saved to `.original_core_pattern` in the workspace root before it was
  changed, so you never lose it. **Simply re-running a core-dump test recovers
  automatically**: the fixture sees the leftover backup, treats it (not the
  current test value) as the original, and restores it on teardown. To fix it by
  hand instead:
  ```
  cat /proc/sys/kernel/core_pattern          # current (likely /tmp/score_cores/...)
  cat .original_core_pattern                 # the value to restore
  # On WSL, write the saved value back from a privileged container:
  docker run --rm --privileged -v "$PWD/.original_core_pattern:/orig:ro" \
    debian:bookworm-slim \
    bash -c 'cat /orig > /proc/sys/kernel/core_pattern'
  rm .original_core_pattern
  ```
  If `.original_core_pattern` is absent, the restore already ran (the file is
  removed on success). The default `core_pattern` differs per system (e.g.
  `|/wsl-capture-crash ...` on WSL, `|/lib/systemd/systemd-coredump ...` on
  systemd hosts, or plain `core`).

