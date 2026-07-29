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

AoU Component Requirements Template
===================================

.. document:: Launch Manager Component AoU
   :id: doc__launch_manager_comp_aou
   :status: draft
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_comp_aou


This page contains Assumption of Use requirement snippets that belong to the
template repository.

Component AoU
-------------

.. code-block:: rst

   .. aou_req:: Next Title
      :id: aou_req__mod_temp_component_name__next_title
      :reqtype: Process
      :security: YES
      :safety: ASIL_B
      :status: invalid

      The Component User shall do xyz to use the component safely/securely

   .. aou_req:: Another Title
      :id: aou_req__mod_temp_component_name__another
      :reqtype: Process
      :security: YES
      :safety: ASIL_B
      :status: invalid
      :tags: environment

      The Component shall only be used in a xyz environment to ensure its proper functioning.
