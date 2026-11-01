/* Production dialogue state machine; deterministic glyph/drawing fixtures. */
#include "src/message.c"
#include <assert.h>

Message gMessage;
TextRender gTextRender;
SaveFile gSave;
u8 gEwram[0x40000];
struct_02000040 gUnk_02000040;
Input gInput;
u8 gTextGfxBuffer[0xD00];
int gPortKoreanRom = 1;
int gActiveRegion = TMC_REGION_JP;
static unsigned reads, drawn, centeredWidth;
static u32 nextChar = 0x401;
static u32 drawnCodes[128];
static const u32* stream;
u32 GetCharacter(Token* t) {
    if (t == &gTextRender.curToken) ++reads;
    return stream ? stream[t->_6++] : nextChar;
}
u32 Port_TextGlyphWidth(u32 c, const WStruct* w) { return c == 0x120 ? 4 : c >= 0x300 ? 16 : 8; }
u32 sub_0805F7DC(u32 c, WStruct* w) {
    u32 width = Port_TextGlyphWidth(c, w);
    assert(w->unk6 + width <= w->unk4);
    assert(drawn < 128);
    drawnCodes[drawn++] = c;
    w->unk6 += width;
    return width;
}
void sub_0805F8E4(u32 c, WStruct* w) {}
u32 GetFontStrWith(Token* t, u32 n) { return centeredWidth; }
void WriteBit(u32* p, u32 b) { *p |= 1u << b; }
void SoundReq(u32 s) {}
void MemClear(void* p, u32 n) { memset(p, 0, n); }
/* GCC's ASan retains the message dispatch table. Its unused initialization
 * handlers must link too, and must fail loudly if the fixture enters them. */
void MemCopy(const void* s, void* d, u32 n) { memcpy(d, s, n); }
u32 DecToHex(u32 n) { abort(); }
void sub_0805EEB4(Token* t, u32 id) { abort(); }
bool32 sub_0805EF40(Token* t, const u8* s) { abort(); }
void MemFill32(u32 v, void* p, u32 n) { memset(p, 0xff, n); }
void LoadResourceAsync(const void* s, void* d, u32 n) {}
void sub_0805F918(u32 a, u32 b, void* p) {}
bool Port_Config_GetHoldToAdvanceText(void) { return false; }
bool Port_TTS_GetEnabled(void) { return false; }
void Port_TTS_Speak(const char* text, const PortTtsOptions* options) {}

static void Reset(void) {
    memset(&gTextRender, 0, sizeof(gTextRender));
    memset(&gMessageChoices, 0, sizeof(gMessageChoices));
    memset(&gUnk_02000040, 0, sizeof(gUnk_02000040));
    gTextRender._98.bytes.b1 = 2;
    gTextRender.renderStatus = RENDER_UPDATE;
    gTextRender._50.unk4 = 208;
    gTextRender._97 = 1;
    reads = drawn = 0;
    nextChar = 0x401;
    stream = NULL;
}

int main(void) {
    Reset();
    /* 65 wide characters: repeated automatic lines/pages, no byte rewinds,
     * no dropped/duplicated characters, no half-glyph clipping. */
    for (unsigned i = 0; i < 65; ++i) {
        unsigned before = drawn;
        while (drawn == before) {
            RunTextCommand(&gTextRender);
            if (gTextRender.renderStatus == RENDER_WAIT) TextDispRoll(&gTextRender);
        }
    }
    assert(reads == 65 && drawn == 65);
    for (unsigned i = 0; i < drawn; ++i) assert(drawnCodes[i] == 0x401);
    Reset();
    /* A three-syllable word (the player's Tabanta example) cannot start in
     * a one-glyph gap. Keep the entire word on the next line/page. */
    static const u32 word[] = {0xCCD, 0x78A, 0xCCD, 0x120, 0};
    stream = word;
    gTextRender._50.unk6 = 192;
    RunTextCommand(&gTextRender);
    assert(drawn == 0 && reads == 1 && gTextRender._98.bytes.lineNo == 1);
    for (unsigned i = 0; i < 3; ++i) RunTextCommand(&gTextRender);
    assert(drawn == 3 && reads == 3 && gTextRender._50.unk6 == 256);
    for (unsigned i = 0; i < 3; ++i) assert(drawnCodes[i] == word[i]);
    Reset();
    stream = word;
    gTextRender._98.bytes.lineNo = 1;
    gTextRender._50.unk4 = 416;
    gTextRender._50.unk6 = 400;
    RunTextCommand(&gTextRender);
    assert(drawn == 0 && reads == 1 && gTextRender.renderStatus == RENDER_WAIT);
    TextDispRoll(&gTextRender);
    RunTextCommand(&gTextRender);
    assert(drawn == 1 && reads == 1 && drawnCodes[0] == word[0]);
    Reset();
    nextChar = 0x120;
    gTextRender._50.unk6 = 208;
    RunTextCommand(&gTextRender);
    assert(drawn == 0 && gTextRender._50.unk6 == 208 && gTextRender._97);
    Reset();
    stream = word;
    gUnk_02000040.unk_00 = 1;
    gTextRender._50.unk6 = 192;
    RunTextCommand(&gTextRender);
    assert(drawn == 1 && gTextRender._98.bytes.lineNo == 0);
    Reset();
    gTextRender._50.unk6 = 207;
    RunTextCommand(&gTextRender);
    assert(drawn == 0 && reads == 1 && gTextRender.curToken.extended == 0x401);
    assert(gTextRender._50.unk6 == 208 && gTextRender._98.bytes.lineNo == 1);
    RunTextCommand(&gTextRender);
    assert(drawn == 1 && reads == 1 && gTextRender._50.unk6 == 224);
    /* A token that was already pending (e.g. after opening the box) must
     * survive a page break without calling GetCharacter again. */
    gTextRender._50.unk6 = 416;
    gTextRender.curToken.extended = 0x402;
    RunTextCommand(&gTextRender);
    assert(gTextRender.renderStatus == RENDER_WAIT && reads == 1);
    TextDispRoll(&gTextRender);
    RunTextCommand(&gTextRender);
    assert(drawnCodes[1] == 0x402 && reads == 1);
    Reset();
    nextChar = 4; centeredWidth = 400;
    RunTextCommand(&gTextRender);
    assert(gTextRender._50.unk6 == 0);
    centeredWidth = 100;
    RunTextCommand(&gTextRender);
    assert(gTextRender._50.unk6 == 54);
    Reset();
    nextChar = 1;
    RunTextCommand(&gTextRender);
    assert(gTextRender._98.bytes.lineNo == 1);
    RunTextCommand(&gTextRender);
    assert(gTextRender.renderStatus == RENDER_WAIT);
    Reset();
    nextChar = 12; gTextRender.curToken._6 = 0x1234;
    RunTextCommand(&gTextRender);
    assert(gMessageChoices.choiceCount == 1 && gMessageChoices.unk_10[0] == 0x1234);
    assert(gMessageChoices.unk_08[0] == 0 && gTextRender._50.unk6 == 8);
    puts("PASS: production Korean word/page wrapping, pending glyphs, centering, authored newlines and choice controls");
    return 0;
}
