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
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"

namespace score::mw::lifecycle::internal
{
/// @brief Test-only projection for Transition<IComponent*>, mirroring component_of.hpp's
/// production overload. Declared in this namespace so Transition<IComponent*> finds it via ADL.
IComponent& componentOf(IComponent* node)
{
    return *node;
}
}  // namespace score::mw::lifecycle::internal

#define SCORE_LCM_COMPONENT_OF_HPP_INCLUDED

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/process_group_manager/details/dependency_graph.hpp"
#include "score/mw/launch_manager/process_group_manager/details/transition.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace score::mw::lifecycle::internal
{

/// @brief Mock IComponent backed by two flags. Transition only ever reads active()/stopped();
/// the rest of the interface is mocked purely to satisfy IComponent and is never exercised.
class MockComponent : public IComponent
{
  public:
    MockComponent()
    {
        static std::size_t index{0};
        name_ = IdentifierHash{std::to_string(index++)};

        ON_CALL(*this, active()).WillByDefault(::testing::ReturnPointee(&active_));
        ON_CALL(*this, getIndex()).WillByDefault(::testing::ReturnPointee(&name_));
    }

    MOCK_METHOD(RequestResult, activate, (score::cpp::stop_token), (override));
    MOCK_METHOD(RequestResult, deactivate, (score::cpp::stop_token), (override));
    MOCK_METHOD(RequestResult, tryHandleTermination, (int32_t), (override));
    MOCK_METHOD(IdentifierHash, getIndex, (), (const, override));
    MOCK_METHOD(bool, active, (), (const, override));

    /// @brief Flip both flags together, mirroring a real component reaching a terminal state.
    void setActive(bool is_active)
    {
        active_ = is_active;
        stopped_ = !is_active;
    }

    // Default: inactive and fully stopped.
    bool active_ = false;
    bool stopped_ = true;

    IdentifierHash name_;
};

using ComponentType = internal::IComponent*;

/// @brief Base fixture: owns a DependencyGraph plus the address-stable mocks its nodes point to,
/// and offers small helpers shared by every test.
class TransitionTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }

    /// @brief (Re)create the underlying graph sized for exactly @p node_count nodes, plus a builder
    /// over it. The graph's capacity is fixed here, so the builder can be constructed before any
    /// nodes are added.
    void makeGraph(std::size_t node_count)
    {
        graph_ = std::make_unique<DependencyGraph<IdentifierHash, ComponentType>>(node_count);
        components_.clear();
        builder_ = std::make_unique<TransitionBuilder<IdentifierHash, ComponentType>>(*graph_);
    }

    /// @brief Add a fresh mock-backed node and return its index.
    IdentifierHash addNode()
    {
        auto component = std::make_unique<::testing::NiceMock<internal::MockComponent>>();
        const IdentifierHash name = component->name_;
        const auto res = components_.emplace(name, std::move(component));
        graph_->try_emplace(name, res.first->second.get());
        return name;
    }

    internal::MockComponent& componentAt(IdentifierHash i)
    {
        return *components_.at(i);
    }

    /// @brief Mark @p node active and report it finished — mimics an activation completing.
    void activate(Transition<IdentifierHash, ComponentType>& t, IdentifierHash node)
    {
        componentAt(node).setActive(true);
        t.onNodeFinished(node);
    }

    /// @brief Mark @p node stopped and report it finished — mimics a deactivation completing.
    void deactivate(Transition<IdentifierHash, ComponentType>& t, IdentifierHash node)
    {
        componentAt(node).setActive(false);
        t.onNodeFinished(node);
    }

    /// @brief Drain everything ready right now into a vector so it can be matched.
    /// @details Iterating CONSUMES the frontier, so call this once per step (after each
    /// onNodeFinished()), not repeatedly for the same step.
    static std::vector<ReadyNode<IdentifierHash>> collectReady(Transition<IdentifierHash, ComponentType>& t)
    {
        std::vector<ReadyNode<IdentifierHash>> out;
        for (const ReadyNode<IdentifierHash> rn : t)
        {
            out.push_back(rn);
        }
        return out;
    }

    std::unique_ptr<DependencyGraph<IdentifierHash, ComponentType>> graph_;
    std::unordered_map<IdentifierHash, std::unique_ptr<::testing::NiceMock<internal::MockComponent>>> components_;
    std::unique_ptr<TransitionBuilder<IdentifierHash, ComponentType>> builder_;
};

// ---------------------------------------------------------------------------
// Empty graph
// ---------------------------------------------------------------------------

/// @brief Fixture for the degenerate empty graph (no nodes). Every possible node index is out of
/// range, so the builder's create*() methods have no valid argument.
class EmptyGraphTest : public TransitionTest
{
  protected:
    void SetUp() override
    {
        TransitionTest::SetUp();
        makeGraph(0);
    }
};

using EmptyGraphDeathTest = EmptyGraphTest;

TEST_F(EmptyGraphTest, BuilderConstructsButGraphIsEmpty)
{
    RecordProperty(
        "Description",
        "A TransitionBuilder can be constructed over an empty graph (no nodes). There are no node "
        "indices, so no transition can be created or driven.");

    EXPECT_EQ(graph_->size(), 0U);
}

TEST_F(EmptyGraphDeathTest, CreateTransitionAssertsOnOutOfRangeTarget)
{
    RecordProperty(
        "Description", "createTransition() with an out-of-range target index aborts via a futurecpp assert.");

    EXPECT_DEATH(builder_->createTransition(IdentifierHash{"not real"}), "");
}

// ---------------------------------------------------------------------------
// Single node
// ---------------------------------------------------------------------------

/// @brief Fixture for a graph with exactly one node and no edges. That node is both a leaf (no
/// dependencies) and a root (no dependents), so it is always ready the instant a transition needs
/// it — nothing gates it.
class SingleNodeGraphTest : public TransitionTest
{
  protected:
    void SetUp() override
    {
        TransitionTest::SetUp();
        makeGraph(1);
        node_ = addNode();
    }

    IdentifierHash node_{};
};

TEST_F(SingleNodeGraphTest, TransitionStartsTheNode)
{
    RecordProperty(
        "Description",
        "A single inactive node with no dependencies is immediately ready to start; the transition "
        "finishes once the node reports active.");

    auto& transition = builder_->createTransition(node_);

    EXPECT_FALSE(transition.isFinished());
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{node_, Action::Start}));

    activate(transition, node_);

    EXPECT_THAT(collectReady(transition), ::testing::IsEmpty());
    EXPECT_TRUE(transition.isFinished());
}

TEST_F(SingleNodeGraphTest, TransitionToAlreadyActiveNodeIsImmediatelyFinished)
{
    RecordProperty(
        "Description",
        "A node that is already active is not started again; the transition is finished "
        "immediately with nothing ready.");

    componentAt(node_).setActive(true);

    auto& transition = builder_->createTransition(node_);

    EXPECT_THAT(collectReady(transition), ::testing::IsEmpty());
    EXPECT_TRUE(transition.isFinished());
}

TEST_F(SingleNodeGraphTest, TransitionToOffStopsTheRunningNodeThenStartsOff)
{
    RecordProperty(
        "Description",
        "Transitioning to the Off target stops a running node not needed by Off, then activates the "
        "dependency-less Off node; the transition finishes once both reach their terminal state.");

    makeGraph(2);
    const IdentifierHash node = addNode();
    const IdentifierHash off = addNode();
    componentAt(node).setActive(true);  // node running; Off node stopped (default)

    auto& transition = builder_->createTransition(off);

    // Stopping phase: the running node is stopped first (Off does not depend on it).
    EXPECT_FALSE(transition.isFinished());
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{node, Action::Stop}));
    deactivate(transition, node);

    // Starting phase: the Off node is now ready to activate.
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{off, Action::Start}));
    activate(transition, off);

    EXPECT_THAT(collectReady(transition), ::testing::IsEmpty());
    EXPECT_TRUE(transition.isFinished());
}

TEST_F(SingleNodeGraphTest, TransitionToOffFromAllStoppedStartsTheOffNode)
{
    RecordProperty(
        "Description",
        "With nothing running, transitioning to the Off target skips the stopping phase and simply "
        "activates the dependency-less Off node, leaving the stopped application node untouched.");

    makeGraph(2);
    const IdentifierHash node = addNode();  // an application node, left stopped
    const IdentifierHash off = addNode();

    auto& transition = builder_->createTransition(off);

    EXPECT_FALSE(transition.isFinished());
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{off, Action::Start}));
    activate(transition, off);

    EXPECT_THAT(collectReady(transition), ::testing::IsEmpty());
    EXPECT_TRUE(transition.isFinished());
    EXPECT_TRUE(componentAt(node).stopped_);
    EXPECT_TRUE(componentAt(off).active_);
}

// ---------------------------------------------------------------------------
// Two RunTargets sharing a node
// ---------------------------------------------------------------------------

/// @brief Fixture for the graph from the original transition_UT.cpp: two RunTargets that share a
/// common dependency, plus the standalone Off RunTarget.
///
///        ┌────────┐              ┌────────┐          ┌────────┐
///        │  RT2   │              │  RT1   │          │  Off   │
///        └───┬─┬──┘              └──┬─┬───┘          └────────┘
///            │ └───────┐    ┌───────┘ │          (no dependencies)
///            │         │    │         │
///         ┌──▼──┐   ┌──▼────▼───┐   ┌──▼──┐
///         │  A  │   │     B     │   │  C  │
///         └─────┘   └───────────┘   └─────┘
///
/// (X -> Y means X depends on Y)
///   RT1 -> C, B
///   RT2 -> A, B      (B is shared by both RunTargets)
///   Off              (no dependencies: reaching it stops everything else)
class SharedNodeGraphTest : public TransitionTest
{
  protected:
    void SetUp() override
    {
        TransitionTest::SetUp();
        makeGraph(6);
        a_ = addNode();
        b_ = addNode();
        c_ = addNode();
        rt1_ = addNode();
        rt2_ = addNode();
        off_ = addNode();  // Off RunTarget: no dependencies, so reaching it stops everything else
        graph_->addDependency(rt1_, c_);
        graph_->addDependency(rt1_, b_);
        graph_->addDependency(rt2_, a_);
        graph_->addDependency(rt2_, b_);
    }

    IdentifierHash a_{};
    IdentifierHash b_{};
    IdentifierHash c_{};
    IdentifierHash rt1_{};
    IdentifierHash rt2_{};
    IdentifierHash off_{};
};

TEST_F(SharedNodeGraphTest, TransitionStartsDependenciesBeforeDependent)
{
    RecordProperty(
        "Description",
        "Bringing up RT1 from scratch starts its dependencies B and C first; RT1 only becomes ready "
        "once both of them are active.");

    auto& transition = builder_->createTransition(rt1_);
    EXPECT_THAT(
        collectReady(transition),
        ::testing::UnorderedElementsAre(
            ReadyNode<IdentifierHash>{c_, Action::Start}, ReadyNode<IdentifierHash>{b_, Action::Start}));

    activate(transition, c_);
    EXPECT_THAT(collectReady(transition), ::testing::IsEmpty());  // still blocked on B

    activate(transition, b_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{rt1_, Action::Start}));

    activate(transition, rt1_);
    EXPECT_THAT(collectReady(transition), ::testing::IsEmpty());
    EXPECT_TRUE(transition.isFinished());
}

TEST_F(SharedNodeGraphTest, NextReadyInterleavedWithOnNodeFinishedKeepsPendingSibling)
{
    RecordProperty(
        "Description",
        "Popping one of two simultaneously-ready siblings via nextReady() and finishing it before the "
        "other is popped must not discard the sibling still in the frontier.");

    auto& transition = builder_->createTransition(rt1_);

    const auto first = transition.nextReady();
    ASSERT_TRUE(first.has_value());
    ASSERT_THAT(first->node, ::testing::AnyOf(c_, b_));
    EXPECT_EQ(first->action, Action::Start);
    activate(transition, first->node);  // must not discard the sibling

    const IdentifierHash sibling = (first->node == c_) ? b_ : c_;
    const auto second = transition.nextReady();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->node, sibling);
    activate(transition, second->node);

    // Only now that both of RT1's dependencies are active does RT1 itself become ready.
    const auto third = transition.nextReady();
    ASSERT_TRUE(third.has_value());
    EXPECT_EQ(third->node, rt1_);
    EXPECT_FALSE(transition.nextReady().has_value());

    activate(transition, rt1_);
    EXPECT_FALSE(transition.nextReady().has_value());
    EXPECT_TRUE(transition.isFinished());
}

TEST_F(SharedNodeGraphTest, TransitionBetweenRunTargetsKeepsSharedNodeActive)
{
    RecordProperty(
        "Description",
        "Transitioning RT1 -> RT2 stops RT1 then its exclusive node C, then starts A; the shared node "
        "B stays active throughout because RT2 still needs it.");

    componentAt(b_).setActive(true);
    componentAt(c_).setActive(true);
    componentAt(rt1_).setActive(true);

    auto& transition = builder_->createTransition(rt2_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{rt1_, Action::Stop}));

    deactivate(transition, rt1_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{c_, Action::Stop}));

    // Finishing the last stop-set node auto-advances into the starting phase.
    deactivate(transition, c_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{a_, Action::Start}));

    // B was shared and stayed up the whole time.
    EXPECT_TRUE(componentAt(b_).active_);
    EXPECT_FALSE(componentAt(b_).stopped_);

    activate(transition, a_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{rt2_, Action::Start}));

    activate(transition, rt2_);
    EXPECT_THAT(collectReady(transition), ::testing::IsEmpty());
    EXPECT_TRUE(transition.isFinished());
}

TEST_F(SharedNodeGraphTest, OnNodeFinishedDuringIterationDrivesWholeTransitionInOneLoop)
{
    RecordProperty(
        "Description",
        "A single range-for loop that reports each node finished mid-iteration (as a synchronous "
        "driver would) discovers all newly-ready successors until the transition completes.");

    componentAt(b_).setActive(true);
    componentAt(c_).setActive(true);
    componentAt(rt1_).setActive(true);

    auto& transition = builder_->createTransition(rt2_);

    std::vector<ReadyNode<IdentifierHash>> visited;
    for (const auto rn : transition)
    {
        visited.push_back(rn);
        // Report completion immediately, mid-iteration, as a synchronously completing node would.
        componentAt(rn.node).setActive(rn.action == Action::Start);
        transition.onNodeFinished(rn.node);
    }

    EXPECT_THAT(
        visited,
        ::testing::ElementsAre(
            ReadyNode<IdentifierHash>{rt1_, Action::Stop},
            ReadyNode<IdentifierHash>{c_, Action::Stop},
            ReadyNode<IdentifierHash>{a_, Action::Start},
            ReadyNode<IdentifierHash>{rt2_, Action::Start}));
    EXPECT_TRUE(transition.isFinished());

    // B was shared and never touched.
    EXPECT_TRUE(componentAt(b_).active_);
    EXPECT_FALSE(componentAt(b_).stopped_);
}

TEST_F(SharedNodeGraphTest, FailedTransitionIsRecoveredByAFreshFallbackTransition)
{
    RecordProperty(
        "Description",
        "When activating C never completes, the RT2 -> RT1 transition is stuck; a fresh RT1 -> RT2 "
        "transition on the same builder recovers from the inconsistent state and brings RT2 back up.");

    componentAt(a_).setActive(true);
    componentAt(b_).setActive(true);
    componentAt(rt2_).setActive(true);

    auto& toRt1 = builder_->createTransition(rt1_);
    EXPECT_THAT(collectReady(toRt1), ::testing::ElementsAre(ReadyNode<IdentifierHash>{rt2_, Action::Stop}));

    deactivate(toRt1, rt2_);
    EXPECT_THAT(collectReady(toRt1), ::testing::ElementsAre(ReadyNode<IdentifierHash>{a_, Action::Stop}));

    // Finishing the last stop-set node auto-advances into the starting phase.
    deactivate(toRt1, a_);
    EXPECT_THAT(collectReady(toRt1), ::testing::ElementsAre(ReadyNode<IdentifierHash>{c_, Action::Start}));

    // C fails to activate: it never reports finished, so the transition is stuck forever.
    EXPECT_FALSE(componentAt(c_).active_);
    EXPECT_FALSE(componentAt(rt1_).active_);
    EXPECT_FALSE(toRt1.isFinished());

    // Fallback RT1 -> RT2: RT1's exclusive nodes are already stopped, so the stopping phase is a
    // no-op and the transition starts straight in the starting phase.
    auto& toRt2 = builder_->createTransition(rt2_);
    EXPECT_THAT(collectReady(toRt2), ::testing::ElementsAre(ReadyNode<IdentifierHash>{a_, Action::Start}));

    activate(toRt2, a_);
    EXPECT_THAT(collectReady(toRt2), ::testing::ElementsAre(ReadyNode<IdentifierHash>{rt2_, Action::Start}));

    activate(toRt2, rt2_);
    EXPECT_THAT(collectReady(toRt2), ::testing::IsEmpty());
    EXPECT_TRUE(toRt2.isFinished());

    EXPECT_TRUE(componentAt(b_).active_);
    EXPECT_FALSE(componentAt(b_).stopped_);
}

TEST_F(SharedNodeGraphTest, StopsOrphanLeftRunningOutsideTheLastTargetSubgraph)
{
    RecordProperty(
        "Description",
        "A node left running outside the current target's subgraph — here A, orphaned by an earlier "
        "aborted RT2 attempt while RT1 is up — is still stopped. The stop set is every running node "
        "derived from live component state, not a traversal of one target's subgraph, so an orphan that "
        "is unreachable from RT1 is not missed.");

    // RT1 is up (needs B and C). A is a stray still running from an aborted RT2 attempt; nothing
    // reachable from RT1 leads to it, so a source-subgraph traversal from RT1 would leave it running.
    componentAt(a_).setActive(true);
    componentAt(b_).setActive(true);
    componentAt(c_).setActive(true);
    componentAt(rt1_).setActive(true);

    auto& transition = builder_->createTransition(off_);

    // Drive the whole transition in one loop, reporting each node's terminal state as it comes up.
    // Everything running is stopped (the orphan A included), then the Off node is activated.
    std::vector<ReadyNode<IdentifierHash>> stopped_nodes;
    bool off_started = false;
    for (const auto rn : transition)
    {
        if (rn.node == off_)
        {
            EXPECT_EQ(rn.action, Action::Start);
            off_started = true;
            componentAt(rn.node).setActive(true);
        }
        else
        {
            EXPECT_EQ(rn.action, Action::Stop);
            stopped_nodes.push_back(rn);
            componentAt(rn.node).setActive(false);
        }
        transition.onNodeFinished(rn.node);
    }

    EXPECT_TRUE(transition.isFinished());
    EXPECT_TRUE(off_started);
    EXPECT_TRUE(componentAt(off_).active_);
    // The orphan A is the key assertion: it was included in the stop set and is now stopped.
    EXPECT_TRUE(componentAt(a_).stopped_);
    EXPECT_TRUE(componentAt(b_).stopped_);
    EXPECT_TRUE(componentAt(c_).stopped_);
    EXPECT_TRUE(componentAt(rt1_).stopped_);
    EXPECT_THAT(
        stopped_nodes,
        ::testing::UnorderedElementsAre(
            ReadyNode<IdentifierHash>{a_, Action::Stop},
            ReadyNode<IdentifierHash>{b_, Action::Stop},
            ReadyNode<IdentifierHash>{c_, Action::Stop},
            ReadyNode<IdentifierHash>{rt1_, Action::Stop}));
}

// ---------------------------------------------------------------------------
// Linear chain A -> B -> C -> D
// ---------------------------------------------------------------------------

/// @brief Fixture for a linear dependency chain, plus the standalone Off RunTarget.
///
///   A -> B -> C -> D        (X -> Y means X depends on Y)
///   Off                     (no dependencies: reaching it stops the whole chain)
///
/// A is the top root (nothing depends on it); D is the deepest leaf (depends on nothing).
/// Activation flows bottom-up (D, C, B, A); deactivation flows top-down (A, B, C, D).
/// Off stands alone, so a transition to it stops A..D and then activates Off.
class LinearGraphTest : public TransitionTest
{
  protected:
    void SetUp() override
    {
        TransitionTest::SetUp();
        makeGraph(5);
        a_ = addNode();
        b_ = addNode();
        c_ = addNode();
        d_ = addNode();
        off_ = addNode();  // Off RunTarget: no dependencies, so reaching it stops the whole chain
        graph_->addDependency(a_, b_);
        graph_->addDependency(b_, c_);
        graph_->addDependency(c_, d_);
    }

    /// @brief Bring the whole chain up front — a common precondition for the tests below.
    void activateWholeChain()
    {
        componentAt(a_).setActive(true);
        componentAt(b_).setActive(true);
        componentAt(c_).setActive(true);
        componentAt(d_).setActive(true);
    }

    IdentifierHash a_{};
    IdentifierHash b_{};
    IdentifierHash c_{};
    IdentifierHash d_{};
    IdentifierHash off_{};
};

TEST_F(LinearGraphTest, TransitionToAStartsChainBottomUp)
{
    RecordProperty(
        "Description",
        "Bringing up A from scratch starts the whole chain in dependency order: D, then C, then B, "
        "then A. Each node becomes ready only once its single dependency is active.");

    auto& transition = builder_->createTransition(a_);

    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{d_, Action::Start}));
    activate(transition, d_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{c_, Action::Start}));
    activate(transition, c_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{b_, Action::Start}));
    activate(transition, b_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{a_, Action::Start}));
    activate(transition, a_);

    EXPECT_THAT(collectReady(transition), ::testing::IsEmpty());
    EXPECT_TRUE(transition.isFinished());
}

TEST_F(LinearGraphTest, TransitionToOffStopsChainTopDownThenStartsOff)
{
    RecordProperty(
        "Description",
        "Transitioning to Off from a fully-active chain stops it in reverse dependency order: A, then B, "
        "then C, then D (each ready only once its single dependent is stopped), then activates the "
        "dependency-less Off node.");

    activateWholeChain();

    auto& transition = builder_->createTransition(off_);

    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{a_, Action::Stop}));
    deactivate(transition, a_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{b_, Action::Stop}));
    deactivate(transition, b_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{c_, Action::Stop}));
    deactivate(transition, c_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{d_, Action::Stop}));
    deactivate(transition, d_);

    // Chain fully stopped: the Off node is now ready to activate.
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{off_, Action::Start}));
    activate(transition, off_);

    EXPECT_THAT(collectReady(transition), ::testing::IsEmpty());
    EXPECT_TRUE(transition.isFinished());
}

TEST_F(LinearGraphTest, TransitionToCStopsNodesNotNeededByC)
{
    RecordProperty(
        "Description",
        "Transitioning to C while the whole chain A..D is active stops the higher-level nodes A and B "
        "that C does not need: the stop set is every running node outside the target subgraph, drained "
        "top-down (A, then B once A is stopped). C and its dependency D stay active throughout.");

    activateWholeChain();

    auto& transition = builder_->createTransition(c_);

    // A has no dependents, so it is ready to stop first; B becomes ready once A is stopped.
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{a_, Action::Stop}));
    deactivate(transition, a_);
    EXPECT_THAT(collectReady(transition), ::testing::ElementsAre(ReadyNode<IdentifierHash>{b_, Action::Stop}));
    deactivate(transition, b_);

    // C and D are already active, so there is nothing to start: the transition is done.
    EXPECT_THAT(collectReady(transition), ::testing::IsEmpty());
    EXPECT_TRUE(transition.isFinished());

    EXPECT_FALSE(componentAt(a_).active_);
    EXPECT_FALSE(componentAt(b_).active_);
    EXPECT_TRUE(componentAt(c_).active_);
    EXPECT_TRUE(componentAt(d_).active_);
}

}  // namespace score::mw::lifecycle::internal
