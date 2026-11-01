/* No ROM needed: run the actual decoder and nested-token logic over short
 * control-code fixtures, including the translation's result-only choices. */
#include "src/text.c"
#include <assert.h>

int gPortKoreanRom = 1;
int gActiveRegion = TMC_REGION_JP;
u8* gRomData;
u32 gRomSize;
u8 gEwram[0x40000];
u32* gTranslations[7];
u8* gTextVariableSources[5];
u32 gUnk_08109244;
u32* gUnk_08109248[9];
u8 gUnk_081092D4[346];
const void* port_resolve_copy_src(const void* p, u32 n) { return p; }
void Port_LogTextLookup(u32 lang, u32 text) {}
void MemCopy(const void* s, void* d, u32 n) { memcpy(d, s, n); }
void MemClear(void* p, u32 n) { memset(p, 0, n); }

int main(void) {
    for (int region = TMC_REGION_USA; region <= TMC_REGION_JP; ++region) {
        gActiveRegion = region;
        gPortKoreanRom = region == TMC_REGION_JP;
        for (unsigned value = 0; value < 256; ++value) {
            u8 stream[] = {5, value, 0x0C, 0x50, 0};
            Token token = {0};
            sub_0805EF40(&token, stream);
            assert(GetCharacter(&token) == 12);
            bool onlyResult = value == 255 || (gPortKoreanRom && value == 0);
            assert(token._6 == (onlyResult ? 0 : (value << 8) | 0x0C));
            assert(token.buf[0] == stream + (onlyResult ? 2 : 3));
            if (onlyResult) {
                assert(GetCharacter(&token) == (gPortKoreanRom ? 0x550 : 0x750));
                assert(token.buf[0] == stream + 4);
            }
        }
    }
    puts("PASS: 768 production choice decodes preserve Hangul bytes, full text targets and USA/EU controls");
    return 0;
}
