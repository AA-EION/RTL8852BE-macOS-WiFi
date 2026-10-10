// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

#include "RTL8852BEUserClient.hpp"
#include "RTL8852BE.hpp"
#include <string.h>

#define super IOUserClient
OSDefineMetaClassAndStructors(RTL8852BEUserClient, IOUserClient);

const IOExternalMethodDispatch RTL8852BEUserClient::sMethods[kNumberOfMethods] = {
    { // kMethodScan
        (IOExternalMethodAction)&RTL8852BEUserClient::sScan,
        0, 0,
        0, 0
    },
    { // kMethodGetScanResults
        (IOExternalMethodAction)&RTL8852BEUserClient::sGetScanResults,
        0, 0,
        0, sizeof(RTL8852BEScanResults)
    },
    { // kMethodConnect
        (IOExternalMethodAction)&RTL8852BEUserClient::sConnect,
        0, sizeof(RTL8852BEConnectParams),
        0, 0
    },
    { // kMethodDisconnect
        (IOExternalMethodAction)&RTL8852BEUserClient::sDisconnect,
        0, 0,
        0, 0
    },
    { // kMethodGetStatus
        (IOExternalMethodAction)&RTL8852BEUserClient::sGetStatus,
        0, 0,
        0, sizeof(RTL8852BEStatus)
    },
    { // kMethodSetAntenna
        (IOExternalMethodAction)&RTL8852BEUserClient::sSetAntenna,
        1, 0, // 1 scalar input (antenna: 0=Auto/2x2, 1=Main, 2=Aux)
        0, 0
    },
    { // kMethodReadReg
        (IOExternalMethodAction)&RTL8852BEUserClient::sReadReg,
        2, 0, // addr, width
        1, 0  // val
    },
    { // kMethodWriteReg
        (IOExternalMethodAction)&RTL8852BEUserClient::sWriteReg,
        3, 0, // addr, width, val
        0, 0
    },
    { // kMethodReadRF
        (IOExternalMethodAction)&RTL8852BEUserClient::sReadRF,
        2, 0, // path, addr
        1, 0  // val
    },
    { // kMethodWriteRF
        (IOExternalMethodAction)&RTL8852BEUserClient::sWriteRF,
        3, 0, // path, addr, val
        0, 0
    },
    { // kMethodGetDiag
        (IOExternalMethodAction)&RTL8852BEUserClient::sGetDiag,
        0, 0,
        0, sizeof(RTL8852BEDiag)
    },
    { // kMethodIQCalibrate
        (IOExternalMethodAction)&RTL8852BEUserClient::sIQCalibrate,
        0, 0,
        1, 0
    }
};

bool RTL8852BEUserClient::initWithTask(task_t owningTask, void *securityToken, UInt32 type) {
    if (!super::initWithTask(owningTask, securityToken, type)) return false;
    fTask = owningTask;
    fOwner = NULL;
    fIsAdmin = (clientHasPrivilege(securityToken, kIOClientPrivilegeAdministrator) == kIOReturnSuccess);
    return true;
}

bool RTL8852BEUserClient::start(IOService *provider) {
    if (!super::start(provider)) return false;
    fOwner = OSDynamicCast(RTL8852BE, provider);
    return (fOwner != NULL);
}

void RTL8852BEUserClient::stop(IOService *provider) {
    fOwner = NULL;
    super::stop(provider);
}

IOReturn RTL8852BEUserClient::clientClose() {
    terminate();
    return kIOReturnSuccess;
}

IOReturn RTL8852BEUserClient::externalMethod(uint32_t selector,
                                            IOExternalMethodArguments *arguments,
                                            IOExternalMethodDispatch *dispatch,
                                            OSObject *target,
                                            void *reference) {
    if (!fOwner || isInactive()) return kIOReturnNotAttached;
    if (selector >= kNumberOfMethods) return kIOReturnBadArgument;
    dispatch = (IOExternalMethodDispatch *)&sMethods[selector];
    target = this;
    return super::externalMethod(selector, arguments, dispatch, target, reference);
}

IOReturn RTL8852BEUserClient::sScan(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner) return kIOReturnNotAttached;
    return me->fOwner->triggerScan();
}

IOReturn RTL8852BEUserClient::sGetScanResults(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner || !args->structureOutput) return kIOReturnNotAttached;
    if (args->structureOutputSize < sizeof(RTL8852BEScanResults)) return kIOReturnBadArgument;
    me->fOwner->getScanResults((RTL8852BEScanResults *)args->structureOutput);
    args->structureOutputSize = sizeof(RTL8852BEScanResults);
    return kIOReturnSuccess;
}

IOReturn RTL8852BEUserClient::sConnect(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner || !args->structureInput) return kIOReturnNotAttached;
    if (args->structureInputSize < sizeof(RTL8852BEConnectParams)) return kIOReturnBadArgument;
    const RTL8852BEConnectParams *params = (const RTL8852BEConnectParams *)args->structureInput;
    uint8_t hasBssid = 0;
    for (unsigned i = 0; i < 6; ++i) hasBssid |= params->bssid[i];
    return me->fOwner->connectToNetwork(params->ssid, params->passphrase, hasBssid ? params->bssid : nullptr);
}

IOReturn RTL8852BEUserClient::sDisconnect(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner) return kIOReturnNotAttached;
    return me->fOwner->disconnectNetwork();
}

IOReturn RTL8852BEUserClient::sGetStatus(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner || !args->structureOutput) return kIOReturnNotAttached;
    if (args->structureOutputSize < sizeof(RTL8852BEStatus)) return kIOReturnBadArgument;
    me->fOwner->getStatus((RTL8852BEStatus *)args->structureOutput);
    args->structureOutputSize = sizeof(RTL8852BEStatus);
    return kIOReturnSuccess;
}

IOReturn RTL8852BEUserClient::sSetAntenna(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner) return kIOReturnNotAttached;
    if (args->scalarInput[0] > 2) return kIOReturnBadArgument;
    uint8_t ant = (uint8_t)args->scalarInput[0];
    return me->fOwner->setAntennaMode(ant);
}

IOReturn RTL8852BEUserClient::sReadReg(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner) return kIOReturnNotAttached;
    if (args->scalarInput[0] > UINT32_MAX || args->scalarInput[1] > UINT32_MAX) return kIOReturnBadArgument;
    uint32_t addr = (uint32_t)args->scalarInput[0];
    uint32_t width = (uint32_t)args->scalarInput[1];
    uint32_t val = 0;
    IOReturn ret = me->fOwner->diagReadReg(addr, width, &val);
    if (ret == kIOReturnSuccess) {
        args->scalarOutput[0] = val;
        args->scalarOutputCount = 1;
    }
    return ret;
}

IOReturn RTL8852BEUserClient::sWriteReg(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner) return kIOReturnNotAttached;
    if (!me->fIsAdmin) return kIOReturnNotPrivileged;
    if (args->scalarInput[0] > UINT32_MAX || args->scalarInput[1] > UINT32_MAX) return kIOReturnBadArgument;
    uint32_t addr = (uint32_t)args->scalarInput[0];
    uint32_t width = (uint32_t)args->scalarInput[1];
    if (args->scalarInput[2] > UINT32_MAX) return kIOReturnBadArgument;
    uint32_t val = (uint32_t)args->scalarInput[2];
    return me->fOwner->diagWriteReg(addr, width, val);
}

IOReturn RTL8852BEUserClient::sReadRF(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner) return kIOReturnNotAttached;
    if (args->scalarInput[0] > 1 || args->scalarInput[1] > 0xFF) return kIOReturnBadArgument;
    uint8_t path = (uint8_t)args->scalarInput[0];
    uint32_t addr = (uint32_t)args->scalarInput[1];
    uint32_t val = 0;
    IOReturn ret = me->fOwner->diagReadRF(path, addr, &val);
    if (ret == kIOReturnSuccess) {
        args->scalarOutput[0] = val;
        args->scalarOutputCount = 1;
    }
    return ret;
}

IOReturn RTL8852BEUserClient::sWriteRF(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner) return kIOReturnNotAttached;
    if (!me->fIsAdmin) return kIOReturnNotPrivileged;
    if (args->scalarInput[0] > 1 || args->scalarInput[1] > 0xFF) return kIOReturnBadArgument;
    uint8_t path = (uint8_t)args->scalarInput[0];
    uint32_t addr = (uint32_t)args->scalarInput[1];
    if (args->scalarInput[2] > UINT32_MAX) return kIOReturnBadArgument;
    uint32_t val = (uint32_t)args->scalarInput[2];
    return me->fOwner->diagWriteRF(path, addr, val);
}

IOReturn RTL8852BEUserClient::sGetDiag(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner || !args->structureOutput) return kIOReturnNotAttached;
    if (args->structureOutputSize < sizeof(RTL8852BEDiag)) return kIOReturnBadArgument;
    me->fOwner->getDiag((RTL8852BEDiag *)args->structureOutput);
    args->structureOutputSize = sizeof(RTL8852BEDiag);
    return kIOReturnSuccess;
}

IOReturn RTL8852BEUserClient::sIQCalibrate(OSObject *target, void *reference, IOExternalMethodArguments *args) {
    RTL8852BEUserClient *me = OSDynamicCast(RTL8852BEUserClient, target);
    if (!me || !me->fOwner) return kIOReturnNotAttached;
    if (!me->fIsAdmin) return kIOReturnNotPrivileged;
    uint32_t res = 0;
    IOReturn ret = me->fOwner->triggerIQK(&res);
    if (ret == kIOReturnSuccess) {
        args->scalarOutput[0] = res;
        args->scalarOutputCount = 1;
    }
    return ret;
}
