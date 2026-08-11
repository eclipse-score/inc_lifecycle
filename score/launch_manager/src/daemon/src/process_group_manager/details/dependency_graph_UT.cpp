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

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/process_group_manager/details/dependency_graph.hpp"

#include <vector>

namespace score::mw::lifecycle
{

using namespace score::mw::lifecycle;

TEST(DependencyGraphTest, EmplaceAndAccessByIndex)
{
    const std::string_view text = "AAAAA";
    DependencyGraph<IdentifierHash> graph(1);
    const auto res = graph.emplace(text);

    auto& hash = graph[res];
    EXPECT_EQ(hash, IdentifierHash{text});
}

TEST(DependencyGraphTest, EmplaceReturnsSequentialIndices)
{
    DependencyGraph<IdentifierHash> graph(3);
    const auto first = graph.emplace("a");
    const auto second = graph.emplace("b");
    const auto third = graph.emplace("c");

    EXPECT_EQ(first, 0U);
    EXPECT_EQ(second, 1U);
    EXPECT_EQ(third, 2U);
}

TEST(DependencyGraphTest, AddDependencyWiresDependsOnAndDependents)
{
    DependencyGraph<IdentifierHash> graph(2);
    const auto dep = graph.emplace("dep");
    const auto root = graph.emplace("root");
    graph.addDependency(root, dep);

    EXPECT_THAT(graph.dependsOn(root), ::testing::ElementsAre(dep));
    EXPECT_THAT(graph.dependents(dep), ::testing::ElementsAre(root));
    EXPECT_TRUE(graph.dependsOn(dep).empty());
    EXPECT_TRUE(graph.dependents(root).empty());
}

TEST(DependencyGraphTest, SizeReflectsNumberOfEmplacedNodes)
{
    DependencyGraph<IdentifierHash> graph(2);
    EXPECT_EQ(graph.size(), 0U);
    graph.emplace("a");
    EXPECT_EQ(graph.size(), 1U);
    graph.emplace("b");
    EXPECT_EQ(graph.size(), 2U);
}

TEST(DependencyGraphTest, TraverseVisitsWholeChainThroughDependsOn)
{
    // root -> mid -> leaf (X -> Y means X depends_on Y)
    DependencyGraph<IdentifierHash> graph(3);
    const auto leaf = graph.emplace("leaf");
    const auto mid = graph.emplace("mid");
    const auto root = graph.emplace("root");
    graph.addDependency(root, mid);
    graph.addDependency(mid, leaf);

    std::vector<IdentifierHash> visited;
    graph.traverse(root, [&](GraphIndex i) -> const std::vector<GraphIndex>& {
        visited.push_back(graph[i]);
        return graph.dependsOn(i);
    });

    EXPECT_THAT(
        visited,
        ::testing::UnorderedElementsAre(IdentifierHash{"root"}, IdentifierHash{"mid"}, IdentifierHash{"leaf"}));
}

TEST(DependencyGraphTest, TraverseVisitsSharedDependencyExactlyOnce)
{
    // Diamond: both a and b depend on shared; root depends on both a and b.
    DependencyGraph<IdentifierHash> graph(4);
    const auto shared = graph.emplace("shared");
    const auto a = graph.emplace("a");
    const auto b = graph.emplace("b");
    const auto root = graph.emplace("root");
    graph.addDependency(a, shared);
    graph.addDependency(b, shared);
    graph.addDependency(root, a);
    graph.addDependency(root, b);

    std::size_t shared_visits = 0;
    graph.traverse(root, [&](GraphIndex i) -> const std::vector<GraphIndex>& {
        if (i == shared)
        {
            ++shared_visits;
        }
        return graph.dependsOn(i);
    });

    EXPECT_EQ(shared_visits, 1U);
}

}  // namespace score::mw::lifecycle
