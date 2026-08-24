<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Lifecycle Coding Guidelines

## `Create` Method

When a class can fail in the constructor create a static Create method that
returns a `Result<T>`.

## `auto` Usage

Only use `auto` when it is obvious what the type will be.

1. The full expression already specifies the type.
   ```cpp
   auto something = std::make_shared<int>(1);
   ```
1. The right hand side of the assignment is a standard (ISO C++, POSIX, SCORE)
   function.
   ```cpp
    // Fine, this is a std function so it's understood what the types are.
    std::unordered_map<int, std::string> data {};
    auto res = data.insert(...);

    // Fine, same as above
    std::vector<int> other_data {}
    for(auto& val: other_data)
    {...}
   ```
1. Creating a lambda function.
1. The type wraps another type, and is only used to check validity before
   unwrapping to an object of another type.
   ```cpp
    auto something_res = SomeType::Create();
    if (!something_res)
    {...}
    SomeType something = something_res.value();
   ```

Not allowed:
```cpp
// Not allowed, you'd have to look at the definition of the method to see what
// the type is.
auto something = someMethod();
```

## No Yoda Conditions

https://en.wikipedia.org/wiki/Yoda_conditions#Criticism

```cpp
if (42 == value){}  // Bad

if (value == 42){} // Good
```

## Namespaces

For the namespace you shall use the following

```
score/
├── health_monitor       // namespace score::mw::health
│   └── src
│       └── cpp          // Public API score::mw::health
│           └── details  // Private API score::mw::health::internal::<component name>
└── launch_manager       // namespace score::mw::lifecycle
    └── src
        └── alive        // Public API namespace score::mw::lifecycle
            └── details  // Private API namespace score::mw::lifecycle::internal::<component name>
```

## Class Mocking

The projects chosen method of mocking is dependency injection.
And so all classes shall be designed such that they allow injecting mocks
classes.

## Bazel Visibility & Folder Structure

The following rules shall be followed:

1. Component directory (e.g. `osal`) can be visible to any target **inside**
   the module.
1. The visiblity in the Component directory shall be as strict as
   possible.
1. The `details` directory shall only be visible to the parent component.

```
score/launch_manager/src/daemon/src/
└── osal   <- visibility = ["//score:__subpackages__"],
    └── details   <- visibility = ["//score/launch_manager/src/daemon/src/osal:__subpackages__"],
```

## File Naming Conventions

* All mocks shall be called `mock_<unit>.hpp`.
* Headers with an interface shall have the `i` prefix. e.g. `icomponent.hpp`
