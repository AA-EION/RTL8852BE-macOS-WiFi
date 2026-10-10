// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

#ifndef RTL8852BE_TABLES_HPP
#define RTL8852BE_TABLES_HPP

#include <stdint.h>
#include <stddef.h>

struct Rtw89Reg2Def {
    uint32_t addr;
    uint32_t val;
};

extern const Rtw89Reg2Def RTL8852BE_PHY_BB_REGS[1013];
extern const size_t RTL8852BE_PHY_BB_REGS_COUNT;

extern const Rtw89Reg2Def RTL8852BE_PHY_BB_REG_GAIN[66];
extern const size_t RTL8852BE_PHY_BB_REG_GAIN_COUNT;

extern const Rtw89Reg2Def RTL8852BE_PHY_RADIOA_REGS[8295];
extern const size_t RTL8852BE_PHY_RADIOA_REGS_COUNT;

extern const Rtw89Reg2Def RTL8852BE_PHY_RADIOB_REGS[8392];
extern const size_t RTL8852BE_PHY_RADIOB_REGS_COUNT;

extern const Rtw89Reg2Def RTL8852BE_PHY_NCTL_REGS[1320];
extern const size_t RTL8852BE_PHY_NCTL_REGS_COUNT;

#endif // RTL8852BE_TABLES_HPP
