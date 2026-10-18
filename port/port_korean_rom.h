#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* This translation retains JP gameplay/data addresses but replaces the text
 * decoder, glyph bank table, stylized lookup and full-width threshold.
 * Addresses and behavior were checked against its local ARM routines.
 * No game assets are compiled into this header. */
#define PORT_KOREAN_ROM_SIZE 15365627u
#define PORT_KOREAN_ROM_SHA256 "70fd38bd851ab48034fdab04be13ced3ef8a5d95344318ddc98d3d5417830b90"
#define PORT_KOREAN_FONT_TABLE 0xDC9F00u
#define PORT_KOREAN_STYLE_TABLE 0xE4F000u
#define PORT_KOREAN_TRANSLATION 0xE60000u

extern int gPortKoreanRom;

static inline uint32_t Port_KoreanRead32(const uint8_t* p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* Structural recognition for the native loader. The 3DS frontend additionally
 * verifies the full SHA-256 before accepting this JP-header ROM. */
static inline int Port_KoreanRomHasLayout(const uint8_t* rom, size_t size) {
    return rom && size == PORT_KOREAN_ROM_SIZE &&
           memcmp(rom + 0xAC, "BZMJ", 4) == 0 &&
           Port_KoreanRead32(rom + 0x108ED8) == 0x08000000u + PORT_KOREAN_TRANSLATION &&
           Port_KoreanRead32(rom + 0x5F104) == 0x08000000u + PORT_KOREAN_FONT_TABLE &&
           Port_KoreanRead32(rom + 0x5F804) == 0x08000000u + PORT_KOREAN_STYLE_TABLE;
}

/* 0B..0E select banks 4..7. 0F uses a second escape byte:
 * 00..2F = bank 2, 30..3F = bank 3, 40..FF = bank 9+ plus
 * one more byte. The ROM masks the bank nibble at glyph lookup. */
static inline uint32_t Port_KoreanDecodeGlyph(uint32_t lead, uint32_t second, uint32_t third) {
    if (lead >= 0x0B && lead <= 0x0E)
        return (((lead & 7u) + 1u) << 8) | second;
    if (second <= 0x2F) return 0x200u | second;
    if (second <= 0x3F) return 0x300u | (second & 0xFu);
    return (((second & 0xFu) + 9u) << 8) | third;
}

static inline int Port_KoreanGlyphIsWide(uint32_t code) {
    return (code >> 8) > 2;
}

static inline uint32_t Port_KoreanGlyphOffset(const uint8_t* rom, size_t size,
                                             uint32_t code, uint32_t language) {
    uint32_t bank = (code >> 8) & 0xFu;
    uint32_t cell = code & 0xFFu;
    if (bank <= 1 && cell > 0x7F && language != 0) {
        cell -= 0x80;
        bank = 2;
    }
    if (bank >= 3) cell *= 2;
    if (!rom || size < PORT_KOREAN_FONT_TABLE + 16u * 4u) return UINT32_MAX;
    uint32_t address = Port_KoreanRead32(rom + PORT_KOREAN_FONT_TABLE + bank * 4u);
    if (address < 0x08000000u) return UINT32_MAX;
    uint32_t offset = address - 0x08000000u + cell * 64u;
    size_t bytes = Port_KoreanGlyphIsWide(code) ? 128u : 64u;
    if (offset > size || bytes > size - offset) return UINT32_MAX;
    return offset;
}
