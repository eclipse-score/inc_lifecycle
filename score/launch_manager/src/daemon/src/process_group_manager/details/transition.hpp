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
#ifndef SCORE_LCM_TRANSITION_HPP
#define SCORE_LCM_TRANSITION_HPP

#include "score/mw/launch_manager/common/concurrency/fixed_size_queue.hpp"
#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/process_group_manager/details/dependency_graph.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"

#include <score/assert.hpp>

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <iostream>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace score::mw::lifecycle
{

/// @brief What should happen to a ready node right now.
enum class Action : std::uint8_t
{
    Stop,   ///< deactivate the node
    Start,  ///< activate the node
};

/// @brief Contains a node that is ready to be activated/deactivated
struct ReadyNode
{
    GraphIndex node;
    Action action;
};

inline bool operator==(const ReadyNode& lhs, const ReadyNode& rhs)
{
    return lhs.node == rhs.node && lhs.action == rhs.action;
}
inline bool operator!=(const ReadyNode& lhs, const ReadyNode& rhs)
{
    return !(lhs == rhs);
}

template <typename T>
class TransitionBuilder;

namespace detail
{
/// @brief True iff componentOf(U&) is callable (via ADL) and yields a reference convertible to
/// IComponent&. Used to enforce the requirement of the Transition<T> template parameter.
template <typename U, typename = void>
struct is_component_type : std::false_type
{
};

template <typename U>
struct is_component_type<U, std::void_t<decltype(componentOf(std::declval<U&>()))>>
    : std::is_convertible<decltype(componentOf(std::declval<U&>())), internal::IComponent&>
{
};
}  // namespace detail

/// @brief The Transition computes the nodes to activate/deactivate when
///        transitioning between states in the graph.
/// @details The Transition computes the nodes that need to be deactivated
///          (those nodes not reachable from the target state) and those that
///          need to be activated (those reachable from the target that are
///          not yet active).
///
template <typename T>
class Transition
{
    // The transition is split into two phases:
    // - Stopping Phase: Every node that is currently running (@ref stopped() is false) and is
    //   not part of the target subgraph is deactivated. The stop set is derived from live component state across
    //   the whole graph rather than from a source subgraph, so it also captures nodes left running by a previously
    //   aborted transition. Once all nodes in the Stopping phase are finished, the transition moves to the Starting
    //   phase.
    // - Starting Phase: The nodes that need to be activated are processed next. Once all nodes in the
    //   Starting phase are finished, the transition is complete.
    //
    // The template parameter is expected to be a type that can be used to retrieve the corresponding IComponent
    // instance via the componentOf() function, which is expected to be defined for the type T.

    static_assert(
        detail::is_component_type<T>::value,
        "Transition<T> requires an ADL-findable componentOf(T&) that returns a reference "
        "to IComponent&.");

    friend class TransitionBuilder<T>;

  public:
    /// @brief Pop the next ready node, or std::nullopt if none is ready right now
    /// @details Unlike a getter, this CONSUMES: a node is returned at most once and
    /// is gone from the frontier the moment it's returned. Safe to interleave
    /// with onNodeFinished() — nodes onNodeFinished() appends are queued behind
    /// whatever's already pending, never lost, regardless of consumption order.
    std::optional<ReadyNode> nextReady()
    {
        if (state_.next_nodes.empty())
        {
            return std::nullopt;
        }
        return ReadyNode{state_.next_nodes.tryPop().value(), currentAction()};
    }

    /// @brief Input iterator that drains the transition via nextReady().
    /// @details Holds only a pointer + a cached ReadyNode — no allocation.
    /// CONSUMING: advancing pops from the shared frontier, so only one traversal (e.g. one range-for)
    /// should be in flight at a time; a second `begin()` continues where the
    /// first left off, it does not restart.
    class Iterator
    {
      public:
        using value_type = ReadyNode;
        using reference = ReadyNode;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using pointer = void;

        Iterator() = default;  // end sentinel: owner_ == nullptr
        explicit Iterator(Transition* owner) : owner_(owner)
        {
            advance();
        }

        ReadyNode operator*() const
        {
            return *current_;
        }
        Iterator& operator++()
        {
            advance();
            return *this;
        }
        bool operator==(const Iterator& other) const
        {
            return current_.has_value() == other.current_.has_value();
        }
        bool operator!=(const Iterator& other) const
        {
            return !(*this == other);
        }

      private:
        void advance()
        {
            current_ = owner_ ? owner_->nextReady() : std::nullopt;
        }

        Transition* owner_ = nullptr;
        std::optional<ReadyNode> current_;
    };

    Iterator begin()
    {
        return Iterator{this};
    }
    Iterator end()
    {
        return Iterator{};
    }

    /// @brief Mark @p node finished and refresh the list of ready nodes
    /// @details A successor becomes ready when:
    ///   activation:   it is part of the target subgraph, and every dependency it
    ///                 has is active()
    ///   deactivation: every dependent it has is stopped()
    void onNodeFinished(GraphIndex node)
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT(isValidNode(node));

        --state_.pending;  // completion counter; only detects done-ness, never gates order

        if (state_.pending == 0)  // active phase drained
        {
            if (state_.phase == Phase::Stopping)
            {
                advanceToStarting();  // sets up the starting-phase heads
            }
            else
            {
                state_.phase = Phase::Done;
            }
            return;
        }

        // Still within the phase: append the finished node's neighbours in the
        // phase's direction, filtered by readiness, behind whatever's already
        // waiting to be dispatched.
        const auto& successors = state_.phase == Phase::Starting ? graph_.dependents(node) : graph_.dependsOn(node);
        for (const GraphIndex s : successors)
        {
            if (isReady(s) && !state_.enqueued_set.test(s))
            {
                state_.enqueued_set.set(s);
                SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
                    state_.next_nodes.push(s), "Transition queue should never exceed capacity");
            }
        }
    }

    /// @return True once every node participating in this transition has reached
    /// its terminal state.
    bool isFinished() const
    {
        return state_.phase == Phase::Done;
    }

  private:
    /// @brief True iff @p node is a valid index into the underlying graph, i.e. in [0, size()).
    bool isValidNode(GraphIndex node) const
    {
        return node < graph_.size();
    }

    /// @brief Construct a reusable Transition for the given graph.
    /// @details All the memory needed for a transition is allocated here, so that no further allocations are
    /// needed while the transition is in flight. The same transition object is then reused for multiple transitions by
    /// calling @ref setupTransition() with a new target node.
    explicit Transition(DependencyGraph<T>& graph) : state_(graph.capacity()), graph_(graph)
    {
        state_.in_target_subgraph.assign(graph.capacity(), false);
    }

    /// @brief Set up a fresh transition to @p target.
    /// @details Starts in the Stopping Phase: every node currently running and not needed by @p target is
    /// deactivated (derived from live component state across the whole graph, so nodes left running by a previous
    /// aborted transition are captured too). Then moves to the Starting Phase to bring up @p target.
    void setupTransition(GraphIndex target)
    {
        std::fill(state_.in_target_subgraph.begin(), state_.in_target_subgraph.end(), false);

        state_.target_root = target;
        clearNextNodes();
        state_.pending = 0;
        state_.phase = Phase::Stopping;
        state_.enqueued_set.reset();
        setupDeactivation(target);
        if (state_.pending == 0)
        {
            advanceToStarting();
        }
    }

    enum class Phase : std::uint8_t
    {
        Stopping,  ///< Deactivating nodes exclusive to the `source` state
        Starting,  ///< Activating new nodes that are exclusive to the `target` state
        Done,      ///< every participating node has reached its terminal state
    };

    struct State
    {
        /// @brief Per-node membership mask
        /// @details in_target_subgraph[i] is true iff node
        /// i is reachable from target_root (i.e. belongs to the subgraph about to
        /// be running). Recomputed at every setup. Serves two purposes:
        ///   stopping:  the nodes excluded from the whole-graph stop scan, and the
        ///              nodes onNodeFinished must never (re)stop.
        ///   starting:  nodes that are directly or indirectly depended on by
        ///              the target_root.
        std::vector<bool> in_target_subgraph;

        /// @brief The destination subgraph's root (the `target` endpoint)
        GraphIndex target_root{};

        /// @brief The nodes that are ready to be activated/deactivated in the current phase, in the order they were
        /// discovered.
        internal::FixedSizeQueue<GraphIndex> next_nodes;
        std::size_t pending = 0;    // nodes still to reach terminal state in this phase
        Phase phase = Phase::Done;  // active vs deactivation vs finished

        /// @brief Nodes that have been enqueued for the current transition phase
        /// @deprecated This is a workaround for the case where two processes are started in parallel and their events
        /// processed in sequence. Both onNodeFinished() calls detect that all dependents are ready and try to enqueue
        /// successors. Detection of dependency readiness should be reworked to remove this.
        std::bitset<static_cast<std::size_t>(internal::ProcessLimits::kMaxProcesses)> enqueued_set{};

        State(std::size_t nodes) : next_nodes(nodes)
        {
        }
    };

    /// @brief Check if the node is active
    bool active(GraphIndex i)
    {
        return componentOf(graph_[i]).active();
    }

    /// @brief Check if the node is stopped
    bool stopped(GraphIndex i)
    {
        return !componentOf(graph_[i]).active();
    }

    /// @brief Check if all dependencies of the given node are active
    bool allDepsActive(GraphIndex i)
    {
        const auto& d = graph_.dependsOn(i);
        return std::all_of(d.begin(), d.end(), [this](GraphIndex dep) {
            return active(dep);
        });
    }

    /// @brief Check if all dependents of the given node are stopped
    bool allDependentsStopped(GraphIndex i)
    {
        const auto& d = graph_.dependents(i);
        return std::all_of(d.begin(), d.end(), [this](GraphIndex dep) {
            return stopped(dep);
        });
    }

    State state_;
    DependencyGraph<T>& graph_;

    /// @brief The action based on whether the transition is in the Stopping or Starting phase
    Action currentAction() const
    {
        return state_.phase == Phase::Starting ? Action::Start : Action::Stop;
    }

    /// @brief Check if the node is ready to be activated/deactivated in the current phase.
    bool isReady(GraphIndex s)
    {
        return state_.phase == Phase::Starting
                   ? (state_.in_target_subgraph[s] && !active(s) && allDepsActive(s))
                   : (!state_.in_target_subgraph[s] && !stopped(s) && allDependentsStopped(s));
    }

    void clearNextNodes()
    {
        while (!state_.next_nodes.empty())
        {
            static_cast<void>(state_.next_nodes.tryPop());
        }
    }

    /// @brief Leave the Stopping phase for the Starting phase.
    /// @details Sets up activation of the `target` subgraph; if there is nothing to start (everything is
    /// already active()), the transition is finished.
    void advanceToStarting()
    {
        state_.phase = Phase::Starting;
        state_.pending = 0;
        clearNextNodes();
        state_.enqueued_set.reset();

        setupActivation(state_.target_root);
        if (state_.pending == 0)
        {
            state_.phase = Phase::Done;
        }
    }

    /// @brief Setup the starting phase of the transition
    /// @details This will traverse the target subgraph and initialize the following bookkeeping structures:
    /// - in_target_subgraph: marks the nodes that are part of the target subgraph
    /// - next_nodes: the list of nodes that are ready to be activated (those whose dependencies are all active)
    /// - pending: the count of nodes that are still to be activated
    void setupActivation(GraphIndex root)
    {
        graph_.traverse(root, [this](GraphIndex i) -> const std::vector<GraphIndex>& {
            state_.in_target_subgraph[i] = true;
            if (!active(i))
            {
                ++state_.pending;
                if (allDepsActive(i))
                {
                    state_.next_nodes.push(i);
                }
            }
            return graph_.dependsOn(i);
        });
    }

    /// @brief Setup the Stopping phase of the transition
    /// @details First traverses the target subgraph to initialize:
    /// - in_target_subgraph: marks the nodes that are part of the target subgraph (must stay running)
    /// Then scans the complete graph to initialize:
    /// - pending: the count of running nodes that must be deactivated
    /// - next_nodes: those already ready to be deactivated (dependents all stopped)
    /// The stop set is every node that is currently running (@ref stopped() is false) and is not in the target
    /// subgraph. Deriving it from live component state rather than from a source root makes it independent of how the
    /// previous transition ended, so nodes left running by an aborted transition — even ones outside any assumed
    /// source subgraph — are still stopped.
    void setupDeactivation(GraphIndex target)
    {
        graph_.traverse(target, [this](GraphIndex i) -> const std::vector<GraphIndex>& {
            state_.in_target_subgraph[i] = true;
            return graph_.dependsOn(i);
        });
        for (GraphIndex i = 0; i < graph_.size(); ++i)
        {
            if (!state_.in_target_subgraph[i] && !stopped(i))
            {
                ++state_.pending;

                if (allDependentsStopped(i))
                {
                    state_.next_nodes.push(i);
                }
            }
        }
    }
};

/// @brief The TransitionBuilder owns the single Transition object for a given graph, which is reused for each
/// Transition.
/// @details The builder only supports a single transition at a time. It is
/// expected that whenever a new transition is created, the previous one is no longer in use.
/// The reason is that Memory is only allocated during initialization and then reused for each transition.
template <typename T>
class TransitionBuilder final
{
  public:
    explicit TransitionBuilder(DependencyGraph<T>& graph) : transition_(graph)
    {
    }

    /// @brief A transition to @p target
    /// @details First deactivates every node currently running that is not needed by @p target (keeping anything
    /// shared with @p target active), then activates all nodes reachable from @p target. The stop set is derived from
    /// live component state, so this recovers correctly even when a previous transition was aborted mid-flight.
    Transition<T>& createTransition(GraphIndex target)
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT(transition_.isValidNode(target));
        transition_.setupTransition(target);
        return transition_;
    }

  private:
    /// @brief The single reusable transition
    Transition<T> transition_;
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_LCM_TRANSITION_HPP
