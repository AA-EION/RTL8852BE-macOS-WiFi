// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Issen Software Group
#pragma once
#include <stdint.h>

// The same chip routines run through IOKit MMIO and the portable regression bus.
// Callbacks report failed accesses explicitly; an absent BAR is not register data.
struct RtlRegisterIO {
    void *context;
    bool (*read)(void *, uint32_t address, uint32_t width, uint32_t *value);
    bool (*write)(void *, uint32_t address, uint32_t width, uint32_t value);
    void (*delay)(void *, uint32_t microseconds);
};

inline bool rtlUpdateRegister(const RtlRegisterIO &io, uint32_t address,
                              uint32_t width, uint32_t mask, uint32_t value) {
    uint32_t current;
    return io.read(io.context, address, width, &current) &&
           io.write(io.context, address, width, (current & ~mask) | (value & mask));
}
