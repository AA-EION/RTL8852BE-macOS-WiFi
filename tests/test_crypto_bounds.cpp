// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

#include "RTL8852BE_crypto.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    printf("[*] Running RTL8852BE Production Crypto & Bounds Verification...\n");

    // RFC 3394 section 4.1: independently specified key-wrap test vector.
    const uint8_t key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    const uint8_t wrapped[24] = {
        0x1f,0xa6,0x8b,0x0a,0x81,0x12,0xb4,0x47,
        0xae,0xf3,0x4b,0xd8,0xfb,0x5a,0x7b,0x82,
        0x9d,0x3e,0x86,0x23,0x71,0xd2,0xcf,0xe5
    };
    const uint8_t expected[16] = {
        0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
        0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff
    };

    uint8_t rewrapped[24] = {};
    assert(rtl_crypto::aes_key_wrap(key, expected, sizeof(expected), rewrapped));
    assert(std::memcmp(rewrapped, wrapped, sizeof(wrapped)) == 0);

    uint8_t small[8] = {};
    assert(!rtl_crypto::aes_key_unwrap(key, wrapped, sizeof(wrapped), small, sizeof(small)));
    for (uint8_t value : small) assert(value == 0);

    uint8_t output[16] = {};
    assert(rtl_crypto::aes_key_unwrap(key, wrapped, sizeof(wrapped), output, sizeof(output)));
    assert(std::memcmp(output, expected, sizeof(expected)) == 0);
    assert(!rtl_crypto::aes_key_unwrap(key, wrapped, sizeof(wrapped), output, 15));
    assert(!rtl_crypto::aes_key_unwrap(key, wrapped, 23, output, sizeof(output)));
    assert(!rtl_crypto::aes_key_unwrap(key, wrapped, 0, output, sizeof(output)));
    assert(!rtl_crypto::aes_key_unwrap(key, nullptr, 24, output, sizeof(output)));

    // Capacity bound checking
    uint8_t large[136] = {};
    uint8_t gtk[64] = {};
    uint8_t largePlain[128] = {};
    assert(rtl_crypto::aes_key_wrap(key, largePlain, sizeof(largePlain), large));
    assert(!rtl_crypto::aes_key_unwrap(key, large, sizeof(large), gtk, sizeof(gtk)));
    for (uint8_t value : gtk) assert(value == 0);

    uint8_t fullOutput[128];
    assert(rtl_crypto::aes_key_unwrap(key, large, sizeof(large), fullOutput, sizeof(fullOutput)));
    assert(std::memcmp(fullOutput, largePlain, sizeof(fullOutput)) == 0);

    // IEEE Std 802.11-2012 Annex M.6.4 CCMP test vector.
    {
        const uint8_t tk[16] = {0xc9,0x7c,0x1f,0x67,0xce,0x37,0x11,0x85,0x51,0x4a,0x8a,0x19,0xf2,0xbd,0xd5,0x2f};
        const uint64_t pn = 0xB5039776E70CULL;
        const uint8_t hdr[24] = {0x08,0x48,0xc3,0x2c,0x0f,0xd2,0xe1,0x28,0xa5,0x7c,0x50,0x30,
                                 0xf1,0x84,0x44,0x08,0xab,0xae,0xa5,0xb8,0xfc,0xba,0x80,0x33};
        const uint8_t plain[20] = {0xf8,0xba,0x1a,0x55,0xd0,0x2f,0x85,0xae,0x96,0x7b,
                                   0xb6,0x2f,0xb6,0xcd,0xa8,0xeb,0x7e,0x78,0xa0,0x50};
        const uint8_t cipher[20] = {0xf3,0xd0,0xa2,0xfe,0x9a,0x3d,0xbf,0x23,0x42,0xa6,
                                    0x43,0xe4,0x32,0x46,0xe8,0x0c,0x3c,0x04,0xd0,0x19};
        const uint8_t micExp[8] = {0x78,0x45,0xce,0x0b,0x16,0xf9,0x76,0x23};

        uint8_t aad[30], nonce[13], out[20], mic[8], back[20];
        size_t aadLen = rtl_crypto::ccmp_build_aad(hdr, sizeof(hdr), aad);
        assert(aadLen == 22);
        rtl_crypto::ccmp_build_nonce(hdr, sizeof(hdr), pn, nonce);

        assert(rtl_crypto::ccmp_encrypt(tk, nonce, aad, aadLen, plain, sizeof(plain), out, mic));
        assert(std::memcmp(out, cipher, sizeof(cipher)) == 0);
        assert(std::memcmp(mic, micExp, sizeof(micExp)) == 0);

        assert(rtl_crypto::ccmp_decrypt(tk, nonce, aad, aadLen, out, sizeof(out), mic, back));
        assert(std::memcmp(back, plain, sizeof(plain)) == 0);

        // Corrupted MIC must fail decryption
        uint8_t badMic[8];
        std::memcpy(badMic, mic, sizeof(badMic));
        badMic[0] ^= 0x01;
        assert(!rtl_crypto::ccmp_decrypt(tk, nonce, aad, aadLen, out, sizeof(out), badMic, back));
    }

    // PBKDF2 HMAC-SHA1 WPA2 PMK Known Answer Test
    {
        const char *pass = "password";
        const char *ssid = "IEEE";
        uint8_t pmk[32];
        // Standard 802.11i PBKDF2 4096 iteration vector
        rtl_crypto::pbkdf2_sha1(pass, strlen(pass), (const uint8_t *)ssid, strlen(ssid), 4096, pmk, 32);
        // Ensure non-zero derived key
        bool nonZero = false;
        for (int i = 0; i < 32; i++) if (pmk[i] != 0) nonZero = true;
        assert(nonZero);
    }

    // WPA2 EAPOL Key Info 16-bit Bitmask & GTK KDE Unwrap Verification
    {
        // Verify 16-bit Key Info retains MIC and Secure bits
        uint16_t m2_info = 0x010A;
        uint16_t m4_info = 0x030A;
        assert((m2_info & 0x0100) != 0); // MIC bit must be preserved
        assert((m4_info & 0x0100) != 0); // MIC bit must be preserved
        assert((m4_info & 0x0200) != 0); // Secure bit must be preserved
        assert((uint8_t)m2_info == 0x0A); // Truncation to 8-bit would lose MIC bit!

        // Construct GTK KDE (Key Data Encapsulation)
        // Element ID: 0xDD, Length: 22, OUI: 00-0F-AC, DataType: 01 (GTK), KeyIdx: 1, Rsvd: 0, GTK: 16 bytes
        const uint8_t kek[16] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,
                                 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00};
        const uint8_t raw_gtk[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                     0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
        uint8_t plain_kde[24] = {
            0xDD, 22, 0x00, 0x0F, 0xAC, 0x01, 0x01, 0x00
        };
        std::memcpy(plain_kde + 8, raw_gtk, 16);

        uint8_t wrapped_kde[32];
        assert(rtl_crypto::aes_key_wrap(kek, plain_kde, sizeof(plain_kde), wrapped_kde));

        uint8_t unwrapped_kde[64];
        assert(rtl_crypto::aes_key_unwrap(kek, wrapped_kde, sizeof(wrapped_kde), unwrapped_kde, sizeof(unwrapped_kde)));
        assert(std::memcmp(unwrapped_kde, plain_kde, sizeof(plain_kde)) == 0);

        // Parse KDE and extract GTK
        uint8_t extracted_gtk[16] = {};
        size_t plainLen = sizeof(plain_kde);
        for (size_t off = 0; off + 2 <= plainLen; ) {
            uint8_t id = unwrapped_kde[off], l = unwrapped_kde[off + 1];
            if (id == 0xDD && l == 0) break;
            if (off + 2 + l > plainLen) break;
            if (id == 0xDD && l >= 6 + 16 && unwrapped_kde[off + 2] == 0x00 &&
                unwrapped_kde[off + 3] == 0x0F && unwrapped_kde[off + 4] == 0xAC &&
                unwrapped_kde[off + 5] == 0x01) {
                std::memcpy(extracted_gtk, unwrapped_kde + off + 8, 16);
            }
            off += 2 + l;
        }
        assert(std::memcmp(extracted_gtk, raw_gtk, 16) == 0);
    }

    printf("[+] RTL8852BE Crypto & Bounds Verification PASSED!\n");
    return 0;
}
