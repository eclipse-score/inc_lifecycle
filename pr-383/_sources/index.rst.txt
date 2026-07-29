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

Lifecycle Documentation
=======================

Module / Feature Documentation
------------------------------

.. toctree::
   :maxdepth: 1

   docs/features/index
   docs/manuals/index
   docs/safety_mgt/index
   docs/security_mgt/index
   docs/release/index
   docs/verification_report/statistics

Component documentation
------------------------

.. toctree::
   :maxdepth: 1

   score/launch_manager/docs/index
   score/health_monitor/docs/index


.. _quick-start-building-testing:

Quick Start - Building and Testing
==================================

To build the entire module:

.. code-block:: bash

   bazel build //src/...

To run all tests:

.. code-block:: bash

   bazel test //...

To run only unit tests:

.. code-block:: bash

   bazel test //src/...

To run only component or feature integration tests:

.. code-block:: bash

   bazel test //tests/...