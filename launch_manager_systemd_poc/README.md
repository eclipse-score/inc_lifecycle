# Launch Manager Systemd POC

## Overview
This Proof-of-concept showcases multiple Rust applications (`appA`, `appB`, `appC`) managed via systemd services with controlled dependencies.

- **appA** depends on **appB**
- **appA** will be restarted if **appB** crashes
- **appB** depends on **appC**  
- **appB** will be restarted if **appC** crashes
- **appC** represents the actual use-case application (e.g., `app_crash` or `report_error`)

Dependencies are enforced in start order, but **dependent services are not auto-started**. 
Instead, services fail if their dependency is not running. 
Watchdogs are used to monitor app health.

## Directory Structure

```
health_monitoring/
├── alive_monitor
├── launch_manager_systemd_poc
│   ├── examples
│   │   ├── app_crash
│   │   ├── report_error
│   │   └── common
│   │       ├── appA
│   │       ├── appB
│   │       └── utils
├── Cargo.toml
└── README.md
```

## Deployment

A bash script located at `health_monitoring/launch_manager_systemd_poc/deploy.sh` handles:

1. Building the rust projects in release or debug mode (debug mode by default)
2. Creating the directories with correct ownership, users and groups they don't exist 
3. Copying binaries to `/opt/[app_name]/bin/`  
4. Copying systemd service files and drop-ins to `/etc/systemd/system/`
5. Launching the services via `systemctl start app*.service`
6. Stopping the services via `systemctl stop app*.service` after 20s

Example:

```bash
sudo ./deploy.sh app_crash
```

- `app_crash` -> deployed as `appC`  
- `appB` and `appA` binaries also deployed according to dependencies

Ownership:
| App  | User       | Group              |
|------|------------|--------------------|
| appA | scoreuser  | scoregroup_asil_qm |
| appB | scoreuser  | scoregroup_asil_b  |
| appC | scoreuser  | scoregroup_asil_b  |

## Systemd Service Behavior
- Type: `notify` (services notify systemd when ready)  
- Watchdog: `5s` for each service  
- Restart policy: `on-failure`  
- Dependency enforcement **without auto-start** is done via `ExecStartPre` checks (if dependency is missing, service fails immediately) 
- Drop-ins (`*.service.d/`) are used for additional service constraints such as `CPUAffinity`, `NoNewPrivileges`, `ProtectSystem`, and capabilities.

Example cascade:

```
appC fails -> appB fails -> appA fails
```

- Services **do not auto-start dependencies** but the dependant services **will** be restarted if a specific service fails
- Manual start: `sudo systemctl start appA.service` will fail if `appB` is not running  

## Notes
- `ExecStartPre` is used to enforce the "dependency must be running" requirement without auto-starting.  
- Watchdog timers ensure health monitoring for all apps.  

