#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Issen Software Group
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_NAME="RTL8852BE Wireless Utility.app"
APP_BUNDLE="${PROJECT_DIR}/build/${APP_NAME}"
DMG_ROOT="${PROJECT_DIR}/build/dmg_staging"
DMG_OUT="${PROJECT_DIR}/dist/RTL8852BE_WiFi_Installer.dmg"

echo "=== Building Native macOS GUI Application & DMG Installer ==="

# 1. Ensure kext and CLI are built
make -C "${PROJECT_DIR}" all

# Build the shared RTL87-style SwiftUI app and include the completed build artifacts.
make -C "${PROJECT_DIR}" app

# 7. Build the installer package (app -> /Applications, CLI -> /usr/local/bin).
#    The kext stays inside the app; loading is currently blocked. It is never installed
#    where macOS would start it at boot. Nothing touches NVRAM or the EFI partition.
VERSION="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "${PROJECT_DIR}/src/Info.plist")"
PKG_WORK="${PROJECT_DIR}/build/pkg"
PKG_ROOT="${PKG_WORK}/root"
rm -rf "${PKG_WORK}"
mkdir -p "${PKG_ROOT}/Applications" "${PKG_ROOT}/usr/local/bin" "${PKG_WORK}/resources"
ditto "${APP_BUNDLE}" "${PKG_ROOT}/Applications/${APP_NAME}"
cp "${PROJECT_DIR}/tools/rtl8852be_cli" "${PKG_ROOT}/usr/local/bin/rtl8852be_cli"
pkgbuild --analyze --root "${PKG_ROOT}" "${PKG_WORK}/component.plist"
# Never let Installer "relocate" the app onto some other copy with the same bundle ID.
i=0
while /usr/libexec/PlistBuddy -c "Print :${i}" "${PKG_WORK}/component.plist" >/dev/null 2>&1; do
    /usr/libexec/PlistBuddy -c "Delete :${i}:BundleIsRelocatable" "${PKG_WORK}/component.plist" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c "Add :${i}:BundleIsRelocatable bool false" "${PKG_WORK}/component.plist"
    i=$((i + 1))
done
pkgbuild --root "${PKG_ROOT}" --component-plist "${PKG_WORK}/component.plist" \
    --scripts "${PROJECT_DIR}/installer/scripts" \
    --identifier com.rtl8852be.macos.wifi.pkg --version "${VERSION}" --install-location / \
    "${PKG_WORK}/RTL8852BE-component.pkg"
cp "${PROJECT_DIR}/installer/resources/"*.txt "${PKG_WORK}/resources/"
{
  cat "${PROJECT_DIR}/LICENSE"
  if [ -f "${PROJECT_DIR}/NOTICE.md" ]; then printf '\n\n'; cat "${PROJECT_DIR}/NOTICE.md"; fi
  for f in "${PROJECT_DIR}/LICENSES/"*.txt; do
    if [ -f "$f" ]; then printf '\n\n'; cat "$f"; fi
  done
} > "${PKG_WORK}/resources/License.txt"
sed "s/__VERSION__/${VERSION}/" "${PROJECT_DIR}/installer/Distribution.xml" > "${PKG_WORK}/Distribution.xml"
PKG_OUT="${PKG_WORK}/RTL8852BE WiFi ${VERSION}.pkg"
productbuild --distribution "${PKG_WORK}/Distribution.xml" --resources "${PKG_WORK}/resources" \
    --package-path "${PKG_WORK}" "${PKG_OUT}"
pkgutil --check-signature "${PKG_OUT}" >/dev/null 2>&1 || true
echo "[+] Installer package: ${PKG_OUT}"

# 8. Assemble DMG: installer, uninstaller, docs and licence.
rm -rf "${DMG_ROOT}"
mkdir -p "${DMG_ROOT}"
cp "${PKG_OUT}" "${DMG_ROOT}/Install RTL8852BE WiFi.pkg"
cp "${PROJECT_DIR}/scripts/uninstall.sh" "${DMG_ROOT}/uninstall.sh"
cp "${PROJECT_DIR}/installer/resources/welcome.txt" "${DMG_ROOT}/Read Me.txt"
cp "${PROJECT_DIR}/docs/AUDIT.md" "${DMG_ROOT}/Audit.txt"
mkdir -p "${DMG_ROOT}/Licenses"
cp "${PROJECT_DIR}/LICENSE" "${DMG_ROOT}/Licenses/"
cp "${PROJECT_DIR}/NOTICE.md" "${DMG_ROOT}/Licenses/"
cp "${PROJECT_DIR}/LICENSES/"*.txt "${DMG_ROOT}/Licenses/"

mkdir -p "${PROJECT_DIR}/dist"
rm -f "${DMG_OUT}"
hdiutil create -volname "RTL8852BE WiFi ${VERSION}" \
    -srcfolder "${DMG_ROOT}" \
    -ov -format UDZO \
    "${DMG_OUT}"

hdiutil verify "${DMG_OUT}"
echo "[+] DMG Installer successfully built and verified at: ${DMG_OUT}"
