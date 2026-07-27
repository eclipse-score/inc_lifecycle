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

Feature Architecture
====================

.. document:: Lifecycle Module Architecture
   :id: doc__lifecycle_module_architecture
   :status: valid
   :safety: ASIL_B
   :security: YES
   :version: 1
   :realizes: wp__feature_arch[version==1]

Overview
--------

A brief overview of Lifecycle is described :need:`doc__lifecycle`.

Description
-----------

The concept is based on 2 major components:

* **Launch Manager**: Responsible for starting and stopping components based on
  the defined Run States and alive supervision of the started components

* **Health Monitor**: Provides process local monitoring functionalities such as
  deadline monitoring and logical program flow monitoring

<Design Decisions - For the documentation of the decision the :need:`gd_temp__change_decision_record` can be used.>

<Design Constraints>

Requirements
------------

The requirements for the feature architecture are defined in the `requirements` section of the feature documentation in the project repository: :need:`doc__lifecycle_requirements`

Rationale Behind Architecture Decomposition
*******************************************

Mandatory: A motivation for the decomposition

.. note:: Common decisions across features / cross cutting concepts is at the high level.

Static Architecture
-------------------

.. feat_arc_sta:: Lifecycle Static View
   :id: feat_arc_sta__lifecycle__static_view_arch
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1
   :fulfils: feat_req__lifecycle__launch_support[version==1]
   :includes: logic_arc_int__lifecycle__lifecycle_if[version==1], logic_arc_int__lifecycle__alive_if[version==1], logic_arc_int__lifecycle__controlif[version==1], logic_arc_int__lifecycle__deadline_monitor_if[version==1], logic_arc_int__lifecycle__logical_monitor_if[version==1]
   :belongs_to: feat__lifecycle

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_feature(need(), needs) }}

Dynamic Architecture
--------------------

.. code-block:: rst

   .. feat_arc_dyn:: Dynamic View
      :id: feat_arc_dyn__feature_name__dynamic_view
      :security: YES
      :safety: ASIL_B
      :status: invalid
      :fulfils: feat_req__feature_name__some_title
      :belongs_to: feat__feature_name

      Put here a sequence diagram

Logical Interfaces
------------------

The logical interfaces of the feature are defined in the `interfaces` section of the feature documentation in the project repository: :need:`doc__lifecycle_architecture`

Module Viewpoint
----------------

.. mod_view_sta:: Module architecture
   :id: mod_view_sta__lifecycle__all
   :version: 1
   :includes: comp__lifecycle_launch_manager, comp__health_monitor

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_module(need(), needs) }}
      LifecycleApplication --> logic_arc_int__lifecycle__lifecycle_if : implements
      LifecycleApplication --> logic_arc_int__lifecycle__controlif : use
      LifecycleApplication --> logic_arc_int__lifecycle__alive_if : use
      LifecycleApplication --> logic_arc_int__lifecycle__logical_monitor_if : use
      LifecycleApplication --> logic_arc_int__lifecycle__deadline_monitor_if :use
      LifecycleApplication --> posix_signals : implements
      NativeApplication --> posix_signals : implements
      comp__lifecycle_launch_manager --> posix_signals : use

Components Details
------------------

.. toctree::
   :maxdepth: 1
   :glob:

   ./launch_manager
   ./launch_manager_configuration
   ./health_monitor
   ./external_monitoring
