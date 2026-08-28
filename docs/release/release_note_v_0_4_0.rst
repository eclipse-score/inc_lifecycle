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

.. document:: lifecycle Release Note v0.4.0
   :id: doc__lifecycle_release_note_v0_4_0
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__module_sw_release_note[version==1]
   :tags:


Release Note v0.4.0
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

- **Working directory support:** Support the ``deployment_config/working_dir`` value to set a process working directory. `#356 <https://github.com/eclipse-score/lifecycle/pull/356>`_

Improvements
~~~~~~~~~~~~

- **Lifecycle mock for OSS consumers:** Add ``MwLifeCycleManagerMock`` to allow unit tests to set expectations on ``report_running()`` and ``report_shutdown()`` without real IPC. `#391 <https://github.com/eclipse-score/lifecycle/pull/391>`_
- **CPU affinity up to 64 cores:** Extend the default CPU affinity mask from 32 to 64 cores. `#286 <https://github.com/eclipse-score/lifecycle/pull/286>`_
- **Internal refactoring:**
  - **Config simplification:** Removes the legacy flattbuffer Launch Manager config. `#409 <https://github.com/eclipse-score/lifecycle/pull/409>`_
  - **Remove the internal config adapter:** `#455 <https://github.com/eclipse-score/lifecycle/pull/455>`_
  - **Watchdog integration:** Refactor existing watchdog code into ``ProcessGroupManager`` and replace the custom watchdog OSAL with the baselibs OSAL. `#403 <https://github.com/eclipse-score/lifecycle/pull/403>`_
  - Various refactorings to improve code quality and maintainability.
- **Testing and documentation** updates, including QNX integration tests via QEMU. `#328 <https://github.com/eclipse-score/lifecycle/pull/328>`_

Bug Fixes
~~~~~~~~~

- Bug fix: Avoid deadlocks after ``fork()``. `#303 <https://github.com/eclipse-score/lifecycle/pull/303>`_
- Bug fix: Fix missed restart. `#416 <https://github.com/eclipse-score/lifecycle/pull/416>`_
- Bug fix: Lock ``IdentifierHash`` mutex before every ``get_registry`` call. `#384 <https://github.com/eclipse-score/lifecycle/pull/384>`_
- Bug fix: Add missing read protection. `#501 <https://github.com/eclipse-score/lifecycle/pull/501>`_

Other Changes by Label
~~~~~~~~~~~~~~~~~~~~~~

- None

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

- Update include paths and BUILD labels for public mocks. These changes introduce a consistent naming convention for the test doubles in the lifecycle repository. More information in `#422 <https://github.com/eclipse-score/lifecycle/issues/422>`_
   - Rename all usage of `//score/launch_manager:applicationcontext_mock_c` to `//score/launch_manager:mock_application_context_cc`.
   - Rename all usage of `//score/launch_manager:lifecycle_mock_cc` to `//score/launch_manager:mock_lifecycle_cc`.
   - Rename all usage of  `//score/launch_manager:report_running_mock_cc` to `//score/launch_manager:mock_report_running_cc`.
   - Rename all usage of `applicationcontextmock.h` to `mock_application_context.h`.
   - Rename all usage of `lifecyclemanagermock.h` to `mock_lifecycle_manager.h`.
   - Rename all usage of `report_running_mock.h` to `mock_report_running.h`.

- **C and Rust ``report_running`` APIs renamed** to match the C++ API. Update callers of the C/Rust APIs. `#318 <https://github.com/eclipse-score/lifecycle/pull/318>`_

- Backward compatibility with the previous release is not guaranteed.

*For any questions or support, please contact the* `Project Team <https://github.com/orgs/eclipse-score/projects/33>`_ *or raise an issue/discussion.*
