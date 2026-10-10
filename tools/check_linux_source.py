#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Issen Software Group
"""Verify audited hashes, register definitions and all table pairs against Linux v6.12.

Usage: python3 tools/check_linux_source.py /path/to/linux-v6.12
The path can be a kernel checkout or a directory containing the files listed in
docs/linux-source-manifest.json. This check performs no network requests.
It is a source comparison, not a device/lifecycle test.
"""
import ast
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
RTW89 = Path("drivers/net/wireless/realtek/rtw89")


def definitions(text):
    text = text.replace("\\\n", " ")
    return dict(re.findall(r"^#define\s+(\w+)\s+([^\n]+)", text, re.M))


def value(name, defs):
    expression = re.sub(r"/\*.*?\*/|//.*", "", defs[name]).strip()
    expression = re.sub(r"\b(0[xX][0-9a-fA-F]+|[0-9]+)[uUlL]+\b", r"\1", expression)
    tree = ast.parse(expression, mode="eval")

    def visit(node):
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return node.value
        if isinstance(node, ast.Name):
            return value(node.id, defs)
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and not node.keywords:
            args = [visit(a) for a in node.args]
            if node.func.id == "BIT" and len(args) == 1:
                return 1 << args[0]
            if node.func.id == "GENMASK" and len(args) == 2:
                return ((1 << (args[0] + 1)) - 1) ^ ((1 << args[1]) - 1)
        if isinstance(node, ast.BinOp):
            left, right = visit(node.left), visit(node.right)
            operations = {ast.BitOr: lambda: left | right, ast.BitAnd: lambda: left & right,
                          ast.LShift: lambda: left << right, ast.RShift: lambda: left >> right,
                          ast.Add: lambda: left + right, ast.Sub: lambda: left - right}
            if type(node.op) in operations:
                return operations[type(node.op)]()
        raise ValueError(f"Unsupported constant expression: {expression}")
    return visit(tree.body)


def pairs(text, symbol):
    match = re.search(r"\b" + re.escape(symbol) + r"\s*\[[^]]*\]\s*=\s*\{(.*?)\n\};", text, re.S)
    if not match:
        raise ValueError(f"Missing table {symbol}")
    return [(int(a, 16), int(b, 16)) for a, b in
            re.findall(r"\{\s*(0x[\da-fA-F]+)\s*,\s*(0x[\da-fA-F]+)\s*\}", match[1])]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("linux_source", type=Path)
    args = parser.parse_args()
    manifest = json.loads((ROOT / "docs/linux-source-manifest.json").read_text())
    for record in manifest["files"]:
        data = (args.linux_source / record["path"]).read_bytes()
        if hashlib.sha256(data).hexdigest() != record["sha256"]:
            raise ValueError(f"Source hash mismatch: {record['path']} (expected {manifest['ref']})")
    upstream = {}
    for name in ("reg.h", "pci.h", "mac.h", "core.h"):
        source = (args.linux_source / RTW89 / name).read_text()
        upstream.update(definitions(source))
        upstream.update(re.findall(r"\b([A-Z][A-Z_0-9]+)\s*=\s*(0x[0-9a-fA-F]+)", source))
    txrx = (args.linux_source / RTW89 / "txrx.h").read_text()
    upstream.update(re.findall(r"\b(RTW89_TX_QSEL_\w+)\s*=\s*(0x[0-9a-fA-F]+)", txrx))
    aliases = {"B_AX_RXQ_RXBD_INT": "B_AX_RXDMA_INT", "B_AX_ACH0_TXBD_INT": "B_AX_TXDMA_ACH0_INT",
               "B_AX_MGNT_TXBD_INT": "B_AX_TXDMA_CH8_INT", "B_AX_DMA_BUSY1_CHECK": "DMA_BUSY1_CHECK_V1",
               "B_SWSI_DATA_VAL_V1_MASK": "B_SWSI_DATA_VAL_V1",
               "B_SWSI_DATA_ADDR_V1_SHIFT": "B_SWSI_DATA_ADDR_V1",
               "B_SWSI_DATA_PATH_V1_SHIFT": "B_SWSI_DATA_PATH_V1",
               "B_SWSI_READ_ADDR_PATH_V1_SHIFT": "B_SWSI_READ_ADDR_PATH_V1",
               "Q_VO": "RTW89_TX_QSEL_VO_0", "Q_VI": "RTW89_TX_QSEL_VI_0",
               "Q_BE": "RTW89_TX_QSEL_BE_0", "Q_BK": "RTW89_TX_QSEL_BK_0",
               "Q_MGNT": "RTW89_TX_QSEL_B0_MGMT", "Q_HI": "RTW89_TX_QSEL_B0_HI"}
    checked = set()
    for header in ("RTL8852BE_hw.hpp", "RTL8852BE_linux_regs.hpp"):
        local = definitions((ROOT / "src" / header).read_text())
        for name in local:
            other = aliases.get(name, name)
            if other not in upstream:
                continue
            expected = value(other, upstream)
            if name.endswith("_SHIFT"):
                expected = (expected & -expected).bit_length() - 1
            if value(name, local) != expected:
                raise ValueError(f"Register mismatch: {header}:{name}: repo={value(name, local):#x}, Linux={expected:#x}")
            checked.add(name)
    repo_tables = (ROOT / "src/RTL8852BE_tables.cpp").read_text()
    kernel_tables = (args.linux_source / RTW89 / "rtw8852b_table.c").read_text()
    for ours, theirs in (("BB_REGS", "bb_regs"), ("BB_REG_GAIN", "bb_reg_gain"),
                         ("RADIOA_REGS", "radioa_regs"), ("RADIOB_REGS", "radiob_regs"),
                         ("NCTL_REGS", "nctl_regs")):
        actual = pairs(repo_tables, "RTL8852BE_PHY_" + ours)
        expected = pairs(kernel_tables, "rtw89_8852b_phy_" + theirs)
        if actual != expected:
            raise ValueError(f"Table differs from Linux: {ours}")
        print(f"{ours}: all {len(actual)} pairs match Linux {manifest['ref']}")
    print(f"Verified {len(manifest['files'])} source hashes and {len(checked)} register/mask/queue definitions.")


if __name__ == "__main__":
    main()
