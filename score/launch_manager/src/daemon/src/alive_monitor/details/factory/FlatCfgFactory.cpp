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

#include "score/mw/launch_manager/alive_monitor/details/factory/FlatCfgFactory.hpp"

#include <cassert>
#include <cmath>
#include <cstring>

#include <score/assert.hpp>

#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/IPhmFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/common/alive_interface_path.hpp"
#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"

namespace score::mw::lifecycle::internal::saf::factory
{

using RecoveryClient = score::mw::lifecycle::IRecoveryClient;
using NanoSecondType = saf::timers::NanoSecondType;
using IdentifierHash = score::mw::lifecycle::IdentifierHash;

FlatCfgFactory::FlatCfgFactory() : IPhmFactory()
{
}

bool FlatCfgFactory::createObservableEvent(
    std::vector<ifexm::ObservableEvent>& events,
    const IdentifierHash component_id,
    ifexm::ObservableEventReader& event_reader_)
{
    try
    {
        auto& res = events.emplace_back(component_id);
        if (event_reader_.registerObservableEvent(res, component_id))
        {
            LM_LOG_DEBUG() << "Successfully created Observable Event:" << component_id;
            return true;
        }
    }
    catch (const std::exception& f_exception_r)
    {
        LM_LOG_ERROR() << "Could not create Observable Events due to exception:"
                       << std::string_view{f_exception_r.what()};
    }

    return false;
}

bool FlatCfgFactory::initIpcServerWithUidBasedAccess(
    ifappl::CheckpointIpcServer& f_ipcServer_r,
    const std::string& f_ipcPath_r,
    const std::int32_t f_uid) noexcept(false)
{
    constexpr mode_t kOwnerReadWrite{384U};  // 0600 in octal
    if (f_ipcServer_r.init(f_ipcPath_r, kOwnerReadWrite) != ifappl::CheckpointIpcServer::EIpcInitResult::kOk)
    {
        return false;
    }

    const uid_t uid = static_cast<uid_t>(f_uid);
    if (!f_ipcServer_r.setAccessRights(uid))
    {
        LM_LOG_ERROR() << "Could not set ACL permissions (r/w for uid" << uid
                       << ") for Monitor interface IPC with path:" << f_ipcPath_r;
        return false;
    }
    return true;
}

bool FlatCfgFactory::createAliveIfIpc(
    std::vector<ifappl::CheckpointIpcServer>& servers,
    const IdentifierHash component_id,
    const uid_t uid)
{
    try
    {
        const std::string pathInterface = aliveInterfacePath(component_id);
        auto& server = servers.emplace_back();

        if (initIpcServerWithUidBasedAccess(server, pathInterface, uid))
        {
            LM_LOG_DEBUG() << "Successfully created Monitor interface IPC with path:" << pathInterface;
            return true;
        }
        else
        {
            LM_LOG_ERROR() << "Could not create Monitor interface IPC with path:" << pathInterface;
            return false;
        }
    }
    catch (const std::exception& f_exception_r)
    {
        LM_LOG_ERROR() << "Could not create Monitor interface IPC due to exception:"
                       << std::string_view{f_exception_r.what()};
        return false;
    }
}

bool FlatCfgFactory::createAliveIf(
    std::vector<ifappl::MonitorIfDaemon>& interfaces,
    ifappl::CheckpointIpcServer& ipc_server,
    ifexm::ObservableEvent& event)
{
    try
    {
        auto& interface = interfaces.emplace_back(ipc_server, ipc_server.getPath().data());
        event.attachObserver(interface);

        LM_LOG_DEBUG() << "Successfully created MonitorInterface:" << interface.getInterfaceName();
        return true;
    }
    catch (const std::exception& f_exception_r)
    {
        LM_LOG_ERROR() << "Could not create all necessary Monitor interfaces due to exception:"
                       << std::string_view{f_exception_r.what()};
        return false;
    }
}

bool FlatCfgFactory::createSupervisionCheckpoint(
    std::vector<ifappl::Checkpoint>& checkpoints,
    ifappl::MonitorIfDaemon& interface,
    const ifexm::ObservableEvent& event,
    const IdentifierHash component_id)
{
    try
    {
        auto& checkpoint = checkpoints.emplace_back(&event);
        interface.attachCheckpoint(checkpoint);

        LM_LOG_DEBUG() << "Successfully created supervision checkpoint for component:" << component_id;

        return true;
    }
    catch (const std::exception& f_exception_r)
    {
        LM_LOG_ERROR() << "Could not create supervision worker objects, due to exception:"
                       << std::string_view{f_exception_r.what()};
        return false;
    }
}

bool FlatCfgFactory::createAliveSupervision(
    std::vector<supervision::Alive>& supervisions,
    ifappl::Checkpoint& checkpoint,
    ifexm::ObservableEvent& event,
    const std::shared_ptr<IRecoveryClient> recovery_client,
    const IdentifierHash component_id,
    const ComponentAliveSupervision component_config)
{
    try
    {
        auto& alive = supervisions.emplace_back(
            component_id, component_config, recovery_client, checkpoint, kDefaultAliveSupCheckpointBufferElements);

        event.attachObserver(alive);

        LM_LOG_DEBUG() << "Successfully created alive supervision worker object:" << alive.getConfigName();
        return true;
    }
    catch (const std::exception& f_exception_r)
    {
        LM_LOG_ERROR() << "Could not create all necessary alive supervision "
                          "worker objects, due to exception:"
                       << std::string_view{f_exception_r.what()};
        return false;
    }
}

}  // namespace score::mw::lifecycle::internal::saf::factory
