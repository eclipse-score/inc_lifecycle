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

Safety Plan
***********

.. document:: Lifecycle Safety Plan
  :id: doc__lifecycle_safety_plan
  :status: draft
  :safety: ASIL_B
  :security: NO
  :realizes: wp__module_safety_plan

:note: The module safety plan shall be continuously maintained during the project.
       Deviations to the module safety plan should be documented :ref:`here <lifecycle_safety_package_deviations>`

Functional Safety Management Context
====================================

This Safety Plan adds to the project's :need:`wp__platform_safety_plan` all the module development relevant work products needed for ISO 26262 conformity.

Functional Safety Management Scope
==================================

This Safety Plan's scope is a SW module of the `SW platform <https://eclipse-score.github.io/score/main/modules/index.html>`_.
The module consists of one or more SW components and will be qualified as a SEooC.

Functional Safety Management Roles
==================================

.. list-table:: Module roles
        :header-rows: 1

        * - Role
          - Assignee

        * - Safety Manager
          - <link to Module's Safety Manager assignment or name>

        * - Module Project Manager
          - <link to Module's Project Manager assignment or name>

Tailoring
=========

Additional to the tailoring in the SW platform project as defined in the project's :need:`wp__platform_safety_plan` we define here the additional tailoring on module level.

- Excluded for this module are additionally the following work products (and their related requirements):

  - <work product/requirement> - <Argumentation why it is not needed or replaced by another work product or activity.>

Functional Safety Module Work products
======================================

One set of work products for the module and one set for each component of the module:

Module Work products List
-------------------------

.. list-table:: Module Work products
        :header-rows: 1

        * - Work product Id
          - Link to process
          - Process status
          - Link to WP

        * - :need:`wp__module_safety_plan`
          - :need:`gd_guidl__saf_plan_definitions`
          - :ndf:`copy('status', need_id='gd_guidl__saf_plan_definitions')`
          - this document

        * - :need:`wp__module_safety_package`
          - :need:`gd_guidl__saf_package`
          - :ndf:`copy('status', need_id='gd_guidl__saf_package')`
          - this document (including the linked documentation)

        * - :need:`wp__fdr_reports` (module Safety Plan)
          - :need:`gd_chklst__safety_plan`
          - :ndf:`copy('status', need_id='gd_chklst__safety_plan')`
          - :need:`doc__lifecycle_safety_plan_fdr`

        * - :need:`wp__fdr_reports` (module Safety Package)
          - :need:`gd_chklst__safety_package`
          - :ndf:`copy('status', need_id='gd_chklst__safety_package')`
          - :need:`doc__lifecycle_safety_package_fdr`

        * - :need:`wp__fdr_reports` (module's Safety Analyses & DFA)
          - :need:`gd_chklst__safety_analysis`
          - :ndf:`copy('status', need_id='gd_chklst__safety_analysis')`
          - :need:`doc__lifecycle_safety_analysis_fdr`

        * - :need:`wp__audit_report`
          - performed by external experts
          - n/a
          - <Link to WP>

        * - :need:`wp__module_safety_manual`
          - :need:`gd_temp__safety_manual`
          - :ndf:`copy('status', need_id='gd_temp__safety_manual')`
          - :need:`doc__lifecycle_safety_manual`

        * - :need:`wp__verification_module_ver_report`
          - :need:`gd_temp__mod_ver_report`
          - :ndf:`copy('status', need_id='gd_temp__mod_ver_report')`
          - :ref:`life_statistics`

        * - :need:`wp__module_sw_release_note`
          - :need:`gd_temp__rel_mod_rel_note`
          - :ndf:`copy('status', need_id='gd_temp__rel_mod_rel_note')`
          - :need:`doc__lifecycle_release_note_v0_3_0`

Component Health Monitor Work products List
-------------------------------------------

.. list-table:: Component Health Monitor Work products
        :header-rows: 1

        * - Work product Id
          - Link to process
          - Process status
          - Link to WP

        * - :need:`wp__requirements_comp`
          - :need:`gd_temp__req_comp_req`
          - :ndf:`copy('status', need_id='gd_temp__req_comp_req')`
          - :need:`doc__health_monitor_requirements`

        * - :need:`wp__requirements_comp_aou`
          - :need:`gd_temp__req_aou_req`
          - :ndf:`copy('status', need_id='gd_temp__req_aou_req')`
          - :need:`doc__health_monitor_requirements`

        * - :need:`wp__requirements_inspect`
          - :need:`gd_chklst__req_inspection`
          - :ndf:`copy('status', need_id='gd_chklst__req_inspection')`
          - :need:`doc__health_monitor_req_inspection`

        * - :need:`wp__component_arch`
          - :need:`gd_temp__arch_comp`
          - :ndf:`copy('status', need_id='gd_temp__arch_comp')`
          - :need:`doc__health_monitor_architecture`

        * - :need:`wp__sw_arch_verification`
          - :need:`gd_chklst__arch_inspection_checklist`
          - :ndf:`copy('status', need_id='gd_chklst__arch_inspection_checklist')`
          - :need:`doc__health_monitor_arc_inspection`

        * - :need:`wp__sw_component_fmea`
          - :need:`gd_temp__comp_saf_fmea`
          - :ndf:`copy('status', need_id='gd_temp__comp_saf_fmea')`
          - :need:`doc__health_monitor_fmea`

        * - :need:`wp__sw_component_dfa`
          - :need:`gd_temp__comp_saf_dfa`
          - :ndf:`copy('status', need_id='gd_temp__comp_saf_dfa')`
          - :need:`doc__health_monitor_dfa`

        * - :need:`wp__sw_implementation`
          - :need:`gd_guidl__implementation`
          - :ndf:`copy('status', need_id='gd_guidl__implementation')`
          - :need:`doc__health_monitor_detailed_design` & <Link to code>

        * - :need:`wp__verification_sw_unit_test`
          - :need:`gd_guidl__verification_guide`
          - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
          - <Link to WP>

        * - :need:`wp__sw_implementation_inspection`
          - :need:`gd_chklst__impl_inspection_checklist`
          - :ndf:`copy('status', need_id='gd_chklst__impl_inspection_checklist')`
          - :need:`doc__health_monitor_impl_inspection`

        * - :need:`wp__verification_comp_int_test`
          - :need:`gd_guidl__verification_guide`
          - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
          - <Link to WP>

Component Launch Manager Work products List
-------------------------------------------

.. list-table:: Component Launch Manager Work products
        :header-rows: 1

        * - Work product Id
          - Link to process
          - Process status
          - Link to WP

        * - :need:`wp__requirements_comp`
          - :need:`gd_temp__req_comp_req`
          - :ndf:`copy('status', need_id='gd_temp__req_comp_req')`
          - :need:`doc__launch_manager_requirements`

        * - :need:`wp__requirements_comp_aou`
          - :need:`gd_temp__req_aou_req`
          - :ndf:`copy('status', need_id='gd_temp__req_aou_req')`
          - :need:`doc__launch_manager_requirements`

        * - :need:`wp__requirements_inspect`
          - :need:`gd_chklst__req_inspection`
          - :ndf:`copy('status', need_id='gd_chklst__req_inspection')`
          - :need:`doc__launch_manager_req_inspection`

        * - :need:`wp__component_arch`
          - :need:`gd_temp__arch_comp`
          - :ndf:`copy('status', need_id='gd_temp__arch_comp')`
          - :need:`doc__launch_manager_architecture`

        * - :need:`wp__sw_arch_verification`
          - :need:`gd_chklst__arch_inspection_checklist`
          - :ndf:`copy('status', need_id='gd_chklst__arch_inspection_checklist')`
          - :need:`doc__launch_manager_arc_inspection`

        * - :need:`wp__sw_component_fmea`
          - :need:`gd_temp__comp_saf_fmea`
          - :ndf:`copy('status', need_id='gd_temp__comp_saf_fmea')`
          - :need:`doc__launch_manager_fmea`

        * - :need:`wp__sw_component_dfa`
          - :need:`gd_temp__comp_saf_dfa`
          - :ndf:`copy('status', need_id='gd_temp__comp_saf_dfa')`
          - :need:`doc__launch_manager_dfa`

        * - :need:`wp__sw_implementation`
          - :need:`gd_guidl__implementation`
          - :ndf:`copy('status', need_id='gd_guidl__implementation')`
          - :need:`doc__launch_manager_detailed_design` & <Link to code>

        * - :need:`wp__verification_sw_unit_test`
          - :need:`gd_guidl__verification_guide`
          - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
          - <Link to WP>

        * - :need:`wp__sw_implementation_inspection`
          - :need:`gd_chklst__impl_inspection_checklist`
          - :ndf:`copy('status', need_id='gd_chklst__impl_inspection_checklist')`
          - :need:`doc__launch_manager_impl_inspection`

        * - :need:`wp__verification_comp_int_test`
          - :need:`gd_guidl__verification_guide`
          - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
          - <Link to WP>

Link to project planning
------------------------

`Lifecycle & Health Feature Team Planning <https://github.com/orgs/eclipse-score/projects/33>`_

Module Safety Package
=====================

To create the safety package (according to :need:`gd_guidl__saf_package`) the following
documents and work products status have to go to "valid" (after the relevant verification were performed).

Module Documents Status
-----------------------

For all the work product documents the status can be seen by following the "Link to WP".
A summary of the status is also documented in the project's documentation management plan.

See <add here the section reference to the documentation management plan>

Component Documents Status
--------------------------

For all the work product documents the status can be seen by following the "Link to WP".
A summary of the status is also documented in the project's documentation management plan.

See <add here the section reference to the documentation management plan>

Component Requirements Status
-----------------------------

health_monitor:

.. needtable::
  :filter: docname is not None and "health_monitor" in docname and "requirements" in docname
  :style: table
  :types: comp_req
  :tags: health_monitor
  :columns: id;status;tags
  :colwidths: 25,25,25
  :sort: title

launch_manager:

.. needtable::
  :filter: docname is not None and "launch_manager" in docname and "requirements" in docname
  :style: table
  :types: comp_req
  :tags: launch_manager
  :columns: id;status;tags
  :colwidths: 25,25,25
  :sort: title

Component AoU Status
--------------------

health_monitor:

.. needtable::
  :filter: docname is not None and "health_monitor" in docname and "requirements" in docname
  :style: table
  :types: aou_req
  :tags: health_monitor
  :columns: id;status;tags
  :colwidths: 25,25,25
  :sort: title

launch_manager:

.. needtable::
  :filter: docname is not None and "launch_manager" in docname and "requirements" in docname
  :style: table
  :types: aou_req
  :tags: launch_manager
  :columns: id;status;tags
  :colwidths: 25,25,25
  :sort: title

Component Architecture Status
-----------------------------

health_monitor:

.. needtable::
  :filter: docname is not None and "health_monitor" in docname and "architecture" in docname
  :style: table
  :types: comp_arc_sta; comp_arc_dyn
  :tags: health_monitor
  :columns: id;status;tags
  :colwidths: 25,25,25
  :sort: title

launch_manager:

.. needtable::
  :filter: docname is not None and "launch_manager" in docname and "architecture" in docname
  :style: table
  :types: comp_arc_sta; comp_arc_dyn
  :tags: launch_manager
  :columns: id;status;tags
  :colwidths: 25,25,25
  :sort: title

.. _lifecycle_safety_package_deviations:

Deviations from Module Safety Plan
----------------------------------

The following deviations from the module safety plan are present in the module safety package.
These are deviations from planned processes execution and/or workproduct results,
safety anomalies in the sense of known bugs in the software are reported in the release notes.

<Describe here the deviations, whether they have an impact on module's safety functions,
how these can be mitigated or argued and if and when a resolution is planned.>
