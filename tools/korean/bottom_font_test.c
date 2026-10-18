/* Exercise the production font selection and button compositor. No game
 * data is embedded; optional local ROM input generates a visual preview. */
#include "port/port_second_screen_theme.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int gPortKoreanRom;
int gActiveRegion;
u8* gRomData;
u32 gRomSize;
void* gUnk_08109248[9];

static u8 small[256 * 64], big[256 * 128];
static uint32_t plate[24 * 18], pixels[320 * 48];
static const uint32_t ink = 0xFF443322u, paper = 0xFFF4F8F8u;

static unsigned InkPixels(void) {
    unsigned result = 0;
    for (unsigned i = 0; i < sizeof(pixels) / sizeof(*pixels); ++i)
        result += pixels[i] == ink;
    return result;
}

static void MakeGlyph(u8* glyph) {
    glyph[0] = 0; /* two ink columns followed by six metric padding columns */
    for (unsigned y = 1; y < 12; ++y) glyph[y * 4] = 0xEE;
}

static void Setup(void) {
    memset(small, 0xFF, sizeof(small)); memset(big, 0xFF, sizeof(big));
    for (unsigned c = 'A'; c <= 'Z'; ++c) {
        MakeGlyph(small + c * 64); MakeGlyph(big + c * 128);
    }
    gUnk_08109248[0] = small; gUnk_08109248[8] = big;
    for (unsigned i = 0; i < sizeof(plate) / sizeof(*plate); ++i) plate[i] = paper;
    sBtnPlate = (SecondScreenThemeSprite){plate, 24, 18};
    sBtnW = 24; sBtnH = 18; sBtnCleanCol = 12; sBtnCleanRow = 9;
    sBuilt = sBtnOk = 1;
    for (unsigned style = 0; style < SS_TEXT_STYLE_COUNT; ++style) {
        for (unsigned i = 1; i < 15; ++i) sBigPal[style][i] = sBtnPal[i] = ink;
        sTextLut[style][14] = 14; sTextLut[style][30] = 14 << 4;
    }
    sMsgPal[14] = ink;
}

int main(int argc, char** argv) {
    Setup();
    for (int region = TMC_REGION_USA; region <= TMC_REGION_JP; ++region) {
        gActiveRegion = region; gPortKoreanRom = 0;
        SelectLatinFontBanks();
        assert(sFontOk == (region != TMC_REGION_JP));
        assert(sBigFontOk == (region != TMC_REGION_JP));
    }
    gPortKoreanRom = 1;
    SelectLatinFontBanks();
    assert(sFontOk && sBigFontOk);
    assert(Port_SecondScreenTheme_TextWidth("QUEST", 1) > 0);
    assert(Port_SecondScreenTheme_BigTextWidth("QUEST", 1) > 0);
    assert(Port_SecondScreenTheme_DrawText(pixels, 320, 48, 320, 0, 0, 1, SS_TEXT_INK, "QUEST") > 0);
    assert(InkPixels() > 0);
    memset(pixels, 0, sizeof(pixels));
    assert(Port_SecondScreenTheme_DrawMenuButton(pixels, 320, 48, 320, 0, 0, 96, 25, "QUEST", 0));
    assert(InkPixels() > 0);
    gPortKoreanRom = 0;
    SelectLatinFontBanks();
    memset(pixels, 0, sizeof(pixels));
    assert(!Port_SecondScreenTheme_DrawMenuButton(pixels, 320, 48, 320, 0, 0, 96, 25, "QUEST", 0));
    for (unsigned i = 0; i < sizeof(pixels) / sizeof(*pixels); ++i) assert(pixels[i] == 0);
    assert(Port_SecondScreenTheme_DrawMenuButton(pixels, 320, 48, 320, 0, 0, 25, 25, "", 0));

    if (argc == 3) {
        FILE* f = fopen(argv[1], "rb"); assert(f);
        fseek(f, 0, SEEK_END); gRomSize = (u32)ftell(f); rewind(f);
        gRomData = malloc(gRomSize); assert(gRomData);
        assert(fread(gRomData, 1, gRomSize, f) == gRomSize); fclose(f);
        assert(Port_KoreanRomHasLayout(gRomData, gRomSize));
        for (unsigned i = 0; i < 9; ++i)
            gUnk_08109248[i] = gRomData + Port_ReadU32(gRomData + 0x108F0C + i * 4) - 0x08000000u;
        gPortKoreanRom = 1; SelectLatinFontBanks();
        for (unsigned i = 0; i < sizeof(pixels) / sizeof(*pixels); ++i) pixels[i] = paper;
        const char* labels[] = {"QUEST", "MAP", "ITEMS"};
        for (unsigned i = 0; i < 3; ++i) {
            unsigned before = InkPixels();
            assert(Port_SecondScreenTheme_DrawMenuButton(pixels, 320, 48, 320, 4 + i * 106, 8, 98, 28, labels[i], 0));
            assert(InkPixels() > before);
        }
        f = fopen(argv[2], "wb"); assert(f); fprintf(f, "P6\n320 48\n255\n");
        for (unsigned i = 0; i < sizeof(pixels) / sizeof(*pixels); ++i) {
            fputc(pixels[i] & 255, f); fputc((pixels[i] >> 8) & 255, f); fputc((pixels[i] >> 16) & 255, f);
        }
        fclose(f); free(gRomData);
    }
    puts("PASS: Korean bottom font selection, visible labels and missing-font fallback");
    return 0;
}
