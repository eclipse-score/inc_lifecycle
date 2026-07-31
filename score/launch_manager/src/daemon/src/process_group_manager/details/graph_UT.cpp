#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "score/mw/launch_manager/process_group_manager/details/graph.hpp"
#include "score/mw/launch_manager/process_group_manager/mock_iprocess.hpp"
#include "score/mw/launch_manager/process_group_manager/process_group_manager.hpp"

namespace score::lcm::internal
{

using namespace testing;
using namespace score::mw::lifecycle;
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

class MockConfigurationManager : public IConfigurationManager
{
  public:
    MOCK_METHOD(IdentifierHash, getNameOfOffState, (const IdentifierHash& pg_name), (const, override));
    MOCK_METHOD(
        std::optional<const OsProcess*>,
        getOsProcessConfiguration,
        (const IdentifierHash& pg_name, const uint32_t index),
        (const, override));
    MOCK_METHOD(
        std::optional<const std::vector<ProcessGroupState>*>,
        getListOfProcessGroupStates,
        (const IdentifierHash& pg_name),
        (const, override));
    MOCK_METHOD(
        std::optional<const DependencyList*>,
        getOsProcessDependencies,
        (const IdentifierHash& process_group_name, const uint32_t index),
        (const, override));
};

class GraphTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");

        ON_CALL(config_, getNameOfOffState).WillByDefault(Return(off_state));
        ON_CALL(config_, getOsProcessConfiguration).WillByDefault(Return(&default_process_));
        ON_CALL(config_, getListOfProcessGroupStates).WillByDefault(Return(&simple_states));
        ON_CALL(config_, getOsProcessDependencies).WillByDefault(Return(&empty));

        ON_CALL(mock_process_state_notifier_, queuePosixProcess).WillByDefault(Return(true));

        setProcessDefaultConfig();
    }

    void setProcessDefaultConfig(osal::CommsType comms_type = osal::CommsType::kNoComms, bool self_terminating = false)
    {
        default_process_.startup_config_.executable_path_ = "/dev/null";
        default_process_.startup_config_.short_name_ = "test_process";
        default_process_.startup_config_.comms_type_ = comms_type;
        default_process_.pgm_config_.is_self_terminating_ = self_terminating;
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

    NiceMock<MockConfigurationManager> config_{};
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

    IdentifierHash pg_name{"TestPG"};
    IdentifierHash off_state{"Off"};
    const DependencyList empty = {};
    const std::vector<ProcessGroupState> simple_states = {
        ProcessGroupState{IdentifierHash{"Startup"}, {}},
        ProcessGroupState{IdentifierHash{"Running"}, {first_process_index_}},
        ProcessGroupState{off_state, {}},
    };
    OsProcess default_process_ = {};
    const std::uint32_t pg_index_ = 0;
    const std::uint32_t first_process_index_ = 0;
};

class GraphStartTransitionTest : public GraphTest
{
};

TEST_F(GraphStartTransitionTest, simpleTransition)
{
    RecordProperty(
        "Description", "Test that a simple transition activates the expected run target and process successfully");

    graph_.initProcessGroupNodes(pg_name, 1, pg_index_);

    const auto target = IdentifierHash{"Running"};
    // Sanity check to make sure we actually change the state
    EXPECT_NE(graph_.getProcessGroupState(), target);

    EXPECT_TRUE(graph_.startTransition({pg_name, target}));
    EXPECT_EQ(graph_.getState(), GraphState::kInTransition);

    const auto job = job_queue_->pop();
    ASSERT_TRUE(job->has_value()) << "startTransition didn't push anything to the queue";
    EXPECT_EQ(job->value().type, ComponentTaskType::kActivate);
    EXPECT_EQ(job->value().component.get().getIndex(), first_process_index_);

    executeJobSuccessfully(job->value());

    graph_.handleComponentEvent(ActivationSuccessful{0});

    ASSERT_EQ(graph_.getState(), GraphState::kSuccess);
    EXPECT_EQ(graph_.getProcessGroupState(), target);
}

}  // namespace score::lcm::internal