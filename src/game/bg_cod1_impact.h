/* SPDX-License-Identifier: GPL-3.0-or-later
 * IMPACT_R1: opt-in metadata; no trace_t/playerState_t/entityState_t layout
 * changes, no new entity-event IDs or net-field widths.
 * cplane_t.pad[2] was unused in this source tree. Only CoD collision writers
 * stamp it. Normal/type/signbits and every existing surface flag are untouched.
 * Zeroed/legacy traces are UNKNOWN, never silently a known CoD default.
 */
#ifndef BG_COD1_IMPACT_H
#define BG_COD1_IMPACT_H
#define COD1_IMPACT_VERSION 1
#define COD1_IMPACT_TAG 0x49310000u
#define COD1_MARK_TRIANGLES 0x10000
#define CI_KNOWN 0x20
#define CI_NOIMPACT 0x40
#define CI_NOMARKS 0x80
#define CI_RIFLE 0x100
static ID_INLINE void CI_TraceSurface(trace_t *t, unsigned flags) {
    t->plane.pad[0]=(byte)(0x80u|((flags>>20)&31u)|
        ((flags&(SURF_SKY|SURF_NOIMPACT))?0x20u:0)|((flags&SURF_NOMARKS)?0x40u:0));
    t->plane.pad[1]=0xc1;
}
static ID_INLINE int CI_TraceBits(const trace_t *t) {
    unsigned b=t->plane.pad[0];
    if(t->plane.pad[1]!=0xc1 || !(b&0x80))return 0;
    return (b&31)|CI_KNOWN|((b&0x20)?CI_NOIMPACT:0)|((b&0x40)?CI_NOMARKS:0);
}
static ID_INLINE int CI_IsEvent(const entityState_t *s) {
    return ((unsigned)s->time2&0xfffffe00u)==COD1_IMPACT_TAG;
}
/* Only a caller that owns an EV_BULLET_HIT_WALL event may use these fields.
 * eventParm retains reflected direction for older cgame's legacy path.
 * frame=normal byte; origin=unsnapped contact; origin2=actual segment start.
 * time2=version/material/class/visibility; weapon=captured, NOT read at receipt.
 */
static ID_INLINE void CI_WriteEvent(entityState_t *s,const trace_t *t,const vec3_t contact,
    const vec3_t start,const vec3_t end,int weapon,int rifle) {
    vec3_t direction,reflected,normal;float dot;int bits=CI_TraceBits(t);
    if(t->surfaceFlags&(SURF_SKY|SURF_NOIMPACT))bits|=CI_NOIMPACT;
    if(t->surfaceFlags&SURF_NOMARKS)bits|=CI_NOMARKS;
    if(t->fraction>=1 || t->startsolid || t->allsolid)bits|=CI_NOIMPACT;
    VectorSubtract(end,start,direction);
    if(VectorNormalize(direction)==0)bits|=CI_NOIMPACT;
    dot=DotProduct(direction,t->plane.normal);
    VectorMA(direction,-2*dot,t->plane.normal,reflected);VectorNormalize(reflected);
    s->time2=(int)(COD1_IMPACT_TAG|(unsigned)bits|(rifle?CI_RIFLE:0));
    VectorCopy(t->plane.normal,normal);s->frame=DirToByte(normal);s->eventParm=DirToByte(reflected);
    s->weapon=weapon;VectorCopy(contact,s->origin);VectorCopy(start,s->origin2);
}
#endif
