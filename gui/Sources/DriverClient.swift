// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group
import Foundation

enum DriverClientError: Error, LocalizedError {
    case request(Int32)
    case scanTimeout
    case invalidNetwork

    var errorDescription: String? {
        switch self {
        case .request(let code): return "Driver request failed (IOReturn 0x\(String(UInt32(bitPattern: code), radix: 16)))."
        case .scanTimeout: return "The driver did not finish scanning within 10 seconds."
        case .invalidNetwork: return "Choose a WPA2 Personal network and enter an 8–63 byte password."
        }
    }
}

// Imported IPC structures contain value fields only, with no pointers.
struct DriverSnapshot: @unchecked Sendable {
    let status: RTL8852BEStatus
    let diagnostics: RTL8852BEDiag?
}

protocol WirelessDriverClient: Sendable {
    func snapshot() throws -> DriverSnapshot
    func scan() async throws -> [WiFiNetwork]
    func join(_ network: WiFiNetwork, password: String) throws
    func disconnect() throws
    func setAntenna(_ value: Int) throws
    func interfaceName() -> String
}

// Each operation owns and closes its IOKit connection, including error paths.
struct IOKitDriverClient: WirelessDriverClient {
    private func withConnection<T>(_ action: (io_connect_t) throws -> T) throws -> T {
        var connection: io_connect_t = 0
        let result = rtl_open_client(&connection)
        guard result == 0 else { throw DriverClientError.request(result) }
        defer { rtl_close_client(connection) }
        return try action(connection)
    }

    private func check(_ result: Int32) throws {
        if result != 0 { throw DriverClientError.request(result) }
    }

    func snapshot() throws -> DriverSnapshot {
        try withConnection { connection in
            var status = RTL8852BEStatus()
            try check(rtl_get_status(connection, &status))
            var diagnostics = RTL8852BEDiag()
            let valid = rtl_get_diag(connection, &diagnostics) == 0 && diagnostics.version >= 1
            return DriverSnapshot(status: status, diagnostics: valid ? diagnostics : nil)
        }
    }

    func scan() async throws -> [WiFiNetwork] {
        var connection: io_connect_t = 0
        try check(rtl_open_client(&connection))
        defer { rtl_close_client(connection) }
        try check(rtl_trigger_scan(connection))
        for _ in 0..<100 {
            try await Task.sleep(for: .milliseconds(100))
            var status = RTL8852BEStatus()
            try check(rtl_get_status(connection, &status))
            if status.state != UInt32(kRTLStateScanning.rawValue) {
                var results = RTL8852BEScanResults()
                try check(rtl_get_scan_results(connection, &results))
                return Self.networks(results)
            }
        }
        throw DriverClientError.scanTimeout
    }

    func join(_ network: WiFiNetwork, password: String) throws {
        guard network.secured, (1...32).contains(network.ssid.utf8.count),
              (8...63).contains(password.utf8.count), network.bssidBytes.count == 6,
              !network.ssid.utf8.contains(0), !password.utf8.contains(0) else {
            throw DriverClientError.invalidNetwork
        }
        var parameters = RTL8852BEConnectParams()
        withUnsafeMutableBytes(of: &parameters.ssid) { $0.copyBytes(from: network.ssid.utf8) }
        withUnsafeMutableBytes(of: &parameters.passphrase) { $0.copyBytes(from: password.utf8) }
        let bssid = network.bssidBytes
        parameters.bssid = (bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5])
        try withConnection { try check(rtl_connect_ap($0, &parameters)) }
    }

    func disconnect() throws { try withConnection { try check(rtl_disconnect_ap($0)) } }
    func setAntenna(_ value: Int) throws {
        guard (0...2).contains(value) else { throw DriverClientError.request(Int32(bitPattern: 0xE00002C2)) }
        try withConnection { try check(rtl_set_antenna($0, UInt8(value))) }
    }

    func interfaceName() -> String {
        var name = [CChar](repeating: 0, count: 32)
        return rtl_copy_bsd_name(&name, name.count) == 0 ? String(cString: name) : ""
    }

    static func string<T>(_ field: T) -> String {
        withUnsafeBytes(of: field) { String(decoding: $0.prefix { $0 != 0 }, as: UTF8.self) }
    }

    static func networks(_ raw: RTL8852BEScanResults) -> [WiFiNetwork] {
        var copy = raw
        let count = min(Int(copy.count), Int(RTL8852BE_MAX_SCAN_APS))
        return withUnsafeBytes(of: &copy.aps) { buffer in
            let entries = buffer.bindMemory(to: RTL8852BEDiscoveredAP.self)
            var networks: [String: WiFiNetwork] = [:]
            for index in 0..<count {
                let ap = entries[index]
                let bssid = [ap.bssid.0, ap.bssid.1, ap.bssid.2, ap.bssid.3, ap.bssid.4, ap.bssid.5]
                guard bssid.contains(where: { $0 != 0 }), bssid[0] & 1 == 0,
                      ap.channel > 0, ap.channel <= 165 else { continue }
                let ssid = string(ap.ssid)
                guard ssid.utf8.count <= Int(RTL8852BE_MAX_SSID_LEN) else { continue }
                let id = AppModel.mac(ap.bssid)
                let network = WiFiNetwork(ssid: ssid, bssid: id, bssidBytes: bssid,
                                          channel: Int(ap.channel), rssi: Int(ap.rssi), secured: ap.is_wpa2 != 0)
                if let existing = networks[id], existing.rssi >= network.rssi { continue }
                networks[id] = network
            }
            return networks.values.sorted { $0.rssi == $1.rssi ? $0.bssid < $1.bssid : $0.rssi > $1.rssi }
        }
    }
}
