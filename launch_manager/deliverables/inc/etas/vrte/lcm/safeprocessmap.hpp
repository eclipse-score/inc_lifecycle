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

#ifndef SAFE_PROCESS_MAP_HPP_INCLUDED
#define SAFE_PROCESS_MAP_HPP_INCLUDED

#include <atomic>
#include <etas/vrte/lcm/osal/iprocess.hpp>
#include <etas/vrte/lcm/processinfonode.hpp>
#include <cstdint>

namespace etas {

namespace vrte {

namespace lcm {

/// @brief Struct representing data in a map item
struct ProcessInfoData {
    int32_t status_ = -1;             ///< Exit status for process
    ProcessInfoNode* pin_ = nullptr;  ///< Pointer to the ProcessInfoNode associated with this item.
};
/// @brief Struct representing an item in the map.
struct ProcessTreeNode {
    uint32_t pid_left_ = 0xFFFFFFFF;   ///< Odd branch for binary tree of process IDs (left child).
    uint32_t pid_right_ = 0xFFFFFFFF;  ///< Even branch for binary tree of process IDs (right child).
    osal::ProcessID pid_ = -1;         ///< Process ID associated with this item.
    ProcessInfoData data_;
};

/// @brief The SafeProcessMap class provides a thread-safe mapping of unique process IDs (ProcessID) to pointers of ProcessInfoNode objects.
/// It ensures safe concurrent access and modification of the mapping, using atomic operations.
class SafeProcessMap final {
   public:
    /// @brief Constructs a SafeProcessMap with a specified capacity.
    /// This constructor initializes the SafeProcessMap with the given capacity.
    /// @param capacity The maximum number of entries that the SafeProcessMap can hold.
    explicit SafeProcessMap(uint32_t capacity);

    /// @brief Destructor to clean up resources used by the SafeProcessMap object.
    ~SafeProcessMap() = default;

    /// @brief Finds a terminated process in the map.
    /// This method is called from OsHandler when a process terminates. It looks up the given process ID (key)
    /// in the map and returns the associated ProcessInfoNode pointer if found. If the key is not found,
    /// it is inserted in the map with the value "already_terminated" and that value is returned.
    /// In the case of a clash due to PID re-use, this method yields until the situation is resolved.
    /// @param key The process ID to look for in the map.
    /// @return 0 if the process was found and updated with the provided `pin_`,
    ///         1 if the process was found and updated with the provided `object`,
    ///         -1 if an error occurred during insertion (e.g., out of memory),
    ///         or -2 if the provided process ID (`key`) is not valid (< 0).
    int32_t findTerminated(osal::ProcessID key, int32_t status);

    /// @brief Inserts a process into the map if it has not already terminated.
    /// This method is called by a worker thread after starting a process. It attempts to insert the given process ID (key)
    /// and its associated ProcessInfoNode pointer into the map, ensuring that the process is not already marked as terminated.
    /// In the case of a clash due to PID re-use, this method yields until the situation is resolved.
    /// @param key The process ID to insert into the map.
    /// @param object A pointer to the ProcessInfoNode associated with the process.
    /// @return 0 if the key (Process ID) was not found and a new entry was made,
    ///         1 if the key was found (indicating the process has terminated), and updated with the provided object,
    ///         -1 if an error occurred during insertion (e.g., out of memory),
    ///         or -2 if the provided process ID (`key`) is not valid ( < 0).
    int32_t insertIfNotTerminated(osal::ProcessID key, ProcessInfoNode* object);

   private:
    /// @brief Searches for a process with the given process ID (key) in the map.
    /// If found, updates or removes the entry based on provided conditions.
    /// If the provided process ID (key) is valid (> 0):
    ///          - If the process ID (key) is found in the map:
    ///              - If `pin_` (ProcessInfoNode pointer) is not nullptr, uses it to set the return status in ProcessInfoNode, removes the key, and returns 0.
    ///              - If `pin_` is nullptr, uses the provided ProcessInfoNode pointer to set the stored status, removes the key, and returns 1.
    ///              - Behaviour under anamolous conditions (PID re-use where either both data.pin_ and stored pin_ are nullptr or both are not nullptr):
    ///                 yield() and then repeat the operation.
    ///          - If the process ID (key) is not found in the map:
    ///              - Adds the key (`key`), `pin_`, and `status` to the map.
    ///              - Returns -1 on failure to add (e.g., out of memory).
    /// @param key The process ID to search for or insert into the map.
    /// @param data The data to associate with the key
    ///        data.object A pointer to the ProcessInfoNode associated with the process.
    ///        data.status The status to set for the process if inserted.
    /// @return 0 if the process was found and updated with the provided `pin_`,
    ///         1 if the process was found and updated with the provided `object`,
    ///         -1 if an error occurred during insertion (e.g., out of memory),
    ///         or -2 if the provided process ID (`key`) is not valid ( < 0).
    int32_t search(osal::ProcessID key, ProcessInfoData data);

    /// @brief Finds the node in the process map tree for the given process ID.
    /// This function searches for a node in the SafeProcessMap whose process ID matches the
    /// provided key. It uses a rover mechanism to traverse the map in a safe manner.
    /// @param mask Reference to the bitmask used for traversal.
    /// @param last Reference to an integer where the index of the last visited node will be stored.
    /// This parameter is updated during the traversal to keep track of the last node visited.
    /// @param key The process ID to find in the tree.
    void findNode(uint32_t& mask, uint32_t& last, osal::ProcessID key);

    /// @brief Inserts a node into the SafeProcessMap with the given process ID and associated information.
    /// This function inserts a node into the SafeProcessMap using a rover mechanism for safe traversal and insertion.
    /// It updates the map's internal structure based on the process ID's bit pattern and manages memory allocation.
    /// @param mask Reference to the bitmask used for traversal.
    /// @param last Reference to an integer storing the index of the last visited node during traversal.
    /// This parameter is updated with the index where the new node is inserted.
    /// @param key Reference to the process ID of the node to insert. After insertion, this parameter may be updated
    /// to reflect any changes necessary in the node's position within the map.
    /// @param data Reference to the data pair for this key
    /// After insertion, this parameter may be updated with the status of the newly inserted node.
    /// @return int32_t Returns an integer indicating the success of the insertion operation:
    /// - 0 if the node was successfully inserted.
    /// - 1 if the insertion was successful but the object pointer was null.
    /// - -1 if the insertion failed due to memory constraints (out of memory).
    int32_t insertNode(uint32_t& mask, uint32_t& last, osal::ProcessID& key, ProcessInfoData& data);

    /// @brief Removes a node from the process map tree.
    /// This function removes the node currently pointed to by the rover in the SafeProcessMap.
    /// It updates the target status and target pointer based on the removal operation and sets
    /// the status according to the success or failure of the removal.
    /// @param target Reference to the data that will be updated for the removed node.
    /// It will be updated based on the success or failure of the removal.
    /// @param data Reference to the data to use.
    /// @param last Index of the last object
    /// @param local_root Index of the first object
    /// @return int32_t Returns 0 if the node was successfully removed and `object` was nullptr,
    /// 1 if `object` was not nullptr, and -2 if the removal failed due to PID re-use
    int32_t removeNode(ProcessInfoData& target, ProcessInfoData& data, uint32_t& last, uint32_t& local_root);

    /// @brief Finds the leaf node in the SafeProcessMap starting from the given node.
    /// This function traverses the SafeProcessMap starting from the specified node `leaf`
    /// to find the first leaf node (a node without left and right children).
    /// @param leaf Current leaf.Reference to an integer representing the starting node from which to find the leaf node.
    /// Upon successful execution, this parameter will store the index of the found leaf node.
    /// @param previous Reference to an integer that will store the index of the parent node of the found leaf node.
    /// If the `leaf` node itself is the root or a leaf, `previous` will be set to the same as `leaf`.
    void findLeaf(uint32_t& leaf, uint32_t& previous);

    /// @brief Deletes a node from the SafeProcessMap, handling reorganization and freeing of resources.
    /// This function deletes a node from the SafeProcessMap structure based on the given parameters,
    /// reorganizing the map if necessary and returning the deleted node to the free list for reuse.
    /// @param last Reference to an integer storing the index of the last visited node during traversal.
    /// This parameter is used to update the link of the parent node after deletion.
    /// @param leaf Reference to an integer representing the index of the node to be deleted from the map.
    /// Upon successful deletion, this parameter will be returned to the free list for reuse.
    /// @param local_root Reference to an integer storing the index of the root node of the current tree structure.
    /// If the deleted node is the root, this parameter is updated to LINK_NO_VALUE, indicating an empty tree.
    /// @param previous Reference to an integer storing the index of the parent node of the deleted node.
    /// This parameter is used to update the link of the parent node after deletion.
    void deleteNode(uint32_t& last, uint32_t& leaf, uint32_t& local_root, uint32_t& previous);

    ///@brief Unique pointer managing an array of ProcessTreeNode objects.
    std::unique_ptr<ProcessTreeNode[]> items_;

    /// @brief Value indicating that no node is assigned.
    static constexpr uint32_t LINK_NO_VALUE = 0xFFFFFFFF;

    /// @brief Value indicating that a node is locked.
    static constexpr uint32_t LINK_LOCKED = 0xFFFFFFFE;

    /// @brief Root of the binary tree used to find an entry by process ID (pid).
    std::atomic_uint32_t pid_root_{LINK_NO_VALUE};

    /// @brief Root of the list of free entries.
    uint32_t free_root_{LINK_NO_VALUE};

    /// @brief Current rover index in the SafeProcessMap.
    /// This variable represents the current index used for traversal within the SafeProcessMap.
    /// It initially starts with LINK_NO_VALUE, indicating no valid position.
    uint32_t rover_{LINK_NO_VALUE};
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  /// SAFE_PROCESS_MAP_HPP_INCLUDED
