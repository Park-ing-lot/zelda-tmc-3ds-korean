/* Opt-in emulator diagnostics. This file is excluded from normal builds.
 * Screenshots, state observations and scripted input stay on the local SD. */
#include "korean_runtime_probe.h"
#include "platform_3ds.h"
#include "main.h"
#include "message.h"
#include "room.h"
#include "game.h"
#include "save.h"
#include "sound.h"
#include "port_second_screen_3ds.h"
#include <stdio.h>

void KoreanRuntimeProbe_Input(uint32_t* held, uint32_t* down) {
    static unsigned remaining, mask, previous, polls;
    if (remaining == 0 && (++polls % 30) == 0) {
        FILE* tap = fopen("probe-touch.txt", "r");
        if (tap) {
            int x, y;
            if (fscanf(tap, "%d %d", &x, &y) == 2 && x >= 0 && x < 320 && y >= 0 && y < 240) {
                int tabBefore, pageBefore, tabAfter, pageAfter;
                BottomFrameState3DSStats before, after;
                Port_SecondScreen_3DS_GetFrameStats(&before);
                Port_SecondScreen_3DS_ProbeUi(&tabBefore, &pageBefore);
                Port_SecondScreen_3DS_OnTap(x, y, 0);
                Port_SecondScreen_3DS_ProbeUi(&tabAfter, &pageAfter);
                Port_SecondScreen_3DS_GetFrameStats(&after);
                FILE* log = fopen("probe-touch-log.txt", "a");
                if (log) {
                    fprintf(log, "%d %d tab=%d->%d page=%d->%d rejected=%u\n", x, y,
                            tabBefore, tabAfter, pageBefore, pageAfter,
                            after.touchGenerationRejects + after.touchModeRejects -
                            before.touchGenerationRejects - before.touchModeRejects);
                    fclose(log);
                }
            }
            fclose(tap);
            remove("probe-touch.txt");
        }
        FILE* input = fopen("probe-keys.txt", "r");
        if (input) {
            unsigned keys, frames;
            if (fscanf(input, "%u %u", &keys, &frames) == 2 && frames <= 3600) {
                mask = keys;
                remaining = frames;
            }
            fclose(input);
            remove("probe-keys.txt");
        }
    }
    if (remaining) {
        *held = mask;
        *down = mask & ~previous;
        previous = mask;
        --remaining;
    } else {
        previous = 0;
    }
}

void KoreanRuntimeProbe_Frame(void) {
    static unsigned frames;
    ++frames;
    if ((frames % 30) == 0 && gMain.task == TASK_GAME && gMain.substate == GAMEMAIN_UPDATE) {
        FILE* command = fopen("probe-dialog.txt", "r");
        if (command) {
            unsigned index;
            if (fscanf(command, "%x", &index) == 1 && index <= 0x4FFF) MessageRequest(index);
            fclose(command);
            remove("probe-dialog.txt");
        }
        command = fopen("probe-room.txt", "r");
        if (command) {
            unsigned area, room, x, y;
            if (fscanf(command, "%x %x %u %u", &area, &room, &x, &y) == 4 &&
                area < 0x90 && room < 64 && x <= 1023 && y <= 1023) {
                const Transition warp = { .warp_type = WARP_TYPE_AREA, .endX = x, .endY = y,
                    .area = area, .room = room, .layer = 1, .transition_type = TRANSITION_TYPE_INSTANT,
                    .facing_direction = 4 };
                DoExitTransition(&warp);
            }
            fclose(command);
            remove("probe-room.txt");
        }
    }
    if ((frames % 180) != 0) return;
    Platform3DS_SaveDisplayedScreens("probe-top.bmp", "probe-bottom.bmp");
    FILE* state = fopen("probe-state.txt", "w");
    if (state) {
        int tab, page;
        Port_SecondScreen_3DS_ProbeUi(&tab, &page);
        fprintf(state, "frames=%u task=%u state=%u substate=%u tab=%d page=%d area=%02X room=%02X "
                "text=%04X msg=%u render=%u line=%u x=%u limit=%u choice=%d saveSlot=%u\n",
                frames, gMain.task, gMain.state, gMain.substate, tab, page,
                gRoomControls.area, gRoomControls.room, gTextRender.curToken.textIndex,
                gMessage.state, gTextRender.renderStatus, gTextRender._98.bytes.lineNo,
                gTextRender._50.unk6, gTextRender._50.unk4, sub_08056338(), gSaveHeader->saveFileId);
        fclose(state);
    }
}
