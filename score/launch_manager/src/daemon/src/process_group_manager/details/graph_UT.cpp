#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/configuration/configuration_adapter.hpp"
#include "score/mw/launch_manager/process_group_manager/details/graph.hpp"
#include "score/mw/launch_manager/process_group_manager/mock_iprocess.hpp"
#include "score/mw/launch_manager/process_group_manager/process_group_manager.hpp"

namespace score::lcm::internal
{

#ifdef USE_NEW_CONFIGURATION
using ConfigurationType = ConfigurationAdapter;
using Config = score::mw::launch_manager::configuration::Config;
#else
using ConfigurationType = IConfigurationManager;
#endif

using namespace testing;
using namespace score::mw::lifecycle;
using namespace score::mw::launch_manager::configuration;
using namespace std::chrono_literals;

class MockProcessMap : public SafeProcessMapInserter
{
  public:
    MOCK_METHOD(SafeProcessMapReturnType, insertIfNotTerminated, (osal::ProcessID key, IComponent* object), (override));
};

class MockProcessStateNotifier : public IProcessStateNotifier
{
  public:
    MOCK_METHOD(std::unique_ptr<score::lcm::IProcessStateReceiver>, constructReceiver, (), (override));
    MOCK_METHOD(bool, queuePosixProcess, (const score::lcm::PosixProcess& f_posixProcess), (override, noexcept));
};

class MockTransitionResultPublisher : public ITransitionResultPublisher
{
  public:
    MOCK_METHOD(void, setInitialStateTransitionResult, (ControlClientCode result), (override));
};

class GraphTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");

        ON_CALL(mock_process_state_notifier_, queuePosixProcess).WillByDefault(Return(true));

        auto procs = SetConfig();

        graph_.initProcessGroupNodes(pg_name, procs, pg_index_);
    }

    virtual uint32_t SetConfig()
    {
        auto procs = generateProcessComponents(1);
        auto count = procs.size();
        auto rts = generateRunTargets(1);
        rts[1].depends_on = {procs[0].name};
        const auto config = ConfigBuilder{}
                                .setComponents(std::move(procs))
                                .setRunTargets(std::move(rts))
                                .setInitialRunTarget("Startup")
                                .setFallbackRunTarget(std::move(fallback))
                                .build();
        config_.initialize(config);

        return count;
    }

    std::vector<ComponentConfig> generateProcessComponents(int count)
    {
        std::vector<ComponentConfig> components{};
        for (int i = 0; i < count; i++)
        {
            ComponentConfig config{};
            config.name = process_name(i);
            components.push_back(std::move(config));
        }
        return components;
    }

    std::vector<RunTargetConfig> generateRunTargets(int count)
    {
        std::vector<RunTargetConfig> rts{};
        rts.push_back(startup);
        for (int i = 0; i < count; i++)
        {
            RunTargetConfig config{};
            config.name = run_target_name(i);
            rts.push_back(std::move(config));
        }
        rts.push_back(off);
        return rts;
    }

    std::string process_name(int index)
    {
        return "test_process_" + std::to_string(index);
    }

    std::string run_target_name(int index)
    {
        return "RunTarget" + std::to_string(index);
    }

    IdentifierHash state_name(std::string_view run_target)
    {
        const auto left = std::string{pg_string};
        const auto right = std::string{run_target};
        return IdentifierHash{left + "/" + right};
    }

    void executeJobSuccessfully(const ComponentTask& job)
    {
        IComponent::RequestResult res;
        if (job.type == ComponentTaskType::kActivate)
        {
            EXPECT_CALL(process_interface_, startProcess).WillOnce(Return(osal::OsalReturnType::kSuccess));
            EXPECT_CALL(*mock_process_map, insertIfNotTerminated).WillOnce(Return(SafeProcessMapReturnType::kOk));
            res = job.component.get().activate(job.stop_token);
        }
        else if (job.type == ComponentTaskType::kDeactivate)
        {
            EXPECT_CALL(process_interface_, requestTermination)
                .WillOnce(DoAll(
                    InvokeWithoutArgs([job] {
                        static_cast<void>(job.component.get().tryHandleTermination(0));
                    }),
                    Return(osal::OsalReturnType::kSuccess)));
            res = job.component.get().deactivate(job.stop_token);
        }

        ASSERT_TRUE(res.has_value());
        ASSERT_EQ(res.value(), IComponent::RequestState::kSuccess);
    }

    ConfigurationAdapter config_{};
    std::shared_ptr<WorkerQueue> job_queue_ = std::make_shared<WorkerQueue>();
    StrictMock<osal::MockIProcess> process_interface_{};
    std::shared_ptr<MockProcessMap> mock_process_map = std::make_shared<MockProcessMap>();
    NiceMock<MockProcessStateNotifier> mock_process_state_notifier_{};
    MockTransitionResultPublisher mock_transition_result_publisher_{};
    Graph graph_{
        10U,
        &config_,
        job_queue_,
        &process_interface_,
        mock_process_map,
        &mock_process_state_notifier_,
        &mock_transition_result_publisher_};

    static constexpr std::string_view pg_string{"MainPG"};
    const IdentifierHash pg_name{pg_string};
    const int pg_index_ = 0;

    RunTargetConfig startup = {"Startup", "", {}, 10, {}};
    RunTargetConfig off = {"Off", "", {}, 10, {}};
    FallbackRunTargetConfig fallback = {
        "",
        {},
        10,
    };
};

class GraphOrdinaryTransitionTest : public GraphTest
{
  protected:
    uint32_t SetConfig() override
    {
        auto procs = generateProcessComponents(2);
        auto count = procs.size();
        auto rts = generateRunTargets(2);
        rts[1].depends_on = {procs[0].name};
        rts[2].depends_on = {procs[1].name};
        const auto config = ConfigBuilder{}
                                .setComponents(std::move(procs))
                                .setRunTargets(std::move(rts))
                                .setInitialRunTarget("Startup")
                                .setFallbackRunTarget(std::move(fallback))
                                .build();
        config_.initialize(config);

        return count;
    }

    /// @brief Execute a run target activation that activates or deactivates a single node
    void oneProcessTransition(IdentifierHash target, std::uint32_t node_index)
    {
        graph_.startTransition({pg_name, target});

        auto job = job_queue_->pop();
        ASSERT_TRUE(job->has_value());
        executeJobSuccessfully(job->value());
        if (job->value().type == ComponentTaskType::kActivate)
        {
            graph_.handleComponentEvent(ActivationSuccessful{node_index});
        }
        else
        {
            graph_.handleComponentEvent(DeactivationComplete{node_index});
        }

        ASSERT_EQ(graph_.getState(), GraphState::kSuccess);
        ASSERT_EQ(graph_.getProcessGroupState(), target);
    }
};

TEST_F(GraphOrdinaryTransitionTest, correctJobDetails)
{
    RecordProperty("Description", "Test that, in a simple transition, the correct job information is passed");

    const auto target = state_name(run_target_name(1));

    graph_.startTransition({pg_name, target});

    const auto job = job_queue_->pop();
    ASSERT_TRUE(job->has_value()) << "startTransition didn't push anything to the queue";
    EXPECT_EQ(job->value().type, ComponentTaskType::kActivate);
    EXPECT_EQ(job->value().component.get().getIndex(), 1);
}

TEST_F(GraphOrdinaryTransitionTest, simpleActivationTransition)
{
    RecordProperty(
        "Description", "Test that a simple transition activates the expected run target and process successfully");

    const auto target = state_name(run_target_name(0));

    const auto start_res = graph_.startTransition({pg_name, target});

    const auto job = job_queue_->pop();
    executeJobSuccessfully(job->value());
    graph_.handleComponentEvent(ActivationSuccessful{0});

    EXPECT_TRUE(start_res);
    ASSERT_EQ(graph_.getState(), GraphState::kSuccess);
    EXPECT_EQ(graph_.getProcessGroupState(), target);
}

TEST_F(GraphOrdinaryTransitionTest, simpleDeactivationTransition)
{
    RecordProperty(
        "Description", "Test that a simple transition deactivates the expected run target and process successfully");

    oneProcessTransition(state_name(run_target_name(0)), 0);

    const auto target = state_name(off.name);
    const auto start_res = graph_.startTransition({pg_name, target});

    const auto job = job_queue_->pop();
    executeJobSuccessfully(job->value());
    graph_.handleComponentEvent(DeactivationComplete{0});

    EXPECT_TRUE(start_res);
    ASSERT_EQ(graph_.getState(), GraphState::kSuccess);
    EXPECT_EQ(graph_.getProcessGroupState(), target);
}

class GraphInitialTransitionTest : public GraphTest
{
};

TEST_F(GraphInitialTransitionTest, nothingToDo)
{
    RecordProperty("Description", "Test that the initial transition to an empty run target succeeds immediately");

    EXPECT_CALL(
        mock_transition_result_publisher_,
        setInitialStateTransitionResult(ControlClientCode::kInitialMachineStateSuccess));

    const auto res = graph_.startInitialTransition({pg_name, state_name(startup.name)});

    EXPECT_TRUE(res);
    EXPECT_EQ(graph_.getState(), GraphState::kSuccess);
}

TEST_F(GraphInitialTransitionTest, startTransitionFailure)
{
    RecordProperty("Description", "Test that startTransition() reacts correctly to an invalid machine state");

    EXPECT_CALL(
        mock_transition_result_publisher_,
        setInitialStateTransitionResult(ControlClientCode::kInitialMachineStateFailed));
    // Hmm... can't inject a failure here yet.
    const auto res = graph_.startInitialTransition({pg_name, state_name(startup.name)});

    EXPECT_FALSE(res);
    EXPECT_EQ(graph_.getState(), GraphState::kUndefinedState);
}

}  // namespace score::lcm::internal