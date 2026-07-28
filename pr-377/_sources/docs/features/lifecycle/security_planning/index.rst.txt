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

Feature Security Work Products List
###################################

.. document:: Lifecycle Security WPs
   :id: doc__lifecycle_security_planning_wp
   :status: draft
   :safety: ASIL_B
   :security: YES
   :realizes: wp__platform_security_plan

Tailoring
=========

Additional to the tailoring in the SW platform project as defined in the project's :need:`wp__platform_security_plan` we define here the additional tailoring on feature level.

- Excluded for this feature are additionally the following work products (and their related requirements):

  - <work product/requirement> - <Argumentation why it is not needed or replaced by another work product or activity.>


Security Work products List
===========================

.. list-table:: Feature Lifecycle Work products
    :header-rows: 1

    * - Work product Id
      - Link to process
      - Process status
      - Link to WP

    * - :need:`wp__feat_request`
      - :need:`gd_temp__change_feature_request`
      - :ndf:`copy('status', need_id='gd_temp__change_feature_request')`
      - :need:`doc__lifecycle`

    * - :need:`wp__requirements_feat`
      - :need:`gd_temp__req_feat_req`
      - :ndf:`copy('status', need_id='gd_temp__req_feat_req')`
      - doc__lifecycle_requirements

    * - :need:`wp__requirements_feat_aou`
      - :need:`gd_temp__req_aou_req`
      - :ndf:`copy('status', need_id='gd_temp__req_aou_req')`
      - :need:`doc__lifecycle_feat_aou`

    * - :need:`wp__feature_arch`
      - :need:`gd_temp__arch_feature`
      - :ndf:`copy('status', need_id='gd_temp__arch_feature')`
      - :need:`doc__lifecycle_architecture`

    * - :need:`wp__requirements_inspect`
      - :need:`gd_chklst__req_inspection`
      - :ndf:`copy('status', need_id='gd_chklst__req_inspection')`
      - doc__lifecycle_req_inspection

    * - :need:`wp__sw_arch_verification`
      - :need:`gd_chklst__arch_inspection_checklist`
      - :ndf:`copy('status', need_id='gd_chklst__arch_inspection_checklist')`
      - :need:`doc__lifecycle_arc_inspection`

    * - :need:`wp__verification_feat_int_test`
      - :need:`gd_guidl__verification_guide`
      - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
      - <Link to WP>

.. attention::
    The above table must be updated according to your feature security planning.

    - Fill the work producs links

Feature Security Package
========================

To create the security package (according to :need:`gd_guidl__security_package`) the following
documents and work products status have to go to "valid" (after the relevant verification were performed).

Feature Documents Status
------------------------

For all the work product documents the status can be seen by following the "Link to WP".
A summary of the status is also documented in the project's documentation management plan.

See <add here the section reference to the documentation management plan>

Feature Requirements Status
---------------------------

.. needtable::
   :filter: docname is not None and "lifecycle" in docname and "requirements" in docname
   :style: table
   :types: feat_req
   :tags: lifecycle
   :columns: id;status
   :colwidths: 25,25
   :sort: title

Feature AoU Status
------------------

.. needtable::
   :filter: docname is not None and "lifecycle" in docname and "requirements" in docname
   :style: table
   :types: aou_req
   :tags: lifecycle
   :columns: id;status
   :colwidths: 25,25
   :sort: title

Feature Architecture Status
---------------------------

.. needtable::
   :filter: docname is not None and "lifecycle" in docname and "architecture" in docname
   :style: table
   :types: feat_arc_sta; feat_arc_dyn
   :tags: lifecycle
   :columns: id;status
   :colwidths: 25,25
   :sort: title
