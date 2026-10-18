/* Opt-in emulator diagnostics. This file is excluded from normal builds.
 * Screenshots, state observations and scripted input stay on the local SD. */
#include "korean_runtime_probe.h"
#include "platform_3ds.h"
#include "main.h"
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
    if ((++frames % 180) != 0) return;
    Platform3DS_SaveDisplayedScreens("probe-top.bmp", "probe-bottom.bmp");
    FILE* state = fopen("probe-state.txt", "w");
    if (state) {
        int tab, page;
        Port_SecondScreen_3DS_ProbeUi(&tab, &page);
        fprintf(state, "frames=%u task=%u state=%u tab=%d page=%d\n", frames, gMain.task, gMain.state, tab, page);
        fclose(state);
    }
}
