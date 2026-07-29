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

Security Plan Formal Review Report
==================================

.. document:: Lifecycle Security Plan Formal Review
  :id: doc__lifecycle_security_plan_fdr
  :status: draft
  :safety: ASIL_B
  :security: YES
  :realizes: wp__fdr_reports


**1. Purpose**

The purpose of this review checklist is to provide a guidence for reviewing the Security Plans for each module.
Each Module Security Plan shall have one checklist filled.

**2. Checklist**

See also the review concept for further information about reviews in general and inspection in particular.

.. list-table:: Security Plan Checklist
        :header-rows: 1

        * - Id
          - Security Plan activity
          - Compliant to ISO SAE 21434?
          - Comment

        * - 1
          - Is the rationale for the Security Work Products tailoring included?
          - [YES | NO ]
          - <Rationale for result>

        * - 2
          - Is impact analysis planned in case of re-use of SW (needed for every release following the first formal release)?
          - [YES | NO ]
          - <Rationale for result>

        * - 3
          - Does the Security Plan define all needed activities for security management (including review and security audit)?
          - [YES | NO ]
          - <Rationale for result>

        * - 4
          - Does the Security Plan define all needed activities for SW development, integration and verification?
          - [YES | NO ]
          - <Rationale for result>

        * - 5
          - Does the Security Plan define all needed activities for security analysis?
          - [YES | NO ]
          - <Rationale for result>

        * - 6
          - Does the Security Plan define all needed activities for supporting processes (incl. tool mgt)?
          - [YES | NO ]
          - <Rationale for result>

        * - 7
          - Does the Security Plan document a responsible for all activities?
          - [YES | NO ]
          - <Rationale for result>

        * - 8
          - If OSS software components is used, is it planned to be qualified?
          - [YES | NO ]
          - <Rationale for result>

        * - 9
          - Is a Security Manager and a Project Lead appointed for the project?
          - [YES | NO ]
          - <Rationale for result>

        * - 10
          - Is Security Plan sufficiently linked to the Project Plan?
          - [YES | NO ]
          - <Rationale for result>

        * - 11
          - Is Security Plan updated iteratively to show the progress?
          - [YES | NO ]
          - <Rationale for result>

        * - 12
          - If Out-of-context software components is used, are the assumptions documented?
          - [YES | NO ]
          - <Rationale for result>

        * - 13
          - Does the Security Plan define all needed activities for SBOM generation?
          - [YES | NO ]
          - <Rationale for result>

        * - 14
          - Does the Security Plan define regular vulnerability scans for the generated SBOM?
          - [YES | NO ]
          - <Rationale for result>

.. note::
    Off-the-shelf means existing software which may used without modification, e.g. existing OSS
