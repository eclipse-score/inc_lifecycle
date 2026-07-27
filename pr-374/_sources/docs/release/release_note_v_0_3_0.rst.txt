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

.. document:: lifecycle Release Note v0.3.0
   :id: doc__lifecycle_release_note_v0_3_0
   :status: valid
   :safety: ASIL_B
   :security: NO
   :realizes: wp__module_sw_release_note
   :tags: 


Release Note v0.3.0
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

- **New report running API** Introduce low-level ``void report_running()`` funciton so applications can report a Running state without requiring usage of the full mw::Lifecycle API.

Improvements
~~~~~~~~~~~~

- **Quality improvements:** testing and documentation updates.
- **README improvements:** Updated README.md and removed outdated contents.

Bug Fixes
~~~~~~~~~

- Bug fix: `Spurious recovery action trigger on first run target activation <https://github.com/eclipse-score/lifecycle/issues/198>`_
- Bug fix: `Launching a component with a missing binary <https://github.com/eclipse-score/lifecycle/issues/261>`_
- Bug fix: `Spurious abort for process launch before retries are exhausted. <https://github.com/eclipse-score/lifecycle/issues/284>`_
- Bug fix: `Possible crashes when using HealthMonitor due to stack-use-after-scope in baselibs rust logging. <https://github.com/eclipse-score/baselibs/issues/253>`_

Other Changes by Label
~~~~~~~~~~~~~~~~~~~~~~

- Reorganize repo folder structure `more info <https://github.com/eclipse-score/lifecycle/issues/210>`_

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

- None

Known Vulnerabilities
~~~~~~~~~~~~~~~~~~~~~

- None

Upgrade Instructions
~~~~~~~~~~~~~~~~~~~~

- Align public API namespaces to score::mw::lifecycle and score::mw::health
   - More information in `Pull request 212 <https://github.com/eclipse-score/lifecycle/pull/212>`_

- Rename current "Monitor" API in LaunchManager to "Alive"
   - All public bazel targets are renamed. new targets are listed in Readme.md
   - More information in `Pull request 229 <https://github.com/eclipse-score/lifecycle/pull/229>`_

- Backward compatibility with the previous release is not guaranteed.

*For any questions or support, please contact the* `Project Team <https://github.com/orgs/eclipse-score/projects/33>`_ *or raise an issue/discussion.*
