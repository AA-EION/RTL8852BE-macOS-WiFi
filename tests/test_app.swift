// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group
import Foundation

final class TestDriver: WirelessDriverClient, @unchecked Sendable {
    private let lock = NSLock()
    private var connected = false
    private var absent = false
    private var rejectedScan = false
    private var rejectedDisconnect = false
    let network = WiFiNetwork(ssid: "Test 5 GHz", bssid: "02:11:22:33:44:55",
                              bssidBytes: [2, 17, 34, 51, 68, 85], channel: 36, rssi: -48, secured: true)
    func configure(absent: Bool = false, scanFailure: Bool = false, disconnectFailure: Bool = false) {
        lock.lock(); defer { lock.unlock() }
        self.absent = absent
        rejectedScan = scanFailure
        rejectedDisconnect = disconnectFailure
    }
    func snapshot() throws -> DriverSnapshot {
        lock.lock(); defer { lock.unlock() }
        if absent { throw DriverClientError.request(-1) }
        var status = RTL8852BEStatus()
        status.state = connected ? 5 : 0
        status.channel = 36
        status.ant_mode = 0
        status.rssi = -48
        status.connected_bssid = (2, 17, 34, 51, 68, 85)
        withUnsafeMutableBytes(of: &status.connected_ssid) { $0.copyBytes(from: network.ssid.utf8) }
        var diag = RTL8852BEDiag()
        diag.version = 1
        diag.mac_addr = (2, 1, 2, 3, 4, 5)
        diag.rx_bytes = 500
        diag.tx_bytes = 250
        return DriverSnapshot(status: status, diagnostics: diag)
    }
    func scan() async throws -> [WiFiNetwork] {
        if failScan() { throw DriverClientError.scanTimeout }
        return [network]
    }
    private func failScan() -> Bool { lock.lock(); defer { lock.unlock() }; return rejectedScan }
    func join(_ network: WiFiNetwork, password: String) throws {
        lock.lock(); defer { lock.unlock() }
        if password == "badpassword" { throw DriverClientError.invalidNetwork }
        connected = true
    }
    func disconnect() throws {
        lock.lock(); defer { lock.unlock() }
        if rejectedDisconnect { throw DriverClientError.request(-1) }
        connected = false
    }
    func setAntenna(_ value: Int) throws { throw DriverClientError.request(Int32(bitPattern: 0xE00002C7)) }
    func interfaceName() -> String { "" }
}

@main
struct AppRegressions {
    @MainActor
    static func main() async throws {
        let driver = TestDriver()
        let model = AppModel(client: driver, startMonitoring: false)
        model.refresh()
        precondition(model.driverLoaded && model.phase == .disconnected)
        precondition(model.macAddress == "02:01:02:03:04:05" && model.channel == 36)
        await model.scan()
        precondition(model.networks == [driver.network] && !model.isScanning)
        driver.configure(scanFailure: true)
        await model.scan()
        precondition(!model.isScanning && model.alertMessage != nil)
        driver.configure()
        await model.join(driver.network, password: "goodpassword")
        precondition(model.phase == .connected && !model.isBusy && model.joiningNetwork == nil)
        precondition(model.ssid == driver.network.ssid && model.antenna == 0)
        driver.configure(disconnectFailure: true)
        await model.disconnect()
        precondition(model.phase == .connected && model.alertMessage != nil && !model.isBusy)
        driver.configure()
        await model.disconnect()
        precondition(model.phase == .disconnected)
        driver.configure(absent: true)
        model.refresh()
        precondition(!model.driverLoaded && model.phase == .noDriver && model.diag == nil)
        precondition(model.ssid.isEmpty && model.macAddress.isEmpty && model.channel == 0)
        precondition(model.throughput.totalReceived == 0 && model.connectedSince == nil)
        driver.configure()
        model.refresh()
        precondition(model.driverLoaded && model.phase == .disconnected)

        // UTF-8 byte lengths, not Swift character counts, define the IPC fields.
        do {
            try IOKitDriverClient().join(driver.network, password: String(repeating: "é", count: 40))
            preconditionFailure("Oversized UTF-8 password must be rejected before opening IOKit")
        } catch DriverClientError.invalidNetwork {}

        var raw = RTL8852BEScanResults()
        raw.count = .max // malformed count must remain inside the fixed ABI array
        withUnsafeMutableBytes(of: &raw.aps) { bytes in
            let aps = bytes.bindMemory(to: RTL8852BEDiscoveredAP.self)
            for index in 0..<2 {
                aps[index].bssid = (2, 17, 34, 51, 68, 85)
                aps[index].channel = 36
                aps[index].rssi = index == 0 ? -70 : -48
                aps[index].is_wpa2 = 1
                withUnsafeMutableBytes(of: &aps[index].ssid) { $0.copyBytes(from: "Nearby".utf8) }
            }
            aps[2].bssid = (2, 17, 34, 51, 68, 86)
            aps[2].channel = 1
            withUnsafeMutableBytes(of: &aps[2].ssid) { $0.initializeMemory(as: UInt8.self, repeating: 65) }
        }
        let networks = IOKitDriverClient.networks(raw)
        precondition(networks.count == 1 && networks[0].rssi == -48 && networks[0].ssid == "Nearby")
        print("App scan/join/disconnect, driver loss/reconnect, IPC bounds and UTF-8 regressions passed.")
    }
}
