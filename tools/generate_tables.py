# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Issen Software Group

import urllib.request
import re
import os

def main():
    print("Fetching rtw8852b_table.c from Linux kernel tree...")
    url = "https://raw.githubusercontent.com/torvalds/linux/master/drivers/net/wireless/realtek/rtw89/rtw8852b_table.c"
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    content = urllib.request.urlopen(req).read().decode("utf-8")

    tables = {
        "RTL8852BE_PHY_BB_REGS": "rtw89_8852b_phy_bb_regs",
        "RTL8852BE_PHY_BB_REG_GAIN": "rtw89_8852b_phy_bb_reg_gain",
        "RTL8852BE_PHY_RADIOA_REGS": "rtw89_8852b_phy_radioa_regs",
        "RTL8852BE_PHY_RADIOB_REGS": "rtw89_8852b_phy_radiob_regs",
        "RTL8852BE_PHY_NCTL_REGS": "rtw89_8852b_phy_nctl_regs"
    }

    hpp_lines = [
        "// SPDX-License-Identifier: MIT",
        "// Copyright (c) 2026 Issen Software Group",
        "",
        "#ifndef RTL8852BE_TABLES_HPP",
        "#define RTL8852BE_TABLES_HPP",
        "",
        "#include <stdint.h>",
        "#include <stddef.h>",
        "",
        "struct Rtw89Reg2Def {",
        "    uint32_t addr;",
        "    uint32_t val;",
        "};",
        ""
    ]

    cpp_lines = [
        "// SPDX-License-Identifier: MIT",
        "// Copyright (c) 2026 Issen Software Group",
        "",
        '#include "RTL8852BE_tables.hpp"',
        ""
    ]

    for sym, ksym in tables.items():
        idx = content.find(ksym)
        end = content.find("};", idx)
        block = content[idx:end]
        pairs = re.findall(r"\{\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)\s*\}", block)
        print(f"{sym}: parsed {len(pairs)} pairs")
        hpp_lines.append(f"extern const Rtw89Reg2Def {sym}[{len(pairs)}];")
        hpp_lines.append(f"extern const size_t {sym}_COUNT;")
        hpp_lines.append("")

        cpp_lines.append(f"const size_t {sym}_COUNT = {len(pairs)};")
        cpp_lines.append(f"const Rtw89Reg2Def {sym}[{len(pairs)}] = {{")
        for addr, val in pairs:
            cpp_lines.append(f"    {{{addr}, {val}}},")
        cpp_lines.append("};")
        cpp_lines.append("")

    hpp_lines.append("#endif // RTL8852BE_TABLES_HPP\n")
    cpp_lines.append("")

    os.makedirs("src", exist_ok=True)
    with open("src/RTL8852BE_tables.hpp", "w", encoding="utf-8") as f:
        f.write("\n".join(hpp_lines))

    with open("src/RTL8852BE_tables.cpp", "w", encoding="utf-8") as f:
        f.write("\n".join(cpp_lines))

    print("Tables generated successfully in src/!")

if __name__ == "__main__":
    main()
