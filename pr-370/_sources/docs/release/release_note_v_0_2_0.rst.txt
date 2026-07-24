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

.. document:: lifecycle Release Note v0.2.0
   :id: doc__lifecycle_release_note_v0_2_0
   :status: valid
   :safety: ASIL_B
   :security: NO
   :realizes: wp__module_sw_release_note
   :tags: 


Release Note v0.2.0
===================

Overview
--------

This document provides an overview of the changes, improvements, and bug fixes included in the software module release version named above
as compared to the module's origin release (which is usually the previous release).

Disclaimer
----------

This release note does not "release for production", as it does not come with a safety argumentation and a performed safety assessment.
The work products compiled in the safety package are created with care according to a process satisfying standards, but the as the project,
being a non-profit and open source organization, can not take over any liability for its content.

Changes to the Module
---------------------

New Features
~~~~~~~~~~~~

- **New Configuration Schema:** Introduce first version of JSON schema intended for Launch Manager configuration.
- **Heartbeat monitor API:** Health monitoring library provides heartbeat monitor API.
- **Logic monitor API:** Health monitoring library provides logic monitor API.

Improvements
~~~~~~~~~~~~

- **Quality improvements:** testing and documentation updates.

Bug Fixes
~~~~~~~~~

- 

Other Changes by Label
~~~~~~~~~~~~~~~~~~~~~~

- 

Compatibility
~~~~~~~~~~~~~

- The following platforms are supported using the `bazel_cpp_toolchains <https://github.com/eclipse-score/bazel_cpp_toolchains>`_:

  - `x86_64-unknown-linux-gnu`
  - `aarch64-unknown-linux-gnu`
  - `x86_64-unknown-nto-qnx800`
  - `aarch64-unknown-nto-qnx800`

Performed Verification
~~~~~~~~~~~~~~~~~~~~~~

- Build on all supported platforms
- Unit test execution on all supported platforms

Known Issues
~~~~~~~~~~~~

- 

Known Vulnerabilities
~~~~~~~~~~~~~~~~~~~~~

- 

Upgrade Instructions
~~~~~~~~~~~~~~~~~~~~

- Backward compatibility with the previous release is not guaranteed.

*For any questions or support, please contact the* `Project Team <https://github.com/orgs/eclipse-score/projects/33>`_ *or raise an issue/discussion.*
