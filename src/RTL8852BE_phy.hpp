// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Issen Software Group
// Linux v6.12 rtw89/phy.c table selection/branch semantics.
#pragma once
#include "RTL8852BE_tables.hpp"

using RtlPhyWrite = bool (*)(void *, uint32_t address, uint32_t data);
// Validates flow control before writes, selects one headline, propagates errors.
bool rtl8852bApplyPhyTable(const Rtw89Reg2Def *registers, size_t count,
                         uint8_t rfe, uint8_t cut, RtlPhyWrite write, void *context);
