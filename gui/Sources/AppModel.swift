// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

import AppKit
import Darwin
import Observation

/// What the radio is doing, from the user's point of view.
enum LinkPhase: Equatable {
    case noDriver, disconnected, scanning, connecting, connected

    var label: String {
        switch self {
        case .noDriver: return "Driver not loaded"
        case .disconnected: return "Not connected"
        case .scanning: return "Scanning…"
        case .connecting: return "Connecting…"
        case .connected: return "Connected"
        }
    }
}

enum LogLevel: Int, CaseIterable, Comparable {
    case info, warning, error
    static func < (a: LogLevel, b: LogLevel) -> Bool { a.rawValue < b.rawValue }
    var label: String {
        switch self {
        case .info: return "INFO"
        case .warning: return "WARN"
        case .error: return "ERROR"
        }
    }
}

struct LogEntry: Identifiable {
    let id = UUID()
    let date: Date
    let level: LogLevel
    let message: String
}

struct WiFiNetwork: Identifiable, Hashable, Sendable {
    var id: String { bssid }
    let ssid: String
    let bssid: String
    let bssidBytes: [UInt8]
    let channel: Int
    let rssi: Int
    let secured: Bool

    var displayName: String { ssid.isEmpty ? "Hidden network" : ssid }
    /// 0...1 for the variable-value Wi-Fi symbol.
    var signalLevel: Double {
        switch rssi {
        case (-55)...: return 1.0
        case (-67)...: return 0.7
        case (-78)...: return 0.4
        default: return 0.15
        }
    }
}

struct Throughput {
    var receiveBitsPerSecond: Double = 0
    var transmitBitsPerSecond: Double = 0
    var totalReceived: UInt64 = 0
    var totalSent: UInt64 = 0
}

/// Single source of truth for the UI. Talks to RTL8852BEWiFi.kext through its IOUserClient.
@Observable
@MainActor
final class AppModel {
    // Environment / onboarding
    let hardwareBringupReady = RTL8852BE_HARDWARE_BRINGUP_READY != 0
    var hardwarePresent = false
    var bootArgPresent = false
    var driverLoaded = false
    var driverVersion: String?

    // Link
    var phase: LinkPhase = .noDriver
    var macAddress = ""
    var ssid = ""
    var bssid = ""
    var channel = 0
    var rssi = -100
    var antenna = 0
    var interfaceName = ""
    var ipAddress = ""
    var connectedSince: Date?
    var throughput = Throughput()

    // Scan / join
    var networks: [WiFiNetwork] = []
    var isScanning = false
    var lastScan: Date?
    var joiningNetwork: WiFiNetwork?
    var isBusy = false
    var alertMessage: String?

    // Diagnostics
    var diag: RTL8852BEDiag?

    // Log
    var logs: [LogEntry] = []
    var logLevelFilter: LogLevel = .info
    var filteredLogs: [LogEntry] { logs.filter { $0.level >= logLevelFilter } }

    private let client: any WirelessDriverClient
    private let monitoringEnabled: Bool
    private var timer: Timer?
    private var lastCounters: (rx: UInt64, tx: UInt64, at: Date)?
    private var lastState: UInt32 = .max

    init(client: any WirelessDriverClient = IOKitDriverClient(), startMonitoring: Bool = true) {
        self.client = client
        self.monitoringEnabled = startMonitoring
        log("RTL8852BE Wi-Fi \(Vendor.appVersion) started on macOS "
            + ProcessInfo.processInfo.operatingSystemVersionString)
        guard startMonitoring else { return }
        refreshEnvironment()
        refresh()
        timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            MainActor.assumeIsolated { self?.refresh() }
        }
    }

    var connectedDuration: TimeInterval? {
        connectedSince.map { Date().timeIntervalSince($0) }
    }

    // MARK: - Logging

    func log(_ message: String, _ level: LogLevel = .info) {
        logs.append(LogEntry(date: Date(), level: level, message: message))
        if logs.count > 500 { logs.removeFirst(logs.count - 500) }
    }

    func clearLogs() { logs.removeAll() }

    // MARK: - Polling

    func refreshEnvironment() {
        hardwarePresent = rtl_hardware_present() == 1
        var size = 0
        sysctlbyname("kern.bootargs", nil, &size, nil, 0)
        var buffer = [CChar](repeating: 0, count: max(size, 1))
        sysctlbyname("kern.bootargs", &buffer, &size, nil, 0)
        let bootArgs = String(cString: buffer)
        bootArgPresent = bootArgs.split(separator: " ").contains("rtl8852be_experimental=1")
    }

    func refresh() {
        let snapshot: DriverSnapshot
        do { snapshot = try client.snapshot() }
        catch {
            if driverLoaded { log("Driver is no longer available: \(error.localizedDescription)", .warning) }
            driverLoaded = false
            phase = .noDriver
            interfaceName = ""
            ipAddress = ""
            connectedSince = nil
            driverVersion = nil
            diag = nil
            macAddress = ""
            bssid = ""
            ssid = ""
            channel = 0
            rssi = -100
            throughput = Throughput()
            lastCounters = nil
            lastState = .max
            return
        }
        if !driverLoaded {
            driverLoaded = true
            log("Connected to RTL8852BEWiFi.kext.")
            if monitoringEnabled { Task { await loadDriverVersion() } }
        }
        let st = snapshot.status
        bssid = Self.mac(st.connected_bssid)
        ssid = IOKitDriverClient.string(st.connected_ssid)
        channel = Int(st.channel)
        rssi = Int(st.rssi)
        antenna = Int(st.ant_mode)

        if st.state != lastState {
            if st.state == 5 {
                connectedSince = Date()
                log("Connected to \(ssid) (channel \(channel)).")
            } else if lastState == 5 {
                connectedSince = nil
                log("Disconnected from the network.", .warning)
            }
            lastState = st.state
        }
        switch st.state {
        case 5: phase = .connected
        case 2, 3, 4: phase = .connecting
        case 1: phase = .scanning
        default: phase = isScanning ? .scanning : .disconnected
        }

        diag = snapshot.diagnostics
        macAddress = snapshot.diagnostics.map { Self.mac($0.mac_addr) } ?? ""

        updateInterface()
    }

    private func updateInterface() {
        interfaceName = client.interfaceName()
        guard !interfaceName.isEmpty else {
            ipAddress = ""
            throughput = Throughput()
            lastCounters = nil
            return
        }

        var ifap: UnsafeMutablePointer<ifaddrs>?
        guard getifaddrs(&ifap) == 0, let first = ifap else { return }
        defer { freeifaddrs(ifap) }
        var address = ""
        var rx: UInt64 = 0, tx: UInt64 = 0
        for ptr in sequence(first: first, next: { $0.pointee.ifa_next }) {
            let ifa = ptr.pointee
            guard String(cString: ifa.ifa_name) == interfaceName, let sa = ifa.ifa_addr else { continue }
            if sa.pointee.sa_family == UInt8(AF_INET) {
                var host = [CChar](repeating: 0, count: Int(NI_MAXHOST))
                getnameinfo(sa, socklen_t(sa.pointee.sa_len), &host, socklen_t(host.count), nil, 0, NI_NUMERICHOST)
                address = String(cString: host)
            } else if sa.pointee.sa_family == UInt8(AF_LINK), let data = ifa.ifa_data {
                let stats = data.assumingMemoryBound(to: if_data.self).pointee
                rx = UInt64(stats.ifi_ibytes)
                tx = UInt64(stats.ifi_obytes)
            }
        }
        ipAddress = address
        // macOS 26 leaves if_data input counters at zero for IOKit Ethernet drivers;
        // the driver's own counters (diagnostics v1+) are authoritative when present.
        if let d = diag, d.version >= 1 {
            rx = d.rx_bytes
            tx = d.tx_bytes
        }
        let now = Date()
        if let last = lastCounters, rx >= last.rx, tx >= last.tx {
            let dt = max(now.timeIntervalSince(last.at), 0.1)
            throughput.receiveBitsPerSecond = Double(rx - last.rx) * 8 / dt
            throughput.transmitBitsPerSecond = Double(tx - last.tx) * 8 / dt
        }
        throughput.totalReceived = rx
        throughput.totalSent = tx
        lastCounters = (rx, tx, now)
    }

    private func loadDriverVersion() async {
        let output = await Self.run("/usr/bin/kmutil", ["showloaded", "--list-only", "--bundle-identifier",
                                                         "com.rtl8852be.macos.wifi"])
        if let range = output.range(of: #"\(([0-9.]+)\)"#, options: .regularExpression) {
            driverVersion = String(output[range].dropFirst().dropLast())
        }
    }

    // MARK: - Actions

    func scan() async {
        guard driverLoaded, !isScanning, !isBusy, phase != .connected, phase != .connecting else { return }
        isScanning = true
        phase = .scanning
        defer { isScanning = false; refresh() }
        log("Scanning 2.4 GHz and 5 GHz channels…")
        let client = self.client
        do {
            networks = try await Task.detached { try await client.scan() }.value
            lastScan = Date()
            log("Found \(networks.count) network\(networks.count == 1 ? "" : "s").")
        } catch {
            log("Scan failed: \(error.localizedDescription)", .error)
            alertMessage = error.localizedDescription
        }
    }

    func join(_ network: WiFiNetwork, password: String) async {
        guard driverLoaded, !isBusy, !isScanning else { return }
        isBusy = true
        joiningNetwork = network
        defer { isBusy = false; joiningNetwork = nil }
        log("Joining \(network.displayName) (\(network.bssid), channel \(network.channel))…")
        let client = self.client
        do { try await Task.detached { try client.join(network, password: password) }.value }
        catch {
            alertMessage = error.localizedDescription
            log("Join failed: \(error.localizedDescription)", .error)
            return
        }
        for _ in 0..<30 {
            try? await Task.sleep(for: .milliseconds(500))
            refresh()
            if phase == .connected { return }
            if phase == .disconnected || phase == .noDriver { break }
        }
        alertMessage = "Could not join \(network.displayName). Check the password and that the network is in range."
        log("Join timed out or was rejected.", .error)
    }

    func disconnect() async {
        guard driverLoaded, !isBusy else { return }
        isBusy = true
        defer { isBusy = false }
        let client = self.client
        do { try await Task.detached { try client.disconnect() }.value }
        catch {
            alertMessage = error.localizedDescription
            log("Disconnect failed: \(error.localizedDescription)", .error)
        }
        refresh()
    }

    func setAntenna(_ value: Int) {
        do {
            try client.setAntenna(value)
            antenna = value
            log("Wi-Fi antenna set to \(value == 0 ? "Auto (2x2)" : (value == 2 ? "Path B (Aux)" : "Path A (Main)")).")
        } catch {
            alertMessage = error.localizedDescription
            log("Antenna request failed: \(error.localizedDescription)", .error)
        }
    }

    /// Asks for an administrator password and loads the driver (installed copy first).
    func loadDriver() async {
        guard hardwareBringupReady else {
            alertMessage = "Hardware testing is unavailable in this development build. The driver is incomplete."
            log(alertMessage!, .warning)
            return
        }
        guard let resources = Bundle.main.resourcePath else { return }
        isBusy = true
        defer { isBusy = false }
        let helper = (resources as NSString).appendingPathComponent("load_test_kext.sh")
        let kext = (resources as NSString).appendingPathComponent("RTL8852BEWiFi.kext")
        func q(_ s: String) -> String { "'" + s.replacingOccurrences(of: "'", with: "'\"'\"'") + "'" }
        let command = "/bin/bash \(q(helper)) \(q(kext))"
        let source = "do shell script \"\(command.replacingOccurrences(of: "\\", with: "\\\\").replacingOccurrences(of: "\"", with: "\\\""))\" with administrator privileges"
        log("Loading the driver (administrator password required)…")
        let message: String? = await Task.detached {
            var error: NSDictionary?
            let result = NSAppleScript(source: source)?.executeAndReturnError(&error)
            if let error { return error[NSAppleScript.errorMessage] as? String ?? "Load failed." }
            return result?.stringValue
        }.value
        if let message, !message.isEmpty {
            let failed = message.contains("not approved") || message.contains("disabled") || message.contains("Could not")
            log(message, failed ? .warning : .info)
            if message.contains("not approved") {
                alertMessage = "macOS needs your approval: open System Settings › Privacy & Security, allow the RTL8852BE driver, then restart."
            }
        }
        try? await Task.sleep(for: .seconds(2))
        refreshEnvironment()
        refresh()
    }

    // MARK: - Helpers

    nonisolated static func firmwareVersion(_ value: UInt32) -> String {
        guard value != 0 else { return "Unknown" }
        let parts: [UInt32] = [value >> 24, (value >> 16) & 0xff,
                               (value >> 8) & 0xff, value & 0xff]
        return parts.map { String($0) }.joined(separator: ".")
    }

    nonisolated static func mac(_ m: (UInt8, UInt8, UInt8, UInt8, UInt8, UInt8)) -> String {
        String(format: "%02x:%02x:%02x:%02x:%02x:%02x", m.0, m.1, m.2, m.3, m.4, m.5)
    }

    nonisolated private static func run(_ path: String, _ args: [String]) async -> String {
        await Task.detached {
            let task = Process()
            task.executableURL = URL(fileURLWithPath: path)
            task.arguments = args
            let pipe = Pipe()
            task.standardOutput = pipe
            task.standardError = Pipe()
            guard (try? task.run()) != nil else { return "" }
            let data = pipe.fileHandleForReading.readDataToEndOfFile()
            task.waitUntilExit()
            return String(decoding: data, as: UTF8.self)
        }.value
    }
}
