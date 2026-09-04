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

.. _lm_integration:

Integration
***********

Configuration
=============

The **Launch Manager** is currently configured with a single json file.
See :ref:`lm_config` and :ref:`lm_config_examples` for details about the **Launch Manager** configuration format.

.. _lm_integration_generating_flatbuffer_file:

Generating flatbuffer file
--------------------------

The json configuration has to be converted to a flatbuffer file before it can be used by the **Launch Manager**.

The provided bazel function ``launch_manager_config`` handles the compilation of the json configuration to flatbuffer files.

.. code-block:: python

   load("@score_lifecycle//:defs.bzl", "launch_manager_config")

   # This is your launch manager json configuration
   exports_files(["lm_config.json"])

   # Afterwards, you can refer to the generated flatbuffer files with :example_config_gen
   # The generated flatbuffer file has identical as the json configuration file, but with ".bin" file ending (example: "lm_config.bin")
   launch_manager_config(
       name ="example_config_gen",
       config="//scripts/config_mapping:lm_config.json"
   )

.. _lm_integration_location_of_configuration_file:

Location of the configuration file
----------------------------------

The **Launch Manager** can be started with a CLI flag defining the absolute or relative path to the configuration file:

``launch_manager -c <config path>``

Example: ``launch_manager -c /opt/launch_manager/etc/lm_config.bin``

If ``-c`` is not specified, the **Launch Manager** will attempt to load the configuration from the default path ``etc/launch_manager_config.bin``.
