..
   # *******************************************************************************
   # Copyright (c) 2025 Contributors to the Eclipse Foundation
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

.. _component_architecture_hm:

Component Architecture
======================

.. document:: HealthMonitor Architecture
   :id: doc__health_monitor_architecture
   :status: draft
   :safety: ASIL_B
   :security: NO
   :realizes: wp__component_arch
   :tags: template

.. attention::
    The above directive must be updated according to your needs.

    - Modify ``Your Component Name`` to be your Component Name
    - Modify ``id`` to be your Component Name in upper snake case preceded by ``doc__`` and followed by ``_architecture``
    - Adjust ``status`` to be ``valid``
    - Adjust ``safety`` and ``tags`` according to your needs

Overview
--------

Document describes HealthMonitor component architecture.

.. comp:: Health Monitor
   :id: comp__health_monitor
   :security: YES
   :safety: ASIL_B
   :status: valid
   :belongs_to: feat__lifecycle

   Health Monitor component provides a set of functionalities to verify the health of the system and its components. It includes monitoring deadlines, logic, and heartbeats to ensure proper system functioning.



Description
-----------

This component provides a set of functionalities to manage the lifecycle of programs running on the S-CORE platform.

Requirements Linked to Component Architecture
---------------------------------------------

TODO

Rationale Behind Architecture Decomposition
*******************************************

Architecture is not decomposed.

Static Architecture
-------------------

.. comp_arc_sta:: Deadline Monitor
   :id: comp_arc_sta__deadline_monitor__static_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :implements:
   :fulfils: comp_req__health_monitor__dummy
   :belongs_to: comp__health_monitor
   :includes:

   .. uml::  assets/dm_static_architecture.puml

.. comp_arc_sta:: Logic Monitor
   :id: comp_arc_sta__logic_monitor__static_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :implements:
   :fulfils: comp_req__health_monitor__dummy
   :belongs_to: comp__health_monitor
   :includes:

   .. uml::  assets/lm_static_architecture.puml

.. comp_arc_sta:: Health Monitor  
   :id: comp_arc_sta__health_monitor__static_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :implements:
   :fulfils: comp_req__health_monitor__dummy
   :belongs_to: comp__health_monitor
   :includes:

   .. uml::  assets/hm_static_architecture.puml

Dynamic Architecture
--------------------

.. comp_arc_dyn:: HealthMonitor Creation
   :id: comp_arc_dyn__health_monitor__dynamic_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__health_monitor__dummy
   :belongs_to: comp__health_monitor

   .. uml::  assets/hm_creation.puml

.. comp_arc_dyn:: HealthMonitor background thread
   :id: comp_arc_dyn__health_lib_thread__dynamic_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__health_monitor__dummy
   :belongs_to: comp__health_monitor

   .. uml::  assets/hm_background_thread.puml

.. comp_arc_dyn:: DeadlineMonitor Usage
   :id: comp_arc_dyn__deadline_monitor__dynamic_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__health_monitor__dummy
   :belongs_to: comp__health_monitor

   .. uml::  assets/dm_usage.puml

.. comp_arc_dyn:: LogicMonitor Usage
   :id: comp_arc_dyn__logic_monitor__dynamic_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__health_monitor__dummy
   :belongs_to: comp__health_monitor

   .. uml::  assets/lm_usage.puml

.. comp_arc_dyn:: HeartbeatMonitor Usage
   :id: comp_arc_dyn__heartbeat_monitor__dynamic_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__health_monitor__dummy
   :belongs_to: comp__health_monitor

   .. uml::  assets/hbm_usage.puml

.. comp_arc_dyn:: Health Monitoring Startup Interaction
   :id: comp_arc_dyn__health_monitor__startup_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__health_monitor__dummy
   :belongs_to: comp__health_monitor

   .. uml::  assets/hm_startup.puml

.. comp_arc_dyn:: Health Monitoring Shutdown Interaction
   :id: comp_arc_dyn__health_monitor__shutdown_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__health_monitor__dummy
   :belongs_to: comp__health_monitor

   .. uml::  assets/hm_shutdown.puml

Interfaces
----------

.. real_arc_int:: DeadlineMonitor Interface
   :id: real_arc_int__deadline_monitor__interface
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__health_monitor__dummy
   :language: rust

   .. uml::  assets/dmb_interface.puml

.. real_arc_int:: LogicMonitor Interface
   :id: real_arc_int__logic_monitor__interface
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__health_monitor__dummy
   :language: rust

   .. uml::  assets/lmb_interface.puml

.. real_arc_int:: HeartbeatMonitor Interface
   :id: real_arc_int__heartbeat_monitor__interface
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__health_monitor__dummy
   :language: rust

   .. uml::  assets/hbm_interface.puml

.. real_arc_int:: HealthMonitor Interface
   :id: real_arc_int__health_monitor__interface
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__health_monitor__dummy
   :language: rust

   .. uml::  assets/hmb_interface.puml
