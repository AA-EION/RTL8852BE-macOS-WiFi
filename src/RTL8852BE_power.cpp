// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2020-2022 Realtek Corporation
// Copyright (c) 2026 Issen Software Group
// Generated from Linux v6.12 rtw8852b.c/mac.c by tools/generate_power.py.

#include "RTL8852BE_power.hpp"
#include "RTL8852BE_linux_regs.hpp"
namespace {
using u8 = uint8_t;
using u32 = uint32_t;
struct rtw89_efuse { u8 rfe_type; bool valid; bool power_k_valid; };
struct rtw89_dev {
    const RtlRegisterIO &io;
    rtw89_efuse efuse;
    struct { u8 cv; } hal;
    bool failed = false;
};
static uint32_t readRegister(rtw89_dev *dev, uint32_t address, uint32_t width) {
    uint32_t value = UINT32_MAX;
    if (dev->failed || !dev->io.read(dev->io.context, address, width, &value)) dev->failed = true;
    return value;
}
static u32 rtw89_read32(rtw89_dev *dev, u32 address) { return readRegister(dev, address, 4); }
static u8 rtw89_read8(rtw89_dev *dev, u32 address) { return u8(readRegister(dev, address, 1)); }
static void writeRegister(rtw89_dev *dev, u32 address, u32 width, u32 value) {
    if (!dev->failed && !dev->io.write(dev->io.context, address, width, value)) dev->failed = true;
}
static void rtw89_write32(rtw89_dev *dev, u32 address, u32 value) { writeRegister(dev, address, 4, value); }
static void rtw89_write16(rtw89_dev *dev, u32 address, u32 value) { writeRegister(dev, address, 2, value); }
static u32 fieldPrep(u32 mask, u32 value) {
    u32 shift = 0;
    while (!(mask & (1U << shift))) ++shift;
    return (value << shift) & mask;
}
static void update(rtw89_dev *dev, u32 address, u32 width, u32 mask, u32 value) {
    const u32 current = readRegister(dev, address, width);
    writeRegister(dev, address, width, (current & ~mask) | (value & mask));
}
static void rtw89_write32_set(rtw89_dev *d, u32 a, u32 m) { update(d, a, 4, m, m); }
static void rtw89_write32_clr(rtw89_dev *d, u32 a, u32 m) { update(d, a, 4, m, 0); }
static void rtw89_write8_set(rtw89_dev *d, u32 a, u32 m) { update(d, a, 1, m, m); }
static void rtw89_write8_clr(rtw89_dev *d, u32 a, u32 m) { update(d, a, 1, m, 0); }
static void rtw89_write32_mask(rtw89_dev *d, u32 a, u32 m, u32 v) { update(d, a, 4, m, fieldPrep(m, v)); }
static void rtw89_write16_mask(rtw89_dev *d, u32 a, u32 m, u32 v) { update(d, a, 2, m, fieldPrep(m, v)); }
static void rtlDelay(rtw89_dev *d, u32 us) { if (!d->failed) d->io.delay(d->io.context, us); }
#define FIELD_PREP(mask, val) fieldPrep(mask, val)
#define rtw89_warn(...) ((void)0)
#define read_poll_timeout(read, val, condition, interval, timeout, initial, dev, reg) \
    ([&]() -> int { \
        if (initial) rtlDelay(dev, interval); \
        for (u32 elapsed = 0; elapsed <= timeout; elapsed += interval) { \
            val = read(dev, reg); \
            if (dev->failed || val == UINT32_MAX) return -1; \
            if (condition) return 0; \
            if (elapsed == timeout) break; \
            rtlDelay(dev, interval); \
        } \
        return -1; \
    }())

int rtw89_mac_write_xtal_si(struct rtw89_dev *rtwdev, u8 offset, u8 val, u8 mask)
{
	u32 val32;
	int ret;

	val32 = FIELD_PREP(B_AX_WL_XTAL_SI_ADDR_MASK, offset) |
		FIELD_PREP(B_AX_WL_XTAL_SI_DATA_MASK, val) |
		FIELD_PREP(B_AX_WL_XTAL_SI_BITMASK_MASK, mask) |
		FIELD_PREP(B_AX_WL_XTAL_SI_MODE_MASK, XTAL_SI_NORMAL_WRITE) |
		FIELD_PREP(B_AX_WL_XTAL_SI_CMD_POLL, 1);
	rtw89_write32(rtwdev, R_AX_WLAN_XTAL_SI_CTRL, val32);

	ret = read_poll_timeout(rtw89_read32, val32, !(val32 & B_AX_WL_XTAL_SI_CMD_POLL),
				50, 50000, false, rtwdev, R_AX_WLAN_XTAL_SI_CTRL);
	if (ret) {
		rtw89_warn(rtwdev, "xtal si not ready(W): offset=%x val=%x mask=%x\n",
			   offset, val, mask);
		return ret;
	}

	return 0;
}

int rtw89_mac_read_xtal_si(struct rtw89_dev *rtwdev, u8 offset, u8 *val)
{
	u32 val32;
	int ret;

	val32 = FIELD_PREP(B_AX_WL_XTAL_SI_ADDR_MASK, offset) |
		FIELD_PREP(B_AX_WL_XTAL_SI_DATA_MASK, 0x00) |
		FIELD_PREP(B_AX_WL_XTAL_SI_BITMASK_MASK, 0x00) |
		FIELD_PREP(B_AX_WL_XTAL_SI_MODE_MASK, XTAL_SI_NORMAL_READ) |
		FIELD_PREP(B_AX_WL_XTAL_SI_CMD_POLL, 1);
	rtw89_write32(rtwdev, R_AX_WLAN_XTAL_SI_CTRL, val32);

	ret = read_poll_timeout(rtw89_read32, val32, !(val32 & B_AX_WL_XTAL_SI_CMD_POLL),
				50, 50000, false, rtwdev, R_AX_WLAN_XTAL_SI_CTRL);
	if (ret) {
		rtw89_warn(rtwdev, "xtal si not ready(R): offset=%x\n", offset);
		return ret;
	}

	*val = rtw89_read8(rtwdev, R_AX_WLAN_XTAL_SI_CTRL + 1);

	return 0;
}

static void rtw8852b_pwr_sps_ana(struct rtw89_dev *rtwdev)
{
	struct rtw89_efuse *efuse = &rtwdev->efuse;

	if (efuse->rfe_type == 0x5)
		rtw89_write16(rtwdev, R_AX_SPS_ANA_ON_CTRL2, RTL8852B_RFE_05_SPS_ANA);
}

static int rtw8852b_pwr_on_func(struct rtw89_dev *rtwdev)
{
	u32 val32;
	u32 ret;

	rtw8852b_pwr_sps_ana(rtwdev);

	rtw89_write32_clr(rtwdev, R_AX_SYS_PW_CTRL, B_AX_AFSM_WLSUS_EN |
						    B_AX_AFSM_PCIE_SUS_EN);
	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_DIS_WLBT_PDNSUSEN_SOPC);
	rtw89_write32_set(rtwdev, R_AX_WLLPS_CTRL, B_AX_DIS_WLBT_LPSEN_LOPC);
	rtw89_write32_clr(rtwdev, R_AX_SYS_PW_CTRL, B_AX_APDM_HPDN);
	rtw89_write32_clr(rtwdev, R_AX_SYS_PW_CTRL, B_AX_APFM_SWLPS);

	ret = read_poll_timeout(rtw89_read32, val32, val32 & B_AX_RDY_SYSPWR,
				1000, 20000, false, rtwdev, R_AX_SYS_PW_CTRL);
	if (ret)
		return ret;

	rtw89_write32_set(rtwdev, R_AX_AFE_LDO_CTRL, B_AX_AON_OFF_PC_EN);
	ret = read_poll_timeout(rtw89_read32, val32, val32 & B_AX_AON_OFF_PC_EN,
				1000, 20000, false, rtwdev, R_AX_AFE_LDO_CTRL);
	if (ret)
		return ret;

	rtw89_write32_mask(rtwdev, R_AX_SPS_DIG_OFF_CTRL0, B_AX_C1_L1_MASK, 0x1);
	rtw89_write32_mask(rtwdev, R_AX_SPS_DIG_OFF_CTRL0, B_AX_C3_L1_MASK, 0x3);
	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_EN_WLON);
	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_APFN_ONMAC);

	ret = read_poll_timeout(rtw89_read32, val32, !(val32 & B_AX_APFN_ONMAC),
				1000, 20000, false, rtwdev, R_AX_SYS_PW_CTRL);
	if (ret)
		return ret;

	rtw89_write8_set(rtwdev, R_AX_PLATFORM_ENABLE, B_AX_PLATFORM_EN);
	rtw89_write8_clr(rtwdev, R_AX_PLATFORM_ENABLE, B_AX_PLATFORM_EN);
	rtw89_write8_set(rtwdev, R_AX_PLATFORM_ENABLE, B_AX_PLATFORM_EN);
	rtw89_write8_clr(rtwdev, R_AX_PLATFORM_ENABLE, B_AX_PLATFORM_EN);

	rtw89_write8_set(rtwdev, R_AX_PLATFORM_ENABLE, B_AX_PLATFORM_EN);
	rtw89_write32_clr(rtwdev, R_AX_SYS_SDIO_CTRL, B_AX_PCIE_CALIB_EN_V1);

	rtw89_write32_set(rtwdev, R_AX_SYS_ADIE_PAD_PWR_CTRL, B_AX_SYM_PADPDN_WL_PTA_1P3);

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL,
				      XTAL_SI_GND_SHDN_WL, XTAL_SI_GND_SHDN_WL);
	if (ret)
		return ret;

	rtw89_write32_set(rtwdev, R_AX_SYS_ADIE_PAD_PWR_CTRL, B_AX_SYM_PADPDN_WL_RFC_1P3);

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL,
				      XTAL_SI_SHDN_WL, XTAL_SI_SHDN_WL);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_OFF_WEI,
				      XTAL_SI_OFF_WEI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_OFF_EI,
				      XTAL_SI_OFF_EI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_RFC2RF);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_PON_WEI,
				      XTAL_SI_PON_WEI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_PON_EI,
				      XTAL_SI_PON_EI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_SRAM2RFC);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_SRAM_CTRL, 0, XTAL_SI_SRAM_DIS);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_XTAL_XMD_2, 0, XTAL_SI_LDO_LPS);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_XTAL_XMD_4, 0, XTAL_SI_LPS_CAP);
	if (ret)
		return ret;

	rtw89_write32_set(rtwdev, R_AX_PMC_DBG_CTRL2, B_AX_SYSON_DIS_PMCR_AX_WRMSK);
	rtw89_write32_set(rtwdev, R_AX_SYS_ISO_CTRL, B_AX_ISO_EB2CORE);
	rtw89_write32_clr(rtwdev, R_AX_SYS_ISO_CTRL, B_AX_PWC_EV2EF_B15);

	rtlDelay(rtwdev, 1000);

	rtw89_write32_clr(rtwdev, R_AX_SYS_ISO_CTRL, B_AX_PWC_EV2EF_B14);
	rtw89_write32_clr(rtwdev, R_AX_PMC_DBG_CTRL2, B_AX_SYSON_DIS_PMCR_AX_WRMSK);

	if (!rtwdev->efuse.valid || rtwdev->efuse.power_k_valid)
		goto func_en;

	rtw89_write32_mask(rtwdev, R_AX_SPS_DIG_ON_CTRL0, B_AX_VOL_L1_MASK, 0x9);
	rtw89_write32_mask(rtwdev, R_AX_SPS_DIG_ON_CTRL0, B_AX_VREFPFM_L_MASK, 0xA);

	if (rtwdev->hal.cv == CHIP_CBV) {
		rtw89_write32_set(rtwdev, R_AX_PMC_DBG_CTRL2, B_AX_SYSON_DIS_PMCR_AX_WRMSK);
		rtw89_write16_mask(rtwdev, R_AX_HCI_LDO_CTRL, B_AX_R_AX_VADJ_MASK, 0xA);
		rtw89_write32_clr(rtwdev, R_AX_PMC_DBG_CTRL2, B_AX_SYSON_DIS_PMCR_AX_WRMSK);
	}

func_en:
	rtw89_write32_set(rtwdev, R_AX_DMAC_FUNC_EN,
			  B_AX_MAC_FUNC_EN | B_AX_DMAC_FUNC_EN | B_AX_MPDU_PROC_EN |
			  B_AX_WD_RLS_EN | B_AX_DLE_WDE_EN | B_AX_TXPKT_CTRL_EN |
			  B_AX_STA_SCH_EN | B_AX_DLE_PLE_EN | B_AX_PKT_BUF_EN |
			  B_AX_DMAC_TBL_EN | B_AX_PKT_IN_EN | B_AX_DLE_CPUIO_EN |
			  B_AX_DISPATCHER_EN | B_AX_BBRPT_EN | B_AX_MAC_SEC_EN |
			  B_AX_DMACREG_GCKEN);
	rtw89_write32_set(rtwdev, R_AX_CMAC_FUNC_EN,
			  B_AX_CMAC_EN | B_AX_CMAC_TXEN | B_AX_CMAC_RXEN |
			  B_AX_FORCE_CMACREG_GCKEN | B_AX_PHYINTF_EN | B_AX_CMAC_DMA_EN |
			  B_AX_PTCLTOP_EN | B_AX_SCHEDULER_EN | B_AX_TMAC_EN |
			  B_AX_RMAC_EN);

	rtw89_write32_mask(rtwdev, R_AX_EECS_EESK_FUNC_SEL, B_AX_PINMUX_EESK_FUNC_SEL_MASK,
			   PINMUX_EESK_FUNC_SEL_BT_LOG);

	return 0;
}

static int rtw8852b_pwr_off_func(struct rtw89_dev *rtwdev)
{
	u32 val32;
	u32 ret;

	rtw8852b_pwr_sps_ana(rtwdev);

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_RFC2RF,
				      XTAL_SI_RFC2RF);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_OFF_EI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_OFF_WEI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_WL_RFC_S0, 0, XTAL_SI_RF00);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_WL_RFC_S1, 0, XTAL_SI_RF10);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, XTAL_SI_SRAM2RFC,
				      XTAL_SI_SRAM2RFC);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_PON_EI);
	if (ret)
		return ret;
	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_PON_WEI);
	if (ret)
		return ret;

	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_EN_WLON);
	rtw89_write32_clr(rtwdev, R_AX_WLRF_CTRL, B_AX_AFC_AFEDIG);
	rtw89_write8_clr(rtwdev, R_AX_SYS_FUNC_EN, B_AX_FEN_BB_GLB_RSTN | B_AX_FEN_BBRSTB);
	rtw89_write32_clr(rtwdev, R_AX_SYS_ADIE_PAD_PWR_CTRL, B_AX_SYM_PADPDN_WL_RFC_1P3);

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_SHDN_WL);
	if (ret)
		return ret;

	rtw89_write32_clr(rtwdev, R_AX_SYS_ADIE_PAD_PWR_CTRL, B_AX_SYM_PADPDN_WL_PTA_1P3);

	ret = rtw89_mac_write_xtal_si(rtwdev, XTAL_SI_ANAPAR_WL, 0, XTAL_SI_GND_SHDN_WL);
	if (ret)
		return ret;

	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_APFM_OFFMAC);

	ret = read_poll_timeout(rtw89_read32, val32, !(val32 & B_AX_APFM_OFFMAC),
				1000, 20000, false, rtwdev, R_AX_SYS_PW_CTRL);
	if (ret)
		return ret;

	rtw89_write32(rtwdev, R_AX_WLLPS_CTRL, SW_LPS_OPTION);
	rtw89_write32_set(rtwdev, R_AX_SYS_SWR_CTRL1, B_AX_SYM_CTRL_SPS_PWMFREQ);
	rtw89_write32_mask(rtwdev, R_AX_SPS_DIG_ON_CTRL0, B_AX_REG_ZCDC_H_MASK, 0x3);
	rtw89_write32_set(rtwdev, R_AX_SYS_PW_CTRL, B_AX_APFM_SWLPS);

	return 0;
}

} // namespace
bool rtl8852bPowerOn(const RtlRegisterIO &io, uint8_t cut, uint8_t rfe,
                    bool efuseValid, bool powerCalibrationValid) {
    if (!io.read || !io.write || !io.delay) return false;
    rtw89_dev device{io, {rfe, efuseValid, powerCalibrationValid}, {cut}};
    return rtw8852b_pwr_on_func(&device) == 0 && !device.failed;
}
bool rtl8852bPowerOff(const RtlRegisterIO &io, uint8_t cut, uint8_t rfe) {
    if (!io.read || !io.write || !io.delay) return false;
    rtw89_dev device{io, {rfe, false, false}, {cut}};
    return rtw8852b_pwr_off_func(&device) == 0 && !device.failed;
}
bool rtl8852bReadAnalogCut(const RtlRegisterIO &io, uint8_t *cut) {
    if (!cut || !io.read || !io.write || !io.delay) return false;
    *cut = 0;
    rtw89_dev device{io, {0, false, false}, {0}};
    u8 value = 0;
    if (rtw89_mac_read_xtal_si(&device, XTAL_SI_CV, &value) != 0 || device.failed) return false;
    *cut = value & XTAL_SI_ACV_MASK;
    return true;
}
