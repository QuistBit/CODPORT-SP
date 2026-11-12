#ifndef G_COD1_GRENADE_H
#define G_COD1_GRENADE_H
#include "bg_cod1_grenade_math.h"
/* Retail linear bounce 0x20023d90 + angular response 0x20023b40.
 * The missile runner's separate support trace is still outside this patch. */
static qboolean G_Cod1GrenadeWeapon(int weapon) {
    const cwDef_t *d=CW_WeaponDef(weapon);
    return d && !Q_stricmp(CW_String(d,"weaponType"),"grenade");
}
static qboolean G_Cod1Grenade(const gentity_t *ent) {
    return ent && G_Cod1GrenadeWeapon(ent->s.weapon);
}
static qboolean G_Cod1InitGrenadeAngles( gentity_t *ent, const vec3_t velocity ) {
    uint32_t pitchRandom15,rollRandom15;
    if(!ent || !velocity)return qfalse;
    if(G_Cod1RngOwnerPolicy()==COD1_RNG_POLICY_FRESH_SP) {
        if(!G_Cod1RngDrawPair(&pitchRandom15,&rollRandom15))return qfalse;
    } else {
        /* Explicit old CRT policy outside the admitted fresh GAME scope. */
        pitchRandom15=(uint32_t)rand() & UINT32_C(0x7fff);
        rollRandom15=(uint32_t)rand() & UINT32_C(0x7fff);
    }
    ent->s.apos.trType = TR_LINEAR;
    ent->s.apos.trTime = level.time;
    ent->s.apos.trDuration = 0;
    BG_Cod1GrenadeInitialAngles( velocity, pitchRandom15, rollRandom15,
        ent->s.apos.trBase, ent->s.apos.trDelta );
    VectorCopy( ent->s.apos.trBase, ent->r.currentAngles );
    return qtrue;
}

static void G_Cod1GrenadeImpactAngles( gentity_t *ent, const trace_t *trace,
    int hitTime, qboolean stopped ) {
    vec3_t angles;
    unsigned int random15 = stopped ? 0 : ( (unsigned int)rand() & 0x7fff );
    BG_EvaluateTrajectory( &ent->s.apos, hitTime, angles );
    BG_Cod1GrenadeImpactAngles( angles, ent->s.apos.trDelta,
        trace->plane.normal, stopped, random15 );
    if ( stopped ) {
        G_SetAngle( ent, angles );
    } else {
        VectorCopy( angles, ent->s.apos.trBase );
        ent->s.apos.trTime = level.time;
    }
}

static qboolean G_Cod1BounceGrenade(gentity_t *ent,const trace_t *trace) {
    vec3_t velocity,change,offset;
    float dot,factor;int i,hitTime,contents; qboolean wet;
    contents=trap_PointContents(ent->r.currentOrigin,-1);
    wet=(contents&CONTENTS_WATER)!=0;
    hitTime=level.previousTime+(int)((level.time-level.previousTime)*trace->fraction);
    BG_EvaluateTrajectoryDelta(&ent->s.pos,hitTime,velocity);
    dot=DotProduct(velocity,trace->plane.normal);
    VectorMA(velocity,-2*dot,trace->plane.normal,ent->s.pos.trDelta);
    if(trace->plane.normal[2]>.7f)ent->s.groundEntityNum=trace->entityNum;
    if(ent->s.eFlags&EF_BOUNCE_HALF) {
        /* Map the semantic body/corpse categories, not retail numeric bits. */
        factor=(wet || (trace->contents&(CONTENTS_BODY|CONTENTS_CORPSE)))?.125f:.5f;
        VectorScale(ent->s.pos.trDelta,factor,ent->s.pos.trDelta);
        if(trace->plane.normal[2]>.7f && VectorLength(ent->s.pos.trDelta)<20) {
            G_SetOrigin(ent,ent->r.currentOrigin);
            G_Cod1GrenadeImpactAngles(ent,trace,hitTime,qtrue);
            return qfalse;
        }
    }
    VectorSubtract(ent->s.pos.trDelta,velocity,change);
    VectorScale(trace->plane.normal,.1f,offset);
    if(offset[2]>0)offset[2]=0;
    for(i=0;i<3;i++)ent->r.currentOrigin[i]+=offset[i];
    VectorCopy(ent->r.currentOrigin,ent->s.pos.trBase);ent->s.pos.trTime=level.time;
    G_Cod1GrenadeImpactAngles(ent,trace,hitTime,qfalse);
    return !wet && VectorLength(change)>100;
}
#endif
