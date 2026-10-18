#include "port_korean_rom.h"
#include <assert.h>
#include <stdlib.h>

static void write32(uint8_t* p, uint32_t x) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(x >> (i * 8));
}

int main(void) {
    uint8_t* rom = calloc(1, PORT_KOREAN_ROM_SIZE);
    assert(rom);
    assert(!Port_KoreanRomHasLayout(NULL, PORT_KOREAN_ROM_SIZE));
    for (size_t n = 0; n < 0xB0; ++n) assert(!Port_KoreanRomHasLayout(rom, n));
    assert(!Port_KoreanRomHasLayout(rom, PORT_KOREAN_ROM_SIZE));
    memcpy(rom + 0xAC, "BZMJ", 4);
    write32(rom + 0x108ED8, 0x08000000u + PORT_KOREAN_TRANSLATION);
    write32(rom + 0x5F104, 0x08000000u + PORT_KOREAN_FONT_TABLE);
    write32(rom + 0x5F804, 0x08000000u + PORT_KOREAN_STYLE_TABLE);
    assert(Port_KoreanRomHasLayout(rom, PORT_KOREAN_ROM_SIZE));
    assert(!Port_KoreanRomHasLayout(rom, PORT_KOREAN_ROM_SIZE - 1));
    rom[0xAF] = 'P';
    assert(!Port_KoreanRomHasLayout(rom, PORT_KOREAN_ROM_SIZE));
    rom[0xAF] = 'J';
    rom[0x5F104] ^= 1;
    assert(!Port_KoreanRomHasLayout(rom, PORT_KOREAN_ROM_SIZE));

    /* Invalid font pointers cannot make the native renderer read beyond ROM. */
    assert(Port_KoreanGlyphOffset(NULL, 0, 0x900, 0) == UINT32_MAX);
    assert(Port_KoreanGlyphOffset(rom, PORT_KOREAN_ROM_SIZE, 0x900, 0) == UINT32_MAX);
    write32(rom + PORT_KOREAN_FONT_TABLE + 9 * 4, 0x08000000u + PORT_KOREAN_ROM_SIZE - 64);
    assert(Port_KoreanGlyphOffset(rom, PORT_KOREAN_ROM_SIZE, 0x900, 0) == UINT32_MAX);
    write32(rom + PORT_KOREAN_FONT_TABLE + 9 * 4, 0x08000000u + PORT_KOREAN_ROM_SIZE - 128);
    assert(Port_KoreanGlyphOffset(rom, PORT_KOREAN_ROM_SIZE, 0x900, 0) == PORT_KOREAN_ROM_SIZE - 128);
    assert(Port_KoreanGlyphOffset(rom, PORT_KOREAN_ROM_SIZE, 0x901, 0) == UINT32_MAX);
    free(rom);
    return 0;
}
