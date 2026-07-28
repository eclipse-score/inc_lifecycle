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

Known Limitations
*****************

Configuration
=============

Component
---------

* The sandbox parameters ``max_memory_usage`` and ``max_cpu_usage`` are
  currently not supported and are ignored.
* For ReadyCondition ``process_state:Terminated``, the mapping is only
  supported for Components that have at least one Component depending on it.
* The ``ready_recovery_action`` only supports the RecoveryAction of type
  ``restart``. The parameter ``delay_before_restart`` is currently not
  supported and is ignored. Setting it to a non-zero value will have no effect
  and the component will be restarted immediately.
* The ``recovery_action`` only supports ``switch_run_target`` with the
  ``run_target`` set to ``fallback_run_target``.
* The ``ready_timeout`` is used as the timeout until process state Running is
  reached, even in case the ReadyCondition is ``process_state:Terminated``.
* The parameter ``deployment_config/working_dir`` is currently not supported
  and is ignored.

Run target
----------

* The initial Run Target must be named ``Startup`` and the
  ``initial_run_target`` must be configured to ``Startup``.
* The parameter ``run_targets/<RunTarget>/transition_timeout`` is currently not
  supported and is ignored.
* The ``recovery_action`` only supports ``switch_run_target`` with the
  ``run_target`` set to ``fallback_run_target``.
