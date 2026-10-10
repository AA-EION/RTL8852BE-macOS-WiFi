// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Issen Software Group
// Based on Linux v6.12 rtw89/efuse.c and rtw8852b_common.h.
#pragma once
#include "RTL8852BE_hw.hpp"
#include "RTL8852BE_register_io.hpp"

struct RTL8852BECalibData {
    uint8_t mac_addr[6];
    uint8_t xtal_cap;
    uint8_t rfe_type;
    uint8_t country_code[2];
};

bool rtl8852bReadPhysicalEfuse(const RtlRegisterIO &io, uint8_t cut,
                             uint8_t *physical, size_t capacity);
bool rtl8852bExpandEfuse(const uint8_t *physical, size_t size,
                        uint8_t *logical, size_t capacity);
bool rtl8852bParseCalibration(const uint8_t *logical, size_t size,
                            RTL8852BECalibData *calibration);
