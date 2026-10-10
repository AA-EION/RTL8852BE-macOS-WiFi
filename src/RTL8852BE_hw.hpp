// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Issen Software Group
// Copyright (c) 2020-2022 Realtek Corporation

#ifndef RTL8852BE_HW_HPP
#define RTL8852BE_HW_HPP

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "RTL8852BE_ipc.h"

// Realtek PCI IDs
#define RTL_PCI_VENDOR_ID                0x10EC
#define RTL8852BE_PCI_DEVICE_ID          0xB852
#define RTL8852BE_PCI_DEVICE_ID_ALT      0xB85B

// Lenovo LOQ 15IRH8 (Type 82XV) Subsystem Identifiers
#define LENOVO_PCI_SUBVENDOR_ID          0x17AA
#define LENOVO_LOQ_82XV_SUBSYS_ID        0x4853

// PCIe Configuration Space Offsets
#define PCI_CFG_COMMAND                  0x04
#define PCI_CFG_BAR2                     0x18
#define PCI_EXP_LNKCTL_OFFSET            0x10 // Relative offset within IOPCIExpressCapability
#define PCI_COMMAND_BUS_MASTER           (1 << 2)
#define PCI_COMMAND_MEM_ENABLE           (1 << 1)

// Minimum mapped span covering MAC and the AX PHY register aperture.
// Linux phy.c: rtw89_phy_gen_ax.cr_base; accesses use the actual BAR length.
#define RTL8852BE_PHY_CR_BASE            0x10000
#define RTL8852BE_BAR2_SIZE              0x20000

// Keep start() inert until the rtw89 bring-up blockers in docs/AUDIT.md are fixed.
// A boot argument alone must not enable the incomplete firmware/DMA paths.
// RTL8852BE_HARDWARE_BRINGUP_READY is shared in RTL8852BE_ipc.h.

// Hardware Generation and Chip Constants
#define RTL_CHIP_AX                      0x01
#define RTL_TX_RING_SIZE                 128
#define RTL_RX_RING_SIZE                 128
#define RTL_RX_BUF_SIZE                  2048
#define RTL_TX_MAX_BUF_SIZE              2048

// PCIe Interface Registers (rtw89 pci.h / reg.h)
#define R_AX_PCIE_INIT_CFG1              0x1000
#define B_AX_TXHCI_EN                    (1U << 11)
#define B_AX_RXHCI_EN                    (1U << 13)
#define B_AX_RXBD_MODE                   (1U << 18)
#define B_AX_LATENCY_CONTROL             (1U << 12)

#define R_AX_PCIE_DMA_STOP1              0x1010
#define B_AX_STOP_PCIEIO                 (1U << 20)
#define B_AX_TX_STOP1_MASK_V1            0x00070F00

#define R_AX_TXBD_RWPTR_CLR1             0x1014
#define R_AX_RXBD_RWPTR_CLR              0x1018

#define R_AX_PCIE_DMA_BUSY1              0x101C
#define B_AX_DMA_BUSY1_CHECK             0x00070F00

#define R_AX_RXQ_RXBD_NUM                0x1020
#define R_AX_ACH0_TXBD_NUM               0x1024

#define R_AX_RXQ_RXBD_IDX                0x1050
#define R_AX_ACH0_TXBD_IDX               0x1058

#define R_AX_PCIE_HIMR00                 0x10B0
#define R_AX_PCIE_HISR00                 0x10B4
#define R_AX_PCIE_HIMR10                 0x13B0
#define R_AX_HIMR0                       0x01A0
#define B_AX_RXQ_RXBD_INT                (1U << 0)
#define B_AX_ACH0_TXBD_INT               (1U << 3)
#define B_AX_MGNT_TXBD_INT               (1U << 11)

#define R_AX_INT_MIT_RX                  0x10D4

#define R_AX_TXDMA_ADDR_H                0x10F0
#define R_AX_RXDMA_ADDR_H                0x10F4

#define R_AX_RXQ_RXBD_DESA_L             0x1100
#define R_AX_RXQ_RXBD_DESA_H             0x1104

#define R_AX_ACH0_TXBD_DESA_L            0x1110
#define R_AX_ACH0_TXBD_DESA_H            0x1114

#define R_AX_PCIE_EXP_CTRL               0x13F0
#define B_AX_MAX_TAG_NUM                 0x00070000

// SWSI RF Control Registers (rtw89 reg.h / phy.c)
#define R_SWSI_DATA_V1                   0x0370
#define B_SWSI_DATA_VAL_V1_MASK          0x000FFFFF
#define B_SWSI_DATA_ADDR_V1_SHIFT        20
#define B_SWSI_DATA_PATH_V1_SHIFT        28
#define B_SWSI_READ_ADDR_PATH_V1_SHIFT   8
#define B_SWSI_DATA_BIT_MASK_EN_V1       (1U << 31)

#define R_SWSI_BIT_MASK_V1               0x0374
#define R_SWSI_READ_ADDR_V1              0x0378
#define R_SWSI_V1                        0x174C
#define B_SWSI_W_BUSY_V1                 (1U << 24)
#define B_SWSI_R_BUSY_V1                 (1U << 25)
#define B_SWSI_R_DATA_DONE_V1            (1U << 26)

// Power Sequence and System Control Registers (rtw89 reg.h)
#define R_AX_SYS_ISO_CTRL                0x0000
#define R_AX_SYS_FUNC_EN                 0x0002
#define B_AX_FEN_BBRSTB                  (1U << 0)
#define B_AX_FEN_BB_GLB_RSTN             (1U << 1)

#define R_AX_SYS_PW_CTRL                 0x0004
#define B_AX_APFN_ONMAC                  (1U << 8)
#define B_AX_APFM_OFFMAC                 (1U << 9)
#define B_AX_APFM_SWLPS                  (1U << 10)
#define B_AX_AFSM_WLSUS_EN               (1U << 11)
#define B_AX_AFSM_PCIE_SUS_EN            (1U << 12)
#define B_AX_APDM_HPDN                   (1U << 15)
#define B_AX_EN_WLON                     (1U << 16)
#define B_AX_RDY_SYSPWR                  (1U << 17)
#define B_AX_DIS_WLBT_PDNSUSEN_SOPC      (1U << 18)

#define R_AX_SYS_CLK_CTRL                0x0008
#define B_AX_CPU_CLK_EN                  (1U << 14)

#define R_AX_SYS_WL_EFUSE_CTRL           0x000A
#define B_AX_AUTOLOAD_SUS                (1U << 5)
#define R_AX_EFUSE_CTRL                  0x0030
#define B_AX_EF_RDY                      (1U << 29)
#define B_AX_EF_ADDR_MASK                 0x07FF0000
#define R_AX_EFUSE_CTRL_1_V1              0x0038
#define B_AX_EF_BURST                    (1U << 19)
#define R_AX_PMC_DBG_CTRL2               0x00CC
#define B_AX_SYSON_DIS_PMCR_AX_WRMSK     (1U << 2)
#define B_AX_PWC_EV2EF_B14               (1U << 14)
#define B_AX_PWC_EV2EF_B15               (1U << 15)
#define B_AX_ISO_EB2CORE                 (1U << 8)
#define R_AX_SYS_CFG1                   0x00F0
#define B_AX_CHIP_VER_MASK               0x0000F000

#define R_AX_WLLPS_CTRL                  0x0090
#define B_AX_DIS_WLBT_LPSEN_LOPC         (1U << 1)

#define R_AX_AFE_LDO_CTRL                0x0020
#define B_AX_AON_OFF_PC_EN               (1U << 23)

#define R_AX_PLATFORM_ENABLE             0x0088
#define B_AX_PLATFORM_EN                 (1U << 0)

// AX firmware status lives here, not at the legacy rtlwifi MCUFWDL address.
#define R_AX_WCPU_FW_CTRL                0x01E0

// RF Synth & Channel Configuration
#define RR_CFGCH                         0x18
#define RR_CFGCH_V1                      0x10018
#define RR_LCKST                         0xCF
#define RR_LCKST_BIN                     (1U << 0)

// Queue selection codes
#define Q_VO                             3
#define Q_VI                             2
#define Q_BE                             0
#define Q_BK                             1
#define Q_MGNT                           0x12
#define Q_HI                             0x11

// eFuse Structure Constants
#define EFUSE_PHYSICAL_SIZE_AX           1216
#define EFUSE_SECURITY_CTRL_SIZE_AX      4
#define EFUSE_LOGICAL_SIZE_AX            2048
#define EFUSE_MAC_ADDR_OFFSET_PCIE       0x400
#define EFUSE_XTAL_K_OFFSET              0x2B9
#define EFUSE_RFE_TYPE_OFFSET            0x2CA

#pragma pack(push, 1)

// TX Buffer Descriptor (8 bytes) - Production rtw89 PCI layout
struct Rtw89TxBd {
    uint16_t length;
    uint16_t opt;
#define RTW89_TXBD_OPT_LS                (1U << 14)
#define RTW89_TXBD_OPT_DMA_HI_MASK       0x3FC0
    uint32_t dma;

    inline void set_length(uint16_t len) { length = len; }
    inline uint16_t get_length() const { return length; }
    inline void set_dma(uint32_t addr) { dma = addr; }
    inline uint32_t get_dma() const { return dma; }
    inline void set_ls(bool is_last) {
        if (is_last) opt |= RTW89_TXBD_OPT_LS;
        else opt &= ~RTW89_TXBD_OPT_LS;
    }
    inline bool get_ls() const { return (opt & RTW89_TXBD_OPT_LS) != 0; }
};

// RX Buffer Descriptor (8 bytes) - Production rtw89 PCI layout
struct Rtw89RxBd {
    uint16_t buf_size;
    uint16_t opt;
#define RTW89_RXBD_OPT_DMA_HI_MASK       0x3FC0
    uint32_t dma;

    inline void set_buf_size(uint16_t sz) { buf_size = sz; }
    inline uint16_t get_buf_size() const { return buf_size; }
    inline void set_dma(uint32_t addr) { dma = addr; }
    inline uint32_t get_dma() const { return dma; }
};

// Incomplete TXWD prototype; not the validated rtw89 body/info/address format.
// Hardware use is blocked; see docs/AUDIT.md before implementing this path.
struct Rtw89TxWd {
    uint32_t dword0;
#define RTW89_TXWD_WP_OFFSET_SHIFT       24
#define RTW89_TXWD_MORE_DATA             (1U << 23)
#define RTW89_TXWD_WD_INFO_EN            (1U << 22)
#define RTW89_TXWD_CHANNEL_DMA_SHIFT     16
#define RTW89_TXWD_HDR_LLC_LEN_SHIFT     11
    uint32_t dword1;
#define RTW89_TXWD_MAC_ID_SHIFT          0
#define RTW89_TXWD_QSEL_SHIFT            8
#define RTW89_TXWD_PKT_LEN_SHIFT         16
    uint32_t dword2;
    uint32_t dword3;
    uint32_t dword4;
    uint32_t dword5;
    uint32_t dword6;
    uint32_t dword7;
    uint32_t dword8;
    uint32_t dword9;

    inline void init(uint16_t pkt_len, uint8_t qsel, uint8_t mac_id) {
        dword0 = (40U << RTW89_TXWD_WP_OFFSET_SHIFT) | RTW89_TXWD_WD_INFO_EN;
        dword1 = (mac_id & 0xFF) | ((qsel & 0x1F) << RTW89_TXWD_QSEL_SHIFT) | ((pkt_len & 0xFFFF) << RTW89_TXWD_PKT_LEN_SHIFT);
        dword2 = 0;
        dword3 = 0;
        dword4 = 0;
        dword5 = 0;
        dword6 = 0;
        dword7 = 0;
        dword8 = 0;
        dword9 = 0;
    }
};

// Incomplete RX status prototype; rtw89 has short/long descriptors and prefixes.
struct Rtw89RxDesc {
    uint32_t dword0;
#define RTW89_RXD_PKT_LEN_MASK           0x00003FFF
#define RTW89_RXD_SHIFT_MASK             0x00030000
#define RTW89_RXD_CRC_ERR                (1U << 18)
#define RTW89_RXD_ICV_ERR                (1U << 19)
    uint32_t dword1;
#define RTW89_RXD_MAC_ID_MASK            0x000000FF
    uint32_t dword2;
#define RTW89_RXD_PAGING_MASK            0x000000FF
    uint32_t dword3;
    uint32_t dword4;
    int8_t   rssi;
    uint8_t  rx_rate;
    uint16_t rsvd;
    uint32_t dword6;
    uint32_t dword7;

    inline uint16_t get_pkt_len() const { return (dword0 & RTW89_RXD_PKT_LEN_MASK); }
    inline bool is_crc_err() const { return (dword0 & RTW89_RXD_CRC_ERR) != 0; }
    inline bool is_icv_err() const { return (dword0 & RTW89_RXD_ICV_ERR) != 0; }
    inline int8_t get_rssi() const { return rssi; }
};

#pragma pack(pop)

// Queue selection mapping helper
static inline uint8_t rtl8852be_queue_select(uint8_t q) {
    switch (q) {
        case Q_MGNT: return 0x12;
        case Q_HI:   return 0x11;
        case Q_VO:   return 3;
        case Q_VI:   return 2;
        case Q_BK:   return 1;
        case Q_BE:
        default:     return 0;
    }
}

// 32-bit DMA boundary and alignment validation
static inline bool rtlDmaRangeValid(uint64_t addr, uint32_t size, uint32_t align) {
    if (size == 0) return false;
    if (addr == 0) return false;
    if (align > 1 && (addr % align) != 0) return false;
    if (addr > 0xFFFFFFFFULL || size - 1 > 0xFFFFFFFFULL - addr) return false;
    return true;
}

// Validate the entire access against the actual mapping without integer overflow.
static inline bool rtlMmioRangeValid(uint32_t offset, uint32_t width, uint64_t mapSize) {
    if (width != 1 && width != 2 && width != 4) return false;
    return (offset % width) == 0 && mapSize >= width && offset <= mapSize - width;
}

#endif // RTL8852BE_HW_HPP
