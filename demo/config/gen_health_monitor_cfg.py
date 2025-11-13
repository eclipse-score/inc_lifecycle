# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
import argparse
from pathlib import Path
import os
import json
from gen_common_cfg import get_process_index_range

def get_process(index : int, process_group : str):
    return '''
{
    "index": '''+str(index)+''',
    "shortName": "demo_application'''+str(index)+'''",
    "identifier": "demo_app'''+str(index)+"_"+process_group+'''",
    "processType": "REGULAR_PROCESS",
    "refProcessGroupStates": [
        {
            "identifier": "'''+process_group+'''/Startup"
        }
    ],
    "processExecutionErrors": [
        {
            "processExecutionError": 1
        }
    ]
}
'''

def get_monitor_interfaces(index : int, process_group : str):
    return '''
{
    "instanceSpecifier": "demo/demo_application'''+str(index)+'''/Port1",
    "processShortName": "demo_application'''+str(index)+'''",
    "portPrototype": "Port1",
    "interfacePath": "demo_application_'''+str(index)+"_"+process_group+'''",
    "refProcessIndex": '''+str(index)+''',
    "permittedUid": 0
}
'''

def get_checkpoints(index : int):
    # Every demo app has three checkpoints
    return ['''
{
    "shortName": "Checkpoint'''+str(index)+"_1"+'''",
    "checkpointId": 1,
    "refInterfaceIndex": '''+str(index)+'''
}''', '''
{
    "shortName": "Checkpoint'''+str(index)+"_2"+'''",
    "checkpointId": 2,
    "refInterfaceIndex": '''+str(index)+'''
}''', '''
{
    "shortName": "Checkpoint'''+str(index)+"_3"+'''",
    "checkpointId": 3,
    "refInterfaceIndex": '''+str(index)+'''
}
''']

def get_alive_supervisions(index : int, process_group : str):
    # Every demo app has three checkpoints and the first checkpoint is used for alive supervision
    checkpointIdx = (index * 3)
    return '''
{
    "ruleContextKey": "AliveSupervision'''+str(index)+'''",
    "refCheckPointIndex": '''+str(checkpointIdx)+''',
    "aliveReferenceCycle": 100.0,
    "minAliveIndications": 1,
    "maxAliveIndications": 3,
    "isMinCheckDisabled": false,
    "isMaxCheckDisabled": false,
    "failedSupervisionCyclesTolerance": 1,
    "refProcessIndex": '''+str(index)+''',
    "refProcessGroupStates": [
        {
            "identifier": "'''+process_group+'''/Startup"
        }
    ]
}
'''

def get_deadline_supervisions(index : int, process_group : str):
    # Every demo app has three checkpoints and the first+second checkpoints are used for deadline supervision
    sourceCpIndex = index * 3
    targetCpIndex = sourceCpIndex + 1
    return '''
{
    "ruleContextKey": "DeadlineSupervision'''+str(index)+'''",
    "maxDeadline": 80.0,
    "minDeadline": 40.0,
    "checkpointTransition": {
        "refSourceCPIndex": '''+str(sourceCpIndex)+''',
        "refTargetCPIndex": '''+str(targetCpIndex)+'''
    },
    "refProcessIndices": ['''+str(index)+'''],
    "refProcessGroupStates": [
        {
            "identifier": "'''+process_group+'''/Startup"
        }
    ]
}
'''

def get_logical_supervisions(index : int, process_group : str):
    # Every demo app has three checkpoints and all three checkpoints in sequence are used for logical supervision
    startCpIndex = index * 3
    return '''
{
    "ruleContextKey": "LogicalSupervision'''+str(index)+'''",
    "checkpoints": [
    {
        "refCheckPointIndex": '''+str(startCpIndex)+''',
        "isInitial": true,
        "isFinal": false
    },
    {
        "refCheckPointIndex": '''+str(startCpIndex+1)+''',
        "isInitial": false,
        "isFinal": false
    },
    {
        "refCheckPointIndex": '''+str(startCpIndex+2)+''',
        "isInitial": false,
        "isFinal": true
    }],
    "transitions": [{
        "checkpointSourceIdx": 0,
        "checkpointTargetIdx": 1
    },{
        "checkpointSourceIdx": 1,
        "checkpointTargetIdx": 2
    }],
    "refProcessIndices": ['''+str(index)+'''],
    "refProcessGroupStates": [
        {
            "identifier": "'''+process_group+'''/Startup"
        }
    ]
}
'''

def get_local_supervisions(index : int):
    return '''
{
    "ruleContextKey": "LocalSupervision'''+str(index)+'''",
    "infoRefInterfacePath": "demo_application_'''+str(index)+'''",
    "hmRefAliveSupervision": [
        {
            "refAliveSupervisionIdx": '''+str(index)+'''
        }
    ],
    "hmRefDeadlineSupervision": [
        {
            "refDeadlineSupervisionIdx": '''+str(index)+'''
        }
    ],
    "hmRefLogicalSupervision": [
        {
            "refLogicalSupervisionIdx": '''+str(index)+'''
        }
    ]
}
'''

def get_global_supervisions(process_count : int, process_group_index : int, process_group : str):
    localSupervisionRefs = []
    processRefs = []
    for i in get_process_index_range(process_count, process_group_index):
        localSupervisionRefs.append(json.loads('''
{
    "refLocalSupervisionIndex": '''+str(i)+'''
}'''))
        processRefs.append(json.loads('''
{
    "index": '''+str(i)+'''
}'''))

    globalSupervisions=json.loads('''
{
    "ruleContextKey": "GlobalSupervision_'''+process_group+'''",
    "isSeverityCritical": false,
    "localSupervision": [],
    "refProcesses": [],
    "refProcessGroupStates": [
        {
            "identifier": "'''+process_group+'''/Startup"
        }
    ]
}
''')
    globalSupervisions["localSupervision"].extend(localSupervisionRefs)
    globalSupervisions["refProcesses"].extend(processRefs)
    return json.dumps(globalSupervisions)

def get_recovery_notifications(process_count : int, process_group_index : int, process_group : str):
    return'''
{
    "shortName" : "RecoveryNotification_'''+process_group+'''",
    "recoveryNotificationTimeout" : 4000.0,
    "processGroupMetaModelIdentifier" : "'''+process_group+'''/Recovery",
    "refGlobalSupervisionIndex" : '''+str(process_group_index)+''',
    "instanceSpecifier" : "",
    "shouldFireWatchdog" : false
}
'''
def gen_health_monitor_cfg_for_process_group(config, process_count : int, process_group : str, process_group_index : int):
    processes = []
    monitorInterfaces = []
    checkpoints = []
    hmAliveSupervisions = []
    hmDeadlineSupervisions = []
    hmLogicalSupervisions = []
    hmLocalSupervisions = []
    hmGlobalSupervision = []
    hmRecoveryNotifications = []

    for process_index in get_process_index_range(process_count, process_group_index):
        print(f"process Index {process_index} for FG {process_group}")
        processes.append(json.loads(get_process(process_index, process_group)))
        monitorInterfaces.append(json.loads(get_monitor_interfaces(process_index, process_group)))

        for cp in get_checkpoints(process_index):
            checkpoints.append(json.loads(cp))

        hmAliveSupervisions.append(json.loads(get_alive_supervisions(process_index, process_group)))
        hmDeadlineSupervisions.append(json.loads(get_deadline_supervisions(process_index,process_group)))
        hmLogicalSupervisions.append(json.loads(get_logical_supervisions(process_index, process_group)))
        hmLocalSupervisions.append(json.loads(get_local_supervisions(process_index)))
    
    hmGlobalSupervision.append(json.loads(get_global_supervisions(process_count, process_group_index, process_group)))
    hmRecoveryNotifications.append(json.loads(get_recovery_notifications(process_count, process_group_index, process_group)))

    config["process"].extend(processes)
    config["hmMonitorInterface"].extend(monitorInterfaces)
    config["hmSupervisionCheckpoint"].extend(checkpoints)
    config["hmAliveSupervision"].extend(hmAliveSupervisions)
    config["hmDeadlineSupervision"].extend(hmDeadlineSupervisions)
    config["hmLogicalSupervision"].extend(hmLogicalSupervisions)
    config["hmLocalSupervision"].extend(hmLocalSupervisions)
    config["hmGlobalSupervision"].extend(hmGlobalSupervision)
    config["hmRecoveryNotification"].extend(hmRecoveryNotifications)
    return config

def gen_health_monitor_cfg(process_count : int, process_groups : list):
    config=json.loads('''
{
    "versionMajor": 8,
    "versionMinor": 0,
    "process": [],
    "hmMonitorInterface": [],
    "hmSupervisionCheckpoint": [],
    "hmAliveSupervision": [],
    "hmDeadlineSupervision": [],
    "hmLogicalSupervision": [],
    "hmLocalSupervision": [],
    "hmGlobalSupervision": [],
    "hmRecoveryNotification": []
}
''')
    
    for i in range(0, len(process_groups)):
        gen_health_monitor_cfg_for_process_group(config, process_count, process_groups[i], i)

    return config

if __name__ == '__main__':
    my_parser = argparse.ArgumentParser()
    my_parser.add_argument('-c', "--cppprocesses", action='store', type=int, required=True, help="Number of C++ processes")
    my_parser.add_argument('-r', "--rustprocesses", action='store', type=int, required=True, help="Number of Rust processes")
    my_parser.add_argument('-p','--process_groups', nargs='+', help='Name of a Process Group', required=True)
    my_parser.add_argument('-o', "--out", action='store', type=Path, required=True, help="Output directory")
    args = my_parser.parse_args()

    cfg_out_path = os.path.join(args.out, f'hm_demo.json')
    with open(cfg_out_path, 'w') as f:
        json.dump(gen_health_monitor_cfg(args.cppprocesses + args.rustprocesses, args.process_groups), f, indent=4)
