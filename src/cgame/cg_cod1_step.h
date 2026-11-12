/* CoD1 SP cgamex86.dll 0x30016ce2..0x30016da5 and
 * 0x30024aa0..0x30024ae6. Signed camera steps are independent of the
 * legacy RTCW 200-ms stair events. This changes presentation only.
 */
#ifndef CG_COD1_STEP_H
#define CG_COD1_STEP_H

#define COD1_STEP_TIME 100

static int CG_Cod1StepElapsed(int now, int started) {
    /* Match the native 32-bit subtraction when command time wraps. */
    return (int)((unsigned int)now - (unsigned int)started);
}

static float CG_Cod1StepAccumulate(float change, int started, int now,
                                  int encodedAmount) {
    int elapsed = CG_Cod1StepElapsed(now, started);
    float carry = 0;
    if (elapsed < COD1_STEP_TIME)
        carry = (int)((unsigned int)COD1_STEP_TIME - (unsigned int)elapsed)
            * change * 0.01f * 0.9f;
    change = carry + (encodedAmount - 128);
    if (change > 24) change = 24;
    else if (change < -16) change = -16;
    return change;
}

static float CG_Cod1StepOffset(float change, int *started, int now) {
    int elapsed = CG_Cod1StepElapsed(now, *started);
    if (elapsed < 0) *started = now;
    if (elapsed < COD1_STEP_TIME)
        return (int)((unsigned int)COD1_STEP_TIME - (unsigned int)elapsed)
            * change * 0.01f;
    return 0;
}

#endif
