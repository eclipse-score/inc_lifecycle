# Lifecyclemanager class

## Introduction
Purpose of `score::mw::lifecycle::LifecycleManager` component is to have a layer of abstraction, that unifies API, for execution managers.

> This component has `ASIL-B` safety level.

## External C++ interfaces 
`LifecycleManager` class is a decoration of [score::mw::lifecycle::Application](lifecycle.md) class. It adds POSIX signal handling for SIGTERM signal, for decorated `Application`. 

Methods which have to be implemented in case of adding new lifecycle manager:
### report_running
Hook function for reporting running state in lifecycle manager
### report_shutdown
Hook function for reporting shutdown state in lifecycle manager

### POSIX signals
#### SIGTERM 
Lifecycle manager `handle_signal` function waits for SIGTERM signal, if signal is received it's shuts down application, reports shutdown to execution manager and exits itself.
