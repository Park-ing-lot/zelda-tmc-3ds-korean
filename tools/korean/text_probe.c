/* Local differential-test harness for the production text implementation.
 * ROM assets are supplied at runtime and never embedded in this executable. */
#include "src/text.c"
#include <stdlib.h>

int gPortKoreanRom = 1;
int gActiveRegion = TMC_REGION_JP;
u8* gRomData;
u32 gRomSize;
u8 gEwram[0x40000], gIwram[0x8000], gIoMem[0x400], gVram[0x18000];
u16 gBgPltt[256], gObjPltt[256], gOamMem[512];
u32* gTranslations[7];
u8* gTextVariableSources[5];
u32 gUnk_08109244;
u32* gUnk_08109248[9];
u8 gUnk_081092D4[346];

const void* port_resolve_copy_src(const void* p, u32 n) { (void)n; return p; }
void Port_LogTextLookup(u32 lang, u32 text) { (void)lang; (void)text; }
void MemClear(void* p, u32 n) { memset(p, 0, n); }
void MemCopy(const void* src, void* dst, u32 n) { memcpy(dst, src, n); }

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    FILE* f = fopen(argv[1], "rb");
    if (!f) return 2;
    fseek(f, 0, SEEK_END); gRomSize = (u32)ftell(f); rewind(f);
    gRomData = malloc(gRomSize);
    if (!gRomData || fread(gRomData, 1, gRomSize, f) != gRomSize) return 2;
    fclose(f);
    if (!Port_KoreanRomHasLayout(gRomData, gRomSize)) return 3;
    memcpy(gUnk_081092D4, gRomData + PORT_KOREAN_STYLE_TABLE, sizeof(gUnk_081092D4));
    char mode;
    unsigned a, b, c, style;
    while (scanf(" %c %x %x %x %x", &mode, &a, &b, &c, &style) == 5) {
        gSaveHeader->language = (u8)style;
        if (mode == 'D') {
            gSaveHeader->language = (u8)(style >> 8);
            u8 bytes[] = {(u8)a, (u8)b, (u8)c, 0};
            Token token = {0};
            token.unk05 = style;
            sub_0805EF40(&token, bytes);
            unsigned code = GetCharacter(&token);
            printf("%x %x %x\n", code, (unsigned)(token.buf[0] - bytes), token.extended);
        } else if (mode == 'G') {
            u32* glyph = sub_0805F25C(a);
            printf("%x\n", (unsigned)((u8*)glyph - gRomData));
        } else if (mode == 'S') {
            printf("%x\n", sub_0805F9A0(a));
        } else if (mode == 'W') {
            u8 bytes[] = {(u8)a, (u8)b, (u8)c, 0};
            Token token = {0};
            sub_0805EF40(&token, bytes);
            printf("%x\n", GetFontStrWith(&token, 0));
        } else return 4;
    }
    free(gRomData);
    return 0;
}
