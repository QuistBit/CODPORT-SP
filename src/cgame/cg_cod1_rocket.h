/* SPDX-License-Identifier: GPL-3.0-or-later
 * Native CoD1 rocket_explode selector/audio bridge for projectile impacts.
 * Included after cg_cod1_impacts.h by cg_cod1_efx.h.
 */
#ifndef CG_COD1_ROCKET_H
#define CG_COD1_ROCKET_H
#include "../game/bg_cod1_rocket.h"
#define CR_SOUND_VARIANTS 16
typedef struct { int present,effect; char path[CEFX_PATH]; ciSound sound; } crMaterial;
static struct {
    int prepared,tableErrors,soundTables,soundListFallback,soundMissingFiles,soundRegisterFailures;
    unsigned seed,events,visuals,sounds,unsupported,soundVariants;
    int lastMaterial,lastSoundPick;
    crMaterial mat[CI_MATERIALS];
} crState;
void CG_Cod1RocketReset(void) { memset(&crState,0,sizeof(crState));crState.seed=0x72503131u;crState.lastSoundPick=-1; }
static float crUnit(void){crState.seed=crState.seed*1664525u+1013904223u;return ((crState.seed>>8)&0xffffffu)*(1.0f/16777216.0f);}
static int crPick(ciSound *s){float total=0,pick;int i;if(!s||!s->count)return -1;for(i=0;i<s->count;i++)total+=s->variant[i].native.probability;if(total<=0)return (int)(crUnit()*s->count)%s->count;pick=crUnit()*total;for(i=0;i<s->count;i++){pick-=s->variant[i].native.probability;if(pick<=0)return i;}return s->count-1;}
static int crParseFxCSV(const char *text,int size,char error[256]) {
    int pos=0,row=0,changed=0;error[0]=0;
    while(pos<size) {
        char col[3][CEFX_PATH];int field=0,k=0,quote=0,bad=0,m;memset(col,0,sizeof(col));++row;
        while(pos<size) { char c=text[pos++];
            if(c=='"'){if(quote&&pos<size&&text[pos]=='"'){++pos;continue;}quote=!quote;continue;}
            if(!quote&&(c==','||c=='\r'||c=='\n')){if(c==','){++field;k=0;continue;}if(c=='\r'&&pos<size&&text[pos]=='\n')++pos;break;}
            if(field<3){if(k+1<CEFX_PATH)col[field][k++]=c;else bad=1;}else bad=1;
        }
        if(quote){snprintf(error,256,"unterminated rocket CSV quote row %d",row);return -1;}
        if(!col[0][0]||col[0][0]=='#'||!ceEqual(col[0],"rocket_explode"))continue;
        m=ciIndex(col[1],ciMaterials,CI_MATERIALS);if(bad||field<1||field>2||m<0){snprintf(error,256,"invalid rocket CSV row %d",row);return -1;}
        crState.mat[m].present=1;crState.mat[m].path[0]=0;
        if(col[2][0]&&!cePath(col[2],crState.mat[m].path,1)){snprintf(error,256,"unsafe rocket path row %d",row);return -1;}
        ++changed;
    }
    return changed;
}
static int crLoadSoundText(const char *text,int size) {
    int m,i,j,added=0;char alias[96];
    for(m=0;m<CI_MATERIALS;m++) {
        cwSoundAliasVariant rows[CR_SOUND_VARIANTS+1];int count;ciSound *sound=&crState.mat[m].sound;const char *mat=ciMaterials[m];
        if(sound->count)continue;
        if(m==22)mat="asphault"; /* retail alias typo; FX table spells asphalt */
        snprintf(alias,sizeof(alias),"rocket_explode_%s",mat);
        count=CW_SoundAliasVariants(text,size,alias,rows,CR_SOUND_VARIANTS+1);
        if(count>CR_SOUND_VARIANTS)count=CR_SOUND_VARIANTS;
        for(i=0;i<count&&sound->count<CI_SOUND_VARIANTS;i++) {
            sfxHandle_t h;
            if(!CW_SoundAliasLoadSpecMatches(rows[i].loadspec,cgs.mapname))continue;
            for(j=0;j<sound->count;j++)
                if(!strcmp(sound->variant[j].native.path,rows[i].path)&&sound->variant[j].native.sequence==rows[i].sequence)break;
            if(j<sound->count)continue;
            if(!ciFileExists(rows[i].path)){++crState.soundMissingFiles;continue;}
            h=trap_S_RegisterSound(rows[i].path);
            if(!h){++crState.soundRegisterFailures;continue;}
            sound->variant[sound->count].handle=h;sound->variant[sound->count].native=rows[i];++sound->count;++added;++crState.soundVariants;
        }
    }
    return added;
}
static int crLoadSoundTable(const char *path) {
    char *text;int size=ceRead(path,&text,CE_CSV_LIMIT),added;
    if(size<0)return -1;
    ++crState.soundTables;added=crLoadSoundText(text,size);free(text);return added;
}
static void crLoadSounds(char (*names)[MAX_QPATH]) {
    static const char *retail[]={"soundaliases/iw_sound.csv","soundaliases/iw_sound2.csv"};
    int direct[2],n,t;char path[128];
    /* These are the same retail tables used by the already proven weapon and
     * grenade sound adapters.  Open them directly first: VFS enumeration is a
     * useful override/fallback mechanism, not a prerequisite for stock CoD1. */
    direct[0]=crLoadSoundTable(retail[0]);direct[1]=crLoadSoundTable(retail[1]);
    n=ciFileNames("soundaliases",names);
    if(n<0){if(direct[0]<0&&direct[1]<0)++crState.tableErrors;return;}
    for(t=0;t<n;t++) {
        if((direct[0]>=0&&ceEqual(names[t],"iw_sound.csv")) ||
           (direct[1]>=0&&ceEqual(names[t],"iw_sound2.csv")))continue;
        snprintf(path,sizeof(path),"soundaliases/%s",names[t]);
        if(crLoadSoundTable(path)>=0)++crState.soundListFallback;
    }
}
static void crPrepare(void) {
    char (*names)[MAX_QPATH];int n,i,m,size;char path[128],error[256],*text;
    if(crState.prepared)return;crState.prepared=1;names=(char (*)[MAX_QPATH])malloc(CI_FILES*MAX_QPATH);if(!names){++crState.tableErrors;return;}
    n=ciFileNames("fx",names);if(n<0){++crState.tableErrors;free(names);return;}
    for(i=0;i<n;i++){snprintf(path,sizeof(path),"fx/%s",names[i]);size=ceRead(path,&text,CE_CSV_LIMIT);if(size<0)continue;if(crParseFxCSV(text,size,error)<0){++crState.tableErrors;CG_Printf("CoD1 rocket FX: %s: %s\n",path,error);}free(text);}
    for(m=0;m<CI_MATERIALS;m++)if(crState.mat[m].present&&crState.mat[m].path[0])crState.mat[m].effect=ceRegisterMode(crState.mat[m].path,0,4);
    crLoadSounds(names);free(names);
    if(!crState.mat[0].sound.count)CG_Printf("CoD1 ROCKET_R3: no usable rocket_explode_default sound alias; run /cod1_rocket_info.\n");
    else CG_Printf("CoD1 ROCKET_R3: %u sound variants from %d retail/override tables ready; run /cod1_rocket_info for details.\n",
        crState.soundVariants,crState.soundTables);
}
void CG_Cod1RocketPrecache(void) { CG_Cod1EfxPrepare();crPrepare(); }
qboolean CG_Cod1RocketImpact(const entityState_t *es,const vec3_t position) {
    int bits,m,id,pick;vec3_t normal,basis[3];crMaterial *r;
    if(!CR_IsEvent(es))return qfalse;crPrepare();++crState.events;bits=es->time2&511;m=bits&31;if(!(bits&CI_KNOWN)||m>=CI_MATERIALS)m=0;r=&crState.mat[m];crState.lastMaterial=m;crState.lastSoundPick=-1;
    if(bits&CI_NOIMPACT)return qtrue;
    if(es->frame<0||es->frame>=NUMVERTEXNORMALS)return qtrue;ByteToDir(es->frame,normal);ceBasis(normal,basis);trap_Cvar_Update(&ceEnabled);ceCheckClock();
    id=r->effect;if(r->present&&r->path[0]&&id&&ceEnabled.integer){int old=ceImpactNoMarks;ceImpactNoMarks=CR_NoMarks(es);ceSpawn(id,position,basis,cg.time,0);ceImpactNoMarks=old;++crState.visuals;}
    else if(r->present&&r->path[0])++crState.unsupported;
    pick=crPick(&r->sound);crState.lastSoundPick=pick;if(pick>=0){ciSoundVariant *v=&r->sound.variant[pick];float gain=v->native.volumeMin+(v->native.volumeMax-v->native.volumeMin)*crUnit();float pitch=v->native.pitchMin+(v->native.pitchMax-v->native.pitchMin)*crUnit();vec3_t origin;VectorCopy(position,origin);gain*=0.8f;trap_Cod1StartAliasSound(origin,ENTITYNUM_WORLD,ceEqual(v->native.channel,"voice")?CHAN_VOICE:CHAN_AUTO,v->handle,gain,pitch,v->native.distMin,v->native.distMax);++crState.sounds;}
    return qtrue;
}
void CG_Cod1RocketInfo_f(void) {
    int m;crPrepare();
    CG_Printf("CoD1 ROCKET_R3: errors=%d soundTables=%d enumFallback=%d variants=%u missingWav=%d registerFail=%d events=%u FX=%u sounds=%u unsupported=%u lastMaterial=%s lastSoundPick=%d\n",
        crState.tableErrors,crState.soundTables,crState.soundListFallback,crState.soundVariants,
        crState.soundMissingFiles,crState.soundRegisterFailures,crState.events,crState.visuals,crState.sounds,
        crState.unsupported,ciMaterials[crState.lastMaterial],crState.lastSoundPick);
    for(m=0;m<CI_MATERIALS;m++)CG_Printf("  %-10s FX=%d soundVariants=%d path=%s\n",ciMaterials[m],
        crState.mat[m].effect,crState.mat[m].sound.count,crState.mat[m].path[0]?crState.mat[m].path:"<empty>");
}
void CG_Cod1RocketTest_f(void) {
    char mat[32]="";vec3_t end;trace_t tr;entityState_t es;int m=-1;
    if(!cg.snap){CG_Printf("Load a map first.\n");return;}
    if(trap_Argc()>1)trap_Argv(1,mat,sizeof(mat));
    if(mat[0]&&(m=ciIndex(mat,ciMaterials,CI_MATERIALS))<0){CG_Printf("Usage: cod1_rocket_test [material]; cosmetic only.\n");return;}
    crPrepare();VectorMA(cg.refdef.vieworg,1024,cg.refdef.viewaxis[0],end);
    CG_Trace(&tr,cg.refdef.vieworg,NULL,NULL,end,cg.snap->ps.clientNum,MASK_SOLID);
    if(tr.fraction>=1||tr.startsolid||tr.allsolid||tr.entityNum!=ENTITYNUM_WORLD){CG_Printf("CoD1 ROCKET_R3: aim at static world geometry within 1024 units.\n");return;}
    memset(&es,0,sizeof(es));CR_WriteEvent(&es,&tr);es.event=EV_COD1_ROCKET_EXPLODE;
    if(m>=0)es.time2=(es.time2&~31)|m|CI_KNOWN;
    CG_Cod1RocketImpact(&es,tr.endpos);CG_Printf("CoD1 ROCKET_R3: cosmetic impact test only, no ammo/damage.\n");
}
#endif
