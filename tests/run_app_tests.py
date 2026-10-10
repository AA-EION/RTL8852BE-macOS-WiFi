#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Issen Software Group
"""Run native app regressions with bounded runtime and useful hang diagnostics."""
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
process = subprocess.Popen([str(root / "build/test_app")], cwd=root)
try:
    result = process.wait(timeout=30)
except subprocess.TimeoutExpired:
    print("App tests timed out; collecting macOS thread stacks.", flush=True)
    try:
        subprocess.run(["sample", str(process.pid), "2"], timeout=10, check=False)
    finally:
        process.kill()
        process.wait()
    result = 1
raise SystemExit(result)
