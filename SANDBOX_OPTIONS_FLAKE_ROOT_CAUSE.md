# Root-cause analysis & fix: flaky `sandbox_options` integration test

- **Issue:** [eclipse-score/lifecycle#503](https://github.com/eclipse-score/lifecycle/issues/503) — `//tests/integration/sandbox_options:sandbox_options` is flaky
- **Related:** [eclipse-score/lifecycle#554](https://github.com/eclipse-score/lifecycle/issues/554) — the launch-manager fix (reverted commit reintroduced via patch)
- **Branch:** `bugfix/stabilize-sandbox-options-test`

## TL;DR

The flake is **not** a test bug and **not** a scheduling bug. It is a genuine race in the
launch manager that real-time (`SCHED_FIFO`/`SCHED_RR`) scheduling merely *triggers* on
CPU-constrained hosts. The `NOTE: Cancellation timed out` log line is a **downstream
symptom** of that race, not an independent problem.

The fix is the launch-manager change from issue #554 (it applies cleanly to current `main`
— it is **not** outdated), plus an added regression unit test for the exact #503 code path
and distinct scheduling priorities in the test config.

## Answer: is "NOTE: Cancellation timed out" caused by scheduling?

Not independent, but not a scheduling defect either — it is a symptom of an LM
activation-completion race that RT scheduling triggers. Evidence (all reproduced under a
single-core container pin, `cpuset_cpus="0"`):

| Config                     | Before fix                  | After fix        |
| -------------------------- | --------------------------- | ---------------- |
| FIFO/RR **prio 10**        | fails                       | —                |
| FIFO/RR **prio 1 & 2**     | fails 1/60 (*same rate*)    | —                |
| **all SCHED_OTHER**        | 0/60 (never fails)          | —                |
| FIFO 10 / RR 20            | (the flake)                 | **200/200 pass** |

The identical failure rate at prio 1 vs prio 10 proves priority tuning **cannot** fix it:
any RT priority preempts the launch manager's `SCHED_OTHER` worker and reaper threads
equally. That is why the real fix had to be in the launch manager, not the test config.

## Root cause (the race)

For a self-terminating, `Reporting` process whose `ready_condition` is `Terminated`
(processes a/b/c in this test):

1. A worker thread runs `startProcess()` → `setState(kStarting)` → forks the child → blocks
   in `handleProcessStillStarting` / `waitForkRunning`.
2. The child completes its `report_running` handshake, then exits.
3. The `OsHandler` reaping thread reaps it and calls `tryHandleTermination()`. Because the
   worker has **not yet** reached `setState(kRunning)`, `getState() < kRunning`, so it takes
   the *"Defer to the startup thread"* branch — which sets `kTerminated` and reports **no
   completion**.
4. The worker unblocks, finishes `startProcess()`, and calls `setState(kRunning)` — which
   **fails** (state is already `kTerminated`, and `setState` only advances `new > old`). It
   then reported `tryReportCompletion(kRunning)`, which **never matches** a `Terminated`
   ready condition → returns `kWaiting`.

Result: neither thread reports success. `jobs_in_progress_` never reaches 0 → the graph
never completes the transition → `verification_component` never runs → `test_end` is never
written → `run_until_file_deployed` raises `TimeoutError`. On the subsequent `SIGTERM`, the
cancel path waits on the stuck in-flight job and logs `NOTE: Cancellation timed out`.

Key insight: ready-condition success is reported by **different threads** depending on the
condition — `ready=Running` is reported by the worker (at `kRunning`); `ready=Terminated` is
reported by `tryHandleTermination`. The "defer" branch wrongly assumed the worker would
report, but the worker only ever reported `kRunning`.

### Log evidence (a reproduced failure, process_c)

```
...terminated with status 0            <- OsHandler reaps process_c
Got kRunning for pid 79 process 2      <- worker sets kRunning AFTER the reap
startProcess for process 2 done
                                       <- "Component 2 finished activation successfully" NEVER appears
NOTE: Cancellation timed out           <- downstream symptom
TimeoutError: File '/tmp/tests/test_end' did not appear within 3.0s
```

Processes a and b (whose workers won the race) logged `Got kRunning` *before* their
termination and both reported `finished activation successfully`.

## The fix

Launch manager (`process_info_node.cpp`, from #554):

- `startProcess()` reports completion against the state **actually reached** — `kTerminated`
  if the process already exited during startup — instead of blindly reporting `kRunning`.
- `tryReportCompletion()` treats `new_state >= desired_state` as success, so a `kTerminated`
  report satisfies both `ready=Terminated` and a self-terminated `ready=Running` process.

Unit tests (`process_info_node_UT.cpp`):

- The #554 patch adds a test for the **Native / exits-before-map-insert** (`kYield`) path.
- Added `SelfTerminating_TerminatedReadyCondition_ReapedWhileWaitingForkRunning_ReturnsSuccess`
  for the **Reporting / reaped-during-`waitForkRunning`** path — the exact #503 scenario the
  integration test exercises.

Test config (`tests/integration/sandbox_options/sandbox_options.json`):

- `sandbox_options_process_b` (`SCHED_RR`) priority changed `10 → 20` so the three processes
  use **distinct** priorities: `SCHED_FIFO=10`, `SCHED_RR=20`, `SCHED_OTHER=0`. All three
  `scheduling_policy` values remain verified.

## Verification

- `//score/launch_manager/src/daemon/src/process_group_manager/details:process_info_node_UT`
  — passes (patch test + new #503 regression test).
- `//score/launch_manager/...` — all tests pass.
- Integration test under the single-core repro pin — **200/200 pass** (was 1/60 failing).
- Official command — **200/200 pass**:

```
bazel test //tests/integration/sandbox_options:sandbox_options \
  --config=x86_64-linux --verbose_failures --nocache_test_results --runs_per_test=200
```

> Note: the flake only reproduces under CPU contention. On a multi-core host the official
> command passes regardless of the fix; meaningful reproduction requires pinning the
> container to a single core (`cpuset_cpus="0"`) during investigation.

## Post-rebase follow-up (rebase onto `main`)

After rebasing the fix branch onto `main`, the test suite broke again — for **two reasons
unrelated to the LM race**, both introduced by the rebase interacting with new `main`
commits (#500 "wait-for [file] ready condition", #523 "Use idhash as index",
#565 "Log process startup time"). Both are now fixed.

### Issue 1 — `tryReportCompletion` conflict with the new `FileState` ready condition

`main` #500 added a `FileState` alternative to the ready-condition variant, mapping it to
`desired_state = kRunning`. The #554 fix had gated success on a boolean
`has_process_state_condition` that was only set in the `ProcessState` branch. After the
rebase, a `FileState` ready condition therefore never set the flag, so
`tryReportCompletion` always returned `kWaiting` — success was never reported for
file-based ready conditions.

Symptom: `ProcessInfoNodeFileStateTest` unit tests failed
(`ConditionAlreadyMet_ReturnsSuccess`, `NotExistingCondition_ReturnsSuccess`,
`NativeApplication_DoesNotIgnoreRunning_ReturnsSuccess`) with `reportActivation` never
called / `activate()` returning `kWaiting`.

Fix: the guard represents "the ready condition maps to a comparable target state," which is
true for both `ProcessState` and `FileState`. Renamed it to `has_state_based_condition` and
set it in **both** branches of the `std::visit`.

### Issue 2 — inconsistent `scheduling_priority` for process_b after conflict resolution

The rebase conflict resolution left `sandbox_options_process_b` with **mismatched** priority
fields:

- `process_arguments`: `--scheduling-priority=15` — the value the process *asserts* against
- `sandbox.scheduling_priority`: `20` — the value the launch manager *applies*

The process reads its expected priority from `--scheduling-priority` and checks it against
its actual OS scheduling. The LM correctly applied `20`, but the process expected `15`, so
it failed its own gtest assertion and exited with code 256. That crashed the managed
process before `test_end` was written → `TimeoutError`, and the SIGTERM cancel then logged
`NOTE: Cancellation timed out` again — a good illustration that this log line is a **generic
"a job did not complete" symptom**, not specific to the LM race.

Fix: set process_b's `--scheduling-priority` back to `20` so both fields match (the exact
config verified 200/200 before the rebase). Final config: `SCHED_FIFO=10` (a),
`SCHED_RR=20` (b), `SCHED_OTHER=0` (c) — three distinct priorities, all policies verified.

> Note: a bazel JVM server crash (`error code: 14`, likely WSL2 memory pressure at 200
> concurrent runs) truncated one retry, but the process_b mismatch was the real cause of the
> observed failure, independent of the crash.

### Post-rebase verification

- `//score/launch_manager/...` — **31/31 tests pass** (incl. the `FileState` tests and both
  ProcessState race regression tests).
- Official command — **200/200 pass**.
