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
template <class Key, class T>
class DependencyGraph
{
  private:
    static_assert(
        std::is_trivially_copyable_v<Key>,
        "This class takes copies of keys so they should be trivially copyable");

    /// @brief Wrapper around objects in the graph to store information about dependencies.
    struct GraphNode
    {
        /// @brief The underlying object stored in this graph.
        T value;
        /// @brief Nodes that this node needs to be ready before it can launch.
        std::vector<Key> depends_on;
        /// @brief Nodes that depend on this node being ready before they can launch.
        std::vector<Key> dependents;

        /// @brief Temporary flag set when this node is traversed.
        /// @warning This should be reset at the start of each traversal for valid results.
        bool visited{false};

        /// @brief Constructor to allow in-place construction of T.
        template <typename... Args>
        explicit GraphNode(Args&&... args) : value(std::forward<Args>(args)...)
        {
        }
    };

    using iterator = typename std::unordered_map<Key, GraphNode>::iterator;

  public:
    /// @param count The exact number of nodes that will be added.
    ///
    /// @details The size of the internal traversal queue is either count - 1 or 1. This is because in each traversal
    /// one node is pushed to the queue and then popped. From then on, dependencies are pushed to the queue.
    explicit DependencyGraph(const std::size_t count) : capacity_(count), traversal_queue(std::max(count, 2UL) - 1)
    {
        nodes.reserve(count);
    }

    /// @brief Construct a new node in-place. Returns the node's key.
    /// @warning If the key is already present in the graph, the new node is not inserted.
    template <typename... Args>
    Key try_emplace(Key key, Args&&... args)
    {
        std::pair<iterator, bool> res = nodes.try_emplace(key, std::forward<Args>(args)...);
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
            res.second, "Element was not inserted. This means that the key was already present in the map.");
        return res.first->first;
    }

    /// @brief Add an edge: @p node depends on @p depends_on.
    /// During activation, depends_on will be started before node.
    /// During deactivation, node will be stopped before depends_on.
    /// @pre @p node and @p depends_on must both be present in the graph
    void addDependency(Key node, Key depends_on)
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
            nodes.at(node).depends_on.size() < capacity(), "More dependencies added than there are nodes in the graph");
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
            nodes.at(depends_on).dependents.size() < capacity(),
            "More dependencies added than there are nodes in the graph");
        nodes.at(node).depends_on.push_back(depends_on);
        nodes.at(depends_on).dependents.push_back(node);
    }

    /// @return The number of nodes in the graph.
    std::size_t size() const noexcept
    {
        return nodes.size();
    }

    /// @return The number of nodes this graph can hold without reallocating (the @c count
    /// reserved at construction).
    std::size_t capacity() const noexcept
    {
        return capacity_;
    }

    /// @brief Returns a mutable reference to the node at @p key
    /// @pre @p key must be present in the graph.
    T& operator[](Key key)
    {
        return nodes.at(key).value;
    }

    /// @brief Returns a constant reference to the node at @p key
    /// @pre @p key must be present in the graph.
    const T& operator[](const Key key) const
    {
        return nodes.at(key).value;
    }

    /// @return The nodes that @p key depends on.
    /// @pre @p key must be present in the graph.
    const std::vector<Key>& dependsOn(Key key) const
    {
        return nodes.at(key).depends_on;
    }

    /// @return The nodes that depend on @p key.
    /// @pre @p key must be present in the graph.
    const std::vector<Key>& dependents(Key key) const
    {
        return nodes.at(key).dependents;
    }

    /// @brief Traverse the graph, starting at @p start, performing @p per_node
    ///        on each node and moving to the nodes provided by the return
    ///        value from @p per_node.
    /// @pre @p start must be present in the graph.
    template <typename PerNodeFn>
    void traverse(Key start, PerNodeFn per_node)
    {
        std::for_each(nodes.begin(), nodes.end(), [](std::pair<const Key, GraphNode>& it) {
            GraphNode& node = it.second;
            node.visited = false;
        });
        auto push_res = traversal_queue.push(start);
        static_cast<void>(push_res);
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(push_res, "Traversal queue was already full");
        nodes.at(start).visited = true;
        while (!traversal_queue.empty())
        {
            const auto pop_res = traversal_queue.tryPop();
            SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(pop_res.has_value(), "Pop failed even though queue was not empty");
            const auto current = pop_res.value();

            const auto& neighbors = per_node(current);

            for (const auto neighbor : neighbors)
            {
                if (nodes.at(neighbor).visited)
                {
                    continue;
                }
                push_res = traversal_queue.push(neighbor);
                SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(push_res, "Traversal queue was already full");
                nodes.at(neighbor).visited = true;
            }
        }
    }

    /// @brief Iterator over node values.
    struct ValueIterator
    {
        iterator it;
        std::pair<Key, T&> operator*()
        {
            return std::pair<Key, T&>(it->first, it->second.value);
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

        bool operator==(const ValueIterator& other) const
        {
            return it == other.it;
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

    ValueIterator find(Key key)
    {
        return ValueIterator{nodes.find(key)};
    }

  private:
    /// @brief The number of nodes the graph expects to hold
    std::size_t capacity_;

    std::unordered_map<Key, GraphNode> nodes;

    /// @brief Presized queue reused by single-threaded traversals.
    internal::FixedSizeQueue<Key> traversal_queue;
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_LCM_DEPENDENCY_GRAPH_HPP
