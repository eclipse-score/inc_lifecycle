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
// (c) 2025 ETAS GmbH. All rights reserved.

#include <etas/vrte/lcm/processinfonode.hpp>
#include <etas/vrte/lcm/safeprocessmap.hpp>
#include <cstdio>
#include <cstdlib>
#include <thread>

namespace etas {

namespace vrte {

namespace lcm {

SafeProcessMap::SafeProcessMap(uint32_t capacity) : items_(std::make_unique<ProcessTreeNode[]>(capacity)) {
    if (capacity) {
        free_root_ = 0U;
        pid_root_.store(LINK_NO_VALUE);
        for (std::size_t i = 0U; i < capacity; ++i) {
            items_[i].pid_ = 0;
            items_[i].data_.pin_ = nullptr;
            items_[i].pid_left_ = LINK_NO_VALUE;
            items_[i].pid_right_ = static_cast<uint32_t>(i + 1U);
            items_[i].data_.status_ = -1;
        }
        items_[capacity - 1UL].pid_right_ = LINK_NO_VALUE;
    }
}

inline void SafeProcessMap::findNode(uint32_t& mask, uint32_t& last, osal::ProcessID key) {
    while (rover_ != LINK_NO_VALUE && key != items_[rover_].pid_) {
        last = rover_;

        if (static_cast<uint32_t>(key) & mask) {
            rover_ = items_[last].pid_left_;
        } else {
            rover_ = items_[last].pid_right_;
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
inline int32_t SafeProcessMap::insertNode(uint32_t& mask, uint32_t& last, osal::ProcessID& key, ProcessInfoData& data) {
    int32_t ret_value = -1;

    rover_ = free_root_;

    if (rover_ == LINK_NO_VALUE) {
        // too bad, we are out of memory
        ret_value = -1;
    } else {
        mask = mask >> 1U;
        free_root_ = items_[rover_].pid_right_;
        items_[rover_].pid_ = key;
        items_[rover_].data_ = data;
        items_[rover_].pid_left_ = LINK_NO_VALUE;
        items_[rover_].pid_right_ = LINK_NO_VALUE;

        if (static_cast<uint32_t>(key) & mask) {
            items_[last].pid_left_ = rover_;
        } else {
            items_[last].pid_right_ = rover_;
        }

        if (data.pin_ == nullptr) {
            ret_value = 1;
        } else {
            ret_value = 0;
        }
    }

    return ret_value;
}

// RULECHECKER_comment(1, 1, check_max_parameters, "refactored with WI #9343", true);
inline int32_t SafeProcessMap::removeNode(ProcessInfoData& target, ProcessInfoData& data, uint32_t& last,
                                          uint32_t& local_root) {
    // found key. There are 4 situations:
    // data.pin_ == nullptr, stored pin_ != nullptr: normal findTerminated
    // data.pin_ != nullptr, stored pin_ == nullptr: normal insertIfNotTerminated
    // both data.pin_ and stored pin_ point to a ProcessInfoNode: anomalous
    // both data.pin_ and stored pin_ are null: anomalous
    // In other words, exactly one of data.pin_ and stored pin_ must be nullptr
    // or there is an anomaly and we return -2
    int32_t ret_value = -2;
    if ((nullptr == data.pin_) ^ (nullptr == items_[rover_].data_.pin_)) {
        // found key, we will remove it!
        target = items_[rover_].data_;
        if (target.pin_) {
            target.status_ = data.status_;
        } else {
            target.pin_ = data.pin_;
        }
        if (data.pin_) {
            ret_value = 1;
        } else {
            ret_value = 0;
        }
        // Need to find a suitable leaf to use as the replacement
        uint32_t leaf = rover_;
        uint32_t previous = rover_;
        findLeaf(leaf, previous);
        deleteNode(last, leaf, local_root, previous);
    }
    return ret_value;
}

inline void SafeProcessMap::findLeaf(uint32_t& leaf, uint32_t& previous) {
    while (true) {
        if (items_[leaf].pid_left_ != LINK_NO_VALUE) {
            previous = leaf;
            leaf = items_[leaf].pid_left_;
        } else if (items_[leaf].pid_right_ != LINK_NO_VALUE) {
            previous = leaf;
            leaf = items_[leaf].pid_right_;
        } else {
            break;
        }
    }
}

// RULECHECKER_comment(1, 1, check_max_parameters, "refactored with WI #9343", true);
inline void SafeProcessMap::deleteNode(uint32_t& last, uint32_t& leaf, uint32_t& local_root, uint32_t& previous) {
    if (leaf == local_root) {
        // tree is now empty!
        local_root = LINK_NO_VALUE;
    } else {
        if (leaf == rover_) {
            // simply remove the link to rover
            if (items_[last].pid_left_ == rover_) {
                items_[last].pid_left_ = LINK_NO_VALUE;
            } else {
                items_[last].pid_right_ = LINK_NO_VALUE;
            }
        } else {
            // Put the leaf in place of the item we are replacing
            items_[rover_].pid_ = items_[leaf].pid_;
            items_[rover_].data_ = items_[leaf].data_;

            // Remove the links on the item that previously pointed to the leaf
            if (items_[previous].pid_left_ == leaf) {
                items_[previous].pid_left_ = LINK_NO_VALUE;
            } else {
                items_[previous].pid_right_ = LINK_NO_VALUE;
            }
        }
    }
    // now return the leaf we found to the free list
    items_[leaf].pid_ = 0;
    items_[leaf].data_ = {-1, nullptr};
    items_[leaf].pid_left_ = LINK_NO_VALUE;
    items_[leaf].pid_right_ = free_root_;
    free_root_ = leaf;
}

int32_t SafeProcessMap::search(osal::ProcessID key, ProcessInfoData data) {
    int32_t ret_value = -2;

    if (key >= 0) {
        while (-2 == ret_value) {
            ProcessInfoData target;
            target = {data.status_, nullptr};
            // Gain a lock on the root of the tree
            uint32_t local_root = pid_root_.exchange(LINK_LOCKED);

            while (local_root == LINK_LOCKED) {
                std::this_thread::yield();
                local_root = pid_root_.exchange(LINK_LOCKED);
            }
            rover_ = local_root;

            if (local_root == LINK_NO_VALUE) {
                // no tree, special case.
                rover_ = free_root_;
                free_root_ = items_[rover_].pid_right_;
                items_[rover_].pid_ = key;
                items_[rover_].data_ = data;
                items_[rover_].pid_left_ = LINK_NO_VALUE;
                items_[rover_].pid_right_ = LINK_NO_VALUE;
                local_root = rover_;

                if (data.pin_ == nullptr) {
                    ret_value = 1;
                } else {
                    ret_value = 0;
                }
            } else {
                // Look for the key
                uint32_t last = LINK_NO_VALUE;
                uint32_t mask = 1U;

                findNode(mask, last, key);

                if (rover_ == LINK_NO_VALUE) {
                    // key not found, we will add it
                    ret_value = insertNode(mask, last, key, data);
                } else {
                    // found key, we will remove it!
                    ret_value = removeNode(target, data, last, local_root);
                }
            }
            // release the lock on the tree
            pid_root_.store(local_root);

            if (-2 == ret_value) {
                // allow another thread to run to resolve the anomaly
                std::this_thread::yield();
            } else if (target.pin_) {
                target.pin_->terminated(target.status_);
            }
        }
    }

    return ret_value;
}

int32_t SafeProcessMap::findTerminated(osal::ProcessID key, int32_t status) {
    return search(key, {status, nullptr});
}

int32_t SafeProcessMap::insertIfNotTerminated(osal::ProcessID key, ProcessInfoNode* object) {
    return search(key, {0, object});
}

}  // namespace lcm

}  // namespace vrte

}  // namespace etas
