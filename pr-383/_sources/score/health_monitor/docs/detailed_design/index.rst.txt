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

.. _component_detailed_design_hm:

Detailed Design
###############

.. document:: HealthMonitor Detailed Design
   :id: doc__health_monitor_detailed_design
   :status: draft
   :safety: ASIL_B
   :security: NO
   :realizes: wp__sw_implementation
   :tags: template

.. attention::
    The above directive must be updated according to your Component.

    - Modify ``Your Component Name`` to be your Component Name
    - Modify ``id`` to be your Component Name in upper snake case preceded by ``doc__`` and followed by ``_detailed_design``
    - Adjust ``status`` to be ``valid``
    - Adjust ``safety`` and ``tags`` according to your needs

Detailed Design for Component: HealthMonitor
===============================================

Description
-----------

HealthMonitor component consists of these units:

- `deadline_monitor`
- `logic_monitor`
- `health_monitor`

| Design Decisions - For the documentation of the decision the :need:`gd_temp__change_decision_record` can be used.
| Design Constraints


Implementation structure
------------------------

.. code-block:: bash

    src
    └── health_monitoring_lib
        ├── cpp             # C++ API using Rust FFI
        │   ├── include     # C++ public header files.
        │   │   └── ...
        │   └── tests
        └── rust            # Rust implementation + FFI for C++
            └── deadline

Implementation approach for multi language
*******************************************

The implementation is done primarily in `Rust` language to leverage its memory safety features and concurrency model.
Due to a need to interface with existing C++ codebases, a C-compatible Foreign Function Interface (FFI) is provided by this `Rust` implementation.
`C++` API is designed to be idiomatic wrapping around the `Rust` FFI, ensuring ease of use for C++ developers while maintaining the safe usage.

Rationale Behind Decomposition into Units
******************************************

| mandatory: a motivation for the decomposition into one or more units.

.. note:: Reason for split into multiple units could be-
	    - Based on design principles like SOLID,DRY etc
	    - Based on design pattern's etc.

Static Diagrams for Unit Interactions
-------------------------------------

.. uml::  assets/static_diagram.puml

Dynamic Diagrams for Unit Interactions
--------------------------------------
.. code-block:: rst

   .. dd_dyn:: <Title>
      :id: dd_dyn__<Feature>__<Title>
      :security: <YES|NO>
      :safety: <QM|ASIL_B>
      :status: <valid|invalid>
      :implements: <link to component requirement id>
      :satisfies: <link to component architecture id>

        .. image:: <link to drawio image> or .. uml::  assets/<link to plantuml>
