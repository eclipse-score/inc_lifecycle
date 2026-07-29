..
   # *******************************************************************************
   # Copyright (c) 2026 Contributors to the Eclipse Foundation
   #
   # See the NOTICE file(s) distributed with this work for additional
   # information regarding copyright ownership.
   #
   # This program and the accompanying materials are made available under the
   # terms of the Apache License Version 2.0 which is available at
   # https://www.apache.org/licenses/LICENSE-2.0
   #
   # SPDX-License-Identifier: Apache-2.0
   # *******************************************************************************

.. _lm_ready_conditions:

Ready Conditions
****************

A **Ready Condition** defines what must be true of a component before the
**Launch Manager** considers that component to have reached its **Ready
State**.

The following ready conditions are supported:

.. _lm_ready_condition_running:

Running
=======

**Configuration value:** ``"Running"``

The component is considered ready as soon as its process has reported
``kRunning`` to the **Launch Manager** via the lifecycle API. The process is
expected to remain alive and continue providing its service.

Use this condition for long-running daemons that signal readiness once their
initialization is complete.

.. _lm_ready_condition_terminated:

Terminated
==========

**Configuration value:** ``"Terminated"``

The component is considered ready once its process has reported ``kRunning``
via the lifecycle API **and** subsequently terminated with a successful exit
code.
A process that exits without first reporting ``kRunning`` is treated as a
failure, not as reaching the Ready State.

Use this condition for one-shot tasks (e.g. mounting a filesystem, applying
configuration) that signal completion by exiting cleanly.
