/* SPDX-License-Identifier: GPL-3.0-or-later
 * IMPACT_R1, included only by cg_cod1_efx.h / cg_effects.c.
 * Static-world impact events. Native definitions, bounded compatibility
 * execution. Unsupported definitions are diagnosed as a whole; no fabricated
 * default and no second RTCW puff. Full sound-alias mix parameters deferred.
 */
#ifndef CG_COD1_IMPACTS_H
#define CG_COD1_IMPACTS_H
#include "../game/bg_cod1_impact.h"
#include "../game/bg_cod1_soundalias.h"
#include "cg_cod1_impact_table.h"
#define CI_FILES 512
#define CI_LIST_BYTES 65536
#define CI_SOUND_VARIANTS 64
#define CI_SOUND_BYTES (16*1024*1024)
typedef struct { sfxHandle_t handle; cwSoundAliasVariant native; } ciSoundVariant;
typedef struct { ciSoundVariant variant[CI_SOUND_VARIANTS];int count; } ciSound;
static struct {
    int prepared,capable,tableFiles,tableErrors,unknown,missing;
    ciTable table;int effect[4][CI_MATERIALS],warned[4][CI_MATERIALS],soundWarned[2][CI_MATERIALS];
    ciSound sound[2][CI_MATERIALS];
    unsigned events,visuals,sounds,suppressed,unsupported,seed;
    int lastWeapon,lastMaterial,lastRifle;
} ciState;
static vmCvar_t ciEnabled,ciDebug;
void CG_Cod1ImpactReset(void) {
    memset(&ciState,0,sizeof(ciState));ciState.seed=0xb13974c1u;
    trap_Cvar_Register(&ciEnabled,"cg_cod1ImpactFx","1",CVAR_ARCHIVE);
    trap_Cvar_Register(&ciDebug,"cg_cod1ImpactDebug","0",0);
}
static int ciNameCompare(const void *a,const void *b) {
    return strcmp((const char *)a,(const char *)b);
}
/* Sorted VFS list. PK3 precedence is handled by FS, not guessed from pak names.
 * A full/truncated list is rejected rather than silently missing mod overrides. */
static int ciFileNames(const char *folder,char (*names)[MAX_QPATH]) {
    char *buf=(char *)calloc(1,CI_LIST_BYTES);int n,i,pos=0,out=0;
    if(!buf)return -1;
    n=trap_Cod1FS_GetFileList(folder,".csv",buf,CI_LIST_BYTES);
    if(n<0 || n>=CI_FILES) {free(buf);return -1;}
    for(i=0;i<n;i++) {
        int len=0,j,safe=1;
        while(pos+len<CI_LIST_BYTES && buf[pos+len])++len;
        if(pos+len>=CI_LIST_BYTES-2 || len<=0 || len>=MAX_QPATH) {free(buf);return -1;}
        for(j=0;j<len;j++)if(!(isalnum((unsigned char)buf[pos+j])||buf[pos+j]=='_'||buf[pos+j]=='-'||buf[pos+j]=='.'))safe=0;
        if(safe && !strstr(buf+pos,"..") && len>=4 && ceEqual(buf+pos+len-4,".csv")) {
            strcpy(names[out++],buf+pos);
        }
        pos+=len+1;
    }
    free(buf);qsort(names,(size_t)out,MAX_QPATH,ciNameCompare);return out;
}
static int ciFileExists(const char *path) {
    fileHandle_t f=0;int n=trap_FS_FOpenFile(path,&f,FS_READ);
    if(f)trap_FS_FCloseFile(f);return f && n>0;
}
static void ciLoadSounds(char (*names)[MAX_QPATH]) {
    char *text,*chosen;int n,t,c,m,i,j,total=0;char path[128],alias[96];
    /* Each file is read once. First table with usable variants for an alias
     * wins, matching the existing CoD sound adapter, not full alias merging. */
    n=ciFileNames("soundaliases",names);if(n<0) {++ciState.tableErrors;return;}
    for(t=0;t<n;t++) {
        int size;snprintf(path,sizeof(path),"soundaliases/%s",names[t]);
        size=ceRead(path,&text,CE_CSV_LIMIT);if(size<0)continue;
        if(size>CI_SOUND_BYTES-total) {free(text);++ciState.tableErrors;break;}total+=size;
        for(c=0;c<2;c++)for(m=0;m<CI_MATERIALS;m++) {
            cwSoundAliasVariant rows[CI_SOUND_VARIANTS+1];int count;
            ciSound *s=&ciState.sound[c][m];if(s->count)continue;
            snprintf(alias,sizeof(alias),"bullet_%s_%s",c?"large":"small",ciMaterials[m]);
            count=CW_SoundAliasVariants(text,size,alias,rows,CI_SOUND_VARIANTS+1);
            if(count>CI_SOUND_VARIANTS) {
                CG_Printf("CoD1 IMPACT_R1: %s exceeds %d sound variants; using first supported variants.\n",alias,CI_SOUND_VARIANTS);
                count=CI_SOUND_VARIANTS;
            }
            for(i=0;i<count;i++) {
                sfxHandle_t h;if(!CW_SoundAliasLoadSpecMatches(rows[i].loadspec,cgs.mapname))continue;chosen=rows[i].path;
                for(j=0;j<s->count;j++)if(!strcmp(s->variant[j].native.path,chosen)&&s->variant[j].native.sequence==rows[i].sequence)break;
                if(j<s->count||!ciFileExists(chosen))continue;
                h=trap_S_RegisterSound(chosen);if(!h)continue;
                s->variant[s->count].handle=h;s->variant[s->count].native=rows[i];s->count++;
            }
        }
        free(text);
    }
}
void CG_Cod1ImpactPrepare(void) {
    char value[32],path[128],error[256],*text;char (*names)[MAX_QPATH];
    ciTable *pending;int n,i,m,size,rows;
    if(ciState.prepared)return;
    ciState.prepared=1;
    trap_Cvar_Register(&ciEnabled,"cg_cod1ImpactFx","1",CVAR_ARCHIVE);
    trap_Cvar_Register(&ciDebug,"cg_cod1ImpactDebug","0",0);
    trap_Cvar_VariableStringBuffer("com_cod1ImpactProtocol",value,sizeof(value));
    if(atoi(value)!=COD1_IMPACT_VERSION) {
        CG_Printf("CoD1 IMPACT_R1: matching WolfSP.exe required; keeping legacy impacts.\n");return;
    }
    ciState.capable=1;
    names=(char (*)[MAX_QPATH])malloc(CI_FILES*MAX_QPATH);
    pending=(ciTable *)malloc(sizeof(*pending));
    if(!names||!pending){free(names);free(pending);ciState.capable=0;return;}
    n=ciFileNames("fx",names);
    if(n<0){++ciState.tableErrors;CG_Printf("CoD1 IMPACT_R1: FX CSV list exceeds safe limit.\n");}
    for(i=0;i<n;i++) {
        snprintf(path,sizeof(path),"fx/%s",names[i]);size=ceRead(path,&text,CE_CSV_LIMIT);
        if(size<0){++ciState.tableErrors;continue;}
        *pending=ciState.table;
        rows=ciParseCSV(pending,text,size,error);free(text);
        if(rows<0){++ciState.tableErrors;CG_Printf("CoD1 IMPACT_R1: %s: %s; overlay not applied\n",path,error);continue;}
        ciState.table=*pending;++ciState.tableFiles;
    }
    for(i=0;i<4;i++)for(m=0;m<CI_MATERIALS;m++) {
        ciRow *r=&ciState.table.row[i][m];
        if(!r->present){++ciState.missing;continue;}
        if(r->path[0])ciState.effect[i][m]=ceRegisterMode(r->path,0,2);
    }
    ciLoadSounds(names);free(names);free(pending);
    CG_Printf("CoD1 IMPACT_R1: %d sorted impact CSVs, %d absent rows, %d table errors. Model/water primitives still bounded by parser support; see cod1_impact_info.\n",
        ciState.tableFiles,ciState.missing,ciState.tableErrors);
}
static float ciSoundUnit(void){ciState.seed=ciState.seed*1664525u+1013904223u;return ((ciState.seed>>8)&0xffffffu)*(1.0f/16777216.0f);}
static int ciSoundPick(ciSound *s){float total=0,pick;int i;if(!s||!s->count)return -1;for(i=0;i<s->count;i++)total+=s->variant[i].native.probability;if(total<=0)return (int)(ciSoundUnit()*s->count)%s->count;pick=ciSoundUnit()*total;for(i=0;i<s->count;i++){pick-=s->variant[i].native.probability;if(pick<=0)return i;}return s->count-1;}
static void ciPlay(const entityState_t *es,int test) {
    int bits=es->time2&511,m=bits&31,rifle=!!(bits&CI_RIFLE),k,id;
    vec3_t dirs[2],basis[3];ciSound *sound;
    ++ciState.events;
    if(!(bits&CI_KNOWN)||m>=CI_MATERIALS) {m=0;++ciState.unknown;}
    ciState.lastWeapon=es->weapon;ciState.lastMaterial=m;ciState.lastRifle=rifle;
    if(bits&CI_NOIMPACT){++ciState.suppressed;return;}
    if(es->frame<0||es->frame>=NUMVERTEXNORMALS||es->eventParm<0||es->eventParm>=NUMVERTEXNORMALS)return;
    for(k=0;k<3;k++)if(!(es->origin[k]>-1e6f&&es->origin[k]<1e6f))return;
    ByteToDir(es->frame,dirs[0]);ByteToDir(es->eventParm,dirs[1]);
    /* Preserve legacy visible tracer generation once, not the retrace/puff.
     * Its start is the shot segment already captured by game, not the current gun. */
    if(!test && cg_tracerChance.value>0 && es->otherEntityNum>=0 && es->otherEntityNum<ENTITYNUM_WORLD) {
        vec3_t start,end;VectorCopy(es->origin2,start);VectorCopy(es->origin,end);
        CG_SpawnTracer(es->otherEntityNum,start,end);
    }
    trap_Cvar_Update(&ceEnabled);ceCheckClock();
    for(k=0;k<2;k++) {
        int c=rifle*2+k;ciRow *r=&ciState.table.row[c][m];
        if(!r->present || (r->path[0]&&!ciState.effect[c][m])) {
            ++ciState.unsupported;
            if(!ciState.warned[c][m]) {
                CG_Printf("CoD1 IMPACT_R1: %s/%s %s '%s'; native sound only, no fake default/RTCW puff.\n",
                    ciCategories[c],ciMaterials[m],r->present?"unsupported/missing EFX":"absent row",r->path);
                ciState.warned[c][m]=1;
            }
            continue;
        }
        if(!r->path[0])continue; /* deliberate empty is success */
        id=ciState.effect[c][m];
        if(ceEnabled.integer && id) {
            int old=ceImpactNoMarks;ceImpactNoMarks=!!(bits&CI_NOMARKS);
            ceBasis(dirs[k],basis);ceSpawn(id,es->origin,basis,cg.time,0);
            ceImpactNoMarks=old;++ciState.visuals;
        }
    }
    sound=&ciState.sound[rifle][m];
    if(sound->count) {
        int pick=ciSoundPick(sound);ciSoundVariant *v=pick>=0?&sound->variant[pick]:NULL;
        if(v) {
            float gain=v->native.volumeMin+(v->native.volumeMax-v->native.volumeMin)*ciSoundUnit();
            float pitch=v->native.pitchMin+(v->native.pitchMax-v->native.pitchMin)*ciSoundUnit();
            vec3_t origin;VectorCopy(es->origin,origin);gain*=0.8f;
            trap_Cod1StartAliasSound(origin,ENTITYNUM_WORLD,CHAN_AUTO,v->handle,gain,pitch,v->native.distMin,v->native.distMax);
        }
        ++ciState.sounds;
    } else if(!ciState.soundWarned[rifle][m]) {
        CG_Printf("CoD1 IMPACT_R1: no usable bullet_%s_%s sound (CSV/WAV); not substituting RTCW.\n",rifle?"large":"small",ciMaterials[m]);
        ciState.soundWarned[rifle][m]=1;
    }
    trap_Cvar_Update(&ciDebug);
    if(ciDebug.integer)CG_Printf("CoD1 IMPACT_R1 hit: weapon=%d class=%s material=%s(%s/%d) normal=%d reflect=%d soundVariants=%d\n",
        es->weapon,rifle?"large":"small",ciMaterials[m],(bits&CI_KNOWN)?"native":"unknown",bits&31,es->frame,es->eventParm,sound->count);
}
qboolean CG_Cod1BulletImpact(const entityState_t *es) {
    if(!CI_IsEvent(es))return qfalse;
    CG_Cod1ImpactPrepare();trap_Cvar_Update(&ciEnabled);
    if(!ciState.capable||!ciEnabled.integer)return qfalse;
    ciPlay(es,0);return qtrue;
}
void CG_Cod1ImpactInfo_f(void) {
    int m;CG_Cod1ImpactPrepare();trap_Cvar_Update(&ciEnabled);
    CG_Printf("CoD1 IMPACT_R1: enabled=%d protocol=%d events=%u FX=%u sounds=%u suppressed=%u unsupported=%u unknownMaterials=%d lastWeapon=%d lastClass=%s lastMaterial=%s\n",
        ciEnabled.integer,ciState.capable,ciState.events,ciState.visuals,ciState.sounds,ciState.suppressed,ciState.unsupported,ciState.unknown,
        ciState.lastWeapon,ciState.lastRifle?"large":"small",ciMaterials[ciState.lastMaterial]);
    for(m=0;m<CI_MATERIALS;m++)CG_Printf("  %-10s smallFX=%d largeFX=%d sounds=%d/%d\n",ciMaterials[m],
        ciState.effect[0][m],ciState.effect[2][m],ciState.sound[0][m].count,ciState.sound[1][m].count);
}
void CG_Cod1ImpactTest_f(void) {
    char size[32]="small",mat[32]="";vec3_t end;trace_t tr;entityState_t es;int m=-1;
    if(!cg.snap){CG_Printf("Load a map first.\n");return;}
    if(trap_Argc()>1)trap_Argv(1,size,sizeof(size));
    if(trap_Argc()>2)trap_Argv(2,mat,sizeof(mat));
    if((!ceEqual(size,"small")&&!ceEqual(size,"large"))||(mat[0]&&(m=ciIndex(mat,ciMaterials,CI_MATERIALS))<0)) {
        CG_Printf("Usage: cod1_impact_test [small|large] [material]; cosmetic only.\n");return;
    }
    CG_Cod1ImpactPrepare();if(!ciState.capable)return;
    VectorMA(cg.refdef.vieworg,1024,cg.refdef.viewaxis[0],end);
    CG_Trace(&tr,cg.refdef.vieworg,NULL,NULL,end,cg.snap->ps.clientNum,MASK_SOLID);
    if(tr.fraction>=1||tr.startsolid||tr.allsolid||tr.entityNum!=ENTITYNUM_WORLD) {
        CG_Printf("CoD1 IMPACT_R1: aim at static world geometry within 1024 units.\n");return;
    }
    memset(&es,0,sizeof(es));CI_WriteEvent(&es,&tr,tr.endpos,cg.refdef.vieworg,end,cg.snap->ps.weapon,ceEqual(size,"large"));
    es.otherEntityNum=ENTITYNUM_NONE;
    if(m>=0)es.time2=(es.time2&~63)|m|CI_KNOWN;
    ciPlay(&es,1);
    CG_Printf("CoD1 IMPACT_R1: cosmetic test only, no shot/ammo/damage.\n");
}
#endif
