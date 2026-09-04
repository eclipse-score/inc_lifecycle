/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#include <cstdio>
#include <cstdlib>
#include <thread>

#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"

namespace score::mw::lifecycle::internal
{

SafeProcessMap::SafeProcessMap(uint32_t capacity, IComponentController& termination_handler)
    : items_(std::make_unique<ProcessTreeNode[]>(capacity)), termination_handler_(termination_handler)
{
    if (capacity)
    {
        free_list_head_ = 0U;
        tree_root_.store(NULL_INDEX);
        for (std::size_t i = 0U; i < capacity; ++i)
        {
            items_[i].pid_ = 0;
            items_[i].data_.component_ = nullptr;
            items_[i].pid_left_ = NULL_INDEX;
            items_[i].pid_right_ = static_cast<uint32_t>(i + 1U);
            items_[i].data_.status_ = -1;
        }
        items_[capacity - 1UL].pid_right_ = NULL_INDEX;
    }
}

void SafeProcessMap::findNode(uint32_t& mask, uint32_t& parent, osal::ProcessID key)
{
    while (current_ != NULL_INDEX && key != items_[current_].pid_)
    {
        parent = current_;

        if (static_cast<uint32_t>(key) & mask)
        {
            current_ = items_[parent].pid_left_;
        }
        else
        {
            current_ = items_[parent].pid_right_;
        }

        mask = mask << 1U;

        // Note that by design (key cannot be negative) the mask will never overflow and we don't
        // need the following unreachable code:
        // if( mask == 0U )
        // {
        //    printf("MIN_MASK!\n");
        //    mask = 1U;
        //}
    }
}

// RULECHECKER_comment(1, 1, check_max_parameters, "refactored with WI #9343", true);
int32_t SafeProcessMap::insertNode(uint32_t& mask, uint32_t& parent, osal::ProcessID& key, ProcessInfoData& data)
{
    int32_t ret_value = -1;

    current_ = free_list_head_;

    if (current_ == NULL_INDEX)
    {
        // too bad, we are out of memory
        ret_value = -1;
    }
    else
    {
        mask = mask >> 1U;
        free_list_head_ = items_[current_].pid_right_;
        items_[current_].pid_ = key;
        items_[current_].data_ = data;
        items_[current_].pid_left_ = NULL_INDEX;
        items_[current_].pid_right_ = NULL_INDEX;

        if (static_cast<uint32_t>(key) & mask)
        {
            items_[parent].pid_left_ = current_;
        }
        else
        {
            items_[parent].pid_right_ = current_;
        }

        if (data.component_ == nullptr)
        {
            ret_value = 1;
        }
        else
        {
            ret_value = 0;
        }
    }

    return ret_value;
}

// RULECHECKER_comment(1, 1, check_max_parameters, "refactored with WI #9343", true);
int32_t SafeProcessMap::removeNode(ProcessInfoData& target, ProcessInfoData& data, uint32_t& parent, uint32_t& root)
{
    // found key. There are 4 situations:
    // data.component_ == nullptr, stored component_ != nullptr: normal findTerminated
    // data.component_ != nullptr, stored component_ == nullptr: normal insertIfNotTerminated
    // both data.component_ and stored component_ point to an ITerminationCallback: anomalous
    // both data.component_ and stored component_ are null: anomalous
    // In other words, exactly one of data.component_ and stored component_ must be nullptr
    // or there is an anomaly and we return -2
    int32_t ret_value = -2;
    if ((nullptr == data.component_) ^ (nullptr == items_[current_].data_.component_))
    {
        // found key, we will remove it!
        target = items_[current_].data_;
        if (target.component_)
        {
            target.status_ = data.status_;
        }
        else
        {
            target.component_ = data.component_;
        }
        if (data.component_)
        {
            ret_value = 1;
        }
        else
        {
            ret_value = 0;
        }
        // Need to find a suitable leaf to use as the replacement
        uint32_t leaf = current_;
        uint32_t leaf_parent = current_;
        findLeaf(leaf, leaf_parent);
        deleteNode(parent, leaf, root, leaf_parent);
    }
    return ret_value;
}

void SafeProcessMap::findLeaf(uint32_t& leaf, uint32_t& leaf_parent)
{
    while (true)
    {
        if (items_[leaf].pid_left_ != NULL_INDEX)
        {
            leaf_parent = leaf;
            leaf = items_[leaf].pid_left_;
        }
        else if (items_[leaf].pid_right_ != NULL_INDEX)
        {
            leaf_parent = leaf;
            leaf = items_[leaf].pid_right_;
        }
        else
        {
            break;
        }
    }
}

// RULECHECKER_comment(1, 1, check_max_parameters, "refactored with WI #9343", true);
void SafeProcessMap::deleteNode(uint32_t& parent, uint32_t& leaf, uint32_t& root, uint32_t& leaf_parent)
{
    if (leaf == root)
    {
        // tree is now empty!
        root = NULL_INDEX;
    }
    else
    {
        if (leaf == current_)
        {
            // simply remove the link to current_
            if (items_[parent].pid_left_ == current_)
            {
                items_[parent].pid_left_ = NULL_INDEX;
            }
            else
            {
                items_[parent].pid_right_ = NULL_INDEX;
            }
        }
        else
        {
            // Put the leaf in place of the item we are replacing
            items_[current_].pid_ = items_[leaf].pid_;
            items_[current_].data_ = items_[leaf].data_;

            // Remove the links on the item that previously pointed to the leaf
            if (items_[leaf_parent].pid_left_ == leaf)
            {
                items_[leaf_parent].pid_left_ = NULL_INDEX;
            }
            else
            {
                items_[leaf_parent].pid_right_ = NULL_INDEX;
            }
        }
    }
    // now return the leaf we found to the free list
    items_[leaf].pid_ = 0;
    items_[leaf].data_ = {-1, nullptr};
    items_[leaf].pid_left_ = NULL_INDEX;
    items_[leaf].pid_right_ = free_list_head_;
    free_list_head_ = leaf;
}

int32_t SafeProcessMap::search(osal::ProcessID key, ProcessInfoData data)
{
    int32_t ret_value = -2;

    if (key >= 0)
    {
        while (-2 == ret_value)
        {
            ProcessInfoData target;
            target = {data.status_, nullptr};
            // Gain a lock on the root of the tree
            uint32_t root = tree_root_.exchange(LOCKED_INDEX);

            while (root == LOCKED_INDEX)
            {
                std::this_thread::yield();
                root = tree_root_.exchange(LOCKED_INDEX);
            }
            current_ = root;

            if (root == NULL_INDEX)
            {
                // no tree, special case.
                current_ = free_list_head_;
                free_list_head_ = items_[current_].pid_right_;
                items_[current_].pid_ = key;
                items_[current_].data_ = data;
                items_[current_].pid_left_ = NULL_INDEX;
                items_[current_].pid_right_ = NULL_INDEX;
                root = current_;

                if (data.component_ == nullptr)
                {
                    ret_value = 1;
                }
                else
                {
                    ret_value = 0;
                }
            }
            else
            {
                // Look for the key
                uint32_t parent = NULL_INDEX;
                uint32_t mask = 1U;

                findNode(mask, parent, key);

                if (current_ == NULL_INDEX)
                {
                    // key not found, we will add it
                    ret_value = insertNode(mask, parent, key, data);
                }
                else
                {
                    // found key, we will remove it!
                    ret_value = removeNode(target, data, parent, root);
                }
            }
            // release the lock on the tree
            tree_root_.store(root);

            if (-2 == ret_value)
            {
                // allow another thread to run to resolve the anomaly
                std::this_thread::yield();
            }
            else if (target.component_)
            {
                termination_handler_.terminated(*target.component_, target.status_);
            }
        }
    }

    return ret_value;
}

SafeProcessMapReturnType SafeProcessMap::findTerminated(osal::ProcessID key, int32_t status)
{
    return static_cast<SafeProcessMapReturnType>(search(key, {status, nullptr}));
}

SafeProcessMapReturnType SafeProcessMap::insertIfNotTerminated(osal::ProcessID key, IComponent* object)
{
    return static_cast<SafeProcessMapReturnType>(search(key, {0, object}));
}

}  // namespace score::mw::lifecycle::internal
