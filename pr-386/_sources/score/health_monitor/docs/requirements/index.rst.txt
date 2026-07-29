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

.. _component_health_monitor_requirements:

Requirements
############

.. document:: HealthMonitor Requirements
   :id: doc__health_monitor_requirements
   :status: draft
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_feat
   :tags: template

.. attention::
    The above directive must be updated according to your Feature.

    - Modify ``Your Feature Name`` to be your Feature Name
    - Modify ``id`` to be your Feature Name in upper snake case preceded by ``doc__`` and followed by ``_requirements``
    - Adjust ``status`` to be ``valid``
    - Adjust ``safety`` and ``tags`` according to your needs

<Headlines (for the list of requirements if structuring is needed)>
===================================================================

.. stkh_req:: Dummy Stakeholder
   :id: stkh_req__requirements__dummy
   :reqtype: Non-Functional
   :safety: ASIL_B
   :security: YES
   :rationale: Dummy
   :status: invalid

   The platform shall ...


.. feat_req:: Dummy Feature
   :id: feat_req__requirements__template
   :reqtype: Non-Functional
   :safety: ASIL_B
   :security: YES
   :derived_from: stkh_req__requirements__dummy
   :status: invalid

    Dummy

.. comp_req:: Dummy Component
   :id: comp_req__health_monitor__dummy
   :reqtype: Process
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__requirements__template
   :belongs_to: comp__health_monitor
   :status: invalid

   The Component shall do xyz to the user to bring him to this condition at this time

   Note: (optional, not to be verified)

.. aou_req:: Dummy AoU
   :id: aou_req__health_monitor__dummy
   :reqtype: Process
   :security: YES
   :safety: ASIL_B
   :status: invalid

   The Component User shall do xyz to use the component safely

.. attention::
    The above directives must be updated according to your feature requirements.

    - Replace the example content by the real content for your first requirement (according to :need:`gd_guidl__req_engineering`)
    - Set the status to valid and start the review/merge process
    - Add other needed requirements for your feature

.. needextend:: is_external == False and "__health_monitor__" in id
   :+tags: lifecycle, health_monitor
