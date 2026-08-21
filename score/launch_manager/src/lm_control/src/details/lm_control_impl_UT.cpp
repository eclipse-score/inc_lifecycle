/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "score/mw/lifecycle/details/lm_control_impl.hpp"

namespace score::mw::lifecycle::internal
{
namespace
{

using ::testing::_;
using ::testing::AllOf;
using ::testing::DoAll;
using ::testing::Field;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

// Opaque stand-ins for the mw::com discovery handle types.
struct FakeHandle
{
};
struct FakeFindHandle
{
};

using FindHandler = std::function<void(score::mw::com::ServiceHandleContainer<FakeHandle>, FakeFindHandle)>;

/// @brief The single mock all framework calls are routed to. Owned by the test.
class MwComMock
{
  public:
    MOCK_METHOD(score::Result<FakeFindHandle>, StartFindService, (FindHandler handler));
    MOCK_METHOD(score::Result<void>, StopFindService, ());
    MOCK_METHOD(score::Result<void>, CreateProxy, ());

    MOCK_METHOD(score::Result<void>, Subscribe, (std::size_t max_sample_count));
    MOCK_METHOD(score::Result<void>, SetReceiveHandler, (std::function<void()> handler));
    MOCK_METHOD(void, Unsubscribe, ());
    MOCK_METHOD((score::Result<std::vector<ActivationResult>>), GetNewSamples, (std::size_t max_count));

    MOCK_METHOD(score::Result<ActivateRunTargetResponse>, ActivateRunTarget, (const ActivateRunTargetRequest& request));
    MOCK_METHOD(score::Result<GetActiveRunTargetResponse>, GetActiveRunTarget, ());
};

/// @brief Delegating adapter for LmControlProxy::activation_result.
struct FakeActivationEvent
{
    MwComMock* mock;

    // Mirrors mw::com: a single GetNewSamples() call never hands out more than the
    // max_sample_count agreed in Subscribe(), no matter how large the backlog is.
    std::size_t subscribed_max_count{0U};

    score::Result<void> Subscribe(std::size_t max_sample_count)
    {
        auto result = mock->Subscribe(max_sample_count);
        if (result.has_value())
        {
            subscribed_max_count = max_sample_count;
        }
        return result;
    }

    score::Result<void> SetReceiveHandler(std::function<void()> handler)
    {
        return mock->SetReceiveHandler(std::move(handler));
    }

    void Unsubscribe()
    {
        mock->Unsubscribe();
    }

    template <typename Callback>
    score::Result<std::size_t> GetNewSamples(Callback&& callback, std::size_t max_count)
    {
        const std::size_t effective_max_count = std::min(max_count, subscribed_max_count);
        auto samples_result = mock->GetNewSamples(effective_max_count);
        if (!samples_result.has_value())
        {
            return score::MakeUnexpected(ExecErrc::kCommunicationError);
        }
        auto samples = std::move(samples_result).value();

        // Uphold the mw::com contract even if a test stubs a larger batch than requested.
        const std::size_t delivered = std::min(samples.size(), effective_max_count);
        for (std::size_t i = 0U; i < delivered; ++i)
        {
            callback(&samples[i]);
        }
        return delivered;
    }
};

/// @brief Delegating adapter for LmControlProxy.
struct FakeProxy
{
    MwComMock* mock;
    FakeActivationEvent activation_result;

    explicit FakeProxy(MwComMock* m) : mock{m}, activation_result{m}
    {
    }

    score::Result<std::shared_ptr<ActivateRunTargetResponse>> activate_run_target(const ActivateRunTargetRequest& req)
    {
        auto result = mock->ActivateRunTarget(req);
        if (!result.has_value())
        {
            return score::MakeUnexpected(ExecErrc::kCommunicationError);
        }
        return std::make_shared<ActivateRunTargetResponse>(std::move(result).value());
    }

    score::Result<std::shared_ptr<GetActiveRunTargetResponse>> get_active_run_target()
    {
        auto result = mock->GetActiveRunTarget();
        if (!result.has_value())
        {
            return score::MakeUnexpected(ExecErrc::kCommunicationError);
        }
        return std::make_shared<GetActiveRunTargetResponse>(std::move(result).value());
    }
};

/// @brief Compile-time seam to inject the fake proxy into BasicLmControlImpl and to mock mw::com API calls.
struct FakeProxyTraits
{
    using Proxy = FakeProxy;
    using HandleType = FakeHandle;
    using FindServiceHandle = FakeFindHandle;

    /// @brief Stand-ins for mw::com's pointer-like wrappers, see MwComProxyTraits.
    template <typename ResponseType>
    using MethodResultPtr = std::shared_ptr<ResponseType>;
    template <typename SampleType>
    using SamplePtr = SampleType*;

    static inline MwComMock* mock{nullptr};

    template <typename Handler>
    static score::Result<FindServiceHandle> StartFindService(Handler&& handler, score::mw::com::InstanceSpecifier)
    {
        return mock->StartFindService(FindHandler{std::forward<Handler>(handler)});
    }

    static score::Result<void> StopFindService(FindServiceHandle)
    {
        return mock->StopFindService();
    }

    static score::Result<Proxy> Create(HandleType)
    {
        auto result = mock->CreateProxy();
        if (!result.has_value())
        {
            return score::MakeUnexpected(ExecErrc::kCommunicationError);
        }
        return FakeProxy{mock};
    }
};

using LmControlImplType = BasicLmControlImpl<FakeProxyTraits>;

constexpr std::string_view kValidSpecifier = "StateManager/LaunchManager/Instance";

ActivateRunTargetResponse Accepted()
{
    return ActivateRunTargetResponse{RequestStatus::kAccepted, ExecErrc::kGeneralError};
}

GetActiveRunTargetResponse Available(RunTargetName name)
{
    return GetActiveRunTargetResponse{QueryStatus::kAvailable, name, ExecErrc::kGeneralError};
}

/// @brief Builds a batch of activation results, one per run target name.
std::vector<ActivationResult> MakeSamples(std::initializer_list<std::string_view> run_target_names)
{
    std::vector<ActivationResult> samples{};
    samples.reserve(run_target_names.size());
    for (const auto name : run_target_names)
    {
        samples.push_back(ActivationResult{RunTargetName{name}, RunTargetActivationSource::kStateManagerRequest});
    }
    return samples;
}

class LmControlUT : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");

        FakeProxyTraits::mock = &mock_;

        // Sensible defaults so the "connected and healthy" path needs no per-test
        // wiring; tests override only the calls they care about.
        ON_CALL(mock_, StartFindService(_))
            .WillByDefault(DoAll(SaveArg<0>(&find_handler_), Return(score::Result<FakeFindHandle>{FakeFindHandle{}})));
        ON_CALL(mock_, StopFindService()).WillByDefault(Return(score::Result<void>{}));
        ON_CALL(mock_, CreateProxy()).WillByDefault(Return(score::Result<void>{}));
        ON_CALL(mock_, Subscribe(_)).WillByDefault(Return(score::Result<void>{}));
        ON_CALL(mock_, SetReceiveHandler(_))
            .WillByDefault(DoAll(SaveArg<0>(&receive_handler_), Return(score::Result<void>{})));
        ON_CALL(mock_, GetNewSamples(_)).WillByDefault(Invoke([](std::size_t) {
            return score::Result<std::vector<ActivationResult>>{std::vector<ActivationResult>{}};
        }));
        ON_CALL(mock_, ActivateRunTarget(_)).WillByDefault(Invoke([](const ActivateRunTargetRequest&) {
            return score::Result<ActivateRunTargetResponse>{Accepted()};
        }));
        ON_CALL(mock_, GetActiveRunTarget()).WillByDefault(Invoke([] {
            return score::Result<GetActiveRunTargetResponse>{Available(RunTargetName{"Running"})};
        }));
    }

    void TearDown() override
    {
        FakeProxyTraits::mock = nullptr;
    }

    /// @brief Simulate mw::com discovering one matching instance.
    void ConnectService()
    {
        ASSERT_TRUE(find_handler_) << "StartFindService was never called";
        find_handler_(score::mw::com::ServiceHandleContainer<FakeHandle>{FakeHandle{}}, FakeFindHandle{});
    }

    /// @brief Build a successfully-constructed (but not yet connected) SUT.
    std::unique_ptr<LmControlImplType> MakeLmControl()
    {
        auto sut = std::make_unique<LmControlImplType>();
        EXPECT_TRUE(sut->init(kValidSpecifier).has_value());
        return sut;
    }

    /// @brief Build an already-connected SUT using the fixture defaults.
    std::unique_ptr<LmControlImplType> MakeConnected()
    {
        auto sut = MakeLmControl();
        ConnectService();
        return sut;
    }

    NiceMock<MwComMock> mock_;
    FindHandler find_handler_{};
    std::function<void()> receive_handler_{};
};

// ---------------------------------------------------------------------------
// Construction / discovery lifecycle
// ---------------------------------------------------------------------------

TEST_F(LmControlUT, InvalidSpecifierIsRejectedByInit)
{
    RecordProperty(
        "Description", "init() with an empty instance specifier returns kInvalidArguments and never starts discovery.");

    EXPECT_CALL(mock_, StartFindService(_)).Times(0);

    LmControlImplType sut{};
    const auto result = sut.init("");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kInvalidArguments);
}

TEST_F(LmControlUT, StartFindServiceFailureIsReportedByInit)
{
    RecordProperty(
        "Description",
        "When StartFindService fails, init() returns kCommunicationError and the instance can be destroyed "
        "without stopping a search it never started.");

    EXPECT_CALL(mock_, StartFindService(_)).WillOnce(Return(score::MakeUnexpected(ExecErrc::kCommunicationError)));
    EXPECT_CALL(mock_, StopFindService()).Times(0);
    EXPECT_CALL(mock_, Unsubscribe()).Times(0);

    LmControlImplType sut{};
    const auto result = sut.init(kValidSpecifier);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

TEST_F(LmControlUT, InitStartsBackgroundDiscovery)
{
    RecordProperty(
        "Description", "init() with a valid specifier starts asynchronous service discovery via StartFindService.");

    EXPECT_CALL(mock_, StartFindService(_));

    LmControlImplType sut{};
    EXPECT_TRUE(sut.init(kValidSpecifier).has_value());
    EXPECT_TRUE(find_handler_);
}

TEST_F(LmControlUT, MethodsFailWhileNotYetConnected)
{
    RecordProperty(
        "Description",
        "Before the service is discovered, activate_run_target and get_active_run_target return "
        "kCommunicationError without issuing any proxy calls.");

    EXPECT_CALL(mock_, ActivateRunTarget(_)).Times(0);
    EXPECT_CALL(mock_, GetActiveRunTarget()).Times(0);

    auto sut = MakeLmControl();

    auto activate = sut->activate_run_target("Running", false);
    ASSERT_FALSE(activate.has_value());
    EXPECT_EQ(activate.error(), ExecErrc::kCommunicationError);

    auto get = sut->get_active_run_target();
    ASSERT_FALSE(get.has_value());
    EXPECT_EQ(get.error(), ExecErrc::kCommunicationError);
}

TEST_F(LmControlUT, DiscoveryCreatesProxySubscribesAndSetsHandler)
{
    RecordProperty(
        "Description",
        "On discovering the service, the SUT creates the proxy, subscribes to activation results and sets the "
        "receive handler.");

    EXPECT_CALL(mock_, CreateProxy());
    EXPECT_CALL(mock_, Subscribe(4U));  // must match kActivationResultSampleCount
    EXPECT_CALL(mock_, SetReceiveHandler(_));

    auto sut = MakeConnected();
    EXPECT_TRUE(receive_handler_);
}

TEST_F(LmControlUT, EmptyHandlesAreIgnored)
{
    RecordProperty(
        "Description", "A discovery callback with an empty handle container is ignored and no proxy is created.");

    EXPECT_CALL(mock_, CreateProxy()).Times(0);

    auto sut = MakeLmControl();
    ASSERT_TRUE(find_handler_);
    find_handler_(score::mw::com::ServiceHandleContainer<FakeHandle>{}, FakeFindHandle{});

    EXPECT_FALSE(sut->get_active_run_target().has_value());
}

TEST_F(LmControlUT, SecondDiscoveryCallbackIsIgnored)
{
    RecordProperty(
        "Description", "A second discovery callback after the proxy is already set up does not create another proxy.");

    EXPECT_CALL(mock_, CreateProxy()).Times(1);

    auto sut = MakeConnected();
    // A second notification after setup must not create another proxy.
    find_handler_(score::mw::com::ServiceHandleContainer<FakeHandle>{FakeHandle{}}, FakeFindHandle{});
}

TEST_F(LmControlUT, SynchronousDiscoveryDuringConstructionStopsFindingOnce)
{
    RecordProperty(
        "Description",
        "When discovery completes synchronously while init() is still running, the service discovery is still "
        "stopped exactly once, in the destructor.");

    EXPECT_CALL(mock_, StartFindService(_)).WillOnce(Invoke([](FindHandler handler) {
        handler(score::mw::com::ServiceHandleContainer<FakeHandle>{FakeHandle{}}, FakeFindHandle{});
        return score::Result<FakeFindHandle>{FakeFindHandle{}};
    }));
    // The destructor is the only place that stops discovery. The synchronous handler leaves the
    // search untouched, so init() can store the handle unconditionally.
    EXPECT_CALL(mock_, StopFindService()).Times(1);

    auto sut = MakeLmControl();
    EXPECT_TRUE(sut->get_active_run_target().has_value());
}

TEST_F(LmControlUT, DiscoveryCallbackAfterDestructionIsANoOp)
{
    RecordProperty(
        "Description",
        "A discovery callback dispatched after the instance was destroyed does not reach onServiceFound: "
        "the destructor expires the scope the callback is bound to.");

    {
        auto sut = MakeLmControl();
        ASSERT_TRUE(find_handler_) << "StartFindService was never called";
    }

    EXPECT_CALL(mock_, CreateProxy()).Times(0);

    // mw::com may still dispatch here - StopFindService() does not wait for an in-flight callback.
    find_handler_(score::mw::com::ServiceHandleContainer<FakeHandle>{FakeHandle{}}, FakeFindHandle{});
}

TEST_F(LmControlUT, SubscribeFailureLeavesInstanceDisconnected)
{
    RecordProperty(
        "Description",
        "When Subscribe fails during discovery, the instance stays disconnected and method calls return "
        "kCommunicationError.");

    EXPECT_CALL(mock_, Subscribe(_)).WillOnce(Return(score::MakeUnexpected(ExecErrc::kCommunicationError)));

    auto sut = MakeConnected();

    auto get = sut->get_active_run_target();
    ASSERT_FALSE(get.has_value());
    EXPECT_EQ(get.error(), ExecErrc::kCommunicationError);
}

TEST_F(LmControlUT, SetReceiveHandlerFailureLeavesInstanceDisconnected)
{
    RecordProperty(
        "Description",
        "When installing the receive handler fails during discovery, the instance stays disconnected and method "
        "calls return kCommunicationError.");

    EXPECT_CALL(mock_, SetReceiveHandler(_)).WillOnce(Return(score::MakeUnexpected(ExecErrc::kCommunicationError)));

    auto sut = MakeConnected();

    auto get = sut->get_active_run_target();
    ASSERT_FALSE(get.has_value());
    EXPECT_EQ(get.error(), ExecErrc::kCommunicationError);
}

TEST_F(LmControlUT, ProxyCreateFailureLeavesInstanceDisconnected)
{
    RecordProperty(
        "Description",
        "When proxy creation fails during discovery, no subscription is attempted and the instance stays "
        "disconnected.");

    EXPECT_CALL(mock_, CreateProxy()).WillOnce(Return(score::MakeUnexpected(ExecErrc::kCommunicationError)));
    EXPECT_CALL(mock_, Subscribe(_)).Times(0);

    auto sut = MakeConnected();

    EXPECT_FALSE(sut->get_active_run_target().has_value());
}

TEST_F(LmControlUT, StopFindServiceFailureOnDestructionIsTolerated)
{
    RecordProperty(
        "Description",
        "A StopFindService failure while tearing down is only logged; destruction still completes cleanly.");

    EXPECT_CALL(mock_, StopFindService()).WillOnce(Return(score::MakeUnexpected(ExecErrc::kInvalidArguments)));

    {
        auto sut = MakeConnected();
        EXPECT_TRUE(sut->get_active_run_target().has_value());
    }  // The failure inside the destructor is only logged.
}

// ---------------------------------------------------------------------------
// activate_run_target
// ---------------------------------------------------------------------------

TEST_F(LmControlUT, ActivateForwardsNameAndQueuedModeByDefault)
{
    RecordProperty(
        "Description", "activate_run_target forwards the run target name and defaults to queued activation mode.");

    auto sut = MakeConnected();

    EXPECT_CALL(
        mock_,
        ActivateRunTarget(AllOf(
            Field(&ActivateRunTargetRequest::run_target_name, RunTargetName{"Driving"}),
            Field(&ActivateRunTargetRequest::mode, ActivationMode::kQueued))))
        .WillOnce(Return(Accepted()));

    // Through the interface: that is where the force=false default lives.
    ILmControl& lm_control = *sut;
    EXPECT_TRUE(lm_control.activate_run_target("Driving").has_value());
}

TEST_F(LmControlUT, ActivateForcedMapsToForcedMode)
{
    RecordProperty("Description", "activate_run_target with force=true maps to forced activation mode.");

    auto sut = MakeConnected();

    EXPECT_CALL(mock_, ActivateRunTarget(Field(&ActivateRunTargetRequest::mode, ActivationMode::kForced)))
        .WillOnce(Return(Accepted()));

    EXPECT_TRUE(sut->activate_run_target("Driving", /*force=*/true).has_value());
}

TEST_F(LmControlUT, ActivateRejectionSurfacesRejectionReason)
{
    RecordProperty("Description", "A rejected activation surfaces the rejection reason as the returned error.");

    auto sut = MakeConnected();

    EXPECT_CALL(mock_, ActivateRunTarget(_))
        .WillOnce(Return(ActivateRunTargetResponse{RequestStatus::kRejected, ExecErrc::kRequestQueueIsFull}));

    auto result = sut->activate_run_target("Driving", false);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kRequestQueueIsFull);
}

TEST_F(LmControlUT, ActivateTransportFailureReturnsCommunicationError)
{
    RecordProperty("Description", "A transport failure during activate_run_target is reported as kCommunicationError.");

    auto sut = MakeConnected();

    EXPECT_CALL(mock_, ActivateRunTarget(_)).WillOnce(Return(score::MakeUnexpected(ExecErrc::kFailed)));

    auto result = sut->activate_run_target("Driving", false);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

// ---------------------------------------------------------------------------
// get_active_run_target
// ---------------------------------------------------------------------------

TEST_F(LmControlUT, GetActiveRunTargetReturnsName)
{
    RecordProperty("Description", "get_active_run_target returns the currently active run target name.");

    auto sut = MakeConnected();

    EXPECT_CALL(mock_, GetActiveRunTarget()).WillOnce(Return(Available(RunTargetName{"Running"})));

    auto result = sut->get_active_run_target();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Running");
}

TEST_F(LmControlUT, GetActiveRunTargetNotAvailableSurfacesReason)
{
    RecordProperty(
        "Description",
        "When no active run target is available, get_active_run_target surfaces the reason as the error.");

    auto sut = MakeConnected();

    EXPECT_CALL(mock_, GetActiveRunTarget())
        .WillOnce(Return(
            GetActiveRunTargetResponse{QueryStatus::kNotAvailable, RunTargetName{}, ExecErrc::kActivationInProgress}));

    auto result = sut->get_active_run_target();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kActivationInProgress);
}

TEST_F(LmControlUT, GetActiveRunTargetTransportFailureReturnsCommunicationError)
{
    RecordProperty(
        "Description", "A transport failure during get_active_run_target is reported as kCommunicationError.");

    auto sut = MakeConnected();

    EXPECT_CALL(mock_, GetActiveRunTarget()).WillOnce(Return(score::MakeUnexpected(ExecErrc::kFailed)));

    auto result = sut->get_active_run_target();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kCommunicationError);
}

// ---------------------------------------------------------------------------
// register_run_target_activation_callback + activation_result dispatch
// ---------------------------------------------------------------------------

TEST_F(LmControlUT, RegisterEmptyCallbackRejected)
{
    RecordProperty("Description", "Registering a null activation callback is rejected with kInvalidArguments.");

    auto sut = MakeLmControl();
    auto result = sut->register_run_target_activation_callback(nullptr);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kInvalidArguments);
}

TEST_F(LmControlUT, ActivationResultInvokesRegisteredCallback)
{
    RecordProperty(
        "Description",
        "A received activation_result sample is forwarded to the registered callback with its source and "
        "run target.");

    auto sut = MakeConnected();

    std::optional<RunTargetActivationSource> source{};
    std::optional<RunTargetName> name{};
    ASSERT_TRUE(sut->register_run_target_activation_callback([&](RunTargetActivationSource s, RunTargetName n) {
                       source = s;
                       name = n;
                   })
                    .has_value());

    EXPECT_CALL(mock_, GetNewSamples(_)).WillOnce(Invoke([](std::size_t) {
        std::vector<ActivationResult> samples;
        samples.push_back(ActivationResult{RunTargetName{"Recovery"}, RunTargetActivationSource::kRecoveryAction});
        return score::Result<std::vector<ActivationResult>>{std::move(samples)};
    }));

    ASSERT_TRUE(receive_handler_);
    receive_handler_();

    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "Recovery");
    EXPECT_EQ(source.value(), RunTargetActivationSource::kRecoveryAction);
}

TEST_F(LmControlUT, ActivationResultBacklogExceedingSubscriptionIsDrainedInSeveralCalls)
{
    RecordProperty(
        "Description",
        "Number of available samples larger than subscribed max_sample_count is read with repeated GetNewSamples "
        "calls until a batch smaller than the maximum signals an empty queue; every sample is forwarded exactly once.");

    auto sut = MakeConnected();

    std::vector<RunTargetName> received{};
    ASSERT_TRUE(sut->register_run_target_activation_callback([&](RunTargetActivationSource, RunTargetName n) {
                       received.push_back(n);
                   })
                    .has_value());

    // Nine pending samples, but Subscribe(kActivationResultSampleCount == 4) caps every single
    // GetNewSamples call at four: two full batches, then a short one that ends the drain.
    EXPECT_CALL(mock_, GetNewSamples(4U))
        .WillOnce(Invoke([](std::size_t) {
            return score::Result<std::vector<ActivationResult>>{MakeSamples({"T0", "T1", "T2", "T3"})};
        }))
        .WillOnce(Invoke([](std::size_t) {
            return score::Result<std::vector<ActivationResult>>{MakeSamples({"T4", "T5", "T6", "T7"})};
        }))
        .WillOnce(Invoke([](std::size_t) {
            return score::Result<std::vector<ActivationResult>>{MakeSamples({"T8"})};
        }));

    ASSERT_TRUE(receive_handler_);
    receive_handler_();

    EXPECT_THAT(received, ::testing::ElementsAre("T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7", "T8"));
}

TEST_F(LmControlUT, ActivationResultDrainStopsOnEmptyBatch)
{
    RecordProperty(
        "Description",
        "A receive handler firing with nothing retrievable issues exactly one GetNewSamples call and does not spin.");

    auto sut = MakeConnected();

    EXPECT_CALL(mock_, GetNewSamples(4U)).WillOnce(Invoke([](std::size_t) {
        return score::Result<std::vector<ActivationResult>>{std::vector<ActivationResult>{}};
    }));

    ASSERT_TRUE(receive_handler_);
    receive_handler_();
}

TEST_F(LmControlUT, ActivationResultWithoutCallbackIsDroppedSafely)
{
    RecordProperty(
        "Description", "An activation_result received with no registered callback is dropped without crashing.");

    auto sut = MakeConnected();

    EXPECT_CALL(mock_, GetNewSamples(_)).WillOnce(Invoke([](std::size_t) {
        std::vector<ActivationResult> samples;
        samples.push_back(ActivationResult{RunTargetName{"Running"}, RunTargetActivationSource::kStateManagerRequest});
        return score::Result<std::vector<ActivationResult>>{std::move(samples)};
    }));

    ASSERT_TRUE(receive_handler_);
    receive_handler_();  // no callback registered — must not crash
}

TEST_F(LmControlUT, ThrowingActivationCallbackIsContainedAndDrainContinues)
{
    RecordProperty(
        "Description",
        "An exception escaping the user activation callback is caught, the offending sample is dropped and "
        "the remaining samples of the batch are still forwarded.");

    auto sut = MakeConnected();

    std::vector<RunTargetName> received{};
    ASSERT_TRUE(sut->register_run_target_activation_callback([&](RunTargetActivationSource, RunTargetName n) {
                       received.push_back(n);
                       if (n == "Boom")
                       {
                           throw std::runtime_error{"callback failure"};
                       }
                   })
                    .has_value());

    EXPECT_CALL(mock_, GetNewSamples(_)).WillOnce(Invoke([](std::size_t) {
        return score::Result<std::vector<ActivationResult>>{MakeSamples({"T0", "Boom", "T1"})};
    }));

    ASSERT_TRUE(receive_handler_);
    receive_handler_();  // must neither terminate nor abort the drain

    EXPECT_THAT(received, ::testing::ElementsAre("T0", "Boom", "T1"));
}

TEST_F(LmControlUT, ActivationCallbackThrowingNonStdExceptionIsContained)
{
    RecordProperty(
        "Description", "An activation callback throwing something not derived from std::exception is caught as well.");

    auto sut = MakeConnected();

    ASSERT_TRUE(sut->register_run_target_activation_callback([](RunTargetActivationSource, RunTargetName) {
                       throw 42;
                   })
                    .has_value());

    EXPECT_CALL(mock_, GetNewSamples(_)).WillOnce(Invoke([](std::size_t) {
        return score::Result<std::vector<ActivationResult>>{MakeSamples({"Running"})};
    }));

    ASSERT_TRUE(receive_handler_);
    receive_handler_();  // must not terminate
}

TEST_F(LmControlUT, ActivationResultGetNewSamplesFailureIsTolerated)
{
    RecordProperty(
        "Description", "A GetNewSamples failure while reading activation results is tolerated without crashing.");

    auto sut = MakeConnected();

    EXPECT_CALL(mock_, GetNewSamples(_)).WillOnce(Return(score::MakeUnexpected(ExecErrc::kCommunicationError)));

    ASSERT_TRUE(receive_handler_);
    receive_handler_();  // error branch — must not crash
}

TEST_F(LmControlUT, LatestRegisteredCallbackWins)
{
    RecordProperty("Description", "The most recently registered activation callback replaces any earlier ones.");

    auto sut = MakeConnected();

    int first_calls{0};
    int second_calls{0};
    ASSERT_TRUE(sut->register_run_target_activation_callback([&](RunTargetActivationSource, RunTargetName) {
                       ++first_calls;
                   })
                    .has_value());
    ASSERT_TRUE(sut->register_run_target_activation_callback([&](RunTargetActivationSource, RunTargetName) {
                       ++second_calls;
                   })
                    .has_value());

    EXPECT_CALL(mock_, GetNewSamples(_)).WillOnce(Invoke([](std::size_t) {
        std::vector<ActivationResult> samples;
        samples.push_back(ActivationResult{RunTargetName{"Running"}, RunTargetActivationSource::kStateManagerRequest});
        return score::Result<std::vector<ActivationResult>>{std::move(samples)};
    }));

    ASSERT_TRUE(receive_handler_);
    receive_handler_();

    EXPECT_EQ(first_calls, 0);
    EXPECT_EQ(second_calls, 1);
}

// ---------------------------------------------------------------------------
// Destruction
// ---------------------------------------------------------------------------

TEST_F(LmControlUT, DestructorStopsDiscoveryWhenNeverConnected)
{
    RecordProperty("Description", "Destroying a never-connected instance stops discovery and does not unsubscribe.");

    EXPECT_CALL(mock_, StopFindService()).Times(1);
    EXPECT_CALL(mock_, Unsubscribe()).Times(0);

    {
        auto sut = MakeLmControl();
    }
}

TEST_F(LmControlUT, DestructorUnsubscribesWhenConnected)
{
    RecordProperty("Description", "Destroying a connected instance unsubscribes from the activation_result event.");

    EXPECT_CALL(mock_, Unsubscribe()).Times(1);

    MakeConnected().reset();
}

}  // namespace
}  // namespace score::mw::lifecycle::internal
