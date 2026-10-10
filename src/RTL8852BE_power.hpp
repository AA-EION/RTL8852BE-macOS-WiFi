// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Issen Software Group
#pragma once
#include "RTL8852BE_register_io.hpp"
bool rtl8852bPowerOn(const RtlRegisterIO &io, uint8_t cut, uint8_t rfe,
                    bool efuseValid = false, bool powerCalibrationValid = false);
bool rtl8852bPowerOff(const RtlRegisterIO &io, uint8_t cut, uint8_t rfe);
bool rtl8852bReadAnalogCut(const RtlRegisterIO &io, uint8_t *cut);
