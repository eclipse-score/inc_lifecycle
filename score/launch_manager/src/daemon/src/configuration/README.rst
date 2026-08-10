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


Launch Manager Configuration Schema
###################################

This folder contains the **Launch Manager** configuration JSON Schema. The schema defines and validates the structure of **Launch Manager** configuration files, ensuring consistency and correctness across different deployments.

Overview
********

This project manages the **Launch Manager** configuration schema as a single, self-contained file. When you need to modify or extend the schema, you should directly edit ``launch_manager.schema.json``.

**Project Structure:**

The project is structured to provide a clear organization for the schema, default values, examples, and documentation.

::

    .
    |-- default_values/             # Default values for various configuration aspects.
    |-- docs/                       # Comprehensive documentation for Launch Manager configuration.
    |-- examples/                   # Illustrative example configuration files for the schema.
    |-- launch_manager.schema.json  # The Launch Manager schema definition.
    `-- scripts/                    # Utility scripts, including a validation tool.

Quick Start
***********

For Users and Developers
========================

Whether you are validating a **Launch Manager** configuration against the schema or actively developing and modifying the schema itself, here is how to interact with this project:

1. **Locate the Schema:** The complete schema definition resides in ``launch_manager.schema.json``. This file is the authoritative source for defining the structure of **Launch Manager** configurations.
2. **Explore Examples:** The ``examples/`` folder provides various sample **Launch Manager** configuration files. These examples are invaluable for understanding how the schema applies in practice and how to structure your own configurations.
3. **Validate Your Configuration:** Use the provided validation script to check if your configuration file conforms to the schema:

    .. code-block:: bash

       scripts/validate.py --schema launch_manager.schema.json --instance your_config.json

Documentation
*************

The ``docs`` folder contains comprehensive documentation related to the **Launch Manager** configuration. It is highly recommended to consult these resources for detailed explanations and advanced usage patterns.

* `launch_manager_configuration.rst <docs/launch_manager_configuration.rst>`_: Provides in-depth information about configuring the **Launch Manager**.
* `examples.rst <docs/examples.rst>`_: Offers further explanations and context for the examples provided in the ``examples/`` directory.

Default Values
**************

The ``default_values`` folder contains JSON files specifying default settings for various components of the **Launch Manager**. These defaults are crucial for understanding the baseline behavior of the system and for configuring specific aspects like **Run Target** settings.

Examples
********

The ``examples`` folder contains a set of sample **Launch Manager** configuration files. Each example demonstrates valid configurations according to the ``launch_manager.schema.json``.

**Recommendation:** If you are new to **Launch Manager** configurations, **start by reviewing these examples**. They offer practical insight into the expected structure, available properties, and common use cases defined by the schema.

Scripts
*******

The ``scripts`` folder houses utility scripts designed to assist with schema development and validation.

validate.py
===========

The ``validate.py`` script is a crucial tool for verifying that any given **Launch Manager** configuration instance adheres to the rules defined in ``launch_manager.schema.json``.

**Usage:**

To validate a configuration file (e.g., ``example_conf.json`` from the ``examples`` folder) against the schema:

.. code-block:: bash

   scripts/validate.py --schema launch_manager.schema.json --instance examples/example_conf.json
   Success --> examples/example_conf.json: valid

**When to Use:**

* **During Development:** Run this script frequently whenever you are creating or modifying a **Launch Manager** configuration file. It provides immediate feedback on whether your changes are valid according to the schema.
* **Schema Development:** If you are making changes to ``launch_manager.schema.json`` itself, always run ``validate.py`` against the examples to ensure your schema changes have not inadvertently broken existing, valid configurations. This step is vital for maintaining backward compatibility and preventing regressions.

Typical Workflow
****************

For schema developers or those creating new configurations:

1. **Modify** the ``launch_manager.schema.json`` file (if you are updating the schema definition) or your **Launch Manager** configuration file.
2. **Validate** your changes using the ``scripts/validate.py`` script against relevant example files or your new configuration. This iterative process helps ensure compliance and catch errors early, contributing to a robust configuration system.
