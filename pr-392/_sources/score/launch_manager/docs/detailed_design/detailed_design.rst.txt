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

Example: Detailed Design
========================

.. document:: Launch Manager Detailed Design
   :id: doc__launch_manager_detailed_design
   :status: draft
   :safety: ASIL_B
   :security: NO
   :realizes: wp__sw_implementation


Detailed Design for Component: Launch Manager
=============================================

Description
-----------

| Design Decisions - For the documentation of the decision the :need:`gd_temp__change_decision_record` can be used.
| Design Constraints

Example:

 - component is split into two units unit1 and unit2 based on single responsibility principle.
 - unit2 is injected to unit1 one via dependency injection for testability.

Rationale Behind Decomposition into Units
******************************************
| mandatory: a motivation for the decomposition into one or more units.

.. note:: Reason for split into multiple units could be-
	    - Based on design principles like SOLID,DRY etc
	    - Based on design pattern's etc.

Static Diagrams for Unit Interactions
-------------------------------------

A static view provides an overview of the units and their relationships using
UML 2.0 notations (e.g. class diagrams, component diagrams). Use ``.. uml::``
or ``.. image::`` directives to include the diagram.


Dynamic Diagrams for Unit Interactions (optional)
--------------------------------------------------

A dynamic view illustrates how the units within a component interact over their
interfaces to fulfill a specific use case or functionality. It is optional when the
component's behaviour is straightforward and can be understood from the static view
and interface documentation alone.

Use standard UML behavioural diagrams (sequence diagrams, state machine diagrams)
with ``.. uml::`` or ``.. image::`` directives.


Units within the Component
--------------------------

The relationship between a unit and its parent component is established implicitly
through the file path. Each component has its own directory, and units residing
within that directory belong to it. The unit's attributes and behaviour are documented
in the source code itself. A separate static diagram per unit is not required.

Interface documentation of a software unit is part of the source code (e.g. public
API headers, trait definitions, or documented function signatures).

Example:

- unit1: implements the main logic (see source code for details)
- unit2: injected into unit1 via dependency injection for testability
