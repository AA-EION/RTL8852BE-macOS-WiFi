# Audit checkpoint

Audit date: 2026-10-10. Baseline RTL8852BE commit: `af7c233`. Reference RTL8723BE commit: `4f28a94b837c073f98416d84f8a14e967ce8477e`.

**The driver is not complete or hardware-validated.** Prior COMPLETE labels and declarations that a 40-byte TXWD was verified against rtw89 were incorrect.

The current changes fix firmware container interpretation, register/eFuse constants, arithmetic and frame bounds, unsafe success/status reporting, generator reproducibility, startup containment, and misleading documentation. They add regressions that exercise production helpers.

The remaining implementation work and direct kernel references are recorded in [docs/AUDIT.md](docs/AUDIT.md). The source comparison verifies all five table arrays and 172 matching definitions. New production code implements upstream power/XTAL operations, physical/logical eFuse and conditional RF-table execution, with portable regressions; DMA operation still requires a complete transport and hardware evidence.

Portable tests pass under Clang ASan/UBSan, and all 57 legacy simulator cases pass. The RTL87-style app now uses an RTL88 driver-client adapter, with native model regressions in macOS CI. README, funding and author links match the reference while retaining accurate development status. GitHub CI has passed the complete app, native model tests, kext/CLI and development DMG builds (6c720db); CI checks each subsequent pushed revision. Neither portable nor native build tests establish hardware behavior. Neither repository was installed, and the RTL8723BE reference was left unchanged.
