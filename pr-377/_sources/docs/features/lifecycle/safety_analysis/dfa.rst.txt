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


DFA (Dependent Failure Analysis)
================================

.. document:: Lifecycle DFA
   :id: doc__lifecycle_dfa
   :status: draft
   :safety: ASIL_B
   :security: NO
   :realizes: wp__feature_dfa

.. note:: Use the content of the document to describe e.g. why a fault model is not applicable for the diagram.


The DFA for the feature Lifecycle is performed. To show evidence that all failure initiators are considered, the applicability has to be filled out in the
following tables. For all applicable failure initiators, the DFA has to be performed.

Dependent Failure Initiators
----------------------------

Shared resources
^^^^^^^^^^^^^^^^

The dependent failure initiators related to shared resources are not applicable for the features. The shared resources
will be considered in the platform DFA.

Communication between the two elements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Receiving function is affected by information that is false, lost, sent multiple times, or in the wrong order etc. from the sender.

.. list-table:: DFA communication between elements
  :header-rows: 1
  :widths: 10,20,10,20

  * - ID
    - Violation cause communication between elements
    - Applicability
    - Rationale
  * - CO_01_01
    - Information passed via argument through a function call, or via writing/reading a variable being global to the two software functions (data flow)
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - CO_01_02
    - Data or message corruption / repetition / loss / delay / masquerading or incorrect addressing of information
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - CO_01_03
    - Insertion / sequence of information
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - CO_01_04
    - Corruption of information, inconsistent data
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - CO_01_05
    - Asymmetric information sent from a sender to multiple receivers, so that not all defined receivers have the same information
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - CO_01_06
    - Information from a sender received by only a subset of the receivers
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - CO_01_07
    - Blocking access to a communication channel
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>

Shared information inputs
^^^^^^^^^^^^^^^^^^^^^^^^^

Same information input used by multiple functions.

.. list-table:: DFA shared information inputs
  :header-rows: 1
  :widths: 10,20,10,20

  * - ID
    - Violation cause shared information inputs
    - Applicability
    - Rationale
  * - SI_01_02
    - Configuration data
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - SI_01_03
    - Constants, or variables, being global to the two software functions
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - SI_01_04
    - Basic software passes data (read from hardware register and converted into logical information) to two applications software functions
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - SI_01_05
    - Data / function parameter arguments / messages delivered by software function to more than one other function
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>

Unintended impact
^^^^^^^^^^^^^^^^^

Unintended impacts to function due to various failures.

.. list-table:: DFA unintended impact
  :header-rows: 1
  :widths: 10,20,10,20

  * - ID
    - Violation cause unintended impact
    - Applicability
    - Rationale
  * - UI_01_01
    - Memory miss-allocation and leaks
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - UI_01_02
    - Read/Write access to memory allocated to another software element
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - UI_01_03
    - Stack/Buffer under-/overflow
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - UI_01_04
    - Deadlocks
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - UI_01_05
    - Livelocks
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - UI_01_06
    - Blocking of execution
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - UI_01_07
    - Incorrect allocation of execution time
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - UI_01_08
    - Incorrect execution flow
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - UI_01_09
    - Incorrect synchronization between software elements
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - UI_01_10
    - CPU time depletion
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - UI_01_11
    - Memory depletion
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>
  * - UI_01_12
    - Other HW unavailability
    - <yes | no>
    - <Rationale if not applicable, otherwise link to filled out DFA>


DFA
===

For all identified applicable failure initiators, the DFA is performed in the following section.

.. code-block:: rst

    .. feat_saf_dfa:: <Title>
       :violates: <Feature architecture>
       :id: feat_saf_dfa__<Feature>__<Element descriptor>
       :failure_id: <ID from DFA failure initiators :need:`gd_guidl__dfa_failure_initiators`>
       :failure_effect: "description of failure effect of the failure initiator on the element"
       :mitigated_by: <ID from Feature Requirement | ID from AoU Feature Requirement>
       :mitigation_issue: <ID from Issue Tracker>
       :sufficient: <yes|no>
       :status: <valid|invalid>

 .. note::   Argument is inside the 'content'. Therefore content is mandatory.

.. attention::
    The above directive must be updated according to your feature DFA.

    - The above "code-block" directive must be updated
    - Fill in all the needed information in the <brackets>
