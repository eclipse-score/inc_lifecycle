# Application Lifecycle

## Introduction
Purpose of this component is to have a layer of abstraction, that unifies API, for execution managers and applications so in case of replacing lifecycle manager there will be no need to change applications.

> This component has `ASIL-B` safety level.

## External interfaces 

Component consists of three main parts:
* Application Base class `score::mw::lifecycle::Application` from which application developers shall subclass their own application.
* Lifecycle manager class `score::mw::lifecycle::LifecycleManager` see [lifecyclemanager.md](lifecyclemanager.md) file.
* Runner class `score::mw::lifecycle::Run` which instantiates the user provided application and decides which LifeCycleManager to use.

Overview on how lifecycle application is related to execution manager.
![Lifecycle Manager Application Overview](model/lifecycle_system_view.svg)

### AAS Interfaces
N/A

### External C++ interfaces 
`score:mw::Application` methods which have to be implemented by the application:
#### Initialize
This method shall do basic initialization of application (what was not done in application State Initializing). 
Method returns a `Result`, which either contains `void` on success or an error. In case of an error lifecycle manager will join all running threads and will return with non zero value.  
Input parameter to this method is an instance of `ApplicationContext`, which is a  wrapper around the arguments, which have been given to `main`.

#### Run
This method implements the `Run` state of the app (see State `Run`). This could be a long running
functionality. In case `Run()` returns, this implicitly means, that the app has ended. If
app implementations do spawn some worker threads in the context of `Run()`, those threads
are joined again before` Run()` returns.
To be able to terminate an application `Run()` method from outside asynchronously in a cooperative manner, the `Run()` method
gets a `stop_token`, where it shall synchronize/listen on (see concepts in https://en.cppreference.com/w/cpp/thread/stop_token)
This `stop_token` is controlled by an external `stop_source` owned by the `LifecycleManager`.

ApplicationContext class represents cmd line arguments of an Application that are passed during `Initialize` function call.
#### get_argument
Function gets a argument name as a string and returns it's value if it exists, otherwise empty string is returned.
#### get_arguments
Returns list of cmd line arguments represented as a vector.
Example code:

    std::array<std::string, 3> in_args{};
    if (context.get_arguments().empty())
    {
      score::mw::log::LogError() << "Incorrect arguments given\n";
    } else
    {
      context.get_argument("-i", in_args[0]);
      if (in_args[0].empty()) 
      {
        score::mw::log::LogError() << "Tsync Incorrect arguments given\n";
      } else 
      {
        // argument is in in_args[0] to be used by application
      }
    }

#### run_application template function  
`run_application` function abstracts initialization and running of an application with LifeCycleManager. This function should be used to start application, it take command line arguments as input parameters

### Hardware interfaces
N/A
### Interrupts
N/A
### File system
N/A
### POSIX signals
SIGTERM see [lifecyclemanager.md](lifecyclemanager.md)

## Static architecture
![Structural View](model/structural_view.svg)

## Dynamic architecture

### Activity sequencing
The following sequence diagram shows the interaction between OS and instances of `Application` class, which were
decorated with a `LifecycleManager`:

![Sequence View](model/sequence_view.svg)

### Stateful behavior

The following state machine depicts the states/transitions of an application:

![Application Lifecycle](model/app_lifecycle.svg)

#### State `Terminated` respectively `Not Started`

This is the initial respectively final state of an application, before its executable has been started (forked, executed) and after it
has terminated.

#### State `Initializing`

This state is automatically entered upon start of the executable (fork + exec). It is left towards state `Run`
if the application decides to do so via an active/explicit reporting to the platform, that it has entered `Run`.

##### What shall/can be done in this state
* Initialize internal variables/memory
* Access persistent storage to read/lookup configuration
* Search for/start searching for other service instances to be used to provide application services
  * Note: These operations must not be performed in a blocking manner! Otherwise the predefined maximum amount of time to be spent in machine state `Initializing` might be exceeded and the ECU will restart immediately once that becomes the case.
* access remote service instances if available
  * Also here, keep in mind the note from above!
* access to low-level platform services, which are expected to be available, when an application starts.
* call to exit. F.i. if initialization can't conclude.

##### What must not be done in this state
* Offer any service instance. Reason: If it would offer a service instance, an instant service call to this instance
  could happen and would then be served during state `Initializing`, which isn't acceptable.
* Generally: Providing/Executing main application logic, which is assigned to `Run` state.

#### State `Run`

As mentioned above: This state is entered, when the application reports `Run` state to the platform.
An application should only enter this state, if it is able to provide its core/base functionality. If it isn't able to
do so, it shall not report `Run` at all.

##### Example of what could be done in this state
* Assign invalid/dummy values as defaults to the fields/events of service(s) to be offered
* Offer service(s)
* Prepare DTCs
* Wait for dependent services to become available via their generated proxy classes
* connect to/lookup other required resources
* run core logic

## Variability
* N/A

### Deployment
* There is example app available that can help with deploying this library see example application (`examples/cpp_lifecycle_app`) subdirectory
