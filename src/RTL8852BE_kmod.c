// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

#include <mach/mach_types.h>
#include <libkern/libkern.h>

extern kern_return_t _start(kmod_info_t *ki, void *data);
extern kern_return_t _stop(kmod_info_t *ki, void *data);

__private_extern__ kern_return_t RTL8852BE_start(kmod_info_t *ki, void *data) {
    return KERN_SUCCESS;
}

__private_extern__ kern_return_t RTL8852BE_stop(kmod_info_t *ki, void *data) {
    return KERN_SUCCESS;
}

KMOD_EXPLICIT_DECL(com.issen.driver.RTL8852BEWiFi, "1.0.0", _start, _stop)
__private_extern__ kmod_start_func_t *_realmain = RTL8852BE_start;
__private_extern__ kmod_stop_func_t *_antimain = RTL8852BE_stop;
__private_extern__ int _kext_apple_cc = __APPLE_CC__;
