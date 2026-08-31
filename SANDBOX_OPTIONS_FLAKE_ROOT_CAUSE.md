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
- `//score/launch_manager/...` — 30/30 tests pass.
- Integration test under the single-core repro pin — **200/200 pass** (was 1/60 failing).
- Official command — **200/200 pass**:

```
bazel test //tests/integration/sandbox_options:sandbox_options \
  --config=x86_64-linux --verbose_failures --nocache_test_results --runs_per_test=200
```

> Note: the flake only reproduces under CPU contention. On a multi-core host the official
> command passes regardless of the fix; meaningful reproduction requires pinning the
> container to a single core (`cpuset_cpus="0"`) during investigation.
