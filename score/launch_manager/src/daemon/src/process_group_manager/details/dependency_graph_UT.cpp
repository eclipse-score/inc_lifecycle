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

#include <gtest/gtest.h>

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/process_group_manager/details/dependency_graph.hpp"

namespace score::lcm
{

TEST(DependencyGraphTest, EmplaceAndAccessByIndex)
{
    const std::string_view text = "AAAAA";
    score::lcm::DependencyGraph<IdentifierHash> graph(1);
    const auto res = graph.emplace(text);

    auto& hash = graph[res];
    EXPECT_EQ(hash, IdentifierHash{text});
}

TEST(DependencyGraphTest, EnqueueStartNodesEnqueuesOnlyReadyNodes)
{
    DependencyGraph<IdentifierHash> graph(2);
    const auto dep = graph.emplace("dep");
    const auto root = graph.emplace("root");
    graph.addDependency(root, dep);

    const auto& nodes = graph.activate(root);

    ASSERT_EQ(nodes.size(), 1);
    EXPECT_EQ(graph[nodes[0]], IdentifierHash{"dep"});
}

TEST(DependencyGraphTest, CompletingDependencyEnqueuesDependent)
{
    DependencyGraph<IdentifierHash> graph(2);
    const auto dep = graph.emplace("dep");
    const auto root = graph.emplace("root");
    graph.addDependency(root, dep);

    std::vector<IdentifierHash> enqueued;
    auto collect = [&](IdentifierHash& hash) {
        enqueued.push_back(hash);
    };

    const auto& head_nodes = graph.activate(root);
    for (const auto i : head_nodes) {
        collect(graph[i]);
    }
    graph.enqueueActivationSuccessors(dep, collect);
    EXPECT_TRUE(graph.enqueueActivationSuccessors(root, collect));

    ASSERT_EQ(enqueued.size(), 2);
    EXPECT_EQ(enqueued[0], IdentifierHash{"dep"});
    EXPECT_EQ(enqueued[1], IdentifierHash{"root"});
}

TEST(DependencyGraphTest, DependencyCleanupOrder)
{
    DependencyGraph<IdentifierHash> graph(2);
    const auto dep = graph.emplace("dep");
    const auto root = graph.emplace("root");
    const auto root2 = graph.emplace("1.41");
    graph.addDependency(root, dep);

    std::vector<IdentifierHash> enqueued;
    auto collect = [&](IdentifierHash& hash) {
        enqueued.push_back(hash);
    };

    const auto& activation_head_nodes = graph.activate(root);
    ASSERT_EQ(activation_head_nodes.size(), 1);
    EXPECT_FALSE(graph.enqueueActivationSuccessors(dep, collect));
    EXPECT_TRUE(graph.enqueueActivationSuccessors(root, collect));
    enqueued.clear();
    graph.exclude(root2);
    const auto& deactivation_head_nodes = graph.deactivate(root);
    ASSERT_EQ(deactivation_head_nodes.size(), 1);
    enqueued.push_back(graph[activation_head_nodes[0]]);
    EXPECT_FALSE(graph.enqueueDeactivationSuccessors(root, collect));
    EXPECT_TRUE(graph.enqueueDeactivationSuccessors(dep, collect));

    ASSERT_EQ(enqueued.size(), 2);
    EXPECT_EQ(enqueued[0], IdentifierHash{"root"});
    EXPECT_EQ(enqueued[1], IdentifierHash{"dep"});
}

TEST(DependencyGraphTest, ActivateTopLayerAndThenDeactivate)
{
    DependencyGraph<IdentifierHash> graph(2);
    const auto base = graph.emplace("base");
    const auto icing = graph.emplace("icing");
    graph.addDependency(icing, base);

    std::vector<IdentifierHash> enqueued;
    auto collect = [&](IdentifierHash& hash) {
        enqueued.push_back(hash);
    };

    // Activate base
    enqueued.clear();
    for (const auto i : graph.activate(base)) {
        collect(graph[i]);
    }
    EXPECT_TRUE(graph.enqueueActivationSuccessors(base, collect));

    ASSERT_EQ(enqueued.size(), 1);
    EXPECT_EQ(enqueued[0], IdentifierHash{"base"});

    enqueued.clear();
    // Then activate icing
    graph.exclude(icing);
    // Nothing to deactivate
    EXPECT_EQ(graph.deactivate(base).size(), 0);

    for (const auto i : graph.activate(icing)) {
        collect(graph[i]);
    }
    EXPECT_FALSE(graph.enqueueActivationSuccessors(base, collect));
    EXPECT_TRUE(graph.enqueueActivationSuccessors(icing, collect));

    ASSERT_EQ(enqueued.size(), 2);
    EXPECT_EQ(enqueued[0], IdentifierHash{"base"});
    EXPECT_EQ(enqueued[1], IdentifierHash{"icing"});

    enqueued.clear();
    // Switch back to base
    graph.exclude(base);

    for (const auto i : graph.deactivate(icing)) {
        collect(graph[i]);
    }
    EXPECT_TRUE(graph.enqueueDeactivationSuccessors(icing, collect));

    ASSERT_EQ(enqueued.size(), 1);
    EXPECT_EQ(enqueued[0], IdentifierHash{"icing"});

    enqueued.clear();
    graph.exclude(icing);
    for (const auto i : graph.activate(base)) {
        collect(graph[i]);
    }
    EXPECT_TRUE(graph.enqueueActivationSuccessors(base, collect));

    ASSERT_EQ(enqueued.size(), 1);
    EXPECT_EQ(enqueued[0], IdentifierHash{"base"});
}

}  // namespace score::lcm
