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


FMEA (Failure Modes and Effects Analysis)
=========================================

.. document:: Health Monitor FMEA
   :id: doc__health_monitor_fmea
   :status: draft
   :safety: ASIL_B
   :security: NO
   :realizes: wp__sw_component_fmea
   :tags: template

Failure Mode Evaluation Table
------------------------------

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Title
     - id
     - failure_effect
     - mitigation_proposal
     - sufficient
   * - Missing processing time
     - HM_FMEA_001
     - Background thread does not receive CPU time slice, leading to miss specified alive notification internal towards Launch Daemon
     - | **Detection:**
       |  - Missing notifications will be detected by Launch Daemon and lead to safety reaction at Launch Daemon.

       | **Mitigation:** 
       |  - Provide `AoU` that integrator has to ensure Health Monitor background thread receives sufficient CPU time slice by configuring it's scheduling parameters accordingly.
       |  - All code within process is developed according to **ASIL-B** development process

     - Yes
   * - Loss of execution
     - HM_FMEA_002
     - Background thread does not advance in its execution (ie. deadlock, endless loop failure), leading to miss specified alive notification internal towards Launch Daemon
     - | **Detection:**
       |  -  Missing notifications will be detected by Launch Daemon and lead to safety reaction at Launch Daemon.

       | **Mitigation:**
       |  - All code within process is developed according to **ASIL-B** development process

     - Yes
   * - Memory corruption of monitoring data structures
     - HM_FMEA_003
     - Corruption of internal data structures used for monitoring, leading to missed detection of failure of monitored components (bitflips, out of range data, etc.)
     - | **Detection:**
       |  - Using protected memory pages around internal data structures used for monitoring to detect memory corruption (see below)

       | **Mitigation:**
       |  - All code within process is developed according to **ASIL-B** development process

     - Yes

HM_FMEA_003
------------

Health Monitoring Library is placed in same process as monitored components. Therefore, any other component that shares same process can corrupt memory of Health Monitoring Library. This can lead to missed detection of failure of monitored components.
Since we are using **Rust** as programming language for Health Monitoring Library implementation, we could rely on Rust memory safety guarantees and avoid memory corruption due to programming errors. However we are also supporting C/C++ components 
that can introduce memory issues due to programming errors. Therefore, we need to consider additional detection mechanisms. Below description of possible detection mechanisms:

Checksums
^^^^^^^^^^
One of possible detection mechanisms is to use checksums for data structures used for monitors.

**Pros**

- Low performance overhead

**Cons**

- hard to implement for complex data structures that are mutated frequently

.. note::
   Not implemented due to complexity of implementation for complex data structures.

Protected pages
^^^^^^^^^^^^^^^^
Another possible detection mechanism is to use protected memory pages. Region before and after data structures used for monitors can be marked as non-accessible. 
Since other components do not have knowledge where internal structures were allocated, **likelihood** of memory corruption only in data structures used for monitors and not around them is **low**.

**Pros**

- Easy to implement
- 0 performance overhead

**Cons**

- Increased memory overhead
- Can detect only `pass through` corruption - ie. bulk write over memory area
- Protection can be disabled by malicious component however this shall be judged as **extremely low** likelihood because this requires knowledge of internal memory layout of Health Monitoring Library and code review approval.

Dual banking
^^^^^^^^^^^^^
Another possible detection mechanism is to use dual banking for data structures used for monitors. One bank is actively used, while the other one is kept on side
as either **mirrored data** or  **inverse byte copy**. During runtime, background checking thread will fetch both banks and compare them. If mismatch is detected, then memory corruption is detected.

.. note::
   This pattern can be extended to triple banking where voting can be used to **recover** corrupted data if needed.

**Pros**

- Can recover corrupted data if triple banking with voting is used
- Detects wide range of memory corruption patterns

**Cons**

- Increased memory overhead
- More complex internal implementation

Decision
=========
- Status: Accepted
- Date: 2026-01-09
   
After evaluation of above detection mechanisms, it was decided that **Health Monitoring Library** shall be implemented as library within monitored process as FMEA confirms safety goals are met.

Rationale
==========
- All code within process is developed according to **ASIL-B** development process
- Library will use **protected pages** mechanism to detect `pass through` memory corruption
- Lifecycle CFT will investigate possibility to harden memory protection using **ARM MTE** extension (`more here <https://learn.arm.com/learning-paths/mobile-graphics-and-gaming/mte/>`_ ) in future releases - https://github.com/eclipse-score/score/issues/2397


Failure Mode List
-----------------

.. code-block:: rst

    .. comp_saf_fmea:: <Title>
       :violates: <Component architecture>
       :id: comp_saf_fmea__<Component>__<Element descriptor>
       :fault_id: <ID from fault model :need:`gd_guidl__fault_models`>
       :failure_effect: "description of failure effect of the fault model on the element"
       :mitigated_by: <ID from Component Requirement | ID from AoU Component Requirement>
       :mitigation_issue: <ID from Issue Tracker>
       :sufficient: <yes|no>
       :status: <valid|invalid>

.. note::   argument is inside the 'content'. Therefore content is mandatory

.. attention::
    The above directive must be updated according to your component FMEA.

    - The above "code-block" directive must be updated
    - Fill in all the needed information in the <brackets>
