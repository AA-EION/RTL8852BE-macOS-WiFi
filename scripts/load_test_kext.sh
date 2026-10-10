#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Issen Software Group
set -euo pipefail
if [[ $# -ne 1 ]]; then
    echo "Usage: $0 /absolute/path/RTL8852BEWiFi.kext" >&2
    exit 2
fi
# Keep this refusal in sync with RTL8852BE_HARDWARE_BRINGUP_READY in the IPC header.
# Do not request privilege, unload an existing driver, or alter an installed copy.
echo "RTL8852BE hardware loading is blocked: firmware/eFuse transport and AX DMA bring-up are incomplete. See docs/AUDIT.md. No system changes made." >&2
exit 2
