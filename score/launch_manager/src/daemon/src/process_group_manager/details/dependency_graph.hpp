#ifndef SCORE_LCM_DEPENDENCY_GRAPH_HPP
#define SCORE_LCM_DEPENDENCY_GRAPH_HPP

#include "score/mw/launch_manager/process_group_manager/details/reservable_queue.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <vector>

namespace score::lcm
{

/// @brief Stores a graph of objects with dependencies on one another. Allows access to nodes in order of fulfilled
/// dependencies.
template <typename T>
class DependencyGraph
{
  public:
    /// @brief Index type used to identify nodes in the graph.
    using GraphIndex = std::size_t;

  private:
    /// @brief Wrapper around objects in the graph to store information about dependencies
    struct GraphNode
    {
        T value;
        /// @brief Number of nodes left before this one can begin. Reused for startup and shutdown deps
        std::atomic<std::size_t> remaining_dependencies{};
        std::vector<GraphIndex> depends_on;
        std::vector<GraphIndex> dependents;
        /// @brief True if this node is in the desired subgraph. 'included' nodes are queued for activation/deactivation
        /// when a parent node is activated/deactivated
        bool included{false};

        /// @brief True if there are no nodes that need to execute before this one.
        bool dependencies_fulfilled()
        {
            return remaining_dependencies.load() == 0;
        }

        /// @brief Constructor to allow in-place construction of T
        template <typename... Args>
        GraphNode(Args&&... args) : value(std::forward<Args>(args)...)
        {
        }

        /// @brief Explicit move constructor needed for atomics
        GraphNode(GraphNode&& other) noexcept
            : value(std::move(other.value)),
              remaining_dependencies(other.remaining_dependencies.load()),
              depends_on(std::move(other.depends_on)),
              dependents(std::move(other.dependents)),
              included(std::move(other.included))
        {
        }

        GraphNode(GraphNode& other) = default;
        GraphNode& operator=(const GraphNode& other) = default;
        GraphNode& operator=(GraphNode&& other) = default;
        ~GraphNode() = default;
    };

  public:
    /// @param count The exact number of nodes that will be added.
    DependencyGraph(std::size_t count)
    {
        nodes.reserve(count);
        head_nodes.reserve(count);
        traversal_queue.reserve(std::max(count, std::size_t(2)) - 1);
        visited.resize(count);
    }

    /// @brief Construct a new node in-place. Returns the node's index, which equals the current size
    /// before insertion (i.e. the first node is 0, second is 1, etc.).
    template <typename... Args>
    GraphIndex emplace(Args... args)
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

    /// @brief Begin a activation of @p node .
    /// @param skip Optional criteria to skip nodes and their dependencies by
    /// @return A vector of all directly or indirectly depended
    /// on nodes with no dependencies.
    const std::vector<GraphIndex>& activate(const GraphIndex node)
    {
        return activate(node, [](T&) {
            return false;
        });
    }

    /// @brief Begin a activation of @p node .
    /// @param skip Optional criteria to skip nodes and their dependencies by
    /// @return A vector of all directly or indirectly depended
    /// on nodes with no dependencies.
    template <typename SkipFn>
    const std::vector<GraphIndex>& activate(const GraphIndex node, SkipFn skip)
    {
        head_nodes.clear();
        resetIfRequested();

        simpleTraverse(node, [](GraphNode& current) {
            current.included = true;
            return current.depends_on;
        });

        traverse(
            node,
            [&](GraphIndex index) -> const std::vector<GraphIndex>& {
                auto& current = nodes[index];
                assert(current.remaining_dependencies == 0 && "Job started with leftover dependencies");
                current.remaining_dependencies =
                    std::count_if(current.depends_on.begin(), current.depends_on.end(), [this, skip](GraphIndex dep) {
                        return !skip(nodes[dep].value);
                    });
                if (current.dependencies_fulfilled())
                {
                    head_nodes.push_back(index);
                }
                return current.depends_on;
            },
            [&](GraphIndex index) {
                return !skip(nodes[index].value);
            });
        return head_nodes;
    }

    /// @brief Begin a deactivation of @p node .
    /// @pre @p node must have no active dependents (i.e. it is a "head" in the active subgraph).
    /// @return A vector of all nodes with no active dependents directly or indirectly depended on by @p node . This
    /// will either be @p node , or empty if the node has been excluded (nothing to deactivate).
    const std::vector<GraphIndex>& deactivate(const GraphIndex node)
    {
        head_nodes.clear();
        resetIfRequested();
        assert(leaves_to_deactivate == 0 && "Variables must be reset before deactivated can be called again");

        if (!nodes[node].included)
        {
            return head_nodes;
        }

        traverse(
            node,
            [this](GraphIndex index) -> const std::vector<GraphIndex>& {
                auto& current = nodes[index];
                assert(current.remaining_dependencies == 0 && "Job started with leftover dependencies");

                current.remaining_dependencies =
                    std::count_if(current.dependents.begin(), current.dependents.end(), [this](GraphIndex dep) {
                        return nodes[dep].included;
                    });

                bool has_dependencies =
                    std::any_of(current.depends_on.begin(), current.depends_on.end(), [this](GraphIndex dep) {
                        return nodes[dep].included;
                    });

                if (!has_dependencies)
                {
                    leaves_to_deactivate++;
                }
                return current.depends_on;
            },
            [this](GraphIndex neighbor) {
                return nodes[neighbor].included;
            });

        assert(nodes[node].remaining_dependencies == 0 &&
               "This method should only be called on nodes without active dependents");
        head_nodes.push_back(node);
        return head_nodes;
    }

    /// @brief Notify the graph that @p node has finished activating and @p enqueue any successors.
    /// @warning each node must only be completed once per transition.
    /// @return true if @p node was the root, indicating that the activation is complete.
    template <typename EnqueueFn>
    bool enqueueActivationSuccessors(const GraphIndex node, EnqueueFn enqueue)
    {
        const auto successors = nodes[node].dependents;

        return !enqueueFulfilledSuccessors(successors, enqueue);
    }

    /// @brief Notify the graph that @p node has finished deactivating and @p enqueue any successors.
    /// @warning each node must only be completed once per transition.
    /// @return true if @p node was the last needed for deactivation, indicating that the deactivation is complete.
    template <typename EnqueueFn>
    bool enqueueDeactivationSuccessors(const GraphIndex node, EnqueueFn enqueue)
    {
        const auto successors = nodes[node].depends_on;

        bool is_leaf = !enqueueFulfilledSuccessors(successors, enqueue);
        if (!is_leaf)
        {
            return false;
        }
        assert(leaves_to_deactivate > 0 && "More leaves were deactivated than expected");
        return leaves_to_deactivate.fetch_sub(1) == 1;  // leaves_to_deactivate == 0
    }

    /// @brief Mark @p head and all of its dependencies as excluded. Excluded nodes are
    /// skipped during deactivation.
    /// @warning Must not be called while nodes are being enqueued.
    void exclude(const GraphIndex head)
    {
        simpleTraverse(head, [](GraphNode& current) {
            current.included = false;
            return current.depends_on;
        });
    }

    /// @brief Reset data on all nodes. Useful when a transition has been cancelled, meaning successors
    /// aren't processed.
    /// @details The reset is deferred to prevent a race when a successor enqueue is in progress
    void reset()
    {
        reset_requested = true;
    }

    T& operator[](GraphIndex index)
    {
        return nodes[index].value;
    }

    /// @brief Return the number of nodes in the graph.
    std::size_t size()
    {
        return nodes.size();
    }

    /// @brief Iterator over node values.
    struct ValueIterator
    {
        typename std::vector<GraphNode>::iterator it;
        T& operator*()
        {
            return it->value;
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

    /// @returns Iterator at the beginning of the nodes store
    ValueIterator begin()
    {
        return ValueIterator{nodes.begin()};
    }

    /// @returns Iterator at the end of the nodes store
    ValueIterator end()
    {
        return ValueIterator{nodes.end()};
    }

    /// @returns True if there are no nodes in the graph
    bool empty()
    {
        return nodes.empty();
    }

  private:
    template <typename PerNodeFn>
    void traverse(const GraphIndex start, PerNodeFn per_node)
    {
        traverse(start, per_node, [](GraphIndex) {
            return true;
        });
    }

    /// @brief Traverse the graph, starting at @p start, performing @p per_node on each node and moving to the nodes
    /// provided by the return value from @p per_node. If @p filter is specified, only nodes such that @c filter(node)
    /// is true are traversed. Nodes are visited at most once.
    template <typename PerNodeFn, typename FilterFn>
    void traverse(const GraphIndex start, PerNodeFn per_node, FilterFn filter)
    {
        assert(traversal_queue.empty() && "Traversal queue was not empty");
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

    /// @brief Traverse without maintaining visited set, starting at @p start, performing @p per_node on each node and
    /// moving to the nodes provided by the return value from @p per_node.
    template <typename PerNodeFn>
    void simpleTraverse(const GraphIndex head, PerNodeFn per_node)
    {
        assert(traversal_queue.empty() && "Traversal queue was not empty");
        traversal_queue.push(head);
        while (!traversal_queue.empty())
        {
            auto& current = nodes[traversal_queue.pop()];
            const auto& neighbors = per_node(current);
            for (const auto neighbor : neighbors)
            {
                traversal_queue.push(neighbor);
            }
        }
    }

    /// @brief Decrement remaining dependencies on all included @p successors. If there are no dependencies left, call
    /// @p enqueue on the successor
    template <typename EnqueueFn>
    bool enqueueFulfilledSuccessors(const std::vector<GraphIndex>& successors, EnqueueFn enqueue)
    {
        bool has_successors = false;
        for (const auto successor : successors)
        {
            if (!nodes[successor].included)
            {
                continue;
            }
            has_successors = true;
            // The following is safe as long as remaining_dependencies can't increase
            // while this is happening
            assert(nodes[successor].remaining_dependencies > 0 &&
                   "Dependency counter reached 0 before all dependencies were executed!");
            if (nodes[successor].remaining_dependencies.fetch_sub(1) == 1)  // Value is now 0
            {
                enqueue(nodes[successor].value);
            }
        }
        return has_successors;
    }

    /// @brief If a reset has been requested, clear all data relating to the current traversal.
    void resetIfRequested()
    {
        if (!reset_requested)
        {
            return;
        }
        for (auto& node : nodes)
        {
            node.included = false;
            node.remaining_dependencies = 0;
        }
        leaves_to_deactivate = 0;
        reset_requested = false;
    }

    std::vector<GraphNode> nodes;

    /// @brief Indices of nodes that can be queued immediately. Cleared and returned by const reference on
    /// activate/deactivate calls
    std::vector<GraphIndex> head_nodes;
    /// @brief Presized queue reused by single-threaded traversals.
    ReservableQueue<GraphIndex> traversal_queue;
    /// @brief Presized visited set reused by single-threaded traversals.
    std::vector<bool> visited;
    /// @brief The number of leaf nodes (nodes with empty depends_on) left to deactivate.
    std::atomic<std::size_t> leaves_to_deactivate = 0;
    /// @brief @c reset() has been called but data has not yet been reset
    bool reset_requested = false;
};

}  // namespace score::lcm

#endif  // SCORE_LCM_DEPENDENCY_GRAPH_HPP
