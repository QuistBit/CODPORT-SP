/* SPDX-License-Identifier: GPL-3.0-or-later
 * NATIVE_EFX_R1 + IMPACT_R1. Included by cg_effects.c only; see the
 * opt-in impact protocol and mark projection in bg_cod1_impact.h.
 * Data-driven, experimental CoD EFX subset. Renderer, defaults, emitter
 * sampling and collision details are NOT certified identical to CoDSP.exe.
 * See docs/reverse/NATIVE_EFX_R1.md for the evidence/assumption boundary.
 */
#ifndef CG_COD1_EFX_H
#define CG_COD1_EFX_H
#include "cg_cod1_efx_parse.h"
#include "cg_cod1_muzzle_math.h"
#include "cg_cod1_line_math.h"
#include "cg_cod1_script_fx.h"
#include <limits.h>
#define CE_CACHE 128
#define CE_LIVE 2048
#define CE_DEPTH 8
#define CE_BUDGET 2048
#define CE_MATERIALS 23
#define CE_CSV_LIMIT (1024*1024)

typedef struct {
    char path[CEFX_PATH]; int mode,state; /* 1 loading, 2 ready, -1 failed */
    uint64_t scriptTicket;
    ceDefinition *def;
    qhandle_t shaders[CEFX_PRIMS][CEFX_MEDIA];
    qhandle_t models[CEFX_PRIMS][CEFX_MEDIA];
    int child[CEFX_PRIMS][CEFX_MEDIA], emitted[CEFX_PRIMS][CEFX_MEDIA];
    int impactFx[CEFX_PRIMS][CEFX_MEDIA];
    int deathFx[CEFX_PRIMS][CEFX_MEDIA];
} ceCached;
typedef struct {
    int used,type,defId,primId,shader,flags,depth,nonUniform;
    int muzzle,relative,impact,rocket,noMarks; unsigned attachment,noise;
    int model,impactFx,deathFx; float bounce; vec3_t angles,spin;
    float length0,length1;int lengthLinear;
    double start,end,last,emitTick;
    vec3_t base,pos,vel,acc,axis[3],emitOrigin;
    vec3_t lineEndBase,lineEnd; /* second endpoint: local if attached, world otherwise */
    float rgb0[3],rgb1[3],alpha0,alpha1,size0,size1,size20,size21;
    int rgbLinear,alphaLinear,sizeLinear,size2Linear;
    float rotation,rotationDelta,emitDistance;
    int script;
    uint64_t scriptCacheGeneration,scriptSceneGeneration,scriptSerial;
    int worldOwner;uint64_t worldEpoch;uint32_t worldGeneration;
    vec3_t worldLocal,worldBasis[3];
} ceLive;
/* Nonzero only during the no-callback mode7 primitive commit. */
static struct {int owner;uint64_t epoch;uint32_t generation;} ceWorldSpawning;
static int(*ceWorldResolver)(int,uint64_t,uint32_t,float[16]);
static int ceWorldResolve(int owner,uint64_t epoch,uint32_t generation,float matrix[16]){return ceWorldResolver&&ceWorldResolver(owner,epoch,generation,matrix);}
static const char *ceMaterials[CE_MATERIALS]={"default","bark","brick","carpet","cloth",
    "concrete","dirt","flesh","foliage","glass","grass","gravel","ice","metal","mud",
    "paper","plaster","rock","sand","snow","water","wood","asphalt"};
static struct {
    ceCached cache[CE_CACHE]; ceLive live[CE_LIVE];
    int cacheCount,prepared,tableOK,present[CE_MATERIALS],effect[CE_MATERIALS];
    int budget,budgetTime,allocCursor,clockValid,clock,warnedRenderer;
    unsigned int random,dropped,spawned,collided,played;
    char paths[CE_MATERIALS][CEFX_PATH];
} ceState;
static vmCvar_t ceEnabled;
/* Separate local-view scene state, never a server entity or protocol field. */
#define CE_SHOTS 16
typedef struct {
    int valid,key,model;unsigned generation;orientation_t tag;
} ceViewAttachment;
static struct {
    ceViewAttachment view;
    struct {int effect,key,time;} shot[CE_SHOTS];int count,warnedRenderer;
    unsigned events,played,discarded,missingTags;
} ceMuzzle;
static vmCvar_t ceMuzzleEnabled;
static int ceImpactNoMarks;
void CG_Cod1RocketReset(void);
/* These tokens deliberately live outside memset-cleared ceState. A callback
 * can reset the module/cache while a script registrar owns unpublished data. */
static uint64_t ceScriptCacheGeneration=1,ceScriptSceneGeneration=1;
static uint64_t ceScriptTicket,ceScriptSerial;
static void ceScriptAdvance(uint64_t *value) { if(!++*value)++*value; }
static int ceScriptLiveCurrent(const ceLive *l,uint64_t cache,uint64_t scene,uint64_t serial) {
    return !serial || (l->used && l->script && l->scriptSerial==serial &&
        cache==ceScriptCacheGeneration && scene==ceScriptSceneGeneration &&
        l->scriptCacheGeneration==cache && l->scriptSceneGeneration==scene);
}

static unsigned int ceRandom(void) { ceState.random=1664525U*ceState.random+1013904223U;return ceState.random; }
static float ceUnit(void) { return (ceRandom()>>8)*(1.0f/16777216.0f); }
static float ceSample(ceRange r) { return r.a+(r.b-r.a)*ceUnit(); }
static float ceMix(float a,float b,float t) { return a+(b-a)*t; }
static float ceClamp(float x,float lo,float hi) { return x<lo?lo:x>hi?hi:x; }
static void ceBasis(const vec3_t normal,vec3_t axis[3]) {
    vec3_t up;
    VectorCopy(normal,axis[0]);
    if(VectorNormalize(axis[0])<0.001f) VectorSet(axis[0],0,0,1);
    if(fabs(axis[0][2])>0.99f) VectorSet(up,0,1,0);else VectorSet(up,0,0,1);
    CrossProduct(up,axis[0],axis[1]);VectorNormalize(axis[1]);
    CrossProduct(axis[0],axis[1],axis[2]);VectorNormalize(axis[2]);
}
static void ceTransform(const vec3_t v,const vec3_t axis[3],vec3_t out) {
    int i;for(i=0;i<3;++i) out[i]=v[0]*axis[0][i]+v[1]*axis[1][i]+v[2]*axis[2][i];
}
static void ceSampleVector(const ceRange *v,vec3_t out) {
    int i;for(i=0;i<3;++i) out[i]=ceSample(v[i]);
}
static int ceRead(const char *name,char **text,int max) {
    int n; fileHandle_t f=0;
    *text=NULL;n=trap_FS_FOpenFile(name,&f,FS_READ);
    if(!f) return -1;
    if(n<=0||n>max) { trap_FS_FCloseFile(f);return -1; }
    *text=(char *)malloc((size_t)n+1);
    if(!*text) { trap_FS_FCloseFile(f);return -1; }
    trap_FS_Read(*text,n,f);(*text)[n]=0;trap_FS_FCloseFile(f);return n;
}
void CG_Cod1EfxClear(void) {
    ceScriptAdvance(&ceScriptSceneGeneration);
    memset(ceState.live,0,sizeof(ceState.live));
    ceMuzzle.count=0;ceMuzzle.view.valid=0;
    ceState.clockValid=0;ceState.budget=CE_BUDGET;ceState.budgetTime=INT_MIN;ceState.allocCursor=0;
}
void CG_Cod1EfxShutdown(void) {
    int i;
    ceScriptAdvance(&ceScriptCacheGeneration);
    ceScriptAdvance(&ceScriptSceneGeneration);
    CG_Cod1ImpactReset();CG_Cod1RocketReset();
    for(i=0;i<ceState.cacheCount;++i) free(ceState.cache[i].def);
    memset(&ceState,0,sizeof(ceState));memset(&ceMuzzle,0,sizeof(ceMuzzle));
}
void CG_Cod1EfxReset(void) {
    CG_Cod1EfxShutdown();ceState.random=0xa61579b3U;
    ceState.budgetTime=INT_MIN;
    trap_Cvar_Register(&ceEnabled,"cg_cod1NativeEfx","1",CVAR_ARCHIVE);
    trap_Cvar_Register(&ceMuzzleEnabled,"cg_cod1MuzzleFx","1",CVAR_ARCHIVE);
}
static int ceRendererReady(void) {
    char value[32];
    trap_Cvar_VariableStringBuffer("r_cod1EfxShaders",value,sizeof(value));
    if(atoi(value)>0) return 1;
    if(!ceState.warnedRenderer) {
        CG_Printf("CoD1 NATIVE_EFX_R1: renderer has no fxshaders support/resources; rebuild wolf + renderer and keep original pak5.pk3. Using compatibility FX.\n");
        ceState.warnedRenderer=1;
    }
    return 0;
}
static int ceRegisterMode(const char *name,int depth,int mode) {
    char path[CEFX_PATH],error[256],*text;
    int i,j,id,n,ok=1;ceCached *c;
    if(!cePath(name,path,1)) { CG_Printf("CoD1 NATIVE_EFX_R1: rejected effect path\n");return 0; }
    if(depth>CE_DEPTH) { CG_Printf("CoD1 NATIVE_EFX_R1: effect nesting limit: %s\n",path);return 0; }
    for(i=0;i<ceState.cacheCount;++i) if(ceState.cache[i].mode==mode && !strcmp(path,ceState.cache[i].path)) {
        if(ceState.cache[i].state==1) CG_Printf("CoD1 NATIVE_EFX_R1: cyclic effect dependency: %s\n",path);
        return ceState.cache[i].state==2?i+1:0;
    }
    if(ceState.cacheCount>=CE_CACHE) { CG_Printf("CoD1 NATIVE_EFX_R1: definition cache full\n");return 0; }
    id=++ceState.cacheCount;c=&ceState.cache[id-1];strcpy(c->path,path);c->state=1;c->mode=mode;
    n=ceRead(path,&text,CEFX_FILE_LIMIT);
    if(n<0) { CG_Printf("CoD1 NATIVE_EFX_R1: missing/invalid EFX %s\n",path);c->state=-1;return 0; }
    c->def=(ceDefinition *)malloc(sizeof(*c->def));
    if(!c->def || !ceParseMode(text,(size_t)n,c->def,error,mode)) {
        CG_Printf("CoD1 NATIVE_EFX_R1: cannot read %s: %s (mode %d)\n",path,c->def?error:"allocation failure",mode);
        free(text);free(c->def);c->def=NULL;c->state=-1;return 0;
    }
    free(text);
    for(i=0;i<c->def->count;++i) {
        cePrimitive *p=&c->def->prim[i];
        for(j=0;j<p->shaderCount;++j) {
            c->shaders[i][j]=trap_R_RegisterShader(p->shaders[j]);
            if(!c->shaders[i][j]) { ok=0;CG_Printf("CoD1 NATIVE_EFX_R1: missing shader %s in %s\n",p->shaders[j],path); }
        }
        for(j=0;j<p->playCount;++j) { c->child[i][j]=ceRegisterMode(p->play[j],depth+1,mode);if(!c->child[i][j])ok=0; }
        for(j=0;j<p->emitCount;++j) { c->emitted[i][j]=ceRegisterMode(p->emit[j],depth+1,mode);if(!c->emitted[i][j])ok=0; }
        if(mode==4) {
            for(j=0;j<p->modelCount;++j) {
                c->models[i][j]=trap_R_RegisterModel(p->models[j]);
                if(!c->models[i][j]) { ok=0;CG_Printf("CoD1 NATIVE_EFX_R1: missing model %s in %s\n",p->models[j],path); }
            }
            for(j=0;j<p->impactCount;++j) {
                c->impactFx[i][j]=ceRegisterMode(p->impact[j],depth+1,mode);
                if(!c->impactFx[i][j])ok=0;
            }
            for(j=0;j<p->deathCount;++j) {
                c->deathFx[i][j]=ceRegisterMode(p->death[j],depth+1,mode);
                if(!c->deathFx[i][j])ok=0;
            }
        }
    }
    c->state=ok?2:-1;
    CG_Printf("CoD1 NATIVE_EFX_R1: %s %s (%d components; mode %d)\n",ok?"ready":"incomplete",path,c->def->count,mode);
    return ok?id:0;
}
static int ceRegister(const char *name,int depth) { return ceRegisterMode(name,depth,0); }
/* Accepted grenade path keeps its single-table selection unchanged.
 * Bullet tables below use the capability-gated VFS-list extension instead. */
static int ceImpactTable(const char *text,int size) {
    int pos=0,row=0,seen=0;
    while(pos<size) {
        char col[3][CEFX_PATH];int field=0,k=0,quote=0,overflow=0;
        memset(col,0,sizeof(col));
        while(pos<size) {
            char c=text[pos++];
            if(c=='"') {
                if(quote && pos<size && text[pos]=='"') { ++pos; }
                else { quote=!quote;continue; }
            }
            if(!quote && (c==','||c=='\r'||c=='\n')) {
                if(c==',') { ++field;k=0;continue; }
                if(c=='\r' && pos<size && text[pos]=='\n')++pos;
                break;
            }
            if(field<3) {
                if(k+1<CEFX_PATH) col[field][k++]=c;else overflow=1;
            }
        }
        ++row;
        if(quote) { CG_Printf("CoD1 NATIVE_EFX_R1: unterminated CSV quote at row %d\n",row);return 0; }
        /* Comment columns can legitimately exceed CEFX_PATH. */
        if(!col[0][0] || col[0][0]=='#') continue;
        if(!ceEqual(col[0],"grenade_explode")) continue;
        if(overflow||field>2) { CG_Printf("CoD1 NATIVE_EFX_R1: invalid grenade CSV row %d\n",row);return 0; }
        for(k=0;k<CE_MATERIALS;++k) if(ceEqual(col[1],ceMaterials[k])) break;
        if(k==CE_MATERIALS) { CG_Printf("CoD1 NATIVE_EFX_R1: unknown CSV surface '%s'\n",col[1]);return 0; }
        ceState.present[k]=1;ceState.paths[k][0]=0;
        if(col[2][0] && !cePath(col[2],ceState.paths[k],1)) return 0;
        ++seen;
    }
    return seen>0 && ceState.present[0];
}
void CG_Cod1EfxPrepare(void) {
    char *text;int i,n;
    if(ceState.prepared) return;
    trap_Cvar_Register(&ceEnabled,"cg_cod1NativeEfx","1",CVAR_ARCHIVE);
    if(!ceRendererReady()) return;
    ceState.prepared=1;
    CG_Cod1ImpactPrepare();
    n=ceRead("fx/iw_impacts.csv",&text,CE_CSV_LIMIT);
    if(n>0) { ceState.tableOK=ceImpactTable(text,n);free(text); }
    if(!ceState.tableOK) { CG_Printf("CoD1 NATIVE_EFX_R1: no valid fx/iw_impacts.csv; fallback enabled\n");return; }
    for(i=0;i<CE_MATERIALS;++i) if(ceState.present[i] && ceState.paths[i][0]) ceState.effect[i]=ceRegister(ceState.paths[i],0);
    CG_Printf("CoD1 NATIVE_EFX_R1: file-driven grenade FX enabled (experimental execution, not full CoD EFX equivalence). Unsupported definitions use GRENADE_FEEDBACK_R1.\n");
}
static void ceCheckClock(void) {
    if(ceState.clockValid && cg.time<ceState.clock) CG_Cod1EfxClear();
    ceState.clock=cg.time;ceState.clockValid=1;
    if(ceState.budgetTime!=cg.time) { ceState.budget=CE_BUDGET;ceState.budgetTime=cg.time; }
}
static ceLive *ceAlloc(void) {
    int i,index;
    if(ceState.budget<=0) { ++ceState.dropped;return NULL; }
    --ceState.budget;
    for(i=0;i<CE_LIVE;++i) {
        index=(ceState.allocCursor+i)%CE_LIVE;
        if(!ceState.live[index].used) {
            ceLive *l=&ceState.live[index];memset(l,0,sizeof(*l));l->used=1;
            ceState.allocCursor=(index+1)%CE_LIVE;++ceState.spawned;return l;
        }
    }
    ++ceState.dropped;return NULL;
}
static float ceEmitterStep(const cePrimitive *p) {
    /* Provisional distance interpretation, not particles/second. */
    return fmaxf(0.25f,fabsf(ceSample(p->density)+(2*ceUnit()-1)*ceSample(p->variance)));
}
static void ceSpawn(int id,const vec3_t origin,const vec3_t basis[3],double at,int depth);
static void ceMake(int id,int primId,const vec3_t origin,const vec3_t basis[3],double at,int depth,
    const ceViewAttachment *attach) {
    ceCached *c=&ceState.cache[id-1];const cePrimitive *p=&c->def->prim[primId];
    ceLive *l;vec3_t axis[3],offset,raw,pos,shape;float a,b,r,h,life,linked;int i,j;
    float cull=fabsf(ceSample(p->cull));
    if(cull>0 && DistanceSquared(origin,cg.refdef.vieworg)>cull*cull) return;
    memcpy(axis,basis,sizeof(axis));
    /* Relative primitives integrate in attachment-local coordinates. The tag
     * is evaluated from the FINAL displayed pose, separately for each view. */
    if((attach||ceWorldSpawning.owner) && (p->flags&CE_RELATIVE)) AxisClear(axis);
    ceSampleVector(p->origin,raw);
    if(p->spawn&CE_CHEAP) VectorCopy(raw,offset);else ceTransform(raw,axis,offset);
    if((attach||ceWorldSpawning.owner) && (p->flags&CE_RELATIVE)) VectorCopy(offset,pos);
    else VectorAdd(origin,offset,pos);
    if(p->spawn&(CE_SPHERE|CE_CYLINDER)) {
        a=ceUnit()*6.28318530718f;r=ceSample(p->radius);h=ceSample(p->height);
        if(p->spawn&CE_SPHERE) {
            b=ceUnit()*3.14159265359f;
            VectorSet(shape,sinf(a)*sinf(b)*r,cosf(a)*sinf(b)*r,cosf(b)*h);
        } else {
            b=(ceUnit()-0.5f)*h;
            for(i=0;i<3;++i) shape[i]=r*(cosf(a)*axis[1][i]+sinf(a)*axis[2][i])+b*axis[0][i];
        }
        VectorAdd(pos,shape,pos);
        if(p->spawn&CE_AXIS) ceBasis(shape,axis);
    }
    if(p->type==CE_RUNNER) {
        if(ceState.budget<=0) { ++ceState.dropped;return; }
        --ceState.budget;
        j=(int)(ceRandom()%(unsigned)p->playCount);
        if((p->spawn&CE_RAND_FWD) && (c->mode==4 || c->mode==6)) {
            vec3_t rotated[3];float a=ceUnit()*6.28318530718f,co=cosf(a),si=sinf(a);int k;
            VectorCopy(axis[0],rotated[0]);
            for(k=0;k<3;k++) {
                rotated[1][k]=co*axis[1][k]+si*axis[2][k];
                rotated[2][k]=-si*axis[1][k]+co*axis[2][k];
            }
            ceSpawn(c->child[primId][j],pos,rotated,at,depth+1);
        } else ceSpawn(c->child[primId][j],pos,axis,at,depth+1);
        return;
    }
    life=ceSample(p->life);
    if(at+(double)life<=(double)cg.time) return;
    l=ceAlloc();if(!l)return;
    l->type=p->type;l->defId=id;l->primId=primId;l->flags=p->flags;l->depth=depth;
    l->start=at;l->end=at+life;l->last=at;l->emitTick=at;
    l->muzzle=attach!=NULL;l->relative=attach && (p->flags&CE_RELATIVE);
    l->impact=(c->mode==2||c->mode==4);l->rocket=c->mode==4;l->noMarks=l->impact && ceImpactNoMarks;
    l->script=c->mode==6||c->mode==7;
    if(c->mode==7&&ceWorldSpawning.owner&&(p->flags&CE_RELATIVE)){l->worldOwner=ceWorldSpawning.owner;l->worldEpoch=ceWorldSpawning.epoch;l->worldGeneration=ceWorldSpawning.generation;VectorCopy(pos,l->worldLocal);}
    if(l->script) {
        ceScriptAdvance(&ceScriptSerial);l->scriptSerial=ceScriptSerial;
        l->scriptCacheGeneration=ceScriptCacheGeneration;
        l->scriptSceneGeneration=ceScriptSceneGeneration;
    }
    if(attach)l->attachment=attach->generation;
    l->noise=ceState.random; /* no extra RNG consumption for existing effects */
    VectorCopy(pos,l->base);VectorCopy(pos,l->pos);VectorCopy(pos,l->emitOrigin);memcpy(l->axis,axis,sizeof(axis));
    ceSampleVector(p->velocity,raw);
    if(p->spawn&CE_ABS_VEL) VectorCopy(raw,l->vel);else ceTransform(raw,axis,l->vel);
    ceSampleVector(p->acceleration,raw);
    if(p->spawn&CE_ABS_ACCEL) VectorCopy(raw,l->acc);else ceTransform(raw,axis,l->acc);
    l->acc[2]+=ceSample(p->gravity);
    linked=ceUnit();
    for(i=0;i<3;++i) {
        l->rgb0[i]=ceMix(p->rgb.start[i].a,p->rgb.start[i].b,p->spawn&CE_RGB_LINK?linked:ceUnit());
        l->rgb1[i]=ceMix(p->rgb.end[i].a,p->rgb.end[i].b,p->spawn&CE_RGB_LINK?linked:ceUnit());
    }
    l->alpha0=ceSample(p->alpha.start[0]);l->alpha1=ceSample(p->alpha.end[0]);
    l->size0=ceSample(p->size.start[0]);l->size1=ceSample(p->size.end[0]);
    l->size20=ceSample(p->size2.start[0]);l->size21=ceSample(p->size2.end[0]);
    l->rgbLinear=p->rgb.linear;l->alphaLinear=p->alpha.linear;l->sizeLinear=p->size.linear;l->size2Linear=p->size2.linear;
    l->rotation=ceSample(p->rotation);l->rotationDelta=ceSample(p->rotationDelta);l->nonUniform=p->nonUniform;
    if(p->shaderCount) l->shader=c->shaders[primId][ceRandom()%(unsigned)p->shaderCount];
    if((l->rocket || l->script) && p->modelCount) {
        l->model=c->models[primId][ceRandom()%(unsigned)p->modelCount];
        if(p->impactCount)l->impactFx=c->impactFx[primId][ceRandom()%(unsigned)p->impactCount];
        ceSampleVector(p->angle,l->angles);ceSampleVector(p->angleDelta,l->spin);l->bounce=ceSample(p->bounce);
    }
    if(l->rocket && p->deathCount)
        l->deathFx=c->deathFx[primId][ceRandom()%(unsigned)p->deathCount];
    if(p->type==CE_EMITTER && !l->model) l->emitDistance=ceEmitterStep(p);
    if(p->type==CE_LINE) {
        /* origin2 is relative to the EFFECT origin, not the first endpoint.
         * Additional RNG consumption is restricted to this new primitive. */
        ceSampleVector(p->origin2,raw);ceTransform(raw,axis,offset);
        if(l->relative) VectorCopy(offset,l->lineEndBase);
        else VectorAdd(origin,offset,l->lineEndBase);
        VectorCopy(l->lineEndBase,l->lineEnd);
    }
    if(p->type==CE_TAIL) {
        l->length0=ceSample(p->length.start[0]);l->length1=ceSample(p->length.end[0]);l->lengthLinear=p->length.linear;
    }
}
static void ceSpawn(int id,const vec3_t origin,const vec3_t basis[3],double at,int depth) {
    ceCached *c;int i,j,count;float delay;
    if(id<=0||id>ceState.cacheCount||depth>CE_DEPTH) { ++ceState.dropped;return; }
    c=&ceState.cache[id-1];if(c->state!=2) return;
    for(i=0;i<c->def->count;++i) {
        const cePrimitive *p=&c->def->prim[i];
        count=(int)floorf(ceSample(p->count)+0.5f);
        for(j=0;j<count;++j) {
            if(p->spawn&CE_EVEN) delay=fminf(p->delay.a,p->delay.b)+fabsf(p->delay.b-p->delay.a)*j/count;
            else delay=ceSample(p->delay);
            ceMake(id,i,origin,basis,at+delay,depth,NULL);
        }
    }
}
static void cePosition(const ceLive *l,double at,vec3_t out) {
    double t=(at-l->start)*0.001;int i;
    for(i=0;i<3;++i) out[i]=(float)(l->base[i]+l->vel[i]*t+0.5*l->acc[i]*t*t);
}
/* Swept cosmetic collisions only. No damage, grenade entities or pmove data. */
static int ceMove(ceLive *l,double at) {
    vec3_t end;trace_t trace;int i;
    uint64_t cache=l->script?l->scriptCacheGeneration:0;
    uint64_t scene=l->script?l->scriptSceneGeneration:0;
    uint64_t serial=l->script?l->scriptSerial:0;
    if(!ceScriptLiveCurrent(l,cache,scene,serial))return 0;
    if(l->worldOwner){
        float matrix[16];vec3_t local;int k;float dt=(float)((at-l->last)*.001);
        if(!ceWorldResolve(l->worldOwner-1,l->worldEpoch,l->worldGeneration,matrix)||!ceScriptLiveCurrent(l,cache,scene,serial)){if(ceScriptLiveCurrent(l,cache,scene,serial))l->used=0;return 0;}
        if(dt>0)for(k=0;k<3;k++){l->vel[k]+=l->acc[k]*dt;l->worldLocal[k]+=l->vel[k]*dt+.5f*l->acc[k]*dt*dt;}
        for(k=0;k<3;k++)VectorCopy(matrix+k*4,l->worldBasis[k]);ceTransform(l->worldLocal,l->worldBasis,local);VectorAdd(matrix+12,local,l->pos);
        if((l->type==CE_PARTICLE)&&at>l->last)ceMuzzleRotate(&l->rotation,&l->rotationDelta,(float)(at-l->last));
        l->last=at;return 1;
    }
    /* CoDSP Line::Update 0x483f70: keep both endpoints, update their
     * attachment each scene, then only evaluate visual curves in ceDraw.
     * No velocity, gravity, particle-spin or collision integration here. */
    if(l->type==CE_LINE) {
        if(l->relative) {
            vec3_t world;
            if(!ceMuzzle.view.valid || ceMuzzle.view.generation!=l->attachment) {
                l->used=0;return 0;
            }
            ceTransform(l->base,ceMuzzle.view.tag.axis,world);
            VectorAdd(ceMuzzle.view.tag.origin,world,l->pos);
            ceTransform(l->lineEndBase,ceMuzzle.view.tag.axis,world);
            VectorAdd(ceMuzzle.view.tag.origin,world,l->lineEnd);
        } else {
            VectorCopy(l->base,l->pos);VectorCopy(l->lineEndBase,l->lineEnd);
        }
        l->last=at;return 1;
    }
    if((l->impact || l->script) && at>l->last) {
        int k;float dt=(float)((at-l->last)*0.001);
        /* Inspected CoDSP 0x482f80 order, NOT server grenade physics. */
        for(k=0;k<3;k++) {
            l->vel[k]+=l->acc[k]*dt;
            end[k]=l->pos[k]+l->vel[k]*dt+0.5f*l->acc[k]*dt*dt;
        }
    } else if(l->impact || l->script)VectorCopy(l->pos,end);
    else cePosition(l,at,end);
    if(l->relative) {
        vec3_t world;
        if(!ceMuzzle.view.valid || ceMuzzle.view.generation!=l->attachment) { l->used=0;return 0; }
        ceTransform(end,ceMuzzle.view.tag.axis,world);VectorAdd(ceMuzzle.view.tag.origin,world,end);
    }
    if((l->flags&CE_PHYSICS) && at>l->last) {
        if(l->script) {
            vec3_t start;VectorCopy(l->pos,start);
            CG_Trace(&trace,start,NULL,NULL,end,ENTITYNUM_NONE,MASK_SOLID);
        } else CG_Trace(&trace,l->pos,NULL,NULL,end,ENTITYNUM_NONE,MASK_SOLID);
        if(!ceScriptLiveCurrent(l,cache,scene,serial))return 0;
        if((l->rocket || l->script) && l->model) {
            if(trace.startsolid||trace.allsolid) {
                /* Model debris born on an impact plane may initially overlap it.
                 * Do not manufacture a second impact at an undefined normal. */
                VectorCopy(end,l->pos);l->last=at;return 1;
            }
            if(trace.fraction<1.0f) {
                const cePrimitive *p=&ceState.cache[l->defId-1].def->prim[l->primId];
                vec3_t hitBasis[3];float dot;int k;
                ++ceState.collided;ceBasis(trace.plane.normal,hitBasis);
                if((l->flags&CE_IMPACTFX) && l->impactFx)ceSpawn(l->impactFx,trace.endpos,hitBasis,at,l->depth+1);
                if(l->flags&CE_IMPACT_KILLS) { l->used=0;return 0; }
                dot=DotProduct(l->vel,trace.plane.normal);
                for(k=0;k<3;k++)l->vel[k]=(l->vel[k]-2.0f*dot*trace.plane.normal[k])*l->bounce;
                VectorCopy(trace.endpos,l->pos);
                if(trace.plane.normal[2]>0.0f && fabsf(l->vel[2])<4.0f) {
                    VectorClear(l->vel);VectorClear(l->acc);l->flags&=~CE_PHYSICS;
                }
                l->last=at;return 1;
            }
        } else if(trace.startsolid||trace.allsolid||trace.fraction<1.0f) {
            l->used=0;++ceState.collided;return 0;
        }
    }
    /* CoDSP 0x483a20: frame delta in ms, x0.01 rotation and x0.00065
     * damping. Guard by the simulation timestamp so a stereo redraw cannot
     * integrate spin twice. The old non-muzzle path deliberately stays intact. */
    if((l->muzzle||l->impact||l->script) && (l->type==CE_PARTICLE||l->type==CE_ORIENTED) && at>l->last)
        ceMuzzleRotate(&l->rotation,&l->rotationDelta,(float)(at-l->last));
    if((l->rocket || l->script) && l->model && at>l->last) {
        float ms=(float)(at-l->last);for(i=0;i<3;i++)l->angles[i]+=ms*.01f*l->spin[i];
    }
    VectorCopy(end,l->pos);l->last=at;return 1;
}
static void ceExpire(ceLive *l) {
    if((l->flags&CE_DEATHFX) && l->deathFx) {
        int child=l->deathFx,depth=l->depth+1,previous=ceImpactNoMarks;
        vec3_t pos,basis[3];double when=l->end;
        /* The supported snow particle is nonphysical. Evaluate its natural
         * end position at the exact death timestamp, then launch deathFx. */
        if(l->last<when && !ceMove(l,when)) { l->used=0;return; }
        VectorCopy(l->pos,pos);memcpy(basis,l->axis,sizeof(basis));
        l->used=0;
        if(l->impact)ceImpactNoMarks=l->noMarks;
        ceSpawn(child,pos,basis,when,depth);
        ceImpactNoMarks=previous;
        return;
    }
    l->used=0;
}
static void ceDecal(ceLive *l) {
    vec3_t a,b;trace_t trace;
    if(l->script) {
        ceLive copy=*l;
        int life;
        if(!ceScriptLiveCurrent(l,copy.scriptCacheGeneration,copy.scriptSceneGeneration,copy.scriptSerial))return;
        VectorMA(copy.base,8,copy.axis[0],a);VectorMA(copy.base,-32,copy.axis[0],b);
        CG_Trace(&trace,a,NULL,NULL,b,ENTITYNUM_NONE,MASK_SOLID);
        if(!ceScriptLiveCurrent(l,copy.scriptCacheGeneration,copy.scriptSceneGeneration,copy.scriptSerial))return;
        life=(int)(copy.end-(double)cg.time);
        /* Retire before the external mark submission; a callback can reuse
         * this slot, and the old owner must not clear its replacement. */
        l->used=0;
        if(!trace.startsolid&&!trace.allsolid&&trace.fraction<1 &&
           !(trace.surfaceFlags&(SURF_SKY|SURF_NOIMPACT|SURF_NOMARKS)) && copy.size0>0 && life>0)
            CG_ImpactMark(copy.shader,trace.endpos,trace.plane.normal,copy.rotation,
                ceClamp(copy.rgb0[0],0,1),ceClamp(copy.rgb0[1],0,1),ceClamp(copy.rgb0[2],0,1),
                ceClamp(copy.alpha0,0,1),qtrue,ceClamp(copy.size0,0,8192),qfalse,life);
        return;
    }
    if(l->impact) {
        int life=(int)(l->end-(double)cg.time);
        if(!l->noMarks && l->size0>0 && life>0)
            CG_Cod1ImpactMark(l->shader,l->base,l->axis[0],l->rotation,
                ceClamp(l->rgb0[0],0,1),ceClamp(l->rgb0[1],0,1),ceClamp(l->rgb0[2],0,1),
                ceClamp(l->alpha0,0,1),qtrue,ceClamp(l->size0,0,8192),qfalse,life);
        l->used=0;return;
    }
    /* Explicitly don't project a floating explosion onto a distant floor. */
    VectorMA(l->base,8,l->axis[0],a);VectorMA(l->base,-32,l->axis[0],b);
    CG_Trace(&trace,a,NULL,NULL,b,ENTITYNUM_NONE,MASK_SOLID);
    if(!trace.startsolid&&!trace.allsolid&&trace.fraction<1 &&
       !(trace.surfaceFlags&(SURF_SKY|SURF_NOIMPACT|SURF_NOMARKS)) && l->size0>0) {
        int life=(int)(l->end-(double)cg.time);
        if(life>0) CG_ImpactMark(l->shader,trace.endpos,trace.plane.normal,l->rotation,
            ceClamp(l->rgb0[0],0,1),ceClamp(l->rgb0[1],0,1),ceClamp(l->rgb0[2],0,1),
            ceClamp(l->alpha0,0,1),qtrue,ceClamp(l->size0,0,8192),qfalse,life);
    }
    l->used=0;
}
static void ceTickEmitter(ceLive *l,double now) {
    const cePrimitive *p=&ceState.cache[l->defId-1].def->prim[l->primId];
    int ticks=0,j;double end=fmin(now,l->end);
    /* Fixed temporal sampling plus distance threshold. Catch-up is bounded;
     * old effects aren't replayed indefinitely after a pause/time jump. */
    while(l->emitTick+8<=end && ticks++<128) {
        double t=l->emitTick+8;vec3_t delta;
        if(!ceMove(l,t))return;
        l->emitTick=t;VectorSubtract(l->pos,l->emitOrigin,delta);
        if(VectorLengthSquared(delta)>=l->emitDistance*l->emitDistance) {
            j=(int)(ceRandom()%(unsigned)p->emitCount);
            {
                int previous=ceImpactNoMarks;
                if(l->impact)ceImpactNoMarks=l->noMarks;
                ceSpawn(ceState.cache[l->defId-1].emitted[l->primId][j],l->pos,l->axis,t,l->depth+1);
                ceImpactNoMarks=previous;
            }
            VectorCopy(l->pos,l->emitOrigin);l->emitDistance=ceEmitterStep(p);
        }
    }
    if(ticks>=128) { l->used=0;++ceState.dropped; }
    if(now>=l->end) l->used=0;
}
/* RGB/size/length random flags multiply the start blend weight. The
 * arithmetic is checked against CoDSP instructions; RNG order remains ours. */
static float ceCurveBias(const ceLive *l,const ceCurve *curve,float t,unsigned salt) {
    /* R200 explicit PORT interpolation policy, not a recovered CoDSP formula.
     * The original fire_emitter1 size supplies end=133 and no parm/start.
     * Preserve sampled endpoint values and use a bounded smooth transition.
     * Parser admission limits this to mode6 Particle.size without mixed flags. */
    if(l->script&&curve->nonlinear){t=ceClamp(t,0.0f,1.0f);return t*t*(3.0f-2.0f*t);}
    return ceMuzzleCurveBias(curve->linear,curve->random,t,
        ceMuzzleRandomUnit(l->noise,(unsigned)cg.time,salt));
}
static void ceSubmitQuad(const ceLive *l,const vec3_t center,const vec3_t left,const vec3_t up,const byte color[4]) {
    if(l->script && !ceScriptLiveCurrent(l,l->scriptCacheGeneration,l->scriptSceneGeneration,l->scriptSerial))return;
    if(l->muzzle) {
        refEntity_t e;memset(&e,0,sizeof(e));e.reType=RT_POLY;e.customShader=l->shader;
        VectorCopy(center,e.origin);VectorCopy(center,e.oldorigin);
        VectorCopy(left,e.axis[1]);VectorCopy(up,e.axis[2]);
        VectorScale(cg.refdef.viewaxis[0],-1,e.axis[0]);
        e.radius=VectorLength(left)+VectorLength(up);memcpy(e.shaderRGBA,color,4);
        e.renderfx=RF_FIRST_PERSON|((l->relative || (l->flags&CE_DEPTHHACK))?RF_DEPTHHACK:0);
        e.shaderTime=(float)(l->start*0.001);trap_R_AddRefEntityToScene(&e);
    } else if(l->flags&CE_SHADER_TIME) {
        /* CoD EFX setShaderTime: restart shader animation at FX spawn time.
         * refEntity.shaderTime is subtracted from refdef time by the renderer. */
        refEntity_t e;memset(&e,0,sizeof(e));e.reType=RT_POLY;e.customShader=l->shader;
        VectorCopy(center,e.origin);VectorCopy(center,e.oldorigin);
        VectorCopy(left,e.axis[1]);VectorCopy(up,e.axis[2]);
        VectorScale(cg.refdef.viewaxis[0],-1,e.axis[0]);
        e.radius=VectorLength(left)+VectorLength(up);memcpy(e.shaderRGBA,color,4);
        e.shaderTime=(float)(l->start*0.001);
        trap_R_AddRefEntityToScene(&e);
    } else {
        polyVert_t v[4];int i,k;
        static const float corner[4][2]={{1,1},{-1,1},{-1,-1},{1,-1}};
        static const float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
        memset(v,0,sizeof(v));
        for(i=0;i<4;i++) {
            for(k=0;k<3;k++)v[i].xyz[k]=center[k]+corner[i][0]*left[k]+corner[i][1]*up[k];
            memcpy(v[i].modulate,color,4);memcpy(v[i].st,uv[i],sizeof(uv[i]));
        }
        trap_R_AddPolyToScene(l->shader,4,v);
    }
}
static void ceDraw(const ceLive *l) {
    if(l->script && !ceScriptLiveCurrent(l,l->scriptCacheGeneration,l->scriptSceneGeneration,l->scriptSerial))return;
    float t=(float)(((double)cg.time-l->start)/(l->end-l->start));
    float size=ceMix(l->size0,l->size1,l->sizeLinear?t:0);
    float size2=l->nonUniform?ceMix(l->size20,l->size21,l->size2Linear?t:0):size;
    float alpha=ceClamp(ceMix(l->alpha0,l->alpha1,l->alphaLinear?t:0),0,1);
    const cePrimitive *p=&ceState.cache[l->defId-1].def->prim[l->primId];
    vec3_t rgb;int i;
    if(l->muzzle||l->impact||l->script) {
        size=ceMix(l->size0,l->size1,ceCurveBias(l,&p->size,t,1));
        size2=l->nonUniform?ceMix(l->size20,l->size21,ceCurveBias(l,&p->size2,t,2)):size;
        alpha=ceMuzzleAlpha(l->alpha0,l->alpha1,p->alpha.linear,p->alpha.random,t,
            ceMuzzleRandomUnit(l->noise,(unsigned)cg.time,3));
    }
    for(i=0;i<3;++i)rgb[i]=ceClamp(ceMix(l->rgb0[i],l->rgb1[i],(l->muzzle||l->impact||l->script)?ceCurveBias(l,&p->rgb,t,4):(l->rgbLinear?t:0)),0,1);
    size=ceClamp(size,0,8192);size2=ceClamp(size2,0,8192);
    if(l->model && size>0) {
        refEntity_t e;memset(&e,0,sizeof(e));e.reType=RT_MODEL;e.hModel=l->model;e.nonNormalizedAxes=qtrue;
        VectorCopy(l->pos,e.origin);VectorCopy(l->pos,e.oldorigin);VectorCopy(l->pos,e.lightingOrigin);
        AnglesToAxis(l->angles,e.axis);for(i=0;i<3;i++)VectorScale(e.axis[i],size,e.axis[i]);
        e.renderfx=RF_LIGHTING_ORIGIN;for(i=0;i<3;i++)e.shaderRGBA[i]=(byte)(255*rgb[i]);e.shaderRGBA[3]=(byte)(255*alpha);
        trap_R_AddRefEntityToScene(&e);
    } else if(l->type==CE_LIGHT) {
        if(size>0)trap_R_AddLightToScene(l->pos,size,rgb[0],rgb[1],rgb[2],0);
    } else if(l->type==CE_LINE && l->shader && size>0) {
        vec3_t center,left,up;byte color[4];
        if(!ceMuzzleLineQuad(l->pos,l->lineEnd,cg.refdef.vieworg,size,center,left,up))return;
        for(i=0;i<3;i++)color[i]=(byte)(255*rgb[i]*((l->flags&CE_ALPHA)?1:alpha));
        color[3]=(byte)(255*((l->flags&CE_ALPHA)?alpha:1));
        ceSubmitQuad(l,center,left,up,color);
    } else if(l->type==CE_TAIL && l->shader && size>0) {
        vec3_t direction,eye,side,half,center,raw;byte color[4];float length;
        float elapsed=(float)(((double)cg.time-l->start)*0.001);
        for(i=0;i<3;i++)raw[i]=l->vel[i]+((l->impact||l->script)?0:l->acc[i]*elapsed);
        if(l->worldOwner)ceTransform(raw,l->worldBasis,direction);else if(l->relative)ceTransform(raw,ceMuzzle.view.tag.axis,direction);else VectorCopy(raw,direction);
        if(VectorNormalize(direction)<0.0001f) {
            if(l->worldOwner)VectorCopy(l->worldBasis[0],direction);else if(l->relative)VectorCopy(ceMuzzle.view.tag.axis[0],direction);else VectorCopy(l->axis[0],direction);
        }
        length=ceClamp(ceMix(l->length0,l->length1,ceCurveBias(l,&p->length,t,5)),0,8192);
        if(length<=0)return;
        VectorMA(l->pos,-0.5f*length,direction,center);
        VectorSubtract(cg.refdef.vieworg,center,eye);CrossProduct(direction,eye,side);
        if(VectorNormalize(side)<0.0001f) {
            CrossProduct(direction,cg.refdef.viewaxis[2],side);
            if(VectorNormalize(side)<0.0001f) { CrossProduct(direction,cg.refdef.viewaxis[1],side);VectorNormalize(side); }
        }
        VectorScale(side,size,side);VectorScale(direction,0.5f*length,half);
        for(i=0;i<3;i++)color[i]=(byte)(255*rgb[i]*((l->flags&CE_ALPHA)?1:alpha));
        color[3]=(byte)(255*((l->flags&CE_ALPHA)?alpha:1));
        /* U runs along the streak; original material handles its own UV scale. */
        ceSubmitQuad(l,center,half,side,color);
    } else if(l->type==CE_ORIENTED && l->shader && size>0 && size2>0) {
        /* CoD OrientedParticle: keep the quad in the effect-local plane
         * instead of billboarding it to the camera. axisFromSphere can
         * replace this basis per spawn, which is used by snow blast. */
        vec3_t left,up;float a,si,co;byte color[4];
        a=l->rotation*0.01745329252f;si=sinf(a);co=cosf(a);
        for(i=0;i<3;++i) {
            left[i]=size*(co*l->axis[1][i]-si*l->axis[2][i]);
            up[i]=size2*(si*l->axis[1][i]+co*l->axis[2][i]);
            color[i]=(byte)(255*rgb[i]*((l->flags&CE_ALPHA)?1:alpha));
        }
        color[3]=(byte)(255*((l->flags&CE_ALPHA)?alpha:1));
        ceSubmitQuad(l,l->pos,left,up,color);
    } else if(l->type==CE_PARTICLE && l->shader && size>0 && size2>0) {
        /* Muzzle spin was integrated once in ceMove. Legacy grenade spin
         * remains the accepted R1 absolute-time expression. */
        vec3_t left,up;float a,si,co;byte color[4];
        a=(l->muzzle||l->impact||l->script)?l->rotation:
            l->rotation+l->rotationDelta*(float)(((double)cg.time-l->start)*0.001);
        a*=0.01745329252f;
        si=sinf(a);co=cosf(a);
        for(i=0;i<3;++i) {
            left[i]=size*(co*cg.refdef.viewaxis[1][i]-si*cg.refdef.viewaxis[2][i]);
            up[i]=size2*(si*cg.refdef.viewaxis[1][i]+co*cg.refdef.viewaxis[2][i]);
            color[i]=(byte)(255*rgb[i]*((l->flags&CE_ALPHA)?1:alpha));
        }
        color[3]=(byte)(255*((l->flags&CE_ALPHA)?alpha:1));
        ceSubmitQuad(l,l->pos,left,up,color);
    }
}
void CG_Cod1EfxAdd(void) {
    int i;double now=(double)cg.time;
    trap_Cvar_Update(&ceEnabled);ceCheckClock();
    if(!ceEnabled.integer) { CG_Cod1EfxClear();return; }
    /* Emitters first so their new children can be updated and drawn this frame. */
    for(i=0;i<CE_LIVE;++i) {
        ceLive *l=&ceState.live[i];
        if(l->used && !l->muzzle && l->type==CE_EMITTER && !l->model && now>=l->start) ceTickEmitter(l,now);
    }
    for(i=0;i<CE_LIVE;++i) {
        ceLive *l=&ceState.live[i];
        if(!l->used || l->muzzle || now<l->start || (l->type==CE_EMITTER && !l->model))continue;
        if(now>=l->end) { ceExpire(l);continue; }
        if(l->type==CE_DECAL) { ceDecal(l);continue; }
        if(ceMove(l,now))ceDraw(l);
    }
}
void CG_Cod1EfxPrecacheWeapon(const char *effect) {
    if(effect && *effect && ceRendererReady()) (void)ceRegister(effect,0);
}
/* Narrow projectile-trail contract for the inspected Panzerfaust root:
 * one relative emitFx Emitter.  The root itself is a scheduler; its child FX
 * is spawned in world space along the missile trajectory.  Mode 5 keeps this
 * separate from rocket-impact mode 4 (no impact/noMarks semantics). */
int CG_Cod1EfxRegisterProjectileTrail(const char *effect) {
    int id;ceCached *c;cePrimitive *p;
    if(!effect||!*effect||!ceRendererReady())return 0;
    id=ceRegisterMode(effect,0,5);if(!id)return 0;
    c=&ceState.cache[id-1];
    if(!c->def||c->def->count!=1)return 0;
    p=&c->def->prim[0];
    if(p->type!=CE_EMITTER || !(p->flags&CE_EMIT) || !(p->flags&CE_RELATIVE) ||
       p->emitCount<1 || p->modelCount || p->shaderCount || p->playCount)
        return 0;
    return id;
}
static int ceProjectileTrailInterval(const cePrimitive *p) {
    float density=fabsf(ceSample(p->density));
    float variance=fabsf(ceSample(p->variance));
    float ms=density+(2.0f*ceUnit()-1.0f)*variance;
    return (int)fmaxf(1.0f,floorf(ms+0.5f));
}
void CG_Cod1EfxProjectileTrail(centity_t *ent,const weaponInfo_t *wi) {
    ceCached *c;const cePrimitive *p;entityState_t *es;int id,next,count=0,j;
    vec3_t origin,velocity,basis[3];
    if(!ent||!wi||(id=wi->cod1MissileTrailEffect)<=0||id>ceState.cacheCount)return;
    trap_Cvar_Update(&ceEnabled);if(!ceEnabled.integer||!ceRendererReady())return;
    c=&ceState.cache[id-1];if(c->state!=2||!c->def||c->def->count!=1)return;
    p=&c->def->prim[0];es=&ent->currentState;
    if(es->pos.trType==TR_STATIONARY){ent->trailTime=cg.time;ent->cod1TrailNextTime=cg.time;return;}
    ceCheckClock();
    next=ent->cod1TrailNextTime;
    if(next<=0)next=ent->trailTime;
    /* Do not replay an unbounded backlog after a pause/snapshot jump. */
    if(next<cg.time-1000)next=cg.time-1000;
    while(next<=cg.time && count++<128) {
        BG_EvaluateTrajectory(&es->pos,next,origin);
        if(!(CG_PointContents(origin,-1)&(CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA))) {
            BG_EvaluateTrajectoryDelta(&es->pos,next,velocity);
            ceBasis(velocity,basis);
            j=(int)(ceRandom()%(unsigned)p->emitCount);
            ceSpawn(c->emitted[0][j],origin,basis,(double)next,0);
        }
        next+=ceProjectileTrailInterval(p);
    }
    if(count>=128)next=cg.time+ceProjectileTrailInterval(p);
    ent->cod1TrailNextTime=next;ent->trailTime=cg.time;
}
qboolean CG_Cod1EfxGrenade(const vec3_t origin,const vec3_t direction,
    const char *material,const char *weaponEffect) {
    vec3_t basis[3];int m,id=0,extra=0;
    trap_Cvar_Update(&ceEnabled);
    if(!ceEnabled.integer||!ceRendererReady())return qfalse;
    CG_Cod1EfxPrepare();if(!ceState.tableOK)return qfalse;
    for(m=0;m<CE_MATERIALS;++m)if(ceEqual(material,ceMaterials[m]))break;
    if(m==CE_MATERIALS||!ceState.present[m])m=0;
    id=ceState.effect[m];
    /* Blank row deliberately suppresses surface FX, NOT default fallback. */
    if(ceState.paths[m][0]&&!id)return qfalse;
    if(weaponEffect&&*weaponEffect) { extra=ceRegister(weaponEffect,0);if(!extra)return qfalse; }
    ceCheckClock();ceBasis(direction,basis);
    if(id)ceSpawn(id,origin,basis,cg.time,0);
    if(extra&&extra!=id)ceSpawn(extra,origin,basis,cg.time,0);
    ++ceState.played;return qtrue;
}
void CG_Cod1EfxTest_f(void) {
    char path[CEFX_PATH];vec3_t end,origin,dir,basis[3];trace_t tr;int id;
    if(!cg.snap) { CG_Printf("CoD1 NATIVE_EFX_R1: load a map first\n");return; }
    if(trap_Argc()>1)trap_Argv(1,path,sizeof(path));else strcpy(path,"fx/explosions/grenade2.efx");
    if(!ceRendererReady())return;
    id=ceRegister(path,0);if(!id)return;
    trap_Cvar_Update(&ceEnabled);
    if(!ceEnabled.integer) { CG_Printf("Set cg_cod1NativeEfx 1 before cod1_fx_test\n");return; }
    VectorMA(cg.refdef.vieworg,256,cg.refdef.viewaxis[0],end);
    CG_Trace(&tr,cg.refdef.vieworg,NULL,NULL,end,cg.snap->ps.clientNum,MASK_SOLID);
    if(tr.startsolid||tr.allsolid) { CG_Printf("CoD1 NATIVE_EFX_R1: test origin is inside solid geometry\n");return; }
    if(tr.fraction<1) { VectorCopy(tr.plane.normal,dir);VectorMA(tr.endpos,2,dir,origin); }
    else { VectorCopy(end,origin);VectorSet(dir,0,0,1); }
    ceCheckClock();ceBasis(dir,basis);ceSpawn(id,origin,basis,cg.time,0);++ceState.played;
    CG_Printf("CoD1 NATIVE_EFX_R1: test %s (visual only; no explosion sound or damage)\n",ceState.cache[id-1].path);
}
void CG_Cod1EfxInfo_f(void) {
    int i,live=0;for(i=0;i<CE_LIVE;++i)live+=ceState.live[i].used!=0;
    CG_Printf("CoD1 NATIVE_EFX_R1: enabled=%d definitions=%d live=%d played=%u spawned=%u collided=%u dropped=%u\n",
        ceEnabled.integer,ceState.cacheCount,live,ceState.played,ceState.spawned,ceState.collided,ceState.dropped);
    for(i=0;i<ceState.cacheCount;++i) CG_Printf("  %s: %s\n",ceState.cache[i].state==2?"ready":"FAILED",ceState.cache[i].path);
}
#include "cg_cod1_muzzle.h"
#include "cg_cod1_impacts.h"
#include "cg_cod1_rocket.h"
#include "cg_cod1_efx_script.inc"
#include "cg_cod1_efx_attached.inc"
#endif
