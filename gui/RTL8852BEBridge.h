// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

#ifndef _RTL8852BE_BRIDGE_H_
#define _RTL8852BE_BRIDGE_H_

#include <stdint.h>
#include <IOKit/IOKitLib.h>
#include "../src/RTL8852BE_ipc.h"

static inline int rtl_open_client(io_connect_t *outConn) {
    CFMutableDictionaryRef matchDict = IOServiceMatching("RTL8852BE");
    if (!matchDict) return -1;
    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, matchDict);
    if (service == IO_OBJECT_NULL) return -2;
    io_connect_t conn = IO_OBJECT_NULL;
    kern_return_t kr = IOServiceOpen(service, mach_task_self(), 0, &conn);
    IOObjectRelease(service);
    if (kr != KERN_SUCCESS || conn == IO_OBJECT_NULL) return -3;
    *outConn = conn;
    return 0;
}

static inline void rtl_close_client(io_connect_t conn) {
    if (conn != IO_OBJECT_NULL) {
        IOServiceClose(conn);
    }
}

static inline int rtl_get_status(io_connect_t conn, struct RTL8852BEStatus *outStatus) {
    size_t outSize = sizeof(struct RTL8852BEStatus);
    kern_return_t kr = IOConnectCallStructMethod(conn, kMethodGetStatus, NULL, 0, outStatus, &outSize);
    return (kr == KERN_SUCCESS && outSize == sizeof(*outStatus)) ? 0 : (kr == KERN_SUCCESS ? kIOReturnBadArgument : (int)kr);
}

static inline int rtl_trigger_scan(io_connect_t conn) {
    kern_return_t kr = IOConnectCallMethod(conn, kMethodScan, NULL, 0, NULL, 0, NULL, NULL, NULL, NULL);
    return (kr == KERN_SUCCESS) ? 0 : (int)kr;
}

static inline int rtl_get_scan_results(io_connect_t conn, struct RTL8852BEScanResults *outResults) {
    size_t outSize = sizeof(struct RTL8852BEScanResults);
    kern_return_t kr = IOConnectCallStructMethod(conn, kMethodGetScanResults, NULL, 0, outResults, &outSize);
    return (kr == KERN_SUCCESS && outSize == sizeof(*outResults)) ? 0 : (kr == KERN_SUCCESS ? kIOReturnBadArgument : (int)kr);
}

static inline int rtl_connect_ap(io_connect_t conn, const struct RTL8852BEConnectParams *params) {
    kern_return_t kr = IOConnectCallStructMethod(conn, kMethodConnect, params, sizeof(struct RTL8852BEConnectParams), NULL, NULL);
    return (kr == KERN_SUCCESS) ? 0 : (int)kr;
}

static inline int rtl_disconnect_ap(io_connect_t conn) {
    kern_return_t kr = IOConnectCallMethod(conn, kMethodDisconnect, NULL, 0, NULL, 0, NULL, NULL, NULL, NULL);
    return (kr == KERN_SUCCESS) ? 0 : (int)kr;
}

static inline int rtl_set_antenna(io_connect_t conn, uint8_t ant) {
    uint64_t scalar = (uint64_t)ant;
    kern_return_t kr = IOConnectCallScalarMethod(conn, kMethodSetAntenna, &scalar, 1, NULL, NULL);
    return (kr == KERN_SUCCESS) ? 0 : (int)kr;
}

static inline int rtl_get_diag(io_connect_t conn, struct RTL8852BEDiag *outDiag) {
    size_t outSize = sizeof(struct RTL8852BEDiag);
    kern_return_t kr = IOConnectCallStructMethod(conn, kMethodGetDiag, NULL, 0, outDiag, &outSize);
    return (kr == KERN_SUCCESS && outSize == sizeof(*outDiag)) ? 0 : (kr == KERN_SUCCESS ? kIOReturnBadArgument : (int)kr);
}

/* Copies the BSD name (e.g. "en2") of the driver's network interface. */
static inline int rtl_copy_bsd_name(char *out, size_t outLen) {
    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("RTL8852BE"));
    if (service == IO_OBJECT_NULL) return -1;
    CFTypeRef name = IORegistryEntrySearchCFProperty(service, kIOServicePlane, CFSTR("BSD Name"),
                                                     kCFAllocatorDefault, kIORegistryIterateRecursively);
    IOObjectRelease(service);
    if (!name) return -2;
    Boolean ok = CFGetTypeID(name) == CFStringGetTypeID() &&
                 CFStringGetCString((CFStringRef)name, out, (CFIndex)outLen, kCFStringEncodingUTF8);
    CFRelease(name);
    return ok ? 0 : -3;
}

/* 1 if the RTL8852BE PCI function is present in the IORegistry. */
static inline int rtl_hardware_present(void) {
    /* Same match as the kext personality; ACPI often renames the device (e.g. PXSX). */
    CFMutableDictionaryRef match = IOServiceMatching("IOPCIDevice");
    if (!match) return 0;
    CFDictionarySetValue(match, CFSTR("IOPCIMatch"), CFSTR("0xb85210ec 0xb85b10ec"));
    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, match);
    if (service == IO_OBJECT_NULL) return 0;
    IOObjectRelease(service);
    return 1;
}

#endif // _RTL8852BE_BRIDGE_H_
