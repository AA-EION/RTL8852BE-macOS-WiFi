# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Issen Software Group

SDK_PATH ?= $(shell xcrun --sdk macosx --show-sdk-path 2>/dev/null || echo "")
MACOSX_VERSION_MIN ?= 14.0
CXX ?= clang++
CC ?= clang

KEXT_CXXFLAGS := -arch x86_64 -mmacosx-version-min=$(MACOSX_VERSION_MIN) -mkernel -fno-builtin -fno-common \
                 -fno-exceptions -fno-rtti -nostdinc -std=c++17 -O2 \
                 -DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE -DAPPLE -DNeXT \
                 -Wno-deprecated-declarations \
                 -isysroot "$(SDK_PATH)" \
                 -I"$(SDK_PATH)/System/Library/Frameworks/Kernel.framework/Headers" \
                 -Isrc -MMD -MP

KEXT_CFLAGS   := -arch x86_64 -mmacosx-version-min=$(MACOSX_VERSION_MIN) -mkernel -fno-builtin -fno-common \
                 -nostdinc -O2 \
                 -DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE -DAPPLE -DNeXT \
                 -isysroot "$(SDK_PATH)" \
                 -I"$(SDK_PATH)/System/Library/Frameworks/Kernel.framework/Headers"

KEXT_LDFLAGS  := -arch x86_64 -mmacosx-version-min=$(MACOSX_VERSION_MIN) -Wl,-kext -nostdlib \
                 -lkmodc++ -lkmod -lcc_kext -isysroot "$(SDK_PATH)"

KEXT_BUNDLE   := build/RTL8852BEWiFi.kext
KEXT_BIN      := $(KEXT_BUNDLE)/Contents/MacOS/RTL8852BEWiFi
CLI_BIN       := tools/rtl8852be_cli

KEXT_OBJS     := build/RTL8852BE.o \
                 build/RTL8852BE_efuse.o \
                 build/RTL8852BE_phy.o \
                 build/RTL8852BE_power.o \
                 build/RTL8852BEUserClient.o \
                 build/RTL8852BE_crypto.o \
                 build/RTL8852BE_firmware.o \
                 build/RTL8852BE_firmware_parser.o \
                 build/RTL8852BE_tables.o \
                 build/RTL8852BE_kmod.o

-include $(KEXT_OBJS:.o=.d)

.DEFAULT_GOAL := all
.PHONY: all kext cli app test test-app test-hardware test-crypto test-firmware test-protocol test-bringup test-boot-config clean

all: kext cli

app:
	bash scripts/build_app.sh

test-app:
	@mkdir -p build
	swiftc -parse-as-library -swift-version 5 -target $(shell uname -m)-apple-macosx14.0 \
		-import-objc-header gui/RTL8852BEBridge.h -framework AppKit -framework IOKit \
		gui/Sources/AppModel.swift gui/Sources/DriverClient.swift gui/Sources/Vendor.swift \
		tests/test_app.swift -o build/test_app
	python3 tests/run_app_tests.py

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(KEXT_CXXFLAGS) -c $< -o $@

build/RTL8852BE_kmod.o: src/RTL8852BE_kmod.c
	@mkdir -p build
	$(CC) $(KEXT_CFLAGS) -c $< -o $@

kext: $(KEXT_OBJS) src/Info.plist
	@mkdir -p $(KEXT_BUNDLE)/Contents/MacOS
	$(CXX) $(KEXT_LDFLAGS) $(KEXT_OBJS) -o $(KEXT_BIN)
	cp src/Info.plist $(KEXT_BUNDLE)/Contents/Info.plist
	codesign --force --deep --sign - $(KEXT_BUNDLE)

cli: tools/rtl8852be_cli.cpp src/RTL8852BE_ipc.h src/RTL8852BE_protocol.hpp
	$(CXX) -arch x86_64 -std=c++17 -O2 -framework IOKit -framework CoreFoundation \
		tools/rtl8852be_cli.cpp -o $(CLI_BIN)

test: test-hardware test-crypto test-firmware test-protocol test-bringup test-boot-config
	$(MAKE) -C tests test

test-boot-config:
	python3 tests/test_boot_config.py

test-hardware:
	@mkdir -p build
	$(CXX) -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Isrc tests/test_hardware_contract.cpp src/RTL8852BE_tables.cpp src/RTL8852BE_firmware.cpp src/RTL8852BE_firmware_parser.cpp -o build/test_hardware_contract
	./build/test_hardware_contract

test-crypto:
	@mkdir -p build
	$(CXX) -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Isrc tests/test_crypto_bounds.cpp src/RTL8852BE_crypto.cpp -o build/test_crypto_bounds
	./build/test_crypto_bounds

test-firmware:
	@mkdir -p build
	$(CXX) -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Isrc tests/test_firmware_parser.cpp src/RTL8852BE_firmware.cpp src/RTL8852BE_firmware_parser.cpp -o build/test_firmware_parser
	./build/test_firmware_parser

test-protocol:
	@mkdir -p build
	$(CXX) -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Isrc tests/test_protocol_bounds.cpp -o build/test_protocol_bounds
	./build/test_protocol_bounds

test-bringup:
	@mkdir -p build
	$(CXX) -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Isrc tests/test_bringup.cpp src/RTL8852BE_efuse.cpp src/RTL8852BE_phy.cpp src/RTL8852BE_power.cpp src/RTL8852BE_tables.cpp -o build/test_bringup
	./build/test_bringup

clean:
	$(MAKE) -C tests clean
	rm -rf build $(CLI_BIN)
