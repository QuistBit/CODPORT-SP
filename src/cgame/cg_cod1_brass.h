/* SPDX-License-Identifier: GPL-3.0-or-later
 * BRASS_R1: local-view integration of the inspected shell emitter subset.
 * Included once by cg_effects.c, after the native EFX implementation.
 * Detached particles use their own storage and random stream. No server
 * grenade physics, live player weapon lookup at impact, or fake tag offset.
 */
#ifndef CG_COD1_BRASS_H
#define CG_COD1_BRASS_H
#include "cg_cod1_brass_runtime.h"
#include "../game/bg_cod1_inventory.h"
#include "../game/bg_cod1_soundalias.h"
#define CB_SOUND_ALIASES 8
#define CB_SOUND_VARIANTS 16
typedef struct { sfxHandle_t handle; cwSoundAliasVariant native; } cbGameSoundVariant;
typedef struct { char name[CEFX_PATH];cbGameSoundVariant variant[CB_SOUND_VARIANTS];int count; } cbGameSound;
static struct {
    cbRuntime runtime;
    cbGameSound sound[CB_SOUND_ALIASES];int soundCount;
    int initialized,capable,effect[CW_MAX_DEFS][2],registered[CW_MAX_DEFS];
    int key,tagValid,tagTime,missingModel,missingKey;
    cbTag tag;
    unsigned missingTags,rejected,traceSolid,soundPlayed,deduplicated,sequenceReuse;
    int lastAcceptedSequence,lastAcceptedValid;
    unsigned soundRandom;
} cbGame;
static vmCvar_t cbEnabled,cbDebug;
static int cbReadGame(void *u,const char *name,char **out) {
    (void)u;return ceRead(name,out,CEFX_FILE_LIMIT);
}
static void cbFreeGame(void *u,char *p){(void)u;free(p);}
static void cbDiagnosticGame(void *u,const char *message) {
    (void)u;CG_Printf("CoD1 BRASS_R1: %s\n",message);
}
static int cbModelGame(void *u,const char *name) {
    (void)u;
    if(!ciFileExists(name)) {CG_Printf("CoD1 BRASS_R1: missing model %s\n",name);return 0;}
    return trap_R_RegisterModel(name);
}
static int cbSoundGame(void *u,const char *alias) {
    char (*names)[MAX_QPATH];char path[128],*text;int id,n,t,total=0;
    cbGameSound *s;(void)u;
    for(id=0;id<cbGame.soundCount;id++)if(ceEqual(alias,cbGame.sound[id].name))return cbGame.sound[id].count?id+1:0;
    if(cbGame.soundCount>=CB_SOUND_ALIASES)return 0;
    id=cbGame.soundCount++;s=&cbGame.sound[id];Q_strncpyz(s->name,alias,sizeof(s->name));
    names=(char (*)[MAX_QPATH])malloc(CI_FILES*MAX_QPATH);if(!names)return 0;
    n=ciFileNames("soundaliases",names);
    for(t=0;t<n && !s->count;t++) {
        cwSoundAliasVariant rows[CB_SOUND_VARIANTS+1];int bytes,count,i,j;
        snprintf(path,sizeof(path),"soundaliases/%s",names[t]);
        bytes=ceRead(path,&text,CE_CSV_LIMIT);if(bytes<0)continue;
        if(bytes>CI_SOUND_BYTES-total){free(text);break;}total+=bytes;
        count=CW_SoundAliasVariants(text,bytes,alias,rows,CB_SOUND_VARIANTS+1);free(text);
        if(count>CB_SOUND_VARIANTS){cbDiagnosticGame(NULL,"too many shell sound variants");break;}
        for(i=0;i<count;i++) {
            sfxHandle_t h;if(!CW_SoundAliasLoadSpecMatches(rows[i].loadspec,cgs.mapname))continue;if(!ciFileExists(rows[i].path))continue;
            for(j=0;j<s->count;j++)if(!strcmp(rows[i].path,s->variant[j].native.path)&&rows[i].sequence==s->variant[j].native.sequence)break;
            if(j<s->count)continue;
            h=trap_S_RegisterSound(rows[i].path);if(!h)continue;
            s->variant[s->count].handle=h;s->variant[s->count].native=rows[i];s->count++;
        }
    }
    free(names);
    if(!s->count){CG_Printf("CoD1 BRASS_R1: no usable sound alias %s\n",alias);return 0;}
    return id+1;
}
static float cbSoundUnit(unsigned *state) {
    *state=*state*1664525u+1013904223u;return ((*state>>8)&0xffffffu)*(1.0f/16777216.0f);
}
static int cbSoundPick(cbGameSound *s,unsigned *state) {
    float total=0,pick;int i;if(!s||!s->count)return -1;
    for(i=0;i<s->count;i++)total+=s->variant[i].native.probability;
    if(total<=0)return (int)(cbSoundUnit(state)*(float)s->count)%s->count;
    pick=cbSoundUnit(state)*total;for(i=0;i<s->count;i++){pick-=s->variant[i].native.probability;if(pick<=0)return i;}
    return s->count-1;
}
static void cbTraceGame(void *u,const cbVec start,const cbVec end,cbTrace *out) {
    trace_t tr;(void)u;memset(&tr,0,sizeof(tr));
    /* Fixed world model only: no player's hull, viewmodel, bodies or movers.
     * Zero bounds are intentional: original shell effects do not use useBBox. */
    trap_CM_BoxTrace(&tr,start,end,vec3_origin,vec3_origin,0,MASK_SOLID);
    out->fraction=tr.fraction;VectorCopy(tr.endpos,out->end);VectorCopy(tr.plane.normal,out->normal);
    out->startSolid=tr.startsolid;out->allSolid=tr.allsolid;
    if(tr.startsolid||tr.allsolid)cbGame.traceSolid++;
}
static void cbPlayGame(void *u,int sound,const cbVec pos,int when) {
    cbGameSound *s;cbGameSoundVariant *v;int pick;float gain,pitch;(void)u;(void)when;
    if(sound<1||sound>cbGame.soundCount)return;s=&cbGame.sound[sound-1];if(!s->count)return;
    pick=cbSoundPick(s,&cbGame.soundRandom);if(pick<0)return;v=&s->variant[pick];
    gain=v->native.volumeMin+(v->native.volumeMax-v->native.volumeMin)*cbSoundUnit(&cbGame.soundRandom);
    /* CoDSP 0x443390 applies a 0.8 scalar after native volume randomization. */
    gain*=0.8f;
    pitch=v->native.pitchMin+(v->native.pitchMax-v->native.pitchMin)*cbSoundUnit(&cbGame.soundRandom);
    trap_Cod1StartAliasSound((vec_t *)pos,ENTITYNUM_WORLD,CHAN_AUTO,v->handle,gain,pitch,v->native.distMin,v->native.distMax);
    cbGame.soundPlayed++;
}
static void cbDrawGame(void *u,int model,const cbVec pos,const cbVec *axis,const float *rgba) {
    refEntity_t ent;int i;(void)u;memset(&ent,0,sizeof(ent));
    ent.reType=RT_MODEL;ent.hModel=model;ent.nonNormalizedAxes=qtrue;
    VectorCopy(pos,ent.origin);VectorCopy(pos,ent.oldorigin);VectorCopy(pos,ent.lightingOrigin);
    for(i=0;i<3;i++)VectorCopy(axis[i],ent.axis[i]);
    for(i=0;i<4;i++)ent.shaderRGBA[i]=(byte)(ceClamp(rgba[i],0,1)*255+.5f);
    /* It is a WORLD model after ejection: never RF_DEPTHHACK/FIRST_PERSON. */
    ent.renderfx=RF_LIGHTING_ORIGIN;trap_R_AddRefEntityToScene(&ent);
}
static void cbEnsureGame(void) {
    cbIO io;char capability[32];if(cbGame.initialized)return;
    memset(&io,0,sizeof(io));io.read=cbReadGame;io.freeText=cbFreeGame;io.model=cbModelGame;
    io.sound=cbSoundGame;io.trace=cbTraceGame;io.playSound=cbPlayGame;io.drawModel=cbDrawGame;io.diagnostic=cbDiagnosticGame;
    cbInit(&cbGame.runtime,&io);cbGame.initialized=1;cbGame.soundRandom=0x613347a1u;
    trap_Cvar_Register(&cbEnabled,"cg_cod1BrassFx","1",CVAR_ARCHIVE);
    trap_Cvar_Register(&cbDebug,"cg_cod1BrassDebug","0",0);
    trap_Cvar_VariableStringBuffer("com_cod1BrassProtocol",capability,sizeof(capability));
    cbGame.capable=atoi(capability)==1;
    if(!cbGame.capable)CG_Printf("CoD1 BRASS_R1: matching EXE/game/cgame required; brass disabled.\n");
}
void CG_Cod1BrassShutdown(void) {
    if(cbGame.initialized)cbDestroy(&cbGame.runtime);
    memset(&cbGame,0,sizeof(cbGame));
}
void CG_Cod1BrassClear(void) {
    if(cbGame.initialized){cbClear(&cbGame.runtime);cbGame.runtime.clockValid=0;}
    cbGame.tagValid=0;cbGame.lastAcceptedValid=0;
}
void CG_Cod1BrassPrecache(const cwDef_t *def) {
    const char *name;int id,k;
    if(!def)return;cbEnsureGame();if(!cbGame.capable)return;
    id=CW_Find(def->name);if(id<0 || id>=CW_MAX_DEFS || cbGame.registered[id])return;
    cbGame.registered[id]=1;
    for(k=0;k<2;k++) {
        char canonical[CEFX_PATH];name=CW_String(def,k?"lastShotEjectEffect":"shellEjectEffect");if(!*name)continue;
        if(!cePath(name,canonical,1) || (strcmp(canonical,"fx/shellejects/pistol.efx") && strcmp(canonical,"fx/shellejects/rifle.efx"))) {
            CG_Printf("CoD1 BRASS_R1: %s requires unsupported shell definition %s\n",def->name,name);continue;
        }
        cbGame.effect[id][k]=cbRegister(&cbGame.runtime,canonical,0);
        if(!cbGame.effect[id][k])CG_Printf("CoD1 BRASS_R1: %s shell chain NOT ready; see resource error above\n",def->name);
    }
}
static int cbEquivalentFireCorrection(int oldEvent,int newEvent) {
    int oldFire=oldEvent==EV_FIRE_WEAPON || oldEvent==EV_FIRE_WEAPON_LASTSHOT || oldEvent==EV_FIRE_WEAPONB;
    int newFire=newEvent==EV_FIRE_WEAPON || newEvent==EV_FIRE_WEAPON_LASTSHOT || newEvent==EV_FIRE_WEAPONB;
    return oldFire && newFire;
}
static void cbBrassEvent(centity_t *cent,int oldEvent,int event,int sequence,int correction) {
    const cwDef_t *d;int index,fx,key,bolt;
    if(!cent || cent!=&cg.predictedPlayerEntity || !cg.snap || sequence<0 ||
       cent->currentState.number!=cg.predictedPlayerState.clientNum)return;
    index=cent->currentState.eventParm-1;
    if(index<0 || index>=CW_MAX_DEFS || !(d=CW_Get(index)))return;
    bolt=CW_Number(d,"boltAction")!=0;
    if((event==EV_COD1_EJECT_BRASS)!=bolt)return;
    if(event!=EV_COD1_EJECT_BRASS && event!=EV_FIRE_WEAPON && event!=EV_FIRE_WEAPON_LASTSHOT && event!=EV_FIRE_WEAPONB)return;
    cbEnsureGame();if(!cbGame.capable)return;
    /* The stock RTCW playerstate event readers already distinguish a newly
     * produced sequence from a prediction correction. Suppress only a
     * FIRE/LASTSHOT/B correction of the same already-predicted shot. A second
     * persistent sequence cache is invalid here because prediction can roll
     * eventSequence back and later reuse that number for a genuinely new shot. */
    if(correction && cbEquivalentFireCorrection(oldEvent,event)){cbGame.deduplicated++;return;}
    if(cbGame.lastAcceptedValid && sequence<=cbGame.lastAcceptedSequence)cbGame.sequenceReuse++;
    cbGame.lastAcceptedSequence=sequence;cbGame.lastAcceptedValid=1;
    trap_Cvar_Update(&cbEnabled);trap_Cvar_Update(&cbDebug);
    if(!cbEnabled.integer){cbGame.rejected++;return;}
    CG_Cod1BrassPrecache(d);fx=cbGame.effect[index][0];
    if(event==EV_FIRE_WEAPON_LASTSHOT && cbGame.effect[index][1])fx=cbGame.effect[index][1];
    if(!fx)return;key=CW_FamilyRoot(index)+1;
    cbClock(&cbGame.runtime,cg.time,cg.refdef.vieworg);
    cbQueue(&cbGame.runtime,fx,key,cg.time);
    if(cbDebug.integer)CG_Printf("CoD1 BRASS_R1: event=%d weapon=%s key=%d pending=%d\n",event,d->name,key,cbGame.runtime.pendingCount);
}
void CG_Cod1BrassEvent(centity_t *cent,int event,int sequence) {
    cbBrassEvent(cent,0,event,sequence,0);
}
void CG_Cod1BrassCorrectionEvent(centity_t *cent,int oldEvent,int newEvent,int sequence) {
    cbBrassEvent(cent,oldEvent,newEvent,sequence,1);
}
void CG_Cod1BrassBeginView(void) {
    if(!cbGame.initialized)return;
    trap_Cvar_Update(&cbEnabled);trap_Cvar_Update(&cbDebug);
    cbClock(&cbGame.runtime,cg.time,cg.refdef.vieworg);
    if(!cbEnabled.integer || cg.hyperspace){cbClear(&cbGame.runtime);cbGame.tagValid=0;}
}
void CG_Cod1BrassViewPose(int key,const refEntity_t *gun,
                          const orientation_t *poseTag,qboolean nativePose) {
    orientation_t tag;cbTag world;int i,j,k;
    if(!cbGame.initialized || !cbGame.capable || !cbEnabled.integer || !gun || cg.hyperspace)return;
    cbGame.key=key;
    if(nativePose ? poseTag==NULL : trap_R_LerpTag(&tag,gun,"tag_brass",0)<0) {
        if(cbGame.missingModel!=gun->hModel || cbGame.missingKey!=key) {
            cbGame.missingModel=gun->hModel;cbGame.missingKey=key;cbGame.missingTags++;
            CG_Printf("CoD1 BRASS_R1: model %d key %d missing tag_brass; no guessed offset\n",gun->hModel,key);
        }
        cbGame.tagValid=0;return;
    }
    if(nativePose)tag=*poseTag;
    VectorCopy(gun->origin,world.origin);
    for(i=0;i<3;i++)VectorMA(world.origin,tag.origin[i],gun->axis[i],world.origin);
    for(i=0;i<3;i++)for(j=0;j<3;j++) {
        world.axis[i][j]=0;for(k=0;k<3;k++)world.axis[i][j]+=tag.axis[i][k]*gun->axis[k][j];
    }
    if(!cbFinite(world.origin) || !cbFinite(world.axis[0]) || !cbFinite(world.axis[1]) || !cbFinite(world.axis[2])) {
        cbGame.tagValid=0;cbGame.rejected++;return;
    }
    cbGame.tag=world;cbGame.tagValid=1;cbGame.tagTime=cg.time;
    cbSubmitTag(&cbGame.runtime,key,&world);
}
void CG_Cod1BrassView(int key,const refEntity_t *gun) {
    CG_Cod1BrassViewPose(key,gun,NULL,qfalse);
}
void CG_Cod1BrassAdd(void) {
    if(!cbGame.initialized || !cbGame.capable)return;
    cbEndView(&cbGame.runtime);
    if(!cbEnabled.integer || cg.hyperspace)return;
    cbAdvance(&cbGame.runtime);cbRender(&cbGame.runtime);
}
void CG_Cod1BrassInfo_f(void) {
    int i,live=0,visible=0;cbRuntime *r=&cbGame.runtime;cbEnsureGame();
    for(i=0;i<CB_PARTS;i++)if(r->part[i].used){live++;if(r->part[i].model)visible++;}
    CG_Printf("CoD1 BRASS_R1: enabled=%d capable=%d defs=%d key=%d tag=%d tagTime=%d events=%u played=%u live=%d models=%d sounds=%u collisions=%u stopped=%u dropped=%u discarded=%u missingTags=%u startsolid=%u dedup=%u seqReuse=%u\n",
        cbEnabled.integer,cbGame.capable,r->definitions,cbGame.key,cbGame.tagValid,cbGame.tagTime,r->events,r->played,live,visible,cbGame.soundPlayed,r->collisions,r->stopped,r->dropped,r->discarded,cbGame.missingTags,cbGame.traceSolid,cbGame.deduplicated,cbGame.sequenceReuse);
    CG_Printf("  native EFX/WAV and alias gain/pitch/max-distance cutoff; RTCW spatial falloff remains provisional.\n");
    if(cbGame.tagValid)CG_Printf("  tag_brass world: %.3f %.3f %.3f\n",cbGame.tag.origin[0],cbGame.tag.origin[1],cbGame.tag.origin[2]);
}
#endif
