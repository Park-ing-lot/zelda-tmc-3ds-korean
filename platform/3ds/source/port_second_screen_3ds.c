/*
 * 3DS-only wrapper around the shared Minish Cap second-screen compositor.
 *
 * Keeping the wrapper in platform/3ds makes the title/file-select behavior
 * independent of Android and desktop. The shared compositor is included as
 * this translation unit's implementation so this small adapter can select
 * its existing Settings page without adding title-menu state to every port.
 */
#include "port_second_screen_3ds.h"
#include "bottom_idle_3ds.h"
#include "bottom_frame_state_3ds.h"
#include "bottom_map_anim_3ds.h"

#include <stdbool.h>

/* This replaces the direct port/port_second_screen.c entry in the 3DS CMake
 * source list. All normal exported functions retain their original symbols. */
#include "../../../port/port_second_screen.c"

static volatile uint8_t sIdleSettingsOpen;
static uint8_t sIdleOwnsSettingsTab;
static volatile uint8_t sIdleBackVisible;
static BottomFrameState3DS sFrameState;

/* Two CPU upload buffers, followed by independent submitted/visible copies.
 * The worker may already be painting the next cursor blink when HID arrives.
 * Keep the targets belonging to the displayed image, not just the latest paint. */
typedef struct {
    const uint32_t* pixels;
    uint32_t generation;
    int inGame;
    int tab;
    TargetList targets;
} TouchFrame3DS;
static TouchFrame3DS sTouchBuffers[2], sSubmittedTouch, sVisibleTouch;

static int TouchTargetsMatch(const TargetList* visible) {
    if (visible->n != sTapTargetCount) return 0;
    for (int i = 0; i < visible->n; ++i) {
        const TapTarget* a = &visible->t[i];
        const TapTarget* b = &sTapTargets[i];
        if (a->x0 != b->x0 || a->y0 != b->y0 || a->x1 != b->x1 || a->y1 != b->y1 ||
            a->action != b->action || a->arg != b->arg) return 0;
    }
    return 1;
}

static uint32_t BeginRefresh(void) {
    return BottomFrameState3DS_BeginPaint(&sFrameState);
}

static void FinishRefresh(uint32_t request, const uint32_t* pixels, int inGame) {
    /* A tap may arrive while the worker is painting.  Acknowledge only the
     * generation this frame actually observed; a newer generation remains
     * pending and schedules another paint instead of being lost. */
    UI_LOCK();
    int slot = -1;
    for (int i = 0; i < 2; ++i) {
        if (sTouchBuffers[i].pixels == pixels) { slot = i; break; }
        if (!sTouchBuffers[i].pixels) slot = i;
    }
    if (slot >= 0) {
        TouchFrame3DS* frame = &sTouchBuffers[slot];
        frame->pixels = pixels;
        frame->generation = request;
        frame->inGame = inGame;
        frame->tab = sUi.tab;
        frame->targets.n = sTapTargetCount;
        memcpy(frame->targets.t, sTapTargets, sizeof(TapTarget) * (size_t)sTapTargetCount);
    }
    UI_UNLOCK();
    BottomFrameState3DS_FinishPaint(&sFrameState, request);
}

static uint32_t RequestRefresh(void) {
    return BottomFrameState3DS_Request(&sFrameState);
}

static uint32_t RequestTouchRefresh(void) {
    return BottomFrameState3DS_RequestTouchRefresh(&sFrameState);
}

static void IdleBackRect(int width, int height, float* x0, float* y0, float* x1, float* y1) {
    float u = (float)(width < height ? width : height) / 720.0f;
    int32_t ts = (width < height ? width : height) / 240;
    if (ts < 2) ts = 2;
    if (ts > 6) ts = 6;

    const float panelX = 10.0f * u;
    const float panelY = 10.0f * u;
    const float inset = 6.0f * (float)ts;
    int32_t headerScale = (int32_t)(2.4f * u);
    if (headerScale < 1) headerScale = 1;
    const float headerHeight = MENU_TEXT_BOX * headerScale + 24.0f * u;
    float buttonWidth = 154.0f * u;
    if (buttonWidth < 54.0f) buttonWidth = 54.0f;

    *x0 = panelX + inset + 4.0f * u;
    *y0 = panelY + inset;
    *x1 = *x0 + buttonWidth;
    *y1 = *y0 + headerHeight;
}

static bool IsIdleBackTap(int x, int y, int width, int height) {
    float x0, y0, x1, y1;
    IdleBackRect(width, height, &x0, &y0, &x1, &y1);
    return (float)x >= x0 && (float)x < x1 && (float)y >= y0 && (float)y < y1;
}

static void ResetIdleOnlyState(void) {
    UI_LOCK();
    sTapTargetCount = 0;
    sUi.mapLive = 0;
    sMapCameraMoving = 0;
    sUi.armedRing = 0;
    sUi.floorPreview = SS_NO_FLOOR;
    sUi.playerFloorDisp = SS_NO_FLOOR;
    sUi.regionState = SS_REGION_OFF;
    sUi.questView = SS_QUEST_MAIN;
    sUi.settingsPage = SS_SETTINGS_ROOT;
    sUi.loadConfirmActive = 0;
    sUi.unavailableNoticeActive = 0;
    if (sIdleOwnsSettingsTab) sUi.tab = SS_TAB_MAP;
    UI_UNLOCK();
    sLastFix.valid = 0;
    sCam.valid = 0;
    sIdleOwnsSettingsTab = 0;
    __atomic_store_n(&sIdleBackVisible, 0, __ATOMIC_RELEASE);
}

static void DrawIdleBack(const SSurf* surface, float u, int32_t ts) {
    float x0, y0, x1, y1;
    IdleBackRect(surface->w, surface->h, &x0, &y0, &x1, &y1);
    DrawMenuButton(surface, x0, y0, x1, y1, "BACK", 0, 0, u, ts);
}

static void PaintIntroCinema(uint32_t* pixels, int width, int height, int stride) {
    __atomic_store_n(&sIdleSettingsOpen, 0, __ATOMIC_RELEASE);
    ResetIdleOnlyState();
    SSurf surface = { pixels, width, height, stride };
    FillRect(&surface, 0, 0, width, height, RGB(0, 0, 0));
}

uint32_t Port_SecondScreen_3DS_PaintInto(uint32_t* pixels, int width, int height, int strideInPixels,
                                        const SecondScreenSnapshot* snap, uint32_t tick) {
    if (!pixels || !snap || width <= 0 || height <= 0 || strideInPixels < width) return 0;

    const uint32_t refreshRequest = BeginRefresh();
    const bool settingsOpen = __atomic_load_n(&sIdleSettingsOpen, __ATOMIC_ACQUIRE) != 0;

    if (snap->introCinema) {
        PaintIntroCinema(pixels, width, height, strideInPixels);
        FinishRefresh(refreshRequest, pixels, snap->inGame != 0);
        return refreshRequest;
    }

    if (snap->inGame) {
        if (settingsOpen) __atomic_store_n(&sIdleSettingsOpen, 0, __ATOMIC_RELEASE);
        if (sIdleOwnsSettingsTab) ResetIdleOnlyState();
        Port_SecondScreen_PaintInto(pixels, width, height, strideInPixels, snap, tick);
        FinishRefresh(refreshRequest, pixels, snap->inGame != 0);
        return refreshRequest;
    }

    if (!settingsOpen) {
        /* Preserve every reset the shared non-game cinema path performed:
         * stale map fixes, armed items and submenu state must not cross a
         * title/file-select boundary even though the picture is replaced. */
        ResetIdleOnlyState();
        BottomIdle3DS_Paint(pixels, width, height, strideInPixels, tick, Platform3DS_IsNew3DS());
        FinishRefresh(refreshRequest, pixels, snap->inGame != 0);
        return refreshRequest;
    }

    /* The existing Settings painter only needs the inGame gate to become
     * visible; its values come from persistent port configuration, not from
     * gameplay coordinates. A zeroed title snapshot is therefore safe. */
    if (!sIdleOwnsSettingsTab) ResetIdleOnlyState();
    SecondScreenSnapshot settingsSnapshot = *snap;
    settingsSnapshot.inGame = 1;
    UI_LOCK();
    sUi.tab = SS_TAB_SETTINGS;
    sIdleOwnsSettingsTab = 1;
    UI_UNLOCK();
    Port_SecondScreen_PaintInto(pixels, width, height, strideInPixels, &settingsSnapshot, tick);

    int settingsPage;
    int confirmationOpen;
    UI_LOCK();
    settingsPage = sUi.settingsPage;
    confirmationOpen = sUi.unavailableNoticeActive;
    UI_UNLOCK();
    const bool showIdleBack = settingsPage == SS_SETTINGS_ROOT && !confirmationOpen;
    if (showIdleBack) {
        SSurf surface = { pixels, width, height, strideInPixels };
        float u = (float)(width < height ? width : height) / 720.0f;
        int32_t ts = (width < height ? width : height) / 240;
        if (ts < 2) ts = 2;
        if (ts > 6) ts = 6;
        DrawIdleBack(&surface, u, ts);
    }
    __atomic_store_n(&sIdleBackVisible, showIdleBack ? 1 : 0, __ATOMIC_RELEASE);
    FinishRefresh(refreshRequest, pixels, snap->inGame != 0);
    return refreshRequest;
}

void Port_SecondScreen_3DS_ResetFrameState(void) {
    BottomFrameState3DS_Reset(&sFrameState);
    UI_LOCK();
    memset(sTouchBuffers, 0, sizeof(sTouchBuffers));
    memset(&sSubmittedTouch, 0, sizeof(sSubmittedTouch));
    memset(&sVisibleTouch, 0, sizeof(sVisibleTouch));
    UI_UNLOCK();
}

uint32_t Port_SecondScreen_3DS_RequestRefresh(void) {
    return RequestRefresh();
}

void Port_SecondScreen_3DS_MarkSubmitted(uint32_t generation, int inGame) {
    UI_LOCK();
    sSubmittedTouch.generation = 0;
    for (int i = 0; i < 2; ++i) {
        if (sTouchBuffers[i].generation == generation) {
            sSubmittedTouch = sTouchBuffers[i];
            break;
        }
    }
    UI_UNLOCK();
    BottomFrameState3DS_MarkSubmitted(&sFrameState, generation, inGame);
}

void Port_SecondScreen_3DS_PromoteSubmitted(void) {
    if (BottomFrameState3DS_PromoteSubmitted(&sFrameState)) {
        UI_LOCK();
        sVisibleTouch = sSubmittedTouch;
        UI_UNLOCK();
    }
}

void Port_SecondScreen_3DS_GetFrameStats(BottomFrameState3DSStats* out) {
    BottomFrameState3DS_GetStats(&sFrameState, out);
}

#ifdef TMC_KOREAN_RUNTIME_PROBE
void Port_SecondScreen_3DS_ProbeUi(int* tab, int* page) {
    UI_LOCK();
    *tab = sUi.tab;
    *page = sUi.settingsPage;
    UI_UNLOCK();
}
#endif

void Port_SecondScreen_3DS_OnTap(int x, int y, int longPress) {
    SecondScreenSnapshot currentSnapshot;
    Port_SecondScreenState_Read(&currentSnapshot);
    if (currentSnapshot.introCinema) return;
    TapTarget visibleHit;
    int visibleHitFound = 0;
    UI_LOCK();
    /* Only the item cursor has a proven animation-only input layout. Exact
     * rectangle/action/item equality excludes inventory changes. Map transforms
     * and settings/task transitions retain the strict generation rule. */
    const int matchingItemTargets = currentSnapshot.inGame && sVisibleTouch.generation != 0 &&
        sVisibleTouch.inGame && sVisibleTouch.tab == SS_TAB_ITEMS && sUi.tab == SS_TAB_ITEMS &&
        TouchTargetsMatch(&sVisibleTouch.targets);
    if (matchingItemTargets) {
        for (int i = 0; i < sVisibleTouch.targets.n; ++i) {
            const TapTarget* target = &sVisibleTouch.targets.t[i];
            if (x >= target->x0 && x < target->x1 && y >= target->y0 && y < target->y1) {
                visibleHit = *target;
                visibleHitFound = 1;
                break;
            }
        }
    }
    UI_UNLOCK();
    /* Painting publishes the compositor's new hit boxes before its texture
     * can be submitted or shown.  Dispatch only when those hit boxes belong
     * to the confirmed visible generation and task; otherwise fail closed
     * while leaving the per-tick HID polling cadence untouched. */
    if (!BottomFrameState3DS_CanDispatchTouchWithMatchingTargets(&sFrameState,
            currentSnapshot.inGame != 0, matchingItemTargets)) {
        /* A same-task hidden paint is already on its way to the display; an
         * extra request would only lengthen this fail-closed interval.  Task
         * transitions (and the pre-first-frame state) still need a paint. */
        if (!BottomFrameState3DS_VisibleModeMatches(&sFrameState, currentSnapshot.inGame != 0)) {
            RequestTouchRefresh();
        }
        return;
    }
    if (currentSnapshot.inGame) {
        if (matchingItemTargets) {
            if (visibleHitFound) DispatchTapTarget(visibleHit, x, y, longPress);
        } else {
            Port_SecondScreen_OnTap(x, y, longPress);
        }
        RequestTouchRefresh();
        return;
    }

    if (!__atomic_load_n(&sIdleSettingsOpen, __ATOMIC_ACQUIRE)) {
        __atomic_store_n(&sIdleSettingsOpen, 1, __ATOMIC_RELEASE);
        RequestTouchRefresh();
        return;
    }

    if (__atomic_load_n(&sIdleBackVisible, __ATOMIC_ACQUIRE) &&
        IsIdleBackTap(x, y, 320, 240)) {
        __atomic_store_n(&sIdleSettingsOpen, 0, __ATOMIC_RELEASE);
        RequestTouchRefresh();
        return;
    }

    Port_SecondScreen_OnTap(x, y, longPress);
    RequestTouchRefresh();
}

int Port_SecondScreen_3DS_NeedsRefresh(void) {
    return BottomFrameState3DS_NeedsPaint(&sFrameState);
}

int Port_SecondScreen_3DS_NeedsPeriodicRefresh(const SecondScreenSnapshot* snap, uint32_t tick,
                                               uint32_t paintedTick, int32_t width,
                                               int32_t height) {
    if (!snap || snap->introCinema) return 0;

    const bool idleSettings = __atomic_load_n(&sIdleSettingsOpen, __ATOMIC_ACQUIRE) != 0;
    if (!snap->inGame && !idleSettings) {
        /* Match ALttP PR #32: Old 3DS keeps a steady gold card. */
        return Platform3DS_IsNew3DS();
    }

    int tab;
    int settingsPage;
    uint32_t dumpFlashUntil;
    uint32_t lastTick;
    int regionState;
    int floorPreview;
    int cameraMoving;
    UI_LOCK();
    tab = sUi.tab;
    settingsPage = sUi.settingsPage;
    dumpFlashUntil = sUi.dumpFlashUntil;
    lastTick = sUi.lastTick;
    regionState = sUi.regionState;
    floorPreview = sUi.floorPreview;
    cameraMoving = sMapCameraMoving;
    UI_UNLOCK();

    if (tab == SS_TAB_MAP) {
        if (!Port_Config_BottomMapSkip()) {
            return 1; /* the old unconditional repaint, kept for the A/B */
        }
        /* The camera glide, region bracket and floor preview advance inside
         * paint. Keep repainting until they settle, even when the engine
         * snapshot and marker animation signature have not changed. */
        if (cameraMoving || regionState == SS_REGION_BRACKET || floorPreview != SS_NO_FLOOR) {
            return 1;
        }
        if (BottomMapAnim_NeedsPaint(tick, paintedTick,
                                     (snap->areaFlags & SECOND_SCREEN_AR_IS_DUNGEON) != 0, width,
                                     height)) {
            return 1;
        }
        return 0;
    }
    if (tab == SS_TAB_ITEMS) {
        /* The authentic item cursor blinks even when no values change. */
        return 1;
    }
    if (tab != SS_TAB_SETTINGS) {
        /* Quest status and its two detail lists have no selection cursor. */
        return 0;
    }
    if (settingsPage == SS_SETTINGS_UPDATE) return 1;
    if (settingsPage == SS_SETTINGS_OVERLAY) {
        /* Live diagnostics deliberately retain their slower refresh rate. */
        return 1;
    }
    if (settingsPage == SS_SETTINGS_DEVELOPER &&
        (int32_t)(dumpFlashUntil - lastTick) > 0) {
        /* Keep painting until the post-dump DONE indicator expires. */
        return 1;
    }
    return 0;
}

int Port_SecondScreen_3DS_SnapshotChangeNeedsRefresh(const SecondScreenSnapshot* previous,
                                                     const SecondScreenSnapshot* current,
                                                     int previousValid) {
    if (!previous || !current || !previousValid) return 1;
    if (previous->inGame != current->inGame || previous->introCinema != current->introCinema) return 1;
    if (memcmp(previous, current, sizeof(*current)) == 0) return 0;

    /* Gameplay snapshots continue changing while a settings sheet covers the
     * entire panel, but none of those engine fields are drawn there.  Config
     * mutations originate in the tap handler and request an immediate paint;
     * the overlay and dump indicator are handled by NeedsPeriodicRefresh. */
    if (!current->inGame) return 0;
    int tab;
    UI_LOCK();
    tab = sUi.tab;
    UI_UNLOCK();
    return tab != SS_TAB_SETTINGS;
}
