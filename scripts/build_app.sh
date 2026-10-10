#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Issen Software Group
set -euo pipefail
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_BUNDLE="${PROJECT_DIR}/build/RTL8852BE Wireless Utility.app"
rm -rf "${APP_BUNDLE}"
mkdir -p "${APP_BUNDLE}/Contents/MacOS" "${APP_BUNDLE}/Contents/Resources/Licenses"
swiftc -O -parse-as-library -swift-version 5 -target x86_64-apple-macosx14.0 \
    -import-objc-header "${PROJECT_DIR}/gui/RTL8852BEBridge.h" \
    -framework AppKit -framework SwiftUI -framework IOKit -framework ServiceManagement \
    "${PROJECT_DIR}"/gui/Sources/*.swift "${PROJECT_DIR}"/gui/Sources/Views/*.swift \
    -o "${APP_BUNDLE}/Contents/MacOS/RTL8852BEWirelessUtility"
cp "${PROJECT_DIR}/gui/Info.plist" "${APP_BUNDLE}/Contents/Info.plist"
cp "${PROJECT_DIR}/gui/AppIcon.icns" "${APP_BUNDLE}/Contents/Resources/"
cp "${PROJECT_DIR}/scripts/load_test_kext.sh" "${APP_BUNDLE}/Contents/Resources/"
cp "${PROJECT_DIR}/docs/TESTING.md" "${APP_BUNDLE}/Contents/Resources/Testing Guide.txt"
cp "${PROJECT_DIR}/docs/AUDIT.md" "${APP_BUNDLE}/Contents/Resources/Audit.txt"
cp "${PROJECT_DIR}/docs/linux-source-manifest.json" "${APP_BUNDLE}/Contents/Resources/"
cp "${PROJECT_DIR}/LICENSE" "${PROJECT_DIR}/NOTICE.md" "${APP_BUNDLE}/Contents/Resources/Licenses/"
cp "${PROJECT_DIR}/LICENSES/"*.txt "${APP_BUNDLE}/Contents/Resources/Licenses/"
if [[ -d "${PROJECT_DIR}/build/RTL8852BEWiFi.kext" ]]; then
    cp -R "${PROJECT_DIR}/build/RTL8852BEWiFi.kext" "${APP_BUNDLE}/Contents/Resources/"
fi
if [[ -f "${PROJECT_DIR}/tools/rtl8852be_cli" ]]; then
    cp "${PROJECT_DIR}/tools/rtl8852be_cli" "${APP_BUNDLE}/Contents/Resources/"
fi
codesign --force --deep --sign - "${APP_BUNDLE}"
codesign --verify --deep --strict "${APP_BUNDLE}"
