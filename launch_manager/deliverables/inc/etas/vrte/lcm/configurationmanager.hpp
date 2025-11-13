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

#ifndef CONFIGURATIONMANAGER_HPP_INCLUDED
#define CONFIGURATIONMANAGER_HPP_INCLUDED

#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <flatbuffers/flatbuffers.h>

#include <score/lcm/identifier_hash.hpp>
#include <etas/vrte/lcm/config.hpp>
#include <etas/vrte/lcm/process_group_state_id.hpp>
#include <etas/vrte/lcm/osal/iprocess.hpp>
#include <etas/vrte/lcm/process_state_client/posixprocess.hpp>

#include "lm_flatcfg_generated.h"

namespace etas {

namespace vrte {

namespace lcm {

using IdentifierHash = score::lcm::
    IdentifierHash;  ///< Defines a type alias 'IdentifierHash' for the type 'score::lcm::IdentifierHash'. Type that represents an identity or identifier. Usually this is a path to a short name.

/// @brief Represents the configuration settings for a process group manager.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct PgManagerConfig final {
    bool is_self_terminating_;  ///< true if the adaptive application may terminate without being first asked
    std::chrono::milliseconds
        startup_timeout_ms_;  ///< Number of milliseconds to wait for kRunning before flagging an error
    std::chrono::milliseconds
        termination_timeout_ms_;  ///< Number of milliseconds to wait for process to terminate after requesting termination
    uint32_t number_of_restart_attempts;  ///< Number of times to attempt restart if the initial attempt fails
    uint32_t execution_error_code_;       ///< Code to report if this process fails
};

/// @brief Represents process dependency in a particular process group associated process.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct Dependency final {
    ProcessState process_state_;  ///< The state of the other process upon which starting of this process depends.
    IdentifierHash target_process_id_;  ///< The ID of the target process this dependency is associated with.
    uint32_t os_process_index_;           ///< The index of the OS process in the target process list.
};

using DependencyList = std::vector<Dependency>;

/// @brief Represent configuration needed to start operating system process, plus identifier of a process for which this startup configuration was defined / configured.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct OsProcess final {
    IdentifierHash process_id_;        ///< id of a Process.
    uint32_t process_number_;            ///< unique number for this process & startup configuration combination
    osal::OsalConfig startup_config_{};  ///< Startup configuration.
    PgManagerConfig pgm_config_;         ///< Process group manager operations configuration.
    DependencyList dependencies_;        ///< List of dependencies for each OS process in a specific process group.
};

/// @brief Represents configuration of a process group state.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct ProcessGroupState final {
    IdentifierHash name_;  ///< Name of a process group state.
    std::vector<uint32_t>
        process_indexes_;  ///< Processes that should be started / run in this process group state. It is an array of indexes (aka pointers) to the processes managed by a process group.
};

/// @brief Represents a process group configuration.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct ProcessGroup final {
    IdentifierHash name_;                                 ///< Name of a process group.
    IdentifierHash sw_cluster_;                           ///< Software cluster to which this process group belongs
    IdentifierHash off_state_;                            ///< ID of the "Off" state for this process group
    IdentifierHash recovery_state_;                       ///< ID of the recovery state for this process group
    std::vector<ProcessGroupState> states_;  ///< States configured for this process group.
    std::vector<OsProcess>
        processes_;  ///< Processes that are managed (started / stopped) by this process group.
};

///
/// @brief Manages the configuration of the machine.
///
/// ConfigurationManager is responsible for the entire lifecycle of configuration.
/// It loads configuration from persistent storage, verify that configuration was not tampered with and then makes that data available (read only) to the rest of Launch Manager.
/// It is also responsible for rereading (updating) configuration during software update.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
class ConfigurationManager final {
    //    using namespace ::etas::vrte::ucm::ipc;
   public:
    /// @brief Initializes the configuration manager.
    ///
    /// This function is responsible for loading the configurations.
    /// It performs the following main tasks:
    /// - Checks or sets the FlatBuffer configuration environment variable.
    /// - Obtain the list of software clusters. If this is successful, load the first software cluster from the list.
    /// - Gets every element in the configuration. keeps a local reference to every configuration element for easy access.
    /// TODO: Add more details about the memory allocation.
    ///
    /// @return Returns true if the configurations were loaded successfully, false otherwise.
    bool initialize();

    /// @brief Deinitializes the configuration manager.
    ///
    /// This function is responsible for performing any necessary cleanup or deinitialization tasks
    /// for the configuration manager. It should be called when the configuration manager is no longer
    /// needed or when the application is shutting down.
    void deinitialize();

    /// @brief Get a list of process groups configured for this Machine.
    /// @return Returns a pointer to a vector of process groups.
    std::optional<const std::vector<IdentifierHash>*> getListOfProcessGroups() const;

    /// @brief Get the software cluster id for a given process group id.
    /// @param process_group_id the IdentifierHash of the process group
    /// @return Returns the IdentifierHash of the software cluster to which the process group belongs.
    std::optional<IdentifierHash> getSoftwareCluster(const IdentifierHash& process_group_id) const;

    /// @brief Reload the configuration for a particular software cluster
    /// @param action Action to take (install, remove, or update)
    /// @param sw_cluster_id The sofware cluster to process
    /// @return Returns true if the configurations were loaded successfully, false otherwise.
    // bool reloadConfiguration(
    // const etas::vrte::ucm::ipc::Message::Action action,
    // const IdentifierHash& cluster_id );

    /// @brief Get the number of OS processes configured for a particular process group.
    /// @param[in] pg_name The name of the process group.
    /// @return Returns the number of OS processes.
    /// If process group does not exist then there will be no return value.
    std::optional<uint32_t> getNumberOfOsProcesses(const IdentifierHash& pg_name) const;

    /// @brief Get the ID of the "Off" state for a particular process group.
    /// @param[in] pg_name The name of the process group.
    /// @return Returns the ID of the "Off" state.
    /// If process group does not exist a default value of IdentifierHash("Off") is returned.
    IdentifierHash getNameOfOffState(const IdentifierHash& pg_name) const;

    /// @brief Get the ID of the recovery state for a particular process group.
    /// @param[in] pg_name The name of the process group.
    /// @return Returns the ID of the recovery state.
    /// If process group does not exist a default value of IdentifierHash("Recovery") is returned.
    IdentifierHash getNameOfRecoveryState(const IdentifierHash& pg_name) const;

    /// @brief Get the startup state for the main process group.
    /// @return Return the startup state of the main process group, as specified by AUTOSAR.
    std::optional<const ProcessGroupStateID*> getMainPGStartupState() const;

    /// @brief Get a list of processes configured to run in this process group state.
    /// @param[in] process_group_state_id The ID of the process group state for which to retrieve the list of process IDs.
    /// @return Returns a pointer to a vector of process indexes.
    /// Process group state 'Off' will have vector of size 0.
    /// If Process group does not exist then there will be no return value.
    std::optional<const std::vector<uint32_t>*> getProcessIndexesList(
        const ProcessGroupStateID& process_group_state_id) const;

    /// @brief Get startup configuration for a given process.
    /// @param[in] pg_name_ The name of the process group for which to retrieve the OS Configurations. .
    /// @param[in] index The index of the OS process.
    /// @return Returns a pointer to an OSProcess.
    /// If index/process group does not exist there will be no return value.
    std::optional<const OsProcess*> getOsProcessConfiguration(const IdentifierHash& pg_name_,
                                                                    const uint32_t index) const;

    /// @brief Get dependencies of an OS process within a specific process group.
    ///
    /// Retrieves the dependencies associated with an OS process identified by its index
    /// within a specified process group. Dependencies include the current state of the process
    /// and any related process indices that need to be managed together.
    ///
    /// @param[in] process_group_name The name of the process group containing the OS process.
    /// @param[in] index The index of the OS process within the process group.
    /// @return An optional vector of Dependency objects representing process dependencies,
    ///         or an empty optional if the process group or process index does not exist.
    std::optional<const DependencyList*> getOsProcessDependencies(const IdentifierHash& process_group_name,
                                                                        const uint32_t index) const;

    /// @brief default value for the process execution error, in case it is not defined in the configuration
    static const uint32_t kDefaultProcessExecutionError;

    /// @brief default value for processor affinity mask in case it is not defined in the configuration
    static uint32_t kDefaultProcessorAffinityMask();

    /// @brief default value for scheduling policy in case it is not defined in the configuration
    static const int32_t kDefaultSchedulingPolicy;

    /// @brief default value for scheduling priority in case if is not defined in the configuration and
    ///        if the process is running in real-time scheduling mode.
    static const int32_t kDefaultRealtimeSchedulingPriority;

    /// @brief default value for scheduling priority in case if is not defined in the configuration and
    ///        if the process is running in normal scheduling mode.
    static const int32_t kDefaultNormalSchedulingPriority;

   private:
    /// @brief Initializes the LCM configurations for the software clusters.
    /// This function loads the list of software clusters and then loads the LCM configurations
    /// for the specified software cluster index. It sets the success flag to true if all steps
    ///  are completed successfully.
    ///  @return true if the initialization is successful, false otherwise.
    bool initializeSoftwareClusterConfigurations();

    /// @brief Load the LCM configurations for a specific software cluster.
    /// This function loads the LCM configurations for the software cluster identified by the given index.
    /// @param[in] index The index of the software cluster for which to load the LCM configurations.
    /// @return `true` if the LCM configurations were successfully loaded, `false` otherwise.
    bool loadSWClusterConfiguration(uint8_t index);

    /// @brief Load the software clusters from the flat configuration.
    /// This function loads the list of software clusters from the flat configuration.
    /// It parses the root node and extracts machine-specific configurations for further processing.
    /// @param[in] index The index of the software cluster for which to load the LCM configurations.
    /// @return `true` if the software clusters were successfully loaded and stored, `false` otherwise.
    bool loadListOfSWClusters();

    /// @brief Load machine configurations from the provided root node.
    /// This function loads machine configurations from the specified root node of the LCM EcuCfg.
    /// It retrieves and stores the software clusters internally for further processing.
    /// @param[in] root_node Pointer to the root node of the LCM EcuCfg containing machine configurations.
    /// @param[in] cluster ID if the software cluster to store in each process group structure
    /// @return `true` if machine configurations were successfully loaded and processed, `false` otherwise.
    bool loadMachineConfigs(const LMFlatBuffer::LMEcuCfg* root_node, const IdentifierHash& cluster);

    /// @brief Load process configurations from the provided root node.
    /// This function loads process configurations from the specified root node of the LCM EcuCfg.
    /// It parses the root node and extracts process-specific configurations for further processing.
    /// @param[in] root_node Pointer to the root node of the LCM EcuCfg containing process configurations.
    /// @return `true` if process configurations were successfully loaded and processed, `false` otherwise.
    bool loadProcessConfigs(const LMFlatBuffer::LMEcuCfg* root_node);

    /// @brief Parses mode groups from the provided ModeGroup node and updates the ProcessGroup.
    /// This function iterates through the mode declarations in the ModeGroup node and creates ProcessGroupState objects
    /// for each mode declaration. It then adds these states to the provided ProcessGroups. If the mode declaration
    /// list is empty or null, a warning is logged.
    /// @param node The ModeGroup node containing mode group configurations.
    /// @param process_group_data The ProcessGroup data to update with parsed configurations.
    /// @return true if the mode groups are successfully parsed and added to the ProcessGroup data, false otherwise.
    bool parseModeGroups(const LMFlatBuffer::ModeGroup* node, ProcessGroup& process_group_data);

    /// @brief Parse machine configurations from the provided ModeGroup node.
    /// This function parses machine configurations from the specified ModeGroup node in the LCM FlatBuffer data.
    /// @param[in] node Pointer to the ModeGroup node containing machine configurations.
    /// @param[in] cluster ID if the software cluster to store in each process group structure
    /// @return `true` if machine configurations were successfully parsed, `false` otherwise.
    bool parseMachineConfigurations(const LMFlatBuffer::ModeGroup* node, const IdentifierHash& cluster);

    /// @brief Parse process configurations from the provided Process node.
    /// This function parses process configurations from the specified Process node in the LCM FlatBuffer data.
    /// @param[in] node Pointer to the Process node containing machine configurations.
    /// @return `true` if process configurations were successfully parsed, `false` otherwise.
    bool parseProcessConfigurations(const LMFlatBuffer::Process* node);

    /// @brief Parse process arguments from a list of FlatBuffer process argument nodes.
    /// This function parses process arguments from the provided list of FlatBuffer process argument nodes
    /// and populates the given OsProcess instance with the parsed arguments.
    /// @param[in] process_arg_list Pointer to a vector of FlatBuffer process argument nodes.
    /// @param[out] process_instance Reference to the OsProcess instance where parsed arguments will be stored.
    void parseProcessArguments(
        const flatbuffers::Vector<flatbuffers::Offset<LMFlatBuffer::ProcessArgument>>* process_arg_list,
        OsProcess& process_instance);

    /// @brief Parse environment variables from a list of FlatBuffer environment variable nodes.
    /// This function parses environment variables from the provided list of FlatBuffer environment variable nodes
    /// and populates the given OsProcess instance with the parsed variables.
    /// @param[in] env_var_list Pointer to a vector of FlatBuffer environment variable nodes.
    /// @param[out] process_instance Reference to the OsProcess instance where parsed environment will be stored.
    void parseProcessEnvironmentVars(
        const flatbuffers::Vector<flatbuffers::Offset<LMFlatBuffer::EnvironmentVariable>>* env_var_list,
        OsProcess& process_instance);

    /// @brief Parse process group dependencies of processes from a list of FlatBuffer process process group state dependency nodes.
    /// This function parses process group dependencies from the provided list of FlatBuffer process process group state dependency nodes
    /// and associates the specified OsProcess instance with the relevant process groups.
    /// @param[in] process_pg_list Pointer to a vector of FlatBuffer process process group state dependency nodes.
    /// @param[out] process_instance Reference to the OsProcess instance where parsed environment will be stored.
    /// @return `true` if process group dependencies were successfully parsed and associated, `false` otherwise.
    void parseProcessGroup(
        const flatbuffers::Vector<flatbuffers::Offset<LMFlatBuffer::ProcessGroupStateDependency>>*
            process_pg_list,
        const OsProcess& process_instance);

    /// @brief Parses execution dependencies from a FlatBuffer list and updates the provided OsProcess instance.
    /// This function processes a list of `ProcessExecutionDependency` objects from the FlatBuffer and updates
    /// the specified `OsProcess` instance to reflect these dependencies. Dependencies are used to manage
    /// relationships between different processes.
    /// @param[in] process_dependency_list Pointer to a vector of `ProcessExecutionDependency` objects from the FlatBuffer.
    /// @param[out] process_instance Reference to the `OsProcess` instance that will be updated with the parsed dependencies.
    void parseExecutionDependency(
        const flatbuffers::Vector<flatbuffers::Offset<LMFlatBuffer::ProcessExecutionDependency>>*
            process_dependency_list,
        OsProcess& process_instance);

    /// @brief Retrieves the process state enumeration based on the given state name.
    /// This function looks up the `ProcessState` enumeration value corresponding to the provided state name.
    /// The state name is expected to match one of the defined `ProcessState` values.
    /// @param[in] state_name The name of the process state as an `IdentifierHash`.
    /// @return The `ProcessState` enumeration value associated with the state name. If the state name does not match
    ///         any defined states, the function returns an undefined or default state.
    ProcessState getProcessState(const IdentifierHash& state_name);

    /// @brief Get a pointer to a ProcessGroup by its ID.
    /// This function retrieves a pointer to a ProcessGroup identified by the process group name ID.
    /// @param[in] pg_name The ID of the ProcessGroup to retrieve.
    /// @return A pointer to the ProcessGroup if found, or nullptr if not found.
    ProcessGroup* getProcessGroupByID(const IdentifierHash& pg_name) const;

    /// @brief Get a pointer to a ProcessGroupState within a specified ProcessGroup by its ID.
    /// This function retrieves a pointer to a ProcessGroupState identified by the state name ID within the given ProcessGroup pointer.
    /// @param[in] pg The ProcessGroup in which to search for the ProcessGroupState.
    /// @param[in] state_name The ID of the ProcessGroupState to retrieve.
    /// @return A pointer to the ProcessGroupState if found within the ProcessGroup, or nullptr if not found.
    ProcessGroupState* getProcessGroupStateByID(ProcessGroup& pg, const IdentifierHash& state_name) const;

    /// @brief Get a pointer to a ProcessGroupState by its ID.
    /// This function retrieves a pointer to a ProcessGroupState identified by the PG state ID.
    /// @param[in] pg_id The ID object containing information to identify the ProcessGroupState..
    /// @return A pointer to the ProcessGroupState if found, or nullptr if not found.
    ProcessGroupState* getProcessGroupStateByID(const ProcessGroupStateID& pg_id) const;

    /// @brief Retrieves a pointer to a ProcessGroup based on its name and index.
    /// This function searches for a `ProcessGroup` within the list of process groups based on the provided
    /// process group name and index. It returns a pointer to the `ProcessGroup` if found; otherwise, it returns
    /// `nullptr`. The index is used to specify a particular `ProcessGroup` if there are multiple process groups
    /// with the same name.
    /// @param[in] pg_name The name of the process group as an `IdentifierHash`.
    /// @param[in] index The index of the process group within the list of process groups.
    /// @return A pointer to the `ProcessGroup` if it exists, or `nullptr` if no matching process group is found
    ///         with the given name and index.
    std::optional<const ProcessGroup*> getProcessGroupByNameAndIndex(const IdentifierHash& pg_name,
                                                                             const uint32_t index) const;

    /// @brief Assign an OsProcess instance to a ProcessGroupState identified by ID.
    /// This function assigns the OsProcess instance to a ProcessGroup and process index to ProcessGroupState.
    /// @param[in] pg_id The ID object containing information to identify the ProcessGroup and it's State.
    /// @param[in]  process_instance The OsProcess instance to assign to the ProcessGroup
    void AssignOsProcessInstanceToProcessGroup(const ProcessGroupStateID& pg_id, const OsProcess& process_instance);

    /// @brief Check or set a flat configuration environment variable.
    /// This function checks the existence of a flat configuration environment variable with the specified name.
    /// If the variable exists and its value is different from the provided value, it updates the variable's value.
    /// If the variable does not exist, it sets the variable with the provided name and value
    /// @param[in] name The name of the environment variable to check or set
    /// @param[in] value The value to set for the environment variable.
    /// @return `true` if the environment variable was successfully checked or set, `false` otherwise.
    bool checkOrSetFlatConfigEnvVar(const std::string& name, const std::string& value);

    /// @brief Get the value of an environment variable by name.
    /// This function is wrapper for getenv() system call and it retrieves the value of the environment variable identified by the specified name.
    /// @param[in] name The name of the environment variable to check or set
    /// @return The value of the environment variable as a String, or an empty string if not found.
    std::string getEnvVar(const std::string& name) const;

    /// @brief Set or update an environment variable with the specified name and value.
    /// This function is wrapper for setenv() system call and it sets or updates an environment variable with the provided name and value.
    /// @param[in] name The name of the environment variable to set.
    /// @param[in] value The value to assign to the environment variable.
    /// @param[in] overwrite Flag indicating whether to overwrite an existing variable.
    /// @return Zero if the operation succeeds, or -1 if an error occurs (e.g., permission denied).
    int setEnvVar(const std::string& name, const std::string& value, int overwrite) const;

    /// @brief Get an IdentifierHash from a FlatBuffer string.
    /// This function extracts an `IdentifierHash` from the provided FlatBuffer string.
    /// @param[in] flat_string Pointer to the FlatBuffer string from which to extract the `IdentifierHash`.
    /// @return The extracted `IdentifierHash` or an empty string if `flat_string` is nullptr.
    IdentifierHash getStringViewFromFlatBuffer(const flatbuffers::String* flat_string);

    /// @brief Extract a C-style string from a FlatBuffer string.
    /// This function retrieves a C-style (null-terminated) string from the provided FlatBuffer string.
    /// If the `flat_string` is nullptr, it returns a pointer to an empty string.
    /// @param[in] flat_string Pointer to the FlatBuffer string from which to extract the C-style string.
    /// @return The extracted C-style string or an empty string if `flat_string` is nullptr.
    const char* getStringFromFlatBuffer(const flatbuffers::String* flat_string);

    /// @brief Determines if the process reports its execution state.
    /// This function checks the reporting behavior of a process and logs an appropriate message.
    /// It returns `true` if the process reports its execution state and `false` otherwise.
    /// @param[in] reporting_behaviour The reporting behavior of the process, represented as an enumerator of type LMFlatBuffer::ExecutionStateReportingBehaviorEnum.
    /// @param[in] process_name The name of the process as a FlatBuffer string.
    /// @return `kReporting` if the process reports its execution state, `kNoComms` otherwise.
    osal::CommsType isReportingProcess(const LMFlatBuffer::ExecutionStateReportingBehaviorEnum reporting_behaviour,
                                       const std::string_view process_name);

    /// @brief Determines the communication type for the process.
    /// This function evaluates the process configuration to determine the appropriate communication type.
    /// It checks the reporting behavior of the process and the function cluster affiliation to establish whether
    /// a communication channel is needed and if it involves Control Client management.
    /// @param[in] node A pointer to the process configuration node from which the communication type is derived.
    /// @param[in] short_name Short name of the process
    /// @return The communication type as an enumerator of type `osal::CommsType`. Possible values are `kNoComms`,
    /// `kReporting`, or `kControlClient`.
    osal::CommsType getCommsType(const LMFlatBuffer::Process* node, const char* short_name);

    /// @brief Determine the function cluster affiliation and update the communication type accordingly.
    /// This function examines the given function cluster attribute and adjusts the current communication type if the process
    /// belongs to a specific function cluster, such as "STATE_MANAGEMENT" or "PLATFORM_HEALTH_MANAGEMENT".
    /// @param[in] current_comms The current communication type, which will be updated based on the function cluster affiliation.
    /// @param[in] attribute A C-style string representing the function cluster affiliation of the process.
    /// @return The updated communication type based on the function cluster affiliation.
    osal::CommsType getfunctionClusterAffiliation(osal::CommsType current_comms, const char* attribute);

    /// @brief Determines if the process is self-terminating.
    /// This function checks the termination behavior of a process and logs an appropriate message.
    /// It returns `true` if the process is self-terminating and `false` otherwise.
    /// @param[in] termination_behavior The termination behavior of the process, represented as an enumerator of type LMFlatBuffer::TerminationBehaviorEnum.
    /// @param[in] process_name The name of the process as a FlatBuffer string.
    /// @return `true` if the process is self-terminating, `false` otherwise.
    bool isSelfTerminatingProcess(const LMFlatBuffer::TerminationBehaviorEnum termination_behavior,
                                  const std::string_view process_name);

    /// @brief Assigns OS process indexes in the dependency list
    /// This function assigns process indexes to each OS process in the dependency list
    /// to ensure proper tracking of process dependencies.
    void AssignOsProcessIndexesInDependencyList();

    /// @brief Updates OS process indexes in the dependency list for a given process group.
    /// This function updates the OS process indexes in the dependency list of the specified process group
    /// to reflect the correct dependencies and relationships between processes.
    /// @param[in] pg Reference to the ProcessGroup object for which to update the dependency list.
    /// @param[in] dep_list Reference to the DependencyList object containing the dependencies to be updated.
    void UpdateOsProcessIndexInDependencyList(ProcessGroup& pg, DependencyList& dep_list);

    /// @brief Root node of the LCM FlatBuffer configuration.
    /// This member variable holds a pointer to the root node of the LCM FlatBuffer configuration,
    /// which represents the top-level node of the parsed LCM configuration data.
    /// It is initialized to nullptr by default.
    const LMFlatBuffer::LMEcuCfg* root_node_{};

    /// @brief List of software cluster identifiers.
    /// This member variable represents a vector of strings that stores the identifiers of software clusters.
    std::vector<std::string> sw_clusters_{};

    /// @brief List of Process groups Of the Machine.
    /// This member variable represents a vector that stores instances of the ProcessGroup structure,
    /// which encapsulate information about process groups.
    std::vector<ProcessGroup> process_groups_{};

    /// @brief Vector of identifiers for process group names.
    /// This member variable represents a vector that stores identifiers corresponding to process group names.
    std::vector<IdentifierHash> process_group_names_{};

    /// @brief Default startup state identifier for a machine process group.
    /// This member variable represents a predefined `ProcessGroupStateID` instance that identifies the startup state of a machine process group.
    /// It is initialized with the names "MainPG" (process group name) and "Startup" (state name).
    ProcessGroupStateID main_pg_startup_state_{static_cast<IdentifierHash>("MainPG"),
                                                   static_cast<IdentifierHash>("MainPG/Startup")};

    /// @brief Unique Process index for OS process instance for specific startup configs
    /// This member variable represents the index of the OS process instance with unique startup config.
    /// It is initialized to 0 and typically incremented to assign different indices to distinct OS process instances.
    uint32_t process_index_ = 0;

    /// @brief Flag indicating whether to overwrite existing environment variables.
    /// This member variable represents a constant integer used as a flag to control the behavior
    /// of setting environment variables. A value of 1 indicates that existing environment variables
    /// should be overwritten when setting a new value.
    const int overwrite_ = 1;

    /// @brief Execution depedency string representing the running dependency state of the process
    /// This static member variable will stores the string "Running" to represent the running state.
    static const char* PROCESS_RUNNING_STATE;

    /// @brief  Execution depedency string representing the terminated dependency state of the process.
    /// This static member variable will stores the string "Terminated" to represent the terminated state.
    static const char* PROCESS_TERMINATED_STATE;
};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  /// CONFIGURATIONMANAGER_HPP_INCLUDED
