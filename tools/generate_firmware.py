# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Issen Software Group
"""Regenerate only the embedded bytes; preserve the separately maintained parser."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def main():
    source = ROOT / "src/rtw8852b_fw.bin"
    data = source.read_bytes()
    if not data:
        raise ValueError("Firmware image is empty")
    target = ROOT / "src/RTL8852BE_firmware.cpp"
    with target.open("w", encoding="utf-8", newline="\n") as f:
        f.write("// SPDX-License-Identifier: LicenseRef-rtw89-firmware\n")
        f.write("// Copyright (c) Realtek Semiconductor Corporation\n")
        f.write("// Generated from src/rtw8852b_fw.bin by tools/generate_firmware.py.\n\n")
        f.write('#include "RTL8852BE_firmware.hpp"\n\n')
        f.write(f"const size_t rtw8852b_fw_bin_len = {len(data)};\n\n")
        f.write(f"alignas(16)\nconst uint8_t rtw8852b_fw_bin[{len(data)}] = {{\n")
        for i in range(0, len(data), 16):
            f.write("    " + ", ".join(f"0x{b:02x}" for b in data[i:i+16]) + ",\n")
        f.write("};\n")
    print(f"Embedded {len(data)} bytes in {target}; parser and header preserved.")

if __name__ == "__main__":
    main()
