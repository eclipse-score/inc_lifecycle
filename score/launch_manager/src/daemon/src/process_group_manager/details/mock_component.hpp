#ifndef MOCK_COMPONENT_HPP_INCLUDED
#define MOCK_COMPONENT_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal
{

class MockComponent : public IComponent
{
  public:
    MOCK_METHOD(RequestResult, activate, (score::cpp::stop_token stop_token), (override));
    MOCK_METHOD(RequestResult, deactivate, (score::cpp::stop_token stop_token), (override));
    MOCK_METHOD(RequestResult, tryHandleTermination, (int32_t status), (override));
    MOCK_METHOD(uint32_t, getIndex, (), (override, const));
    MOCK_METHOD(bool, active, (), (override, const));
};

}  // namespace score::mw::lifecycle::internal

#endif