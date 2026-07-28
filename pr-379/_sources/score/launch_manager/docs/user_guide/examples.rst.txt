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

Configuration Examples
======================

This document provides a set of configuration examples that come with the
**Launch Manager** schema. These examples are designed to show common ways to
use **Launch Manager** and to demonstrate how configurations should be
structured. They serve as practical guides, helping users understand how to
apply various features effectively.

`example_conf.json`
-------------------

.. dropdown:: example_conf.json

   .. literalinclude:: ../../../../score/launch_manager/src/daemon/src/configuration/config_schema/examples/example_conf.json
      :language: json

The `example_conf.json` file is a fundamental example within the **Launch
Manager** system. While it presents a relatively simple setup, its main goal is
to clearly show the key ideas behind defining a **Run Target**, specifying its
individual **Components**, and setting up how they depend on each other.

Users can examine this basic example to understand the core principles. These
principles can then be scaled and applied to build much more elaborate **Launch
Manager** configurations, effectively managing even the most complex
application setups.

A high-level overview of this configuration, which highlights its dependencies,
is shown below:


.. figure:: images/example_conf_graph.svg
   :alt: Graphical representation of example configuration
   :width: 70%
   :align: center

