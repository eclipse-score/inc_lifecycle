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
 * SPDX-License-GraphIndex: Apache-2.0
 ********************************************************************************/
#ifndef SCORE_LCM_DEPENDENCY_GRAPH_HPP
#define SCORE_LCM_DEPENDENCY_GRAPH_HPP

#include "score/assert.hpp"
#include "score/mw/launch_manager/common/concurrency/fixed_size_queue.hpp"

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace score::mw::lifecycle
{

/// @brief Stores a set of nodes as a directed acyclic graph (DAG) with edges representing dependencies between nodes.
/// @details The class provides methods to create and traverse the graph.
template <class GraphIndex, class T>
class DependencyGraph
{
  private:
    /// @brief Wrapper around objects in the graph to store information about dependencies.
    struct GraphNode
    {
        T value;
        std::vector<GraphIndex> depends_on;
        std::vector<GraphIndex> dependents;
        bool visited{false};

        /// @brief Constructor to allow in-place construction of T.
        template <typename... Args>
        explicit GraphNode(Args&&... args) : value(std::forward<Args>(args)...)
        {
        }
    };

  public:
    /// @param count The exact number of nodes that will be added.
    ///
    /// @details The size of the internal traversal queue is either count - 1 or 1. This is because in each traversal
    /// one node is pushed to the queue and then popped. From then on, dependencies are pushed to the queue.
    explicit DependencyGraph(const std::size_t count) : traversal_queue(std::max(count, 2UL) - 1)
    {
        nodes.reserve(count);
    }

    /// @brief Construct a new node in-place. Returns the node's index, which equals the current size
    /// before insertion (i.e. the first node is 0, second is 1, etc.).
    template <typename... Args>
    GraphIndex emplace(Args&&... args)
    {
        auto& res = nodes.try_emplace(std::forward<Args>(args)...);
        return res.first->first;
    }

    /// @brief Add an edge: @p node depends on @p depends_on.
    /// During activation, depends_on will be started before node.
    /// During deactivation, node will be stopped before depends_on.
    void addDependency(const GraphIndex node, const GraphIndex depends_on)
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
            nodes[node].depends_on.size() < capacity(), "More dependencies added than there are nodes in the graph");
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
            nodes[depends_on].dependents.size() < capacity(),
            "More dependencies added than there are nodes in the graph");
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
        return nodes.max_size();
    }

    T& operator[](GraphIndex index)
    {
        return nodes[index].value;
    }

    const T& operator[](const GraphIndex index) const
    {
        return nodes.at(index).value;
    }

    /// @return The nodes that @p index depends on.
    const std::vector<GraphIndex>& dependsOn(GraphIndex index) const
    {
        return nodes.at(index).depends_on;
    }

    /// @return The nodes that depend on @p index.
    const std::vector<GraphIndex>& dependents(GraphIndex index) const
    {
        return nodes.at(index).dependents;
    }

    /// @brief Traverse the graph, starting at @p start, performing @p per_node
    ///        on each node and moving to the nodes provided by the return
    ///        value from @p per_node.
    template <typename PerNodeFn>
    void traverse(const GraphIndex start, PerNodeFn per_node)
    {
        for (auto& [key, value] : nodes)
        {
            value.visited = false;
        }
        auto push_res = traversal_queue.push(start);
        static_cast<void>(push_res);
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(push_res, "Traversal queue was already full");
        nodes[start].visited = true;
        while (!traversal_queue.empty())
        {
            const auto pop_res = traversal_queue.tryPop();
            SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(pop_res.has_value(), "Pop failed even though queue was not empty");
            const auto current = pop_res.value();

            const auto& neighbors = per_node(current);

            for (const auto neighbor : neighbors)
            {
                if (nodes[neighbor].visited)
                {
                    continue;
                }
                push_res = traversal_queue.push(neighbor);
                SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(push_res, "Traversal queue was already full");
                nodes[neighbor].visited = true;
            }
        }
    }

    /// @brief Iterator over node values.
    struct ValueIterator
    {
        typename std::unordered_map<GraphIndex, GraphNode>::iterator it;
        T& operator*()
        {
            return it->second.value;
        }

        ValueIterator& operator++()
        {
            ++it;
            return *this;
        }

        bool operator!=(const ValueIterator& other) const
        {
            return it != other.it;
        }
    };

    /// @returns Iterator at the beginning of the nodes store.
    ValueIterator begin()
    {
        return ValueIterator{nodes.begin()};
    }

    /// @returns Iterator at the end of the nodes store.
    ValueIterator end()
    {
        return ValueIterator{nodes.end()};
    }

  private:
    std::unordered_map<GraphIndex, GraphNode> nodes;

    /// @brief Presized queue reused by single-threaded traversals.
    internal::FixedSizeQueue<GraphIndex> traversal_queue;
};

template class DependencyGraph<int, int>;

}  // namespace score::mw::lifecycle

#endif  // SCORE_LCM_DEPENDENCY_GRAPH_HPP
