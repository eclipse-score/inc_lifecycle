..
   # *******************************************************************************
   # Copyright (c) 2024 Contributors to the Eclipse Foundation
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

Health Monitor
##############

The :term:`Health Monitor` is a library, that together with the :term:`Launch Manager` provide a way to monitor
the application health in similar fashion as the AUTOSAR `Platform Health Manager` (PHM).

The main features of the :term:`Health Monitor` are the following monitoring functions:

- Checkpoint supervision
  - Periodic monitoring of checkpoints, which must fit the pre-configured expected number of notifications in the given interval (not too many, not too few)
  - Protecting from running checks too often or too rarely
- Deadline supervision
  - Timing requirement between two checkpoints.
- Logical
  - Specifies in which order two or more checkpoints must be called

The :term:`Health Monitor` itself is monitored via the :term:`Launch Manager` with via alive supervision only.


Overview
========

- Inter process communication (IPC) only needed for the alive monitoring between :term:`Health Monitor` and :term:`Launch Manager`
- Easier configuration
    - The monitoring rules can be configured dynamically on demand basis
    - The monitoring can be started and stopped dynamically
- Debugging of problems possibly easier
    - Mapping of the events from a single process vs. the monitored application and the monitor

Drawbacks over classical external process monitoring
----------------------------------------------------

- Harder safety argumentation. The following chapter describes the issues and the possible solutions.

Safety
------

As the :term:`Health Monitor` is linked as part of the monitored application, it raises the following concerns
with respect to safety:

- How can it be ensured, that the monitored application does not interfere with the monitoring functionality?
- How can it be ensured, that the :term:`Health Monitor` does not incorrectly report alive to the :term:`Launch Manager` when it has detected
  a supervision error?


These concerns are valid, but can be addressed using the following techniques:

- Hiding of the internal data from the user:
    - For example, if the monitoring is implemented in a thread, the thread ID must not be exposed to the calling application.
    - Hide the implementation for example with the pImpl-approach
- Protecting the memory of the library by using guard pages where the application memory is located, and protect it with mprotect()
   - possibly with a help of a custom allocator
- Protecting the data with a checksum and possibly a sequence counter
   - The internal data of the library can be checksum'ed every operation cycle, and by adding for example, a sequence
     counter (or some more complex mathematical function), further checkpoints for detecting misbehavior can be implemented
- Using a safe programming language, which does not allow a raw pointer access
- Testing the application with Valgrid etc.
- Redundant monitoring, if the self monitoring with above is not sufficient, an another library in another memory location can detect sporadic corruption of the other.


Error Reactions
---------------

- When the :term:`Health Monitor` detects a failed supervision, it shall stop triggering alive notifications to the :term:`Launch Manager`.
- Additionally, when the error occurs, the :term:`Health Monitor` triggers a failure notification to the :term:`Launch Manager` to reduce the time
  to react on the error. This obviously will only work if the :term:`Health Monitor` is still working and correctly scheduled. Thus the
  worst case reaction time calculations must be made on the monitoring rules specified in the :term:`Launch Manager` for the monitored application.


Deadline Monitor API
====================

Interface
---------

The deadline monitor interface is defined here: :need:`logic_arc_int__lifecycle__deadline_monitor_if`


Dynamic Architecture
--------------------

.. feat_arc_dyn:: Application health monitoring
   :id: feat_arc_dyn__lifecycle__app_health_mon
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :fulfils: feat_req__lifecycle__liveliness_detection[version==1]
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/application_health_monitoring_dynamic.puml
      :scale: 50
      :align: center

The most important interactions are the following:

.. list-table:: Sequence diagram Description
   :widths: 10 90
   :header-rows: 1

   * - Sequence number
     - Description
   * - 001
     - :term:`Launch Manager` configuration for the alive monitoring of the `Monitored application` is parsed. This contains for example, what is the expected interval of alive notifications,
       how long grace period is given before failing to a missed (never received) alive notification etc.
   * - 002
     - Start the startup grace period timer to allow the application to startup, before timing out to a missed alive notification
   * - 003
     - The `Monitored application` is started. (To simplify, no startup checks drawn here)
   * - 004
     - The `Monitored application` instantiate and configure the HealthMonitor
   * - 006
     - Cyclic reporting aliveness to the monitor.
   * - 007
     - HealthMonitor waking up and checking if the checkpoint(s) have been called
   * - 008
     - Report aliveness to the LM's application specific supervision, observing the health of the HealthMonitor itself
   * - 009
     - Checkpoint sent, but not on time
   * - 010
     - Wake up and check if the checkpoint(s) have been triggered. In this case it was not, and thus actions 011 and 012 are triggered.
   * - 011
     - Trigger a failure event to the Launch Manager. This event allows the monitor react faster than waiting for the timeout to expire.
   * - 012
     - Additionally, triggering alive must be stopped

Requirements
------------

- :need:`feat_req__lifecycle__hm_deadline`

Logical Monitor API
===================

Interface
---------

The logical monitor interface is defined here: :need:`logic_arc_int__lifecycle__logical_monitor_if`


Dynamic Architecture
--------------------

.. feat_arc_dyn:: Logical control flow monitoring
   :id: feat_arc_dyn__lifecycle__app_ctrl_flow_mon
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :fulfils: feat_req__lifecycle__liveliness_detection[version==1]
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/logical_sup.puml
      :scale: 50
      :align: center


Requirements
------------

- :need:`feat_req__lifecycle__hm_logical`
