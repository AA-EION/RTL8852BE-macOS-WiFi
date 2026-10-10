// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Issen Software Group
// Copyright (c) 2020-2022 Realtek Corporation
// Realtek RTL8852BE macOS Driver (Wi-Fi 6 802.11ax PCIe)
// Based on Linux drivers/net/wireless/realtek/rtw89 (rtw8852be, rtw8852b, pci.c, mac.c, phy.c, fw.c, efuse.c)

#ifndef RTL8852BE_HPP
#define RTL8852BE_HPP

#include <IOKit/IOLib.h>
#include <sys/kpi_mbuf.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkMedium.h>
#include <IOKit/network/IOOutputQueue.h>
#include <IOKit/network/IOGatedOutputQueue.h>
#include <IOKit/network/IONetworkStats.h>
#include <IOKit/IOFilterInterruptEventSource.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>

#include "RTL8852BE_hw.hpp"
#include "RTL8852BE_ipc.h"
#include "RTL8852BE_crypto.hpp"
#include "RTL8852BE_tables.hpp"
#include "RTL8852BE_firmware.hpp"

#define RTL8852BE_TX_DESC_COUNT  128
#define RTL8852BE_RX_DESC_COUNT  128
#define RTL8852BE_RX_BUF_SIZE    2048
#define RTL8852BE_TX_BUF_SIZE    2048

#define RTL8852BE_SCAN_DWELL_MS  100
#define RTL8852BE_SCAN_TIMEOUT_MS 8000

struct RTLDmaBuf {
    IOBufferMemoryDescriptor *mem;
    IODMACommand             *cmd;
    void                     *virt;
    uint64_t                  ioaddr;
    size_t                    size;
};

#include "RTL8852BE_efuse.hpp"

class RTL8852BE : public IOEthernetController {
    OSDeclareDefaultStructors(RTL8852BE);
    typedef IOEthernetController super;

public:
    virtual bool init(OSDictionary *properties = nullptr) override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    virtual void free() override;

    // IOEthernetController Interface
    virtual IOReturn enable(IONetworkInterface *netif) override;
    virtual IOReturn disable(IONetworkInterface *netif) override;
    virtual UInt32 outputPacket(mbuf_t m, void *param) override;
    virtual IOReturn getHardwareAddress(IOEthernetAddress *addrP) override;
    virtual IOReturn setPromiscuousMode(bool active) override;
    virtual IOReturn setMulticastMode(bool active) override;
    virtual IOReturn setMulticastList(IOEthernetAddress *addrs, UInt32 count) override;
    virtual IOOutputQueue *createOutputQueue() override;
    virtual bool configureInterface(IONetworkInterface *netif) override;

    // UserClient control APIs; mutations use fCommandGate. Read serialization
    // against teardown remains a bring-up requirement (see docs/AUDIT.md).
    IOCommandGate *getCommandGate() const { return fCommandGate; }
    IOReturn triggerScan();
    IOReturn getScanResults(RTL8852BEScanResults *outResults);
    IOReturn connectToNetwork(const char *ssid, const char *passphrase, const uint8_t *bssid);
    IOReturn disconnectNetwork();
    IOReturn getStatus(RTL8852BEStatus *outStatus);
    IOReturn setAntennaMode(uint8_t ant); // 0=Auto 2x2, 1=Path A (Main), 2=Path B (Aux)
    IOReturn diagReadReg(uint32_t addr, uint32_t width, uint32_t *outValue);
    IOReturn diagWriteReg(uint32_t addr, uint32_t width, uint32_t value);
    IOReturn diagReadRF(uint8_t path, uint32_t addr, uint32_t *outValue);
    IOReturn diagWriteRF(uint8_t path, uint32_t addr, uint32_t value);
    IOReturn getDiag(RTL8852BEDiag *outDiag);
    IOReturn triggerIQK(uint32_t *outResult);

    // Hardware Register Accessors
    uint8_t  mmio_read8(uint32_t offset);
    uint16_t mmio_read16(uint32_t offset);
    uint32_t mmio_read32(uint32_t offset);
    void     mmio_write8(uint32_t offset, uint8_t val);
    void     mmio_write16(uint32_t offset, uint16_t val);
    void     mmio_write32(uint32_t offset, uint32_t val);

    // Masked 32-bit MMIO access
    uint32_t mmio_read32_mask(uint32_t offset, uint32_t mask);
    void     mmio_write32_mask(uint32_t offset, uint32_t mask, uint32_t val);

    // SWSI RF Access (Path A / Path B)
    uint32_t rf_read(uint8_t path, uint32_t addr, uint32_t mask = 0xFFFFF);
    bool     rf_write(uint8_t path, uint32_t addr, uint32_t mask, uint32_t val);

    // Interrupt Callbacks
    static bool interruptFilter(OSObject *owner, IOFilterInterruptEventSource *src);
    static void interruptAction(OSObject *owner, IOInterruptEventSource *src, int count);
    static void scanTimerAction(OSObject *owner, IOTimerEventSource *timer);

private:
    struct ConnectArgs {
        const char *ssid;
        const uint8_t *bssid;
        const uint8_t *pmk;
        bool has_pmk;
    };
    // Gated entry points (executed on workloop)
    static IOReturn gatedScan(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    static IOReturn gatedConnect(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    static IOReturn gatedDisconnect(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    static IOReturn gatedSetAntenna(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    static IOReturn gatedDiagWriteReg(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    static IOReturn gatedDiagWriteRF(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    static IOReturn gatedTriggerIQK(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);

    // Hardware initialization & teardown
    void cleanupResources();
    bool hwInit();
    void hwDisable();
    bool pciWakeAndConfigure();
    bool powerOn();
    void powerOff();
    bool readEfuse(RTL8852BECalibData &outCalib);
    RtlRegisterIO registerIO();
    bool downloadFirmware();
    bool initBBAndRF();
    bool initDMARings();
    void freeDMARings();
    bool allocDMA(RTLDmaBuf &buf, size_t size, size_t alignment);
    void freeDMA(RTLDmaBuf &buf);
    bool setChannel(uint8_t channel);
    void enableInterrupts();
    void disableInterrupts();

    // DMA Ring Operations
    bool transmitRawFrame(const uint8_t *frame, size_t len, uint8_t qsel, uint8_t rate);
    void handleRxInterrupt();
    void handleTxInterrupt();

    // 802.11 Scan & Connection State Machine
    void scanStep();
    void sendProbeRequest();
    void finishScan();
    void processRxFrame(const uint8_t *frame, size_t len, int8_t rssi);
    bool parseBeaconOrProbe(const uint8_t *frame, size_t len, RTL8852BEDiscoveredAP &outAP);
    void handleAuthResponse(const uint8_t *frame, size_t len);
    void handleAssocResponse(const uint8_t *frame, size_t len);
    void handleEAPOLFrame(const uint8_t *frame, size_t len, size_t hdrLen);
    void sendAuthFrame();
    void sendAssocFrame();
    void sendEAPOLFrame(uint16_t key_info, const uint8_t *key_replay, const uint8_t *snonce, const uint8_t *mic, const uint8_t *key_data, size_t key_data_len);
    void handleDeauthFrame(const uint8_t *frame, size_t len);

    // IOKit Objects
    IOPCIDevice                   *fPCIDevice;
    IOMemoryMap                   *fMMIOMap;
    volatile uint8_t              *fMMIOBase;
    IOWorkLoop                    *fWorkLoop;
    IOCommandGate                 *fCommandGate;
    IOFilterInterruptEventSource  *fInterruptSource;
    IOTimerEventSource            *fScanTimer;
    IOEthernetInterface           *fNetif;
    IOGatedOutputQueue            *fOutputQueue;
    OSDictionary                  *fMediumDict;

    // Hardware & Device State
    bool                           fHwReady;
    bool                           fFwReady;
    bool                           fInterruptEnabled;
    volatile uint32_t              fHIMRMask;
    uint32_t                       fInitStage;
    uint16_t                       fPciDeviceId;
    uint16_t                       fSubsysVendor;
    uint16_t                       fSubsysDevice;
    RTL8852BECalibData             fCalib;
    uint8_t                        fCutVersion;
    uint8_t                        fCurrentChannel;
    uint8_t                        fAntennaMode; // 0=2x2 Auto, 1=Main (A), 2=Aux (B)

    // DMA Ring Buffers
    RTLDmaBuf                      fTxRingDma;
    Rtw89TxBd                     *fTxRingVirt;
    RTLDmaBuf                      fTxBufDma[RTL8852BE_TX_DESC_COUNT];
    uint16_t                       fTxWp;
    uint16_t                       fTxRp;

    RTLDmaBuf                      fRxRingDma;
    Rtw89RxBd                     *fRxRingVirt;
    RTLDmaBuf                      fRxBufDma[RTL8852BE_RX_DESC_COUNT];
    uint16_t                       fRxWp;
    uint16_t                       fRxRp;

    // 802.11 State Machine Context
    RTL8852BEWifiState             fWifiState;
    uint8_t                        fScanCurrentChanIdx;
    RTL8852BEScanResults           fScanResults;
    char                           fTargetSSID[RTL8852BE_MAX_SSID_LEN + 1];
    char                           fTargetPassphrase[RTL8852BE_MAX_PASSPHRASE_LEN + 1];
    uint8_t                        fTargetBSSID[6];
    uint8_t                        fTargetChannel;
    uint8_t                        fANonce[32];
    uint8_t                        fSNonce[32];
    uint8_t                        fPMK[32];
    uint8_t                        fPTK[64];
    uint8_t                        fGTK[32];
    uint64_t                       fTxPN;
    uint64_t                       fRxPN;
    uint64_t                       fRxPtkPN;
    uint64_t                       fRxGtkPN[4];
    uint64_t                       fLastReplayCounter;
    bool                           fKeyInstalled;
    bool                           fSecured;
    uint8_t                        fRxPlain[1600];

    // Accounting & Diagnostics
    uint64_t                       fRxBytes;
    uint64_t                       fTxBytes;
    uint32_t                       fRxDescDone;
    uint32_t                       fRxMgmt, fRxBeacons, fRxData, fRxCrcErr, fRxDropped;
    uint32_t                       fTxSubmitted, fTxRingFull;
    uint32_t                       fInterruptCount;
};

#endif // RTL8852BE_HPP
