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

.. _lm_user_guide:

Overview
********

.. _lm_introduction:

Introduction
============

.. _lm_what_is_the_launch_manager:

What is the Launch Manager?
---------------------------

The **Launch Manager** is a system daemon responsible for managing the
execution of software components on an embedded target. It is designed
to run for the lifetime of the target and take care of platform
initialization, ordered start-up and shutdown of components, and
recovery from unexpected failures.

.. _lm_responsibilities:

Responsibilities
----------------

The Launch Manager is responsible for:

-  **Component lifecycle control** - spawning and terminating OS
   processes according to their configured parameters (executable path,
   user/group identity, environment, scheduling policy).
-  **Dependency resolution** - ensuring components start and stop in the
   correct order based on declared startup and shutdown dependencies.
-  **Failure recovery** - detecting unexpected process termination and
   executing configured recovery actions such as restarting a component
   or switching to a recovery Run Target.
-  **Run Target management** - determining which components are active
   at any given time by activating and deactivating named Run Targets in
   response to requests from a Control Client.

.. _lm_key_concepts:

Key concepts
============

.. _lm_components:

Components
----------

A **Component** is an independent, deployable software unit managed by
the Launch Manager. This can be a long-running daemon developed with the
lifecycle API, a one-shot application, or any arbitrary binary such as ``ls``.
Components are defined independently of the
systems they run on, which allows the same component to be deployed
across multiple target configurations without modification.

The specific deployment parameters of a component (such as which run
targets it is included in, its startup timeout, or its termination
behavior) are kept separate from the component's inherent properties.
This separation allows for reusability across different system
configurations.

.. _lm_ready_state:

Ready state
-----------

Each component has a **Ready State**. A component reaches the Ready State when
it has started successfully and has satisfied its configured ready condition.

Supported ready conditions include ``Running``, where the component is
considered ready as soon as its process has reported ``kRunning`` via the
lifecycle API, and ``Terminated``, where the component is considered ready
once it has reported ``kRunning`` and subsequently exited successfully.
For the full list of supported conditions, see :ref:`lm_ready_conditions`.

A component can also be configured to be **non-reporting**. The Launch Manager
considers it to be in the **Ready State** as soon as its process has been
spawned.
This is useful for integrating third-party binaries or system utilities where
the source code cannot be modified to report the running state.

It is important to understand that the Ready State reflects a
component's *functional availability*, not merely the existence of its
OS process. For example, a component that mounts a file system remains
in the Ready State for as long as the mount is active, even if the
process that performed the mount has already exited.

Reaching the **Ready State** signals to the Launch Manager that the
component is fully operational and capable of providing its services,
this point is decided by the application developer. This
allows for other components to safely rely on other components services
and for the startup procedure to be evaluated by the Launch Manager.

.. uml:: ./images/lm_non_reporting.puml

.. uml:: ./images/lm_reporting_running.puml

.. uml:: ./images/lm_reporting_terminated.puml


.. _lm_run_targets:

Run targets
-----------

A **Run Target** defines a named collection of components that are
intended to be active together. Only **one Run Target can be active at a
time**. When a Run Target is activated, the Launch Manager transitions
the system exclusively to that target, any components not assigned to it
are terminated.

When a Run Target is activated, the Launch Manager performs the
following:

- All components currently in the **active** that are **not** assigned to
  the new Run Target are terminated.
- All components that are assigned to the new Run Target but are **not** yet
  **active** are started.


.. _lm_starting_components:

Starting Components
-------------------

With the definitions of **Components**, **Run Targets**, and **Ready State**
established, let us clarify the conditions under which the **Launch Manager**
will initiate a component's startup sequence. Components will be started for
two primary reasons:

* A component is directly assigned to a **Run Target** that is currently being activated.
* Another component, which is assigned to a **Run Target** being activated, explicitly depends on that component.

.. note::
   The exact interleaving of terminations and starts during a transition is not
   defined, new components may be started before, after, or concurrently with
   the termination of outgoing components. If a component depends on another
   being fully stopped or fully ready before it proceeds, this should be
   expressed using dependencies between components or run-targets rather than
   relying on transition ordering.


.. _lm_dependencies:

Dependencies
------------

A fundamental aspect of **Launch Manager** configuration involves understanding
how components are assigned to a **Run Target** and how dependencies are
declared.

When a component is said to be assigned to a **Run Target**, it means the **Run
Target's** configuration explicitly lists the component's name within its
``depends_on`` parameter.

Similarly, when a component depends on another component, its own
``component_properties.depends_on`` configuration parameter will list the name
of the other component it requires.

Additionally, a **Run Target** can declare a dependency on another **Run
Target**.
In this scenario, the name of the dependent **Run Target** is listed within the
``depends_on`` configuration parameter of the primary **Run Target**.
The most effective way to conceptualize this relationship is to imagine that
the list of components assigned to the dependent **Run Target** is effectively
included in the list of components of the primary **Run Target**.

Dependencies in the **Launch Manager** are resolved transitively.
When a **Run Target** is activated, the **Launch Manager** does not only start
the components and **Run Targets** directly listed in its ``depends_on``, it
follows the full dependency chain recursively until all dependencies are
accounted for.

This means that a component brought in by a **Run Target** dependency is
treated as if it were directly assigned to the activating **Run Target**.
If that component itself depends on further components, those are pulled in as
well.

.. _lm_dependency_rules:

Rules for Configuring Dependencies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When configuring dependencies within the **Launch Manager**, the following
rules must be observed to ensure correct system behavior:

* A **Component** can depend on another **Component**.
* A **Run Target** can depend on a **Component**.
* A **Run Target** can depend on another **Run Target**.
* A **Component** cannot depend on a **Run Target**.

.. _lm_run_targets_example:

Example
^^^^^^^

The following diagram illustrates two Run Targets sharing some
components and each having components exclusive to them. When
transitioning from *Startup* to *Full*, the Launch Manager deactivates
components not assigned to *Full* and activates those that are.

.. uml:: ./images/lm_run_targets.puml

The order in which components are started and stopped within a
transition is determined by their configured dependencies.
And so the following is the sequence diagram showing the
processes being started:

.. uml:: ./images/lm_run_targets_starting.puml
