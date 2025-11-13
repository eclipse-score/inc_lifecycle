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

#include <string_view>
#include <score/lcm/exec_error_domain.h>

#include <etas/vrte/lcm/configurationmanager.hpp>
#include <etas/vrte/lcm/process_group_state_id.hpp>
#include <etas/vrte/lcm/log.hpp>
#include <etas/vrte/lcm/osal/osalnumcores.hpp>

#include <fstream>

using namespace std;
using namespace LMFlatBuffer;

namespace {

/// @brief Retrieves the resource limits configuration from the given config
///        node.
bool setResourceLimits(const LMFlatBuffer::ProcessStartupConfig& startup_config_node,
                       etas::vrte::lcm::OsProcess& instance) {
    // not supported currently
    instance.startup_config_.resource_limits_.stack_ = 0U;  // don't set the stack limit
    instance.startup_config_.resource_limits_.cpu_ = 0U;    // no limit for cpu time
    instance.startup_config_.resource_limits_.data_ = 0U;   // not supported in ar-24-11

    // we don't need to check the upper limit as this is done in the tooling

    // 0 means not configured
    if (startup_config_node.memoryUsage() > 0U)
        instance.startup_config_.resource_limits_.as_ = startup_config_node.memoryUsage();

    return true;

}

std::unique_ptr<char[]> read_flatbuffer_file(const std::string& f_filename_r) {
    const std::string configFilePath = std::string("etc/") + f_filename_r.c_str();

    std::ifstream infile;
    infile.open(configFilePath, std::ios::binary | std::ios::in);
    if (!infile.is_open()) {
        return nullptr;
    }
    infile.seekg(0, std::ios::end);
    const auto length = static_cast<size_t>(infile.tellg());
    infile.seekg(0, std::ios::beg);
    auto data = std::make_unique<char[]>(length);
    infile.read(data.get(), length);
    infile.close();
    return data;
}

}  // namespace

namespace etas {

namespace vrte {

namespace lcm {

const char* ConfigurationManager::PROCESS_RUNNING_STATE = "Running";
const char* ConfigurationManager::PROCESS_TERMINATED_STATE = "Terminated";

// coverity[autosar_cpp14_m3_4_1_violation:INTENTIONAL] The value is used in a global context.
const char* kEnvVarName = "ECUCFG_ENV_VAR_ROOTFOLDER";  ///< Environment variable name
// coverity[autosar_cpp14_m3_4_1_violation:INTENTIONAL] The value is used in a global context.
const char* kEnvVarDefaultValue = "/opt/vrte/launch_manager/etc/ecu-cfg";  ///< Environment variable value

const uint32_t ConfigurationManager::kDefaultProcessExecutionError = 1U;
uint32_t ConfigurationManager::kDefaultProcessorAffinityMask() {
    return (1U << osal::getNumCores()) - 1U;
}
const int32_t ConfigurationManager::kDefaultSchedulingPolicy = SCHED_OTHER;
const int32_t ConfigurationManager::kDefaultRealtimeSchedulingPriority = 99;
const int32_t ConfigurationManager::kDefaultNormalSchedulingPriority = 0;

std::optional<uint32_t> ConfigurationManager::getNumberOfOsProcesses(const IdentifierHash& pg_name) const {
    std::optional<uint32_t> numberOfProcesses = std::nullopt;
    auto pg = getProcessGroupByID(pg_name);

    if (pg) {
        numberOfProcesses = std::optional<uint32_t>(pg->processes_.size());
    }

    return numberOfProcesses;
}

IdentifierHash ConfigurationManager::getNameOfOffState(const IdentifierHash& pg_name) const {
    IdentifierHash nameOfOffState{"Off"};
    auto pg = getProcessGroupByID(pg_name);
    if (pg) {
        nameOfOffState = pg->off_state_;
    }
    return nameOfOffState;
}

IdentifierHash ConfigurationManager::getNameOfRecoveryState(const IdentifierHash& pg_name) const {
    IdentifierHash nameOfRecoveryState{"Recovery"};
    auto pg = getProcessGroupByID(pg_name);
    if (pg) {
        nameOfRecoveryState = pg->recovery_state_;
    }
    return nameOfRecoveryState;
}

std::optional<const ProcessGroupStateID*> ConfigurationManager::getMainPGStartupState() const {
    std::optional<const ProcessGroupStateID*> result = std::nullopt;

    auto pg = getProcessGroupByID(main_pg_startup_state_.pg_name_);

    if (pg) {
        auto pg_state = getProcessGroupStateByID(*pg, main_pg_startup_state_.pg_state_name_);

        if (pg_state) {
            result = &main_pg_startup_state_;
        } else {
            LM_LOG_DEBUG() << "Process group state not found:" << main_pg_startup_state_.pg_state_name_.data();
        }
    } else {
        LM_LOG_DEBUG() << "Process group not found:" << main_pg_startup_state_.pg_name_.data();
    }

    return result;
}

std::optional<const std::vector<IdentifierHash>*> ConfigurationManager::getListOfProcessGroups() const {
    std::optional<const std::vector<IdentifierHash>*> result = std::nullopt;

    if (!process_group_names_.empty()) {
        result = &process_group_names_;
    }

    return result;
}

std::optional<const std::vector<uint32_t>*> ConfigurationManager::getProcessIndexesList(
    const ProcessGroupStateID& pg_state_id) const {
    std::optional<const std::vector<uint32_t>*> result = std::nullopt;

    auto state = getProcessGroupStateByID(pg_state_id);

    if (state) {
        result = &state->process_indexes_;
    } else {
        LM_LOG_DEBUG() << "Process group state '" << pg_state_id.pg_state_name_.data() << "' not found in group '"
                       << pg_state_id.pg_name_.data() << "'.";
    }

    return result;
}

std::optional<const OsProcess*> ConfigurationManager::getOsProcessConfiguration(const IdentifierHash& pg_name,
                                                                           const uint32_t index) const {
    std::optional<const OsProcess*> result = std::nullopt;

    if (auto pg = getProcessGroupByNameAndIndex(pg_name, index)) {
        result = &(*pg)->processes_[index];
    } else {
        LM_LOG_DEBUG() << "Unable to retrieve process configuration for process group" << pg_name.data()
                       << "with index" << index;
    }

    return result;
}

std::optional<const DependencyList*> ConfigurationManager::getOsProcessDependencies(const IdentifierHash& pg_name,
                                                                               const uint32_t index) const {
    std::optional<const DependencyList*> result = std::nullopt;

    if (auto pg = getProcessGroupByNameAndIndex(pg_name, index)) {
        result = &(*pg)->processes_[index].dependencies_;
    } else {
        LM_LOG_DEBUG() << "Unable to retrieve process dependencies for process group" << pg_name.data() << "with index"
                       << index;
    }

    return result;
}

bool ConfigurationManager::initialize() {
    bool result = false;  // Assume failure by default
    process_index_ = 0U;
    LM_LOG_DEBUG() << "Loading LCM Configurations...";

    // Check or set the environment variable
    if (checkOrSetFlatConfigEnvVar(etas::vrte::lcm::kEnvVarName, etas::vrte::lcm::kEnvVarDefaultValue)) {
        LM_LOG_DEBUG() << "ECUCFG_ENV_VAR_ROOTFOLDER set successfully";
        result = initializeSoftwareClusterConfigurations();

        // Assign execution dependencies process based on index
        AssignOsProcessIndexesInDependencyList();
    } else {
        LM_LOG_DEBUG() << "Failed to set ECUCFG_ENV_VAR_ROOTFOLDER";
    }

    return result;
}

void ConfigurationManager::deinitialize() {
    for (auto& process_group : process_groups_) {
        for (auto& process : process_group.processes_) {
            for (size_t i = 0U; i < etas::vrte::lcm::kArgvArraySize && process.startup_config_.argv_[i] != nullptr;
                 ++i) {
                // RULECHECKER_comment(1, 1, check_pointer_qualifier_cast_const, "Remove const for standard library with char type arguments.", true);
                free(const_cast<char*>(process.startup_config_.argv_[i]));
                process.startup_config_.argv_[i] = nullptr;
            }
            for (size_t i = 0U; process.startup_config_.envp_[i] != nullptr; ++i) {
                // RULECHECKER_comment(1,1, check_stdlib_use_alloc, "compiler intrinsic calls", true_no_defect);
                free(process.startup_config_.envp_[i]);
                process.startup_config_.envp_[i] = nullptr;
            }
        }
    }
}

bool ConfigurationManager::initializeSoftwareClusterConfigurations() {
    bool result = false;

    // Load software clusters
    if (loadListOfSWClusters()) {
        result = true;
        for (uint_least8_t i = 0U; i < sw_clusters_.size(); i++) {
            if (loadSWClusterConfiguration(i)) {
                LM_LOG_DEBUG() << "Loading SWCL Nr." << static_cast<int>(i) << "Succeeded";
            } else {
                LM_LOG_ERROR() << "Failed to load SWCL Nr." << static_cast<int>(i);
                result = false;
            }
        }
    } else {
        LM_LOG_DEBUG() << "Failed to load software clusters";
    }

    return result;
}

bool ConfigurationManager::loadSWClusterConfiguration(uint8_t sw_cluster_index) {
    const auto data = read_flatbuffer_file("lm_demo.bin");
    if(!data) {
        return false;
    }

    // Extract root node from the loaded configuration
    auto root_node = LMFlatBuffer::GetLMEcuCfg(data.get());
    if(!root_node) {
        LM_LOG_DEBUG() << "Flatbuffer Parser: Empty configuration";
        return false;
    }

    // Load machine configurations from the root node
    if(!loadMachineConfigs(root_node, IdentifierHash(sw_clusters_[sw_cluster_index]))) {
        LM_LOG_DEBUG() << "Flatbuffer Parser: Failed to read Machine Configuration";
        return false;
    }

    if(!loadProcessConfigs(root_node)) {
        LM_LOG_DEBUG() << "Flatbuffer Parser: Failed to read Process Configuration";
        return false;
    }

    return true;
}

bool ConfigurationManager::loadMachineConfigs(const LMFlatBuffer::LMEcuCfg* root_node, const IdentifierHash& cluster) {
    bool result = false;

    const auto* mode_group = root_node ? root_node->ModeGroup() : nullptr;

    if (mode_group != nullptr) {
        result = true;  // Assume success if we reach this point

        for (uint32_t i = 0U; i < mode_group->size(); i++) {
            if (!parseMachineConfigurations(mode_group->Get(i), cluster)) {
                LM_LOG_WARN() << "Failed to parse mode group configurations";
                result = false;  // Set result to false on failure
                break;           // Exit loop on failure
            }
        }
    } else {
        LM_LOG_DEBUG() << "loadMachineConfigs ModeGroup is null";
    }

    return result;
}

bool ConfigurationManager::parseMachineConfigurations(const ModeGroup* node, const IdentifierHash& cluster) {
    bool result = false;

    if (node) {
        ProcessGroup process_group_data;
        process_group_data.name_ = getStringViewFromFlatBuffer(node->identifier());
        process_group_data.sw_cluster_ = cluster;
        LM_LOG_DEBUG() << "FlatBufferParser::getModeGroupPgName:" << getStringFromFlatBuffer(node->identifier())
                       << "( IdentifierHash:" << process_group_data.name_.data() << ")";

        if (process_group_data.name_ != score::lcm::IdentifierHash(std::string_view(""))) {
            // Add process group name to the PG name list
            process_group_names_.push_back(process_group_data.name_);
            result = parseModeGroups(node, process_group_data);
        } else {
            LM_LOG_WARN() << "parseMachineConfigurations: Process group name is empty furz";
        }
    }
    return result;
}

bool ConfigurationManager::parseModeGroups(const ModeGroup* node, ProcessGroup& process_group_data) {
    bool result = false;

    const auto* mode_declaration_list = node ? node->modeDeclaration() : nullptr;
    if (mode_declaration_list && (mode_declaration_list->size())) {
        process_group_data.off_state_ = IdentifierHash("Off");  // default value if no other path is defined

        const flatbuffers::String* recovery_state_name = node->recoveryMode_name();
        if (recovery_state_name) {
            process_group_data.recovery_state_ = getStringViewFromFlatBuffer(recovery_state_name);
        } else {
            process_group_data.recovery_state_ = IdentifierHash("Recovery");  // default value if nothing defined
        }

        for (const auto* mode_declaration_node : *mode_declaration_list) {
            const flatbuffers::String* flatbuffer_string = mode_declaration_node->identifier();
            if (flatbuffer_string) {
                ProcessGroupState pg_state;
                std::string string_name(flatbuffer_string->c_str(), flatbuffer_string->size());
                pg_state.name_ = getStringViewFromFlatBuffer(flatbuffer_string);
                LM_LOG_DEBUG() << "FlatBufferParser::getModeGroupPgStateName:"
                               << mode_declaration_node->identifier()->c_str()
                               << "( IdentifierHash:" << pg_state.name_.data() << ")";
                process_group_data.states_.push_back(pg_state);
                // Is this the "Off" state, i.e. does it end with "/Off" ?
                auto str_len = string_name.length();
                if ((str_len > 3UL) && (0 == string_name.compare(str_len - 4UL, 4UL, "/Off"))) {
                    process_group_data.off_state_ = pg_state.name_;
                }
            }
        }

        // Successfully parsed machine configurations
        process_groups_.push_back(process_group_data);
        result = true;
    } else {
        LM_LOG_DEBUG() << "parseMachineConfigurations: Mode declarations are not available or list is null";
    }
    return result;
}

bool ConfigurationManager::loadProcessConfigs(const LMFlatBuffer::LMEcuCfg* root_node) {
    bool result = false;

    const auto* process = root_node ? root_node->Process() : nullptr;

    if (process != nullptr) {
        result = true;  // Assume success if we reach this point

        for (uint32_t i = 0U; i < process->size(); i++) {
            if (!parseProcessConfigurations(process->Get(i))) {
                LM_LOG_DEBUG() << "Failed to parse process configurations";
                result = false;  // Mark failure if parsing fails
                break;           // Stop processing further if parsing fails
            }
        }
    } else {
        LM_LOG_DEBUG() << "Process node is null";
    }

    return result;
}

static void setSchedulingParameters(const Process& node, const ProcessStartupConfig& config, OsProcess& instance) {
    instance.startup_config_.cpu_mask_ = ConfigurationManager::kDefaultProcessorAffinityMask();
    instance.startup_config_.scheduling_policy_ = ConfigurationManager::kDefaultSchedulingPolicy;
    instance.startup_config_.scheduling_priority_ = ConfigurationManager::kDefaultNormalSchedulingPriority;
    auto attribute = config.schedulingPolicy();
    if (attribute != nullptr) {
        if (strcasecmp("SCHED_FIFO", attribute->c_str()) == 0) {
            instance.startup_config_.scheduling_policy_ = SCHED_FIFO;
            instance.startup_config_.scheduling_priority_ = ConfigurationManager::kDefaultRealtimeSchedulingPriority;
        } else if (strcasecmp("SCHED_RR", attribute->c_str()) == 0) {
            instance.startup_config_.scheduling_policy_ = SCHED_RR;
            instance.startup_config_.scheduling_priority_ = ConfigurationManager::kDefaultRealtimeSchedulingPriority;
        } else if (strcasecmp("SCHED_OTHER", attribute->c_str()) == 0) {
            instance.startup_config_.scheduling_policy_ = SCHED_OTHER;
        } else {
            LM_LOG_WARN() << "scheduling policy" << attribute->c_str() << "is not supported, using default";
        }
    }
    attribute = config.schedulingPriority();
    if (attribute != nullptr) {
        instance.startup_config_.scheduling_priority_ = std::stoi(attribute->c_str());
    }
    attribute = node.vrte_coremask();
    if (attribute != nullptr) {
        instance.startup_config_.cpu_mask_ = static_cast<uint32_t>(std::stoul(attribute->c_str()) & 0XFFFFFFFFUL);
    }
}

bool ConfigurationManager::parseProcessConfigurations(const Process* node) {
    bool result = false;

    const auto* startup_config_list = node ? node->startupConfig() : nullptr;

    if (startup_config_list && (startup_config_list->size())) {
        result = true;
        for (const auto* startup_config_node : *startup_config_list) {
            // Populate instance details based on startup_config_node data
            OsProcess instance;
            instance.process_number_ = process_index_;  // Each instance gets a unique number
            if (process_index_ < 0XFFFFFFFFU) {
                process_index_++;
            }

            setSchedulingParameters(*node, *startup_config_node, instance);
            // Set executable path from node's vrte_path
            instance.startup_config_.executable_path_ = getStringFromFlatBuffer(node->vrte_path());
            LM_LOG_DEBUG() << "parseProcessConfigurations: Process index:" << instance.process_number_
                           << "executable_path_:" << instance.startup_config_.executable_path_;

            instance.startup_config_.short_name_ = node->identifier() ? node->identifier()->c_str() : "Unknown";
            instance.startup_config_.uid_ = node->vrte_uid() & 0x7FFFFFFFU;
            instance.startup_config_.gid_ = node->vrte_gid() & 0x7FFFFFFFU;

            instance.startup_config_.security_policy_ =
                getStringFromFlatBuffer(node->vrte_securityPolicyDetails());  // Set security policy if available

            // extracting supplementary group IDs from Process configuration
            // and assigning them to this particular startup config (aka OsProcess)
            auto supplementary_gids = node->vrte_sgids();
            size_t supplementary_gids_number = supplementary_gids ? supplementary_gids->size() : 0U;
            if (supplementary_gids_number > 0U) {
                instance.startup_config_.supplementary_gids_.reserve(supplementary_gids_number);
            }
            for (uint32_t i = 0U; i < (supplementary_gids_number & 0XFFFFFFFFU); i++) {
                const ProcessSgid* sgid_conf = supplementary_gids->Get(i);
                if (nullptr != sgid_conf) {
                    instance.startup_config_.supplementary_gids_.push_back(sgid_conf->vrte_sgid());
                }
            }

            instance.startup_config_.comms_type_ = getCommsType(node, instance.startup_config_.short_name_.c_str());

            // startup configs
            result = setResourceLimits(*startup_config_node, instance);
            instance.pgm_config_.is_self_terminating_ = isSelfTerminatingProcess(
                startup_config_node->terminationBehavior(), instance.startup_config_.short_name_);
            instance.pgm_config_.startup_timeout_ms_ =
                std::chrono::milliseconds(startup_config_node->enterTimeoutValue());
            instance.pgm_config_.termination_timeout_ms_ =
                std::chrono::milliseconds(startup_config_node->exitTimeoutValue());

            auto execution_error_string = startup_config_node->executionError();
            if (execution_error_string) {
                instance.pgm_config_.execution_error_code_ =
                    static_cast<uint32_t>(std::stoi(execution_error_string->c_str()));
            } else {
                // default value
                instance.pgm_config_.execution_error_code_ = kDefaultProcessExecutionError;
            }
            instance.pgm_config_.number_of_restart_attempts = node->numberOfRestartAttempts();

            // Set process_id from node's identifier
            instance.process_id_ = getStringViewFromFlatBuffer(node->identifier());

            // Parse process arguments and environment variables
            parseProcessArguments(startup_config_node->processArgument(), instance);
            parseProcessEnvironmentVars(startup_config_node->environmentVariable(), instance);

            // Parse Execution dependency
            parseExecutionDependency(startup_config_node->executionDependency(), instance);

            // Parse ProcessGroup dependency
            parseProcessGroup(startup_config_node->processGroupStateDependency(), instance);
        }
    } else {
        LM_LOG_DEBUG() << "parseProcessConfigurations: Startup configs are not available or list is null";
    }

    return result;
}

void ConfigurationManager::parseProcessArguments(
    const flatbuffers::Vector<flatbuffers::Offset<LMFlatBuffer::ProcessArgument>>* process_arg_list,
    OsProcess& process_instance) {
    // Initialize the argument index to 0 for setting the executable path
    size_t arg_index = 0U;

    // Set the executable path as the first argument
    process_instance.startup_config_.argv_[arg_index] =
        strdup(process_instance.startup_config_.executable_path_.c_str());

    // LM_LOG_DEBUG() << "Executable path set as the first argument:" << process_instance.executable_path_;

    // Increment the argument index for the next argument
    ++arg_index;

    if (process_arg_list) {
        // Convert the size of process_arg_list to size_t and log the number of arguments
        size_t arg_count = static_cast<size_t>(process_arg_list->size());

        // Calculate the maximum number of arguments to process, considering the argv size limit
        size_t max_args = std::min(arg_count, static_cast<size_t>(etas::vrte::lcm::kMaxArg));


        // Check if the number of arguments exceeds the maximum allowed size and log a warning if it does
        if (arg_count > static_cast<std::size_t>(etas::vrte::lcm::kMaxArg)) {
            LM_LOG_DEBUG() << "Number of process arguments exceeds maximum allowed size (kMaxArg ="
                           << static_cast<size_t>(etas::vrte::lcm::kMaxArg) << "). Only the first"
                           << static_cast<size_t>(etas::vrte::lcm::kMaxArg) << "arguments will be processed.";
        }

        // Iterate through the process arguments and add them to the argv array
        for (size_t i = 0U; i < max_args; ++i) {
            auto process_arg_node = process_arg_list->Get(static_cast<uint32_t>(i));

            if (process_arg_node) {
                // Convert the flatbuffer argument to a C string and add it to the argv array
                auto argument = getStringFromFlatBuffer(process_arg_node->argument());
                // coverity[autosar_cpp14_a4_7_1_violation:FALSE] Arg count is checked above - no risk of wraparound.
                process_instance.startup_config_.argv_[arg_index++] = strdup(argument);
            }
        }
    }

    // the argv_ member is std::array and it is initlized so all elements
    // are nullptr to begin with and so we don't need to append one.
}

void ConfigurationManager::parseProcessEnvironmentVars(
    const flatbuffers::Vector<flatbuffers::Offset<LMFlatBuffer::EnvironmentVariable>>* env_var_list,
    OsProcess& process_instance) {
    size_t env_index = 0U;

    if (env_var_list) {
        // Convert the size of env_var_list to size_t and log the number of environment variables
        size_t env_count = static_cast<size_t>(env_var_list->size());
        // LM_LOG_DEBUG() << "Number of process environment variables:" << env_count;

        // Calculate the maximum number of environment variables to process, considering the envp size limit
        size_t max_env = std::min(env_count, static_cast<size_t>(etas::vrte::lcm::kMaxEnv));
        // LM_LOG_DEBUG() << "Number of process environment variables to process:" << max_env;

        // Check if the number of environment variables exceeds the maximum allowed size and log a warning if it does
        if (env_count > static_cast<std::size_t>(etas::vrte::lcm::kMaxEnv)) {
            LM_LOG_WARN() << "Number of process environment variables exceeds maximum allowed size (kMaxEnv ="
                          << static_cast<size_t>(etas::vrte::lcm::kMaxEnv) << "). Only the first"
                          << static_cast<size_t>(etas::vrte::lcm::kMaxEnv) << "variables will be processed.";
        }

        // Iterate through the process environment variables and add them to the envp array
        for (size_t i = 0U; i < max_env; ++i) {
            auto env_var_node = env_var_list->Get(static_cast<uint32_t>(i));

            if (env_var_node) {
                auto key = getStringFromFlatBuffer(env_var_node->key());
                auto value = getStringFromFlatBuffer(env_var_node->value());

                // Format environment variable as "key=value"
                std::string env_str = std::string(key) + "=" + std::string(value);  // TODO

                // coverity[autosar_cpp14_a4_7_1_violation:FALSE] Environment count is checked above - no risk of wraparound.
                process_instance.startup_config_.envp_[env_index++] = strdup(env_str.c_str());
            }
        }
    } else {
        // Log if the process environment variables list is null
        // LM_LOG_DEBUG() << "Process environment variables list is not available";
    }
    // Add a NULL terminator at the end of the array
    process_instance.startup_config_.envp_[env_index] = nullptr;
    // LM_LOG_DEBUG() << "Environment variable array null-terminated at index" << env_index;
}

void ConfigurationManager::parseProcessGroup(
    const flatbuffers::Vector<flatbuffers::Offset<LMFlatBuffer::ProcessGroupStateDependency>>* process_pg_list,
    const OsProcess& process_instance) {
    if (process_pg_list) {
        for (const auto& process_pg_node : *process_pg_list) {
            if (process_pg_node) {
                // Extract information from ProcessGroupStateDependency of flatconfig binary
                ProcessGroupStateID pg_info;
                pg_info.pg_name_ = getStringViewFromFlatBuffer(process_pg_node->stateMachine_name());
                pg_info.pg_state_name_ = getStringViewFromFlatBuffer(process_pg_node->stateName());
                LM_LOG_DEBUG() << "ParseProcessProcessGroup: id:::pg_name_:" << pg_info.pg_name_.data()
                               << ", pg_state_name_:" << pg_info.pg_state_name_.data();

                // Assign OsProcess instance to the process group
                AssignOsProcessInstanceToProcessGroup(pg_info, process_instance);
            }
        }
        // Successfully processed all process groups
    } else {
        LM_LOG_DEBUG() << "ParseProcessProcessGroup: Process process groups are not available";
    }
}

void ConfigurationManager::parseExecutionDependency(
    const flatbuffers::Vector<flatbuffers::Offset<LMFlatBuffer::ProcessExecutionDependency>>* process_dependency_list,
    OsProcess& process_instance) {
    if (process_dependency_list) {
        for (const auto& process_dependency_node : *process_dependency_list) {
            if (process_dependency_node) {
                Dependency dep{};
                auto state_name = getStringViewFromFlatBuffer(process_dependency_node->stateName());
                dep.process_state_ = getProcessState(state_name);
                dep.target_process_id_ = getStringViewFromFlatBuffer(process_dependency_node->targetProcess_identifier());
                LM_LOG_DEBUG() << "ParseProcessExecutionDependency: target process path:"
                                << getStringFromFlatBuffer(process_dependency_node->targetProcess_identifier())
                                << "ID:" << dep.target_process_id_.data();
                process_instance.dependencies_.push_back(dep);
                
            }
        }
    } else {
        LM_LOG_DEBUG() << "ParseProcessExecutionDependency: Process execution dependencies are not available";
    }
}

ProcessState ConfigurationManager::getProcessState(const IdentifierHash& state_name) {
    ProcessState result = ProcessState::kIdle;

    if (state_name == static_cast<IdentifierHash>(PROCESS_RUNNING_STATE)) {
        result = ProcessState::kRunning;
    } else if (state_name == static_cast<IdentifierHash>(PROCESS_TERMINATED_STATE)) {
        result = ProcessState::kTerminated;
    }

    return result;
}

ProcessGroup* ConfigurationManager::getProcessGroupByID(const IdentifierHash& pg_name) const {
    const ProcessGroup* result = nullptr;

    if (!process_groups_.empty()) {
        auto it = find_if(process_groups_.begin(), process_groups_.end(),
                          [&pg_name](const ProcessGroup& pg) { return pg.name_ == pg_name; });

        if (it != process_groups_.end()) {
            result = &(*it);
        }
    }

    // RULECHECKER_comment(1, 1, check_pointer_qualifier_cast_const, "Remove const for standard library with char type arguments.", true);
    return const_cast<ProcessGroup*>(result);
}

ProcessGroupState* ConfigurationManager::getProcessGroupStateByID(const ProcessGroupStateID& pg_id) const {
    ProcessGroupState* result = nullptr;

    ProcessGroup* pg = getProcessGroupByID(pg_id.pg_name_);

    if (pg) {
        auto it = find_if(pg->states_.begin(), pg->states_.end(),
                          [&pg_id](const ProcessGroupState& state) { return state.name_ == pg_id.pg_state_name_; });

        if (it != pg->states_.end()) {
            result = &(*it);
        }
    }

    return result;
}

ProcessGroupState* ConfigurationManager::getProcessGroupStateByID(ProcessGroup& pg,
                                                                    const IdentifierHash& state_name) const {
    ProcessGroupState* foundState = nullptr;

    auto it = find_if(pg.states_.begin(), pg.states_.end(),
                      [&state_name](const ProcessGroupState& state) { return state.name_ == state_name; });

    if (it != pg.states_.end()) {
        foundState = &(*it);
    }

    return foundState;
}

void ConfigurationManager::AssignOsProcessInstanceToProcessGroup(const ProcessGroupStateID& process_pg,
                                                                  const OsProcess& process_instance) {
    // Find the process group by name
    auto pg = getProcessGroupByID(process_pg.pg_name_);
    if (pg != nullptr) {
        // Find the process group state by name within the process group
        auto state = getProcessGroupStateByID(*pg, process_pg.pg_state_name_);
        if (state != nullptr) {
            uint32_t index_in_pg;
            // Add the process instance to the process group if it's not already there
            auto it = find_if(pg->processes_.begin(), pg->processes_.end(),
                              [&process_instance](const OsProcess& inst) -> bool {
                                  return process_instance.process_number_ == inst.process_number_;
                              });
            if (it == pg->processes_.end()) {
                // get index and insert new value
                index_in_pg = static_cast<uint32_t>(pg->processes_.size() & 0XFFFFFFFFUL);
                pg->processes_.push_back(process_instance);
            } else {
                // already there, calculate index of the entry
                index_in_pg = static_cast<uint32_t>((it - pg->processes_.begin()) & 0X7FFFFFFFL);
            }
            // Add the process index if it doesn't already exist in the process group state
            if (find(state->process_indexes_.begin(), state->process_indexes_.end(), index_in_pg) ==
                state->process_indexes_.end()) {
                state->process_indexes_.push_back(index_in_pg);
            }
        } else {
            LM_LOG_WARN() << "Process group state not found:" << process_pg.pg_state_name_.data();
        }
    } else {
        LM_LOG_WARN() << "Process group not found:", process_pg.pg_name_.data();
    }
}

void ConfigurationManager::AssignOsProcessIndexesInDependencyList() {
    for (auto& pg : process_groups_) {
        for (size_t process_index = 0U; process_index < pg.processes_.size(); ++process_index) {
            auto& process = pg.processes_[process_index];
            UpdateOsProcessIndexInDependencyList(pg, process.dependencies_);
        }
    }
}

void ConfigurationManager::UpdateOsProcessIndexInDependencyList(ProcessGroup& pg, DependencyList& dep_list) {
    // Lambda to process each dependency
    auto processDependency = [&](Dependency& dependency) {
        // Lambda to match target process ID and update index
        auto matchAndUpdateOsProcessIndex = [&]() {
            for (size_t process_index = 0U; process_index < pg.processes_.size(); ++process_index) {
                auto& os_process = pg.processes_[process_index];

                if (dependency.target_process_id_ == os_process.process_id_) {
                    dependency.os_process_index_ = static_cast<uint32_t>(process_index & 0xFFFFFFFFUL);

                    return;  // Exit the loop once the update is done
                }
            }
        };

        matchAndUpdateOsProcessIndex();
    };

    // Process each dependency in the dependency list
    for (auto& dependency : dep_list) {
        processDependency(dependency);
    }
}

bool ConfigurationManager::checkOrSetFlatConfigEnvVar(const std::string& name, const std::string& path) {
    bool result = false;
    const char* value = getenv(name.c_str());

    if (value && strlen(value)) {
        LM_LOG_DEBUG() << name.c_str() << "already set. Current value:" << value;
        result = true;
    } else {
        if (setenv(name.c_str(), path.c_str(), overwrite_) == 0) {
            result = true;
        } else {
            LM_LOG_DEBUG() << name.c_str() << "not set, so default flat config binary path loaded";
        }
    }

    return result;
}

bool ConfigurationManager::loadListOfSWClusters() {
    bool result = false;

    // Default value for demonstration
    sw_clusters_.clear();
    sw_clusters_.emplace_back("DefaultSoftwareCluster");
    result = true;

    return result;
}

osal::CommsType ConfigurationManager::getfunctionClusterAffiliation(osal::CommsType current_comms,
                                                                    const char* attribute) {
    osal::CommsType comms_type = current_comms;

    if (attribute && std::string_view(attribute) == "STATE_MANAGEMENT") {
        comms_type = osal::CommsType::kControlClient;
        LM_LOG_DEBUG() << "Process is STATE_MANAGEMENT function Cluster Affiliation";
    } else if (attribute && std::string_view(attribute) == "PLATFORM_HEALTH_MANAGEMENT") {
        //TODO - example introduce PHM enum.
        LM_LOG_DEBUG() << "Process is PLATFORM_HEALTH_MANAGEMENT function Cluster Affiliation";
    } else if (attribute && std::string_view(attribute) == "LAUNCH_MANAGEMENT") {
        comms_type = osal::CommsType::kLaunchManager;
        LM_LOG_DEBUG() << "Process is LAUNCH_MANAGEMENT function Cluster Affiliation";
    } else {
        LM_LOG_DEBUG() << "Process is NOT associated with any function Cluster Affiliation";
    }

    return comms_type;
}

// TODO - This is workaround solution for comms_type. Since reporting behaviour
//        and function cluster affiliation both are different config. we need to seperate it.
osal::CommsType ConfigurationManager::getCommsType(const Process* node, const char* short_name) {
    osal::CommsType comms_type = osal::CommsType::kNoComms;

    if (node != nullptr) {
        // Check reporting behavior
        comms_type = isReportingProcess(node->executable_reportingBehavior(), short_name);

        // Check function cluster affiliation
        comms_type =
            getfunctionClusterAffiliation(comms_type, getStringFromFlatBuffer(node->functionClusterAffiliation()));
    }
    return comms_type;
}

osal::CommsType ConfigurationManager::isReportingProcess(const ExecutionStateReportingBehaviorEnum reporting_behaviour,
                                                         const std::string_view process_name) {
    osal::CommsType reporting_status = osal::CommsType::kNoComms;

    if (reporting_behaviour == ExecutionStateReportingBehaviorEnum::ExecutionStateReportingBehaviorEnum_ReportsExecutionState) {
        reporting_status = osal::CommsType::kReporting;
        LM_LOG_DEBUG() << "Process" << process_name.data() << "is Reporting execution state";
    } else {
        // ExecutionStateReportingBehaviorEnum::DoesNotReportExecutionState
        LM_LOG_DEBUG() << "Process" << process_name.data() << "is NOT Reporting execution state";
    }

    return reporting_status;
}

bool ConfigurationManager::isSelfTerminatingProcess(const TerminationBehaviorEnum termination_behavior,
                                                    const std::string_view process_name) {
    bool termination_status = false;

    if (termination_behavior == TerminationBehaviorEnum::TerminationBehaviorEnum_ProcessIsSelfTerminating) {
        termination_status = true;
        LM_LOG_DEBUG() << "Process" << process_name.data() << "is Self terminating";
    } else {
        // TerminationBehaviorEnum::ProcessIsNotSelfTerminating
        LM_LOG_DEBUG() << "Process" << process_name.data() << "is NOT Self terminating";
    }

    return termination_status;
}

std::optional<const ProcessGroup*> ConfigurationManager::getProcessGroupByNameAndIndex(const IdentifierHash& pg_name,
                                                                                    const uint32_t index) const {
    std::optional<const ProcessGroup*> result = std::nullopt;

    auto pg = getProcessGroupByID(pg_name);

    if (pg) {
        if (index < pg->processes_.size()) {
            result = pg;
        } else {
            LM_LOG_DEBUG() << "Process index" << index << "is out of bounds in process group" << pg_name.data();
        }
    } else {
        LM_LOG_DEBUG() << "Process group not found:" << pg_name.data();
    }

    return result;
}

IdentifierHash ConfigurationManager::getStringViewFromFlatBuffer(const flatbuffers::String* flat_string) {
    return flat_string ? IdentifierHash{flat_string->c_str()} : IdentifierHash{};
}

const char* ConfigurationManager::getStringFromFlatBuffer(const flatbuffers::String* flat_string) {
    return flat_string ? flat_string->c_str() : "";
}

}  // namespace lcm

}  // namespace vrte

}  // namespace etas
