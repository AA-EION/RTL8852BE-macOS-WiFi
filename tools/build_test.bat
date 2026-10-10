@echo off
where cl >nul 2>&1
if %errorlevel% neq 0 (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    )
)
if not exist build mkdir build
echo [*] Compiling test_hardware_contract...
cl /nologo /std:c++17 /EHsc /O2 /Isrc tests/test_hardware_contract.cpp src/RTL8852BE_tables.cpp src/RTL8852BE_firmware.cpp /Fe:build/test_hardware_contract.exe
if errorlevel 1 goto error
echo [*] Running build\test_hardware_contract.exe...
build\test_hardware_contract.exe
if errorlevel 1 goto error

echo.
echo [*] Compiling test_crypto_bounds...
cl /nologo /std:c++17 /EHsc /O2 /Isrc tests/test_crypto_bounds.cpp src/RTL8852BE_crypto.cpp /Fe:build/test_crypto_bounds.exe
if errorlevel 1 goto error
echo [*] Running build\test_crypto_bounds.exe...
build\test_crypto_bounds.exe
if errorlevel 1 goto error

echo.
echo [*] Compiling test_runner...
cl /nologo /std:c++17 /EHsc /O2 /Isrc /Itests /Itests/mock ^
   tests/mock/mock_crypto.cpp ^
   tests/mock/mock_pci_mmio.cpp ^
   tests/mock/mock_dma.cpp ^
   tests/mock/mock_packet_injector.cpp ^
   tests/mock/simulated_device.cpp ^
   tests/tier1_features/test_firmware_loader.cpp ^
   tests/tier1_features/test_efuse_parser.cpp ^
   tests/tier1_features/test_dma_ring.cpp ^
   tests/tier1_features/test_beacon_parser.cpp ^
   tests/tier1_features/test_wpa2_handshake.cpp ^
   tests/tier1_features/test_ccmp_crypto.cpp ^
   tests/tier2_boundaries/test_boundaries.cpp ^
   tests/tier3_pairwise/test_pairwise.cpp ^
   tests/tier4_workloads/test_workloads.cpp ^
   tests/test_runner.cpp ^
   /Fe:build/test_runner.exe
if errorlevel 1 goto error
echo [*] Running build\test_runner.exe...
build\test_runner.exe
if errorlevel 1 goto error

echo.
echo [*] Running python tests\test_boot_config.py...
python tests\test_boot_config.py
if errorlevel 1 goto error

echo.
echo [+] All Contract, Crypto, Mock Suite, and Boot Config Tests Passed Successfully!
exit /b 0

:error
echo [!] Test compilation or execution failed with code %errorlevel%!
exit /b 1
