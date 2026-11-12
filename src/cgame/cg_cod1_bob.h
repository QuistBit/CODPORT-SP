#ifndef CG_COD1_BOB_H
#define CG_COD1_BOB_H
#include "../game/bg_cod1_stance.h"
/* SPDX-License-Identifier: GPL-3.0-or-later
 * PATCH42: numeric reconstruction used by camera and weapon presentation.
 * Retail SP cgame: 0x30024c50, 0x30024cd0, 0x30028d00.
 */
#include <math.h>
static float CBob_Add(float a,float b){volatile float r=a+b;return r;}
static float CBob_Mul(float a,float b){volatile float r=a*b;return r;}
static float CBob_Amplitude(int stance,float speed,float cap){
    float a=CBob_Mul(speed,stance==2?.03f:stance==1?.015f:.008f);
    return a>cap?cap:a;
}
static float cod_bob_vertical(int stance,float phase,float speed,float cap){
    float a=CBob_Amplitude(stance,speed,cap);
    float angle=CBob_Add(CBob_Mul(phase,4),1.5707963705062866f);
    float wave=CBob_Add((float)sin((double)CBob_Add(phase,phase)),(float)(sin((double)angle)*.20000000298023224f));
    return CBob_Mul(CBob_Mul(wave,a),.75f);
}
static float cod_bob_horizontal(int stance,float phase,float speed,float cap){
    return (float)(sin((double)phase)*CBob_Amplitude(stance,speed,cap));
}
static void cod_bob_weapon(int stance,float phase,float speed,float ads,float ads_factor,float *out){
    float p=CBob_Add(CBob_Add(CBob_Mul(.25f,3.1415927410125732f),phase),6.2831854820251465f);
    float s=CBob_Mul(.16f,speed),r,scale;
    out[0]=-cod_bob_vertical(stance,p,s,10);
    out[1]=-cod_bob_horizontal(stance,p,s,10);
    p=CBob_Add(p,-.4712389409542084f);
    r=cod_bob_horizontal(stance,p,CBob_Mul(1.5f,s),10);
    out[2]=r<0?r:0;
    if(ads!=0){
        scale=CBob_Add(1,-CBob_Mul(CBob_Add(1,-ads_factor),ads));
        out[0]=CBob_Mul(out[0],scale);out[1]=CBob_Mul(out[1],scale);out[2]=CBob_Mul(out[2],scale);
    }
}

/* Phase conversion in cgame 0x30025ea1..0x30025ebc. This does not
 * implement the movement-side update of the byte-sized gait cycle. */
static float cod_bob_phase(int cycle){
    volatile float fraction=(float)(cycle&255)/255.0f;
    return CBob_Add(CBob_Mul(fraction,6.2831854820251465f),6.2831854820251465f);
}

static int CG_Cod1BobStance(const playerState_t *s) {
 /* Retail cgame tests desired eye height (ps+0xbc), not segment target. */
 return (s->pm_flags&PMF_COD1_PRONE)?2:(s->pm_flags&PMF_DUCKED)?1:0;
}
static qboolean CG_Cod1BobActive(const playerState_t *s) {
 return s->cod1EyeTarget && s->pm_type==PM_NORMAL && !s->aiChar &&
 !(s->eFlags&EF_MG42_ACTIVE) &&
 !s->persistant[PERS_HWEAPON_USE];
}
/* PATCH54: cgame 0x30025e7b..0x30025f11. On ladders the presentation
 * scalar is SIGNED vertical velocity, suppressed for 500 ms since jump.
 * Shared gait has a separate 300 ms guard; neither clock starts on mount.
 * Keep horizontal speed for the ordinary and legacy presentation paths. */
static float CG_Cod1BobSpeed(const playerState_t *s,int now) {
 if(CG_Cod1BobActive(s) && (s->pm_flags&PMF_LADDER))
  return now-s->jumpTime<500?0.0f:s->velocity[2];
 return (float)sqrt((double)(s->velocity[0]*s->velocity[0]+
                            s->velocity[1]*s->velocity[1]));
}
#endif
