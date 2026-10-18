/* Exercise the actual 3DS adapter's paint/submit target snapshots without ROMs. */
#include <assert.h>
#include "platform/3ds/source/port_second_screen_3ds.c"
void Port_SecondScreen_3DS_LockUI(void) {}
void Port_SecondScreen_3DS_UnlockUI(void) {}

int main(void) {
    uint32_t firstPixel, secondPixel;
    Port_SecondScreen_3DS_ResetFrameState();
    sUi.tab = SS_TAB_ITEMS;
    sTapTargetCount = 1;
    sTapTargets[0] = (TapTarget){10, 20, 30, 40, SS_ACT_ITEM, 13};
    const uint32_t first = RequestRefresh();
    FinishRefresh(first, &firstPixel, 1);
    Port_SecondScreen_3DS_MarkSubmitted(first, 1);
    Port_SecondScreen_3DS_PromoteSubmitted();
    assert(sVisibleTouch.generation == first);
    assert(TouchTargetsMatch(&sVisibleTouch.targets));
    /* A cursor blink preserves exact input semantics despite a newer paint. */
    const uint32_t blink = RequestRefresh();
    FinishRefresh(blink, &secondPixel, 1);
    assert(TouchTargetsMatch(&sVisibleTouch.targets));
    assert(BottomFrameState3DS_CanDispatchTouchWithMatchingTargets(&sFrameState, 1, 1));
    Port_SecondScreen_3DS_MarkSubmitted(blink, 1);
    /* A different item/action/rectangle must not inherit that exception. */
    sTapTargets[0].arg = 9;
    FinishRefresh(RequestRefresh(), &firstPixel, 1);
    assert(!TouchTargetsMatch(&sVisibleTouch.targets));
    Port_SecondScreen_3DS_PromoteSubmitted();
    assert(sVisibleTouch.generation == blink && sVisibleTouch.targets.t[0].arg == 13);
    assert(!TouchTargetsMatch(&sVisibleTouch.targets));
    sTapTargets[0].arg = 13;
    sTapTargets[0].x1++;
    assert(!TouchTargetsMatch(&sVisibleTouch.targets));
    sTapTargets[0].x1--;
    sTapTargets[0].action = SS_ACT_TAB;
    assert(!TouchTargetsMatch(&sVisibleTouch.targets));
    puts("bottom_touch_test: PASS (visible targets, animation, changed item/action/rectangle)");
    return 0;
}
