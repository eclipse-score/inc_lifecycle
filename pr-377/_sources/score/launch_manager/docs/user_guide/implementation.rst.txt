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

Implementation
**************

Starting a process
==================

A reporting process must signal to the Launch Manager that it has
started successfully by calling the lifecycle API for the language it is
implemented in. This is done by reporting the ``kRunning`` execution
state.

A ``ready_timeout`` can be configured for each component. If the
process does not report ``kRunning`` within that timeout, the Launch
Manager terminates the process and registers an error.

Stopping a process
==================

A process can be configured to terminate in 2 ways:

-  **Self-terminating** - The process terminates whenever it wants.
-  **Non-self-terminating** - The process remains alive until LCM requests the process to end using a ``SIGTERM`` signal.

For a process to support being terminated by LCM, it needs to respond to
a ``SIGTERM`` signal by ending ongoing tasks, freeing resources and
exiting.

If a process does not terminate after receiving a ``SIGTERM``, and a
``shutdown_timeout`` is configured, then after the timeout is fired a
``SIGKILL`` is sent and the process is forcefully terminated.

State Management
================

A **state manager** is an application that dictates which run-targets shall be
active by communicating with the launch manager using the ``ControlClient``.
The **control client** exposes a single API ``ActivateRunTarget`` which allows
the user to provide the name of the run-target to activate.
``ActivateRunTarget`` returns a future that resolves once the transition
is complete.
Whether to ``.wait()`` on it is up to the implementation, a state manager may
choose to wait before issuing the next transition, or fire and continue without
blocking.

The **state manager** is itself a managed component and **must** report
``kRunning`` to the Launch Manager using the lifecycle API before calling
``ActivateRunTarget``.
Failing to do so means the Launch Manager does not consider the **state
manager** ready, which blocks the startup procedure from completing.
The configuration also requires that the component that is the **state manager** has
to have the field ``application_profile.application_type`` set to
``State_Manager``.

.. note::
   If ``ActivateRunTarget`` is called while a transition is already in
   progress, the in-flight transition is cancelled, its returned future will
   resolve with a ``kCancelled`` error, and the new transition will be served
   instead. Always check the result of each ``ActivateRunTarget`` call to
   detect cancellation.
