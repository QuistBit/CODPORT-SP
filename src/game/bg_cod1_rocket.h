/* SPDX-License-Identifier: GPL-3.0-or-later
 * CoD1 projectile-explosion metadata shared by qagame/cgame.
 * New code based on observed CoD1 data/behavior; no original source included.
 */
#ifndef BG_COD1_ROCKET_H
#define BG_COD1_ROCKET_H
#include "bg_cod1_impact.h"
#define COD1_ROCKET_TAG 0x52310000u
static ID_INLINE float CR_DamageAtDistance(float inner,float outer,float radius,float dist) {
    float t;
    if(radius<=0.0f)return dist<=0.0f?inner:0.0f;
    if(dist>=radius)return 0.0f;
    if(dist<=0.0f)return inner;
    t=dist/radius;
    return inner+(outer-inner)*t;
}
static ID_INLINE void CR_WriteEvent(entityState_t *s,const trace_t *tr) {
    int bits=CI_TraceBits(tr);vec3_t normal;
    if(tr->surfaceFlags&(SURF_SKY|SURF_NOIMPACT))bits|=CI_NOIMPACT;
    if(tr->surfaceFlags&SURF_NOMARKS)bits|=CI_NOMARKS;
    s->time2=(int)(COD1_ROCKET_TAG|(unsigned)(bits&0x1ff));
    VectorCopy(tr->plane.normal,normal);
    s->frame=DirToByte(normal);s->eventParm=s->frame;
}
static ID_INLINE int CR_IsEvent(const entityState_t *s) {
    return ((unsigned)s->time2&0xfffffe00u)==COD1_ROCKET_TAG;
}
static ID_INLINE int CR_Material(const entityState_t *s) { return s->time2&31; }
static ID_INLINE int CR_NoMarks(const entityState_t *s) { return !!(s->time2&CI_NOMARKS); }
/* A projectile that reaches its server lifetime has no impact trace/material.
 * Keep it in the native rocket presentation path using the known default
 * material, an explicit no-marks flag, and the caller-supplied fallback normal.
 */
static ID_INLINE void CR_WriteExpiryEvent(entityState_t *s,const vec3_t normal) {
    vec3_t n;
    s->time2=(int)(COD1_ROCKET_TAG|(unsigned)(CI_KNOWN|CI_NOMARKS));
    VectorCopy(normal,n);s->frame=DirToByte(n);s->eventParm=s->frame;
}
#endif
