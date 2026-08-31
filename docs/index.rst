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

   features/index
   manuals/index
   module/index
   safety_mgt/index
   security_mgt/index
   release/index
   verification_report/index

Component documentation
------------------------

.. toctree::
   :maxdepth: 1

   components/index


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
