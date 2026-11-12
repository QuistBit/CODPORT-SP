/* R132 GAME-owned scalar RNG foundation. One state in this translation unit;
 * no VM/native binding and no original save/whole-GAME sequence parity claim.
 * No-save SP map epochs, including fast restart, share launch pitch/roll.
 * Explicit legacy policy retains CRT outside that admitted fresh capability. */
typedef struct {
    uint32_t state;
    uint64_t epoch, attempted_epoch;
    int policy, ready, busy;
} cod1RngOwner_t;
static cod1RngOwner_t cod1RngOwner;

static int G_Cod1RngOverlap(const void *a,size_t as,const void *b,size_t bs) {
    uintptr_t aa=(uintptr_t)a,bb=(uintptr_t)b;
    if(!a || !b || !as || !bs)return 0;
    return aa<=bb ? bb-aa<as : aa-bb<bs;
}

void G_Cod1RngOwnerEnd(void) {
    /* Keep the latched policy and consumed state: a retired fresh map must
     * refuse rather than silently become the old CRT implementation. */
    cod1RngOwner.ready=0;
    cod1RngOwner.busy=0;
    cod1RngOwner.epoch=0;
}

int G_Cod1RngOwnerPolicy(void) {
    return cod1RngOwner.policy;
}

qboolean G_Cod1RngOwnerReady(void) {
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    return cod1RngOwner.policy==COD1_RNG_POLICY_FRESH_SP &&
        cod1RngOwner.ready && !cod1RngOwner.busy && cod1RngOwner.epoch &&
        cod1RngOwner.epoch==G_Cod1EntityMapEpoch() &&
        g_gametype.integer==GT_SINGLE_PLAYER && !level.restarted;
#else
    return qfalse;
#endif
}

#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
static int G_Cod1RngBeginSame(uint64_t epoch,int hostTime) {
    return epoch && cod1RngOwner.busy &&
        cod1RngOwner.policy==COD1_RNG_POLICY_FRESH_SP &&
        cod1RngOwner.attempted_epoch==epoch && epoch==G_Cod1EntityMapEpoch() &&
        g_gametype.integer==GT_SINGLE_PLAYER && !level.restarted &&
        level.time==hostTime;
}
static void G_Cod1RngBeginRefused(uint64_t epoch) {
    /* Never clear a newer owner created by a reentrant map bootstrap. */
    if(cod1RngOwner.attempted_epoch==epoch)cod1RngOwner.busy=0;
}
#endif

void G_Cod1RngOwnerBegin(int restart) {
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
    uint64_t epoch=G_Cod1EntityMapEpoch();
    uint32_t seed;
    int hostTime=level.time,loading;
    /* Begin is a map operation, not a constructor operation. The same active
     * incarnation cannot reseed, including after an explicit End/refusal. */
    if(epoch && cod1RngOwner.attempted_epoch==epoch)return;
    G_Cod1RngOwnerEnd();
    cod1RngOwner.attempted_epoch=epoch;
    cod1RngOwner.policy=COD1_RNG_POLICY_LEGACY;
    /* Original restart44B7A0 -> GAME_INIT2001E110 always reaches the
     * syscall4 clock seed at2001E1CD, just as initial init does. Restart is
     * a new map epoch, never a Session/constructor reseed. Save restore is
     * still outside admission below; unknown restart values stay legacy. */
    if(g_gametype.integer!=GT_SINGLE_PLAYER || (restart!=0 && restart!=1))return;
    cod1RngOwner.policy=COD1_RNG_POLICY_FRESH_SP;
    if(!epoch || level.restarted)return;
    cod1RngOwner.busy=1;
    if(!G_Cod1RngBeginSame(epoch,hostTime)) {
        G_Cod1RngBeginRefused(epoch);return;
    }
    loading=trap_Cvar_VariableIntegerValue("savegame_loading");
    if(!G_Cod1RngBeginSame(epoch,hostTime)) {
        G_Cod1RngBeginRefused(epoch);return;
    }
    if(loading!=0) {
        /* An initially loaded map is explicitly outside fresh admission. */
        cod1RngOwner.policy=COD1_RNG_POLICY_LEGACY;
        cod1RngOwner.busy=0;return;
    }
    /* Map semantic clock service, not the port G_INIT randomSeed argument or
     * a session clock. The unsigned cast retains the timer's complete DWORD. */
    seed=(uint32_t)trap_Milliseconds();
    if(!G_Cod1RngBeginSame(epoch,hostTime)) {
        G_Cod1RngBeginRefused(epoch);return;
    }
    loading=trap_Cvar_VariableIntegerValue("savegame_loading");
    if(!G_Cod1RngBeginSame(epoch,hostTime) || loading!=0) {
        G_Cod1RngBeginRefused(epoch);return;
    }
    /* Repeated traps are a bounded lease, not an atomic snapshot. No external
     * callback occurs between this final owner check and seed publication. */
    cod1RngOwner.state=seed;
    cod1RngOwner.epoch=epoch;
    cod1RngOwner.ready=1;
    cod1RngOwner.busy=0;
#else
    (void)restart;
    G_Cod1RngOwnerEnd();
    cod1RngOwner.policy=COD1_RNG_POLICY_LEGACY;
#endif
}

static uint32_t G_Cod1RngStep(uint32_t state) {
    return state*UINT32_C(0x343fd)+UINT32_C(0x269ec3);
}

qboolean G_Cod1RngDraw15(uint32_t *out) {
    uint32_t next,draw;
    if(!out || G_Cod1RngOverlap(out,sizeof(*out),&cod1RngOwner,sizeof(cod1RngOwner)) ||
       !G_Cod1RngOwnerReady())return qfalse;
    next=G_Cod1RngStep(cod1RngOwner.state);draw=next>>17;
    cod1RngOwner.state=next;
    *out=draw;return qtrue;
}

qboolean G_Cod1RngDrawPair(uint32_t *pitch,uint32_t *roll) {
    uint32_t first,second,pitchDraw,rollDraw;
    if(!pitch || !roll || G_Cod1RngOverlap(pitch,sizeof(*pitch),roll,sizeof(*roll)) ||
       G_Cod1RngOverlap(pitch,sizeof(*pitch),&cod1RngOwner,sizeof(cod1RngOwner)) ||
       G_Cod1RngOverlap(roll,sizeof(*roll),&cod1RngOwner,sizeof(cod1RngOwner)) ||
       !G_Cod1RngOwnerReady())return qfalse;
    first=G_Cod1RngStep(cod1RngOwner.state);pitchDraw=first>>17;
    second=G_Cod1RngStep(first);rollDraw=second>>17;
    /* Retail launch stores the final state after both transitions. This
     * integer transaction has no external callback or failure after commit. */
    cod1RngOwner.state=second;
    *pitch=pitchDraw;*roll=rollDraw;return qtrue;
}
