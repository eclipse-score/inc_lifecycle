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
#ifndef SCORE_LCM_DEPENDENCY_GRAPH_HPP
#define SCORE_LCM_DEPENDENCY_GRAPH_HPP

#include "score/mw/launch_manager/process_group_manager/details/reservable_queue.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace score::mw::lifecycle
{

/// @brief Index type used to identify nodes in the graph.
using GraphIndex = std::size_t;

/// @brief Stores a set of nodes as a directed acyclic graph (DAG) with edges representing dependencies between nodes.
/// @details The class provides methods to create and traverse the graph.
template <typename T>
class DependencyGraph
{
  private:
    /// @brief Wrapper around objects in the graph to store information about dependencies.
    struct GraphNode
    {
        T value;
        std::vector<GraphIndex> depends_on;
        std::vector<GraphIndex> dependents;

        /// @brief Constructor to allow in-place construction of T.
        template <typename... Args>
        GraphNode(Args&&... args) : value(std::forward<Args>(args)...)
        {
        }
    };

  public:
    /// @param count The exact number of nodes that will be added.
    DependencyGraph(std::size_t count)
    {
        nodes.reserve(count);
        traversal_queue.reserve(std::max(count, std::size_t(2)) - 1);
        visited.resize(count);
    }

    /// @brief Construct a new node in-place. Returns the node's index, which equals the current size
    /// before insertion (i.e. the first node is 0, second is 1, etc.).
    template <typename... Args>
    GraphIndex emplace(Args&&... args)
    {
        nodes.emplace_back(std::forward<Args>(args)...);
        return nodes.size() - 1;
    }

    /// @brief Add an edge: @p node depends on @p depends_on.
    /// During activation, depends_on will be started before node.
    /// During deactivation, node will be stopped before depends_on.
    void addDependency(const GraphIndex node, const GraphIndex depends_on)
    {
        nodes[node].depends_on.push_back(depends_on);
        nodes[depends_on].dependents.push_back(node);
    }

    /// @return The number of nodes in the graph.
    std::size_t size() const
    {
        return nodes.size();
    }

    /// @return The number of nodes this graph can hold without reallocating (the @c count
    /// reserved at construction).
    std::size_t capacity() const
    {
        return nodes.capacity();
    }

    T& operator[](GraphIndex index)
    {
        return nodes[index].value;
    }

    /// @return The nodes that @p index depends on.
    const std::vector<GraphIndex>& dependsOn(GraphIndex index) const
    {
        return nodes[index].depends_on;
    }

    /// @return The nodes that depend on @p index.
    const std::vector<GraphIndex>& dependents(GraphIndex index) const
    {
        return nodes[index].dependents;
    }

    /// @brief Traverse the graph, starting at @p start, performing @p per_node on each node and moving to the nodes
    /// provided by the return value from @p per_node. Only nodes such that @c filter(node) is true are traversed.
    /// Nodes are visited at most once.
    template <typename PerNodeFn, typename FilterFn>
    void traverse(const GraphIndex start, PerNodeFn per_node, FilterFn filter)
    {
        visited.assign(visited.size(), false);
        traversal_queue.push(start);
        visited[start] = true;
        while (!traversal_queue.empty())
        {
            auto current = traversal_queue.pop();

            const auto& neighbors = per_node(current);

            for (const auto neighbor : neighbors)
            {
                if (visited[neighbor] || !filter(neighbor))
                {
                    continue;
                }
                traversal_queue.push(neighbor);
                visited[neighbor] = true;
            }
        }
    }

  private:
    std::vector<GraphNode> nodes;

    /// @brief Presized queue reused by single-threaded traversals.
    ReservableQueue<GraphIndex> traversal_queue;
    /// @brief Presized visited set reused by single-threaded traversals.
    std::vector<bool> visited;
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_LCM_DEPENDENCY_GRAPH_HPP
