/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License Version 2.0 which is available at
* https://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/
// (c) 2025 ETAS GmbH. All rights reserved.
#ifndef OBSERVER_HPP_INCLUDED
#define OBSERVER_HPP_INCLUDED

#include <algorithm>
#include <cassert>
#include <cstdint>

#include <vector>

namespace etas
{
namespace vrte
{
namespace saf
{
namespace common
{

/// @brief Observer template class
/// @details Template implementation of the "Observer" design pattern, The pattern consists of two classes
///          (Observer & Observable).
///          In the Observer pattern the "Observable" object notifies the attached "Observer" object/s of an update.
/// @todo The current implementation of Observer/Observable class doesn't fully support move construction.
///       If the Observer object is move constructed, the links from the Observable class will be invalid.
///       This limitation should be resolved.
template <typename Type_Observable>
class Observer
{
public:
    /// @brief Constructor
    Observer() = default;

    /// @brief Destructor
    virtual ~Observer() = default;

    /// @brief No Copy Constructor
    Observer(const Observer&) = delete;
    /// @brief No Copy Assignment
    Observer& operator=(const Observer&) = delete;
    /// @brief No Move Assignment
    Observer& operator=(Observer&&) = delete;

    /// @brief Update Data
    /// @details Update method to be called by the observed object to receive updates.
    /// @param [in]  f_observable_r     Observable as reference.
    virtual void updateData(const Type_Observable& f_observable_r) noexcept(true) = 0;

protected:
    /// @brief Move Constructor
    Observer(Observer&&) = default;
};

/// @brief Observable template class
/// @details Template implementation of the "OBSERVER DESIGN PATTERN". The pattern consists of two classes
///          (Observer & Observable).
///          In this pattern the "Observable" object notifies the attached "Observer" object/s of updates in a callback.
/// @todo The current implementation of Observer/Observable class doesn't fully support move construction.
///       If the Observer object is move constructed, the links from the Observable class will be invalid.
///       This limitation should be resolved.
template <typename Type_Observable>
class Observable
{
public:
    /// @brief Default Constructor
    Observable(void) = default;

    /// @brief Default Destructor
    virtual ~Observable() = default;

    /// @brief No Copy Constructor
    Observable(const Observable&) = delete;
    /// @brief No Copy Assignment
    Observable& operator=(const Observable&) = delete;
    /// @brief No Move Assignment
    Observable& operator=(Observable&&) = delete;

    /// @brief Attach Observer
    /// @details Attaches an observer to the Observable object to receive updates.
    /// @param [in] f_observer_r      Observer object reference to be added to the observers array
    /// @warning    Attach may throw std::exceptions
    void attachObserver(Observer<Type_Observable>& f_observer_r) noexcept(false)
    {
        observers.push_back(&f_observer_r);
    }

    /// @brief Detach Observer
    /// @details detaches an observer from the Observable object if it was previously attached
    /// @param [in] f_observer_r      Observer object reference to be removed from the observers array
    void detachObserver(Observer<Type_Observable>& f_observer_r) noexcept(false)
    {
        // cppcheck-suppress uselessCallsRemove
        const auto eraseFirstIt{std::remove(observers.begin(), observers.end(), &f_observer_r)};
        const auto eraseFirstItConst{
            static_cast<typename std::vector<Observer<Type_Observable>*>::const_iterator>(eraseFirstIt)};
        observers.erase(eraseFirstItConst, observers.cend());
    }

protected:
    /// @brief Move Constructor
    /// Cannot be noexcept, since the std::vector move constructor is not noexcept
    Observable(Observable&&) = default;

    /// @brief Push Results To Observers
    /// @details Send updates to all attached observers.
    void pushResultToObservers() noexcept(true)
    {
        for (auto& observer : observers)
        {
            // We can be sure that *this is of type Type_Observable, anything else would be a programming error.
            // The runtime checks performed by dynamic_cast are not necessary.
            assert((dynamic_cast<Type_Observable*>(this)) != NULL);
            observer->updateData(static_cast<Type_Observable&>(*this));
        }
    }

private:
    /// Observers attached to the observable object
    std::vector<Observer<Type_Observable>*> observers{};
};

}  // namespace common
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
