/* Real RestoreGameTask and Crenel weather callback, with deterministic gfx
 * transfers in place of copyrighted assets. The VRAM assertion fails before
 * the layer-ownership fix: the restored ash map is overwritten with zeros. */
#include "game.h"
#include "main.h"
#include "room.h"
#include "screen.h"
#include "save.h"
#include "flags.h"
#include "manager/weatherChangeManager.h"
#include "manager/animatedBackgroundManager.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int gActiveRegion;
Screen gScreen;
RoomControls gRoomControls;
RoomTransition gRoomTransition;
RoomVars gRoomVars;
PlayerEntity gPlayerEntity;
MapLayer gMapBottom, gMapTop;
Area gArea;
SaveFile gSave;
LinkedList gEntityLists[9];
u8 gEwram[0x40000];
u8 gTextGfxBuffer[0xD00];
u16 gPaletteBuffer[0x200], gPalette_549[0x1A0];
u8 gPaletteBufferBackup[0x400];
u32 gUsedPalettes;
static u8 vram[0x10000];
static WeatherChangeManager weather;
static unsigned effectLayer;
void WeatherChangeManager_OnEnterRoom(WeatherChangeManager*);
void MemClear(void* p, u32 n) { memset(p, 0, n); }
void MemCopy(const void* s, void* d, u32 n) { memcpy(d, s, n); }
void LoadGfxGroup(u32 group) {
    if (group >= 43 && group <= 46) memset(vram + 0xE800, group, 0x800);
    if (group == 20) memset(vram + 0xF000, group, 0x800);
}
void LoadPaletteGroup(u32 n) {}
void SetColor(u32 a, u32 b) {}
u32 GetInventoryValue(u32 n) { return 0; }
void CleanUpGFXSlots(void) {}
void sub_080ADE24(void) {}
void InitUI(bool32 n) {}
/* GCC ASan also retains the unused timer callback table in gameUtils.c. */
bool32 CheckLocalFlagByBankB(u32 bank, u32 flag) { abort(); }
void ClearLocalFlagByBankB(u32 bank, u32 flag) { abort(); }
void ClearGlobalFlag(u32 flag) { abort(); }
void MenuFadeIn(u32 state, u32 param) { abort(); }
void LoadStaticBackground(u32 n) { LoadGfxGroup(20); }
void LoadResourceAsync(const void* s, void* d, u32 n) {
    assert((uintptr_t)d == 0x600E800);
    memcpy(vram + 0xE800, s, n);
}
void SoundReq(u32 n) {}
Entity* FindEntityByID(u32 k, u32 i, u32 n) { return NULL; }
void AnimatedBackgroundManager_RestoreBgGfx(AnimatedBackgroundManager* p) {}
void sub_0801AE44(bool32 n) {
    if (effectLayer == 1) WeatherChangeManager_OnEnterRoom(&weather);
    else if (effectLayer == 2) memset(vram + 0xD800, 0x7A, 0x800);
    /* Room renderer only rebuilds map-owned BGs, as the real routine does. */
    if (gMapTop.bgSettings) memset(gBG1Buffer, 0x31, sizeof(gBG1Buffer));
    if (gMapBottom.bgSettings) memset(gBG2Buffer, 0x42, sizeof(gBG2Buffer));
}
void sub_08016CA8(BgSettings* bg) {
    if (bg->updated && bg->subTileMap) {
        bg->updated = 0;
        memcpy(vram + ((bg->control << 3) & 0xF800), bg->subTileMap, 0x800);
    }
}

int main(void) {
    for (unsigned cycle = 0; cycle < 256; ++cycle) {
        memset(&gScreen, 0, sizeof(gScreen));
        gScreen.bg1.subTileMap = gBG1Buffer;
        gScreen.bg2.subTileMap = gBG2Buffer;
        gScreen.bg1.control = 29 << 8;
        gScreen.bg2.control = 27 << 8;
        memset(vram, 0xDD, sizeof(vram)); /* menu uses the same VRAM */
        effectLayer = cycle % 3;
        gMapTop.bgSettings = effectLayer == 1 ? NULL : &gScreen.bg1;
        gMapBottom.bgSettings = effectLayer == 2 ? NULL : (BgSettings*)&gScreen.bg2;
        memset(&weather, 0, sizeof(weather));
        weather.unk_22 = cycle % 4;
        weather.unk_21 = 1;
        weather.base.timer = 5;
        weather.unk_20 = 1;
        gPlayerEntity.base.x.HALF.HI = 600;
        gRoomTransition.frameCount = 1;
        RestoreGameTask(FALSE);
        for (unsigned i = 0; i < 0x800; ++i) {
            assert(vram[0xE800 + i] == (effectLayer == 1 ? 43 + cycle % 4 : 0x31));
            assert(vram[0xD800 + i] == (effectLayer == 2 ? 0x7A : 0x42));
        }
    }
    puts("PASS: 256 production menu restores preserve Crenel ash and overlay-owned BG1/BG2 maps");
    return 0;
}
