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

e.g.
```cpp
// Fine, make_shared<T> says what it is.
auto something = std::make_shared<int>(1);

// Fine, It's understood that T::Create() creates a Result<T>.
auto something = SomeType::Create();

// Not allowed, you'd have to look at the definition of the method to see what
// the type is.
auto something = someMethod();
```

## No Yoda Conditions

```cpp
if (42 == value){}  // Bad

if (value == 42){} // Good
```
