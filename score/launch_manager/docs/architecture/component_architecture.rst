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

Component Architecture Documentation
====================================

.. document:: Launch Manager Architecture
   :id: doc__launch_manager_architecture
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: YES
   :realizes: wp__component_arch[version==1]


Overview
--------

.. comp:: Launch Manager
   :id: comp__lifecycle_launch_manager
   :status: valid
   :version: 1
   :safety: ASIL_B
   :implements: logic_arc_int__lifecycle__controlif[version==1], logic_arc_int__lifecycle__alive_if[version==1]
   :uses: logic_arc_int__logging__logging[version==1], logic_arc_int__baselibs__json[version==1], logic_arc_int__os__unistd[version==1], logic_arc_int__lifecycle__lifecycle_if[version==1]
   :security: NO
   :belongs_to: feat__lifecycle[version==1]

Requirements Linked to Component Architecture
---------------------------------------------

.. code-block:: none

   .. needtable:: Overview of Component Requirements
      :style: table
      :columns: title;id
      :filter: search("comp_arch_sta__archdes$", "fulfils_back")
      :colwidths: 70,30

Description
-----------

<General Description>

<Design Decisions - For the documentation of the decision the :need:`gd_temp__change_decision_record` can be used.>

<Design Constraints>

Rationale Behind Architecture Decomposition
*******************************************

Mandatory: A motivation for the decomposition or reason for not further splitting it into internal components.

.. note:: Common decisions across components / cross cutting concepts is at the higher level.

Static Architecture
-------------------

.. comp_arc_sta:: Launch Manager Static View
   :id: comp_arc_sta__launch_manager__lm
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :belongs_to: comp__lifecycle_launch_manager[version==1]
   :fulfils: comp_req__launch_man__process_launch_args[version==1]

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_component(need(), needs) }}

Dynamic Architecture
--------------------

tbd

Interfaces
----------

tbd

Internal Components
-------------------

tbd
