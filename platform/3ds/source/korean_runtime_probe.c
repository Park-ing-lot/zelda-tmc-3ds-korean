/* Opt-in emulator diagnostics. This file is excluded from normal builds.
 * Screenshots, state observations and scripted input stay on the local SD. */
#include "korean_runtime_probe.h"
#include "platform_3ds.h"
#include "main.h"
#include <stdio.h>

void KoreanRuntimeProbe_Input(uint32_t* held, uint32_t* down) {
    static unsigned remaining, mask, previous, polls;
    if (remaining == 0 && (++polls % 30) == 0) {
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
        fprintf(state, "frames=%u task=%u state=%u\n", frames, gMain.task, gMain.state);
        fclose(state);
    }
}
