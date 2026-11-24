/* R73 shared model lighting and backend color caches. Only the frontend extends visibility; backend
 * consumers own copied prepared records and immutable model candidates.
 * Native finite contracts/proofs: research/model_lighting_r72. */
#ifndef TR_COD1_MODEL_LIGHTING_H
#define TR_COD1_MODEL_LIGHTING_H
#ifdef __FAST_MATH__
#error CoD1 prepared model lighting requires strict FP; disable fast-math
#endif
#include "cod1_model_light_candidates.h"
#include "cod1_model_light_prepare.h"
#include "cod1_model_light_shade_batch.h"
#if defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP>=2))
#include "cod1_light_quad347.h"
#define C1Q347_ENABLED 1
#endif
#include "cod1_scene_point_record.h"
#ifndef _MSC_VER
#include <fenv.h>
#endif

typedef struct {
    c1LightCache cache;
    byte *storage;
    c1mLight *lights;
    c1VisibilityRuntime *visibility;
    c1mWorldCoefficients world;
    c1mPrepareSettings settings;
    float sine[1024], identity;
    unsigned int lightCount, generation;
    int quality, overbright, valid;
    cvar_t *fullbright,*minIntensity,*tailCutoff;
} cod1ModelLightContext_t;
/* Keys name only inputs that affect prepared records or final RGB. Dlight
 * transformed coordinates, flags, pointer identity and refdef time do not. */
typedef struct {
    unsigned int minIntensity,tailCutoff,fullbright,identityByte;
    int roundMode;
    unsigned int dynamicCount;
} cod1ModelLightSettingsKey_t;
typedef struct {
    cod1ModelLightSettingsKey_t settings;
    unsigned int dynamic[MAX_DLIGHTS][7]; /* ordered origin, radius, raw RGB bits */
} cod1ModelLightKey_t;
typedef struct {
    cod1ModelLightKey_t key;
    c1mModelLightFrame frame;
    unsigned int revision;
    int initialized,valid;
    cod1ModelLightColors_t *colors;
} cod1ModelLightRuntime_t;
struct cod1ModelLightSource_s {
    unsigned int generation;
    float center[3];
    c1ModelLightCandidates candidates;
    cod1ModelLightSettingsKey_t bakedKey;
    int bakedKeyValid;
    cod1ModelLightRuntime_t *runtime; /* serial backend writes; no frontend queries */
};
struct cod1ModelLightColors_s {
    unsigned int revision;
    int numVerts,initialized,valid;
    byte *active,*work;
    cod1ModelLightRuntime_t *owner;
    cod1ModelLightColors_t *next;
};
static cod1ModelLightContext_t c1ModelContext;
/* Internal load diagnostics also support headless integration assertions. */
static unsigned int c1ModelQueryAttempts,c1ModelPrepared,c1ModelUnavailable;
static float c1ModelLastQuery[3];
static unsigned int c1ModelFrameCacheHits,c1ModelFrameCacheMisses;
static unsigned int c1ModelBakedColorHits,c1ModelColorCacheHits,c1ModelColorCacheRebuilds;
static unsigned int c1ModelVertexShadeCalls;

static unsigned int R_Cod1ModelFloatBits(float value) {
    unsigned int bits;memcpy(&bits,&value,sizeof(bits));return bits;
}
static int R_Cod1ModelKeyEqual(const cod1ModelLightKey_t *a,const cod1ModelLightKey_t *b) {
    return !memcmp(&a->settings,&b->settings,sizeof(a->settings)) &&
        !memcmp(a->dynamic,b->dynamic,(size_t)a->settings.dynamicCount*sizeof(a->dynamic[0]));
}

static int R_Cod1ModelRoundMode(void) {
#ifdef _MSC_VER
    unsigned int m=_controlfp(0,0)&_MCW_RC;
    if(m==_RC_NEAR)return 0;if(m==_RC_DOWN)return 1;
    if(m==_RC_UP)return 2;if(m==_RC_CHOP)return 3;
#else
    int m=fegetround();
    if(m==FE_TONEAREST)return 0;if(m==FE_DOWNWARD)return 1;
    if(m==FE_UPWARD)return 2;if(m==FE_TOWARDZERO)return 3;
#endif
    return -1;
}
static int R_Cod1ModelKey(const cod1ModelLightSource_t *source,const trRefdef_t *refdef,
                          cod1ModelLightKey_t *key) {
    int i,k,n;unsigned int count=0;
    memset(&key->settings,0,sizeof(key->settings));
    /* Preserve R72's fullbright bypass, including irrelevant malformed scene
     * inputs or unsupported rounding. Only identity white affects this route. */
    if(c1ModelContext.fullbright && c1ModelContext.fullbright->integer) {
        key->settings.fullbright=1;key->settings.identityByte=(byte)tr.identityLightByte;return 1;
    }
    key->settings.roundMode=R_Cod1ModelRoundMode();
    if(key->settings.roundMode<0)return 0;
    key->settings.minIntensity=R_Cod1ModelFloatBits(c1ModelContext.minIntensity->value);
    key->settings.tailCutoff=R_Cod1ModelFloatBits(c1ModelContext.tailCutoff->value);
    n=refdef?refdef->cod1PreparedDlightCount:0;
    if(n<0 || n>MAX_DLIGHTS || (n && !refdef->cod1PreparedDlights))return 0;
    for(i=0;i<n;i++) {
        const dlight_t *dl=&refdef->cod1PreparedDlights[i];c1mLight decoded;
        if(!C1M_DecodeScenePoint(dl->origin,dl->radius,dl->color,c1ModelContext.identity,&decoded) ||
           !C1M_DynamicEligible(&decoded,source->center))continue;
        for(k=0;k<3;k++)key->dynamic[count][k]=R_Cod1ModelFloatBits(dl->origin[k]);
        key->dynamic[count][3]=R_Cod1ModelFloatBits(dl->radius);
        for(k=0;k<3;k++)key->dynamic[count][4+k]=R_Cod1ModelFloatBits(dl->color[k]);
        count++;
    }
    key->settings.dynamicCount=count;return 1;
}
static cod1ModelLightColors_t *R_Cod1ModelColorCacheAlloc(int numVerts) {
    cod1ModelLightColors_t *cache;
    if(numVerts<1 || numVerts>(INT_MAX-(int)sizeof(*cache))/8)return NULL;
    cache=ri.Hunk_Alloc(sizeof(*cache)+(size_t)numVerts*8,h_low);
    memset(cache,0,sizeof(*cache));cache->numVerts=numVerts;
    cache->active=(byte*)(cache+1);cache->work=cache->active+(size_t)numVerts*4;return cache;
}
void R_Cod1ModelLightingReset(void) {
    unsigned int next=c1ModelContext.generation+1;
    free(c1ModelContext.storage);free(c1ModelContext.lights);
    free(c1ModelContext.visibility);
    memset(&c1ModelContext,0,sizeof(c1ModelContext));
    c1ModelContext.generation=next;c1Visibility=NULL;c1Lighting=0;
    c1ModelQueryAttempts=c1ModelPrepared=c1ModelUnavailable=0;
    memset(c1ModelLastQuery,0,sizeof(c1ModelLastQuery));
    c1ModelFrameCacheHits=c1ModelFrameCacheMisses=0;
    c1ModelBakedColorHits=c1ModelColorCacheHits=c1ModelColorCacheRebuilds=0;
    c1ModelVertexShadeCalls=0;
}
static int R_Cod1ModelLightingInit(const cod1Bsp_t *bsp) {
    const byte *lightData;unsigned int size,i;size_t bytes;
    byte *cursor;c1LightCache *cache=&c1ModelContext.cache;
    c1mPrepareSettings *settings=&c1ModelContext.settings;
    lightData=C1_Lump(bsp->base,bsp->length,30,&size);
    if(!lightData || size%72 || !C1M_WorldCoefficients(&c1WorldLight,tr.identityLight,&c1ModelContext.world))return 0;
    c1ModelContext.lightCount=size/72;
    bytes=(size_t)c1Cache.np*16+(size_t)c1Cache.nn*36+
        (size_t)c1Cache.nl*36+(size_t)c1Cache.nr*2+3145728+size;
    c1ModelContext.storage=(byte*)malloc(bytes);
    c1ModelContext.lights=(c1mLight*)calloc(c1ModelContext.lightCount?c1ModelContext.lightCount:1,sizeof(c1mLight));
    c1ModelContext.visibility=(c1VisibilityRuntime*)calloc(1,sizeof(c1VisibilityRuntime));
    if(!c1ModelContext.storage || !c1ModelContext.lights || !c1ModelContext.visibility)return 0;
    *cache=c1Cache;cursor=c1ModelContext.storage;
#define C1M_COPY_CACHE(field,length) do {memcpy(cursor,c1Cache.field,(length));cache->field=cursor;cursor+=(length);} while(0)
    C1M_COPY_CACHE(planes,(size_t)cache->np*16);
    C1M_COPY_CACHE(nodes,(size_t)cache->nn*36);
    C1M_COPY_CACHE(leaves,(size_t)cache->nl*36);
    C1M_COPY_CACHE(refs,(size_t)cache->nr*2);
    C1M_COPY_CACHE(slots,3145728);
#undef C1M_COPY_CACHE
    memcpy(cursor,lightData,size);
    for(i=0;i<c1ModelContext.lightCount;i++)if(!C1M_DecodeBspLight72(cursor+i*72,tr.identityLight,
        &c1ModelContext.world,&c1ModelContext.lights[i]))return 0;
    c1ModelContext.quality=ri.Cvar_Get("r_diffuseSunSteps","3",CVAR_ARCHIVE)->integer;
    if(c1ModelContext.quality<1)c1ModelContext.quality=1;
    if(c1ModelContext.quality>5)c1ModelContext.quality=5;
    C1M_DefaultPrepareSettings(settings);
    settings->maxLights=ri.Cvar_Get("r_maxEntLights","8",CVAR_ARCHIVE)->integer;
    c1ModelContext.minIntensity=ri.Cvar_Get("r_minEntLightIntensity","0.02",CVAR_ARCHIVE);
    settings->minIntensity=c1ModelContext.minIntensity->value;
    c1ModelContext.tailCutoff=ri.Cvar_Get("r_entLightCutoff","0.2",CVAR_ARCHIVE);
    settings->tailCutoff=c1ModelContext.tailCutoff->value;
    settings->diffuseSunQuality=ri.Cvar_Get("r_diffuseSunQuality","2",CVAR_ARCHIVE)->integer;
    settings->hardwareMaxLights=tr.cod1ModelLightHardwareLimit;
    c1ModelContext.fullbright=ri.Cvar_Get("r_entFullbright","0",0);
    c1ModelContext.identity=tr.identityLight;c1ModelContext.overbright=tr.overbrightBits;
    C1M_InitShadeSinTable(c1ModelContext.sine);
    c1ModelContext.visibility->quality=c1ModelContext.quality;
    c1ModelContext.visibility->lightRecords=cursor;
    c1ModelContext.visibility->numLights=c1ModelContext.lightCount;
    c1ModelContext.valid=1;c1Visibility=c1ModelContext.visibility;
    return 1;
}
static int R_Cod1ModelCandidates(const float center[3],c1ModelLightCandidates *out,int allowFullbrightBypass) {
    c1ModelQueryAttempts++;VectorCopy(center,c1ModelLastQuery);
    if(allowFullbrightBypass && c1ModelContext.valid && c1ModelContext.fullbright && c1ModelContext.fullbright->integer) {
        memset(out,0,sizeof(*out));c1ModelPrepared++;return 1;
    }
    if(!c1ModelContext.valid || !C1M_Candidates(&c1ModelContext.cache,center,
        c1ModelContext.lightCount,c1SunIndex,c1ModelContext.quality,c1SunDir,
        c1ModelContext.visibility,R_Cod1VisibilityTrace,R_Cod1VisibilityContents,NULL,out)) {
        c1ModelUnavailable++;return 0;
    }
    c1ModelPrepared++;return 1;
}
static int R_Cod1ModelPrepare(const float center[3],const c1ModelLightCandidates *map,
                             const trRefdef_t *refdef,c1mModelLightFrame *published) {
    const c1mLight *pointers[15+MAX_DLIGHTS];
    c1mLight dynamic[MAX_DLIGHTS];float weights[15+MAX_DLIGHTS];
    c1mModelLightFrame frame;c1mPrepareSettings settings;int i,count=0,dynamicCount=0,n;
    memset(published,0,sizeof(*published));
    if(!c1ModelContext.valid || !map || map->unavailable)return 0;
    if(c1ModelContext.fullbright && c1ModelContext.fullbright->integer) {
        memset(&frame,0,sizeof(frame));frame.world=c1ModelContext.world;VectorCopy(center,frame.center);
        frame.valid=1;*published=frame;return 1;
    }
    for(i=0;i<map->count;i++) {
        if(map->indices[i]>=c1ModelContext.lightCount)return 0;
        pointers[count]=&c1ModelContext.lights[map->indices[i]];
        weights[count++]=map->weights[i];
    }
    n=refdef?refdef->cod1PreparedDlightCount:0;
    if(n<0 || n>MAX_DLIGHTS || (n && !refdef->cod1PreparedDlights))return 0;
    for(i=0;i<n;i++) {
        const dlight_t *dl=&refdef->cod1PreparedDlights[i];
        c1mLight *light=&dynamic[dynamicCount];
        if(!C1M_DecodeScenePoint(dl->origin,dl->radius,dl->color,c1ModelContext.identity,light))continue;
        if(!C1M_DynamicEligible(light,center))continue;
        pointers[count]=light;weights[count++]=1;dynamicCount++;
    }
    memset(&frame,0,sizeof(frame));frame.world=c1ModelContext.world;VectorCopy(center,frame.center);
    settings=c1ModelContext.settings;
    settings.minIntensity=c1ModelContext.minIntensity->value;
    settings.tailCutoff=c1ModelContext.tailCutoff->value;
    if(!C1M_Prepare(pointers,weights,count,center,map->remainingSky,&frame.world,
        c1ModelContext.identity,c1ModelContext.overbright,&settings,&frame.prepared))return 0;
    frame.valid=1;*published=frame;return 1;
}
/* Prepare from the captured key, not live cvars reread after capture. This
 * keeps a frontend cvar change from tagging a different result with this key.
 * The uncached R72 function above remains the independent result oracle. */
static int R_Cod1ModelPrepareKey(const cod1ModelLightSource_t *source,
                                const cod1ModelLightKey_t *key,c1mModelLightFrame *published) {
    const c1ModelLightCandidates *map=&source->candidates;
    const c1mLight *pointers[15+MAX_DLIGHTS];c1mLight dynamic[MAX_DLIGHTS];
    float weights[15+MAX_DLIGHTS];c1mModelLightFrame frame;c1mPrepareSettings settings;
    int i,k,count=0;
    memset(published,0,sizeof(*published));
    if(!c1ModelContext.valid || map->unavailable)return 0;
    memset(&frame,0,sizeof(frame));frame.world=c1ModelContext.world;VectorCopy(source->center,frame.center);
    if(key->settings.fullbright){frame.valid=1;*published=frame;return 1;}
    for(i=0;i<map->count;i++) {
        if(map->indices[i]>=c1ModelContext.lightCount)return 0;
        pointers[count]=&c1ModelContext.lights[map->indices[i]];weights[count++]=map->weights[i];
    }
    for(i=0;i<(int)key->settings.dynamicCount;i++) {
        float origin[3],radius,rgb[3];
        for(k=0;k<3;k++)memcpy(&origin[k],&key->dynamic[i][k],sizeof(float));
        memcpy(&radius,&key->dynamic[i][3],sizeof(float));
        for(k=0;k<3;k++)memcpy(&rgb[k],&key->dynamic[i][4+k],sizeof(float));
        if(!C1M_DecodeScenePoint(origin,radius,rgb,c1ModelContext.identity,&dynamic[i]))return 0;
        pointers[count]=&dynamic[i];weights[count++]=1;
    }
    settings=c1ModelContext.settings;
    memcpy(&settings.minIntensity,&key->settings.minIntensity,sizeof(float));
    memcpy(&settings.tailCutoff,&key->settings.tailCutoff,sizeof(float));
    if(!C1M_Prepare(pointers,weights,count,source->center,map->remainingSky,&frame.world,
        c1ModelContext.identity,c1ModelContext.overbright,&settings,&frame.prepared))return 0;
    frame.valid=1;*published=frame;return 1;
}
static cod1ModelLightSource_t *R_Cod1ModelSource(const float center[3],c1mModelLightFrame *frame) {
    c1ModelLightCandidates candidates;cod1ModelLightSource_t *source;
    if(!R_Cod1ModelCandidates(center,&candidates,0) || !R_Cod1ModelPrepare(center,&candidates,NULL,frame))return NULL;
    source=ri.Hunk_Alloc(sizeof(*source),h_low);memset(source,0,sizeof(*source));
    source->generation=c1ModelContext.generation;VectorCopy(center,source->center);
    source->candidates=candidates;
    source->runtime=ri.Hunk_Alloc(sizeof(*source->runtime),h_low);
    memset(source->runtime,0,sizeof(*source->runtime));
    if(R_Cod1ModelKey(source,NULL,&source->runtime->key)) {
        source->bakedKey=source->runtime->key.settings;source->bakedKeyValid=1;
        source->runtime->frame=*frame;source->runtime->revision=1;
        source->runtime->initialized=source->runtime->valid=1;
    }
    return source;
}
qboolean R_Cod1PrepareStaticLighting(const cod1ModelLightSource_t *source,
                                    const trRefdef_t *refdef,c1mModelLightFrame *frame) {
    if(!frame)return qfalse;frame->valid=0;
    if(c1StaticLightDebug || !source || !c1ModelContext.valid || source->generation!=c1ModelContext.generation ||
       (refdef && (refdef->rdflags&RDF_NOWORLDMODEL)))return qfalse;
    return R_Cod1ModelPrepare(source->center,&source->candidates,refdef,frame)?qtrue:qfalse;
}
/* Colors are immutable during a surface copy. Rebuild into a separate buffer
 * and publish only after every source vertex succeeds; fallback stays atomic. */
qboolean R_Cod1StaticModelColors(const srfTriangles_t *surface,const trRefdef_t *refdef,
                                const byte **rgba) {
    const cod1ModelLightSource_t *source;cod1ModelLightRuntime_t *runtime;
    cod1ModelLightColors_t *colors;cod1ModelLightKey_t key;
    const c1mModelLightFrame *frame;byte *swap;int i;
    if(!rgba)return qfalse;*rgba=NULL;
    if(c1StaticLightDebug || !surface || !(source=surface->cod1ModelLightSource) ||
       !c1ModelContext.valid || source->generation!=c1ModelContext.generation || source->candidates.unavailable ||
       (refdef && (refdef->rdflags&RDF_NOWORLDMODEL)) || !source->runtime ||
       !surface->verts || surface->numVerts<1)return qfalse;
    if(!R_Cod1ModelKey(source,refdef,&key))return qfalse;
    /* Most draws use already baked lighting. Still report prepared so the
     * legacy RTCW projector cannot add point light a second time. */
    if(surface->cod1ModelLightBakedValid && source->bakedKeyValid &&
       !key.settings.dynamicCount && !memcmp(&key.settings,&source->bakedKey,sizeof(source->bakedKey))) {
        c1ModelBakedColorHits++;return qtrue;
    }
    runtime=source->runtime;colors=surface->cod1ModelLightColors;
    if(!colors || colors->numVerts!=surface->numVerts)return qfalse;
    if(!colors->owner) {
        colors->owner=runtime;colors->next=runtime->colors;runtime->colors=colors;
    } else if(colors->owner!=runtime)return qfalse;
    if(runtime->initialized && R_Cod1ModelKeyEqual(&runtime->key,&key))c1ModelFrameCacheHits++;
    else {
        c1ModelFrameCacheMisses++;
        runtime->valid=R_Cod1ModelPrepareKey(source,&key,&runtime->frame);
        runtime->key.settings=key.settings;
        memcpy(runtime->key.dynamic,key.dynamic,(size_t)key.settings.dynamicCount*sizeof(key.dynamic[0]));
        runtime->initialized=1;runtime->revision++;
        /* A wrapped revision must never match an older surface snapshot. */
        if(!runtime->revision) {
            cod1ModelLightColors_t *entry;
            for(entry=runtime->colors;entry;entry=entry->next)entry->initialized=0;
            runtime->revision=1;
        }
    }
    if(!runtime->valid)return qfalse;
    if(colors->initialized && colors->revision==runtime->revision) {
        c1ModelColorCacheHits++;
        if(!colors->valid)return qfalse;*rgba=colors->active;return qtrue;
    }
    frame=&runtime->frame;c1ModelColorCacheRebuilds++;
    for(i=0;i<surface->numVerts;i++) {
        const drawVert_t *v=&surface->verts[i];byte *color=colors->work+(size_t)i*4;
        if(key.settings.fullbright)color[0]=color[1]=color[2]=(byte)key.settings.identityByte;
        else {
            c1ModelVertexShadeCalls++;
            if(!C1M_Shade(&frame->world,frame->prepared.remainingSky,frame->prepared.lights,
                frame->prepared.weights,frame->prepared.count,v->xyz,v->normal,
                key.settings.roundMode,c1ModelContext.sine,color))break;
        }
        color[3]=v->color[3];
    }
    colors->initialized=1;colors->revision=runtime->revision;colors->valid=(i==surface->numVerts);
    if(!colors->valid)return qfalse;
    swap=colors->active;colors->active=colors->work;colors->work=swap;
    *rgba=colors->active;return qtrue;
}
qboolean R_Cod1ShadePreparedVertex(const c1mModelLightFrame *frame,const float *worldXYZ,
                                  const float *worldNormal,byte *rgba) {
    if(!frame || !frame->valid || !rgba || !c1ModelContext.valid)return qfalse;
    if(c1ModelContext.fullbright && c1ModelContext.fullbright->integer) {
        rgba[0]=rgba[1]=rgba[2]=(byte)tr.identityLightByte;rgba[3]=255;return qtrue;
    }
    return C1M_Shade(&frame->world,frame->prepared.remainingSky,frame->prepared.lights,
        frame->prepared.weights,frame->prepared.count,worldXYZ,worldNormal,
        R_Cod1ModelRoundMode(),c1ModelContext.sine,rgba)?qtrue:qfalse;
}
qboolean R_Cod1ShadeEntityVertex(const trRefEntity_t *ent,const float *xyz,const float *normal,byte *rgba) {
    float world[3],n[3],cross[3][3],det;int i,j;
    if(!ent || !ent->cod1ModelLighting.valid)return qfalse;
    CrossProduct(ent->e.axis[1],ent->e.axis[2],cross[0]);
    CrossProduct(ent->e.axis[2],ent->e.axis[0],cross[1]);
    CrossProduct(ent->e.axis[0],ent->e.axis[1],cross[2]);
    det=DotProduct(ent->e.axis[0],cross[0]);if(det==0 || !C1M_Finite(det))return qfalse;
    for(i=0;i<3;i++) {
        world[i]=ent->e.origin[i];n[i]=0;
        for(j=0;j<3;j++){world[i]+=xyz[j]*ent->e.axis[j][i];n[i]+=normal[j]*cross[j][i]/det;}
    }
    if(C1M_ShadeNormalize(n)<0)return qfalse;
    return R_Cod1ShadePreparedVertex(&ent->cod1ModelLighting,world,n,rgba);
}
/* The backend owns this immutable prepared frame for the whole surface.
 * No callbacks occur within the loop. Preserve per-vertex arithmetic order,
 * including division before accumulation and fullbright geometry validation. */
qboolean R_Cod1ShadeEntityVertices(const trRefEntity_t *ent,int count,
        const vec4_t *xyz,const vec4_t *normal,byte *rgba) {
    float world[3],n[3],cross[3][3],det;int i,j,v,roundMode,fullbright;
    const c1mModelLightFrame *frame;c1mShadeBatch batch;c1jFp fp;int useBatch=0,quadSeparate=0;
    if(count==0)return qtrue;
    if(count<0 || !ent || !xyz || !normal || !rgba ||
       !ent->cod1ModelLighting.valid || !c1ModelContext.valid)return qfalse;
    frame=&ent->cod1ModelLighting;
    CrossProduct(ent->e.axis[1],ent->e.axis[2],cross[0]);
    CrossProduct(ent->e.axis[2],ent->e.axis[0],cross[1]);
    CrossProduct(ent->e.axis[0],ent->e.axis[1],cross[2]);
    det=DotProduct(ent->e.axis[0],cross[0]);if(det==0 || !C1M_Finite(det))return qfalse;
    fullbright=c1ModelContext.fullbright && c1ModelContext.fullbright->integer;
    roundMode=fullbright?0:R_Cod1ModelRoundMode();
    if(!fullbright){
        C1J_FpGet(&fp);
        /* Hoisting is valid with masked exceptions. Otherwise preserve the
         * scalar trap order, including branches whose light inputs go unread. */
        if((fp.control&63)==63&&(fp.mxcsr&0x1f80)==0x1f80){
            if(!C1M_ShadeBatchPrepare(&frame->world,frame->prepared.remainingSky,frame->prepared.lights,frame->prepared.weights,frame->prepared.count,roundMode,c1ModelContext.sine,&batch))return qfalse;
            useBatch=1;
        }
    }
#ifdef C1Q347_ENABLED
    if(useBatch&&count<=SHADER_MAX_VERTEXES)
        quadSeparate=C1Q347_Separate(rgba,(size_t)count*4,xyz,(size_t)count*sizeof(*xyz))&&
                     C1Q347_Separate(rgba,(size_t)count*4,normal,(size_t)count*sizeof(*normal))&&
                     C1Q347_Separate(rgba,(size_t)count*4,ent,sizeof(*ent));
#endif
    for(v=0;v<count;v++) {
#ifdef C1Q347_ENABLED
        if(quadSeparate&&count-v>=4){
            c1jFp prior;C1J_FpGet(&prior);
            if(C1Q347_Entity(&batch,ent->e.origin,ent->e.axis,cross,det,xyz+v,normal+v,rgba+(size_t)v*4)){v+=3;continue;}
            C1J_FpRestore(&prior,0,0);
        }
#endif
        byte *color=rgba+(size_t)v*4;
        for(i=0;i<3;i++) {
            world[i]=ent->e.origin[i];n[i]=0;
            for(j=0;j<3;j++){world[i]+=xyz[v][j]*ent->e.axis[j][i];n[i]+=normal[v][j]*cross[j][i]/det;}
        }
        if(C1M_ShadeNormalize(n)<0)return qfalse;
        if(fullbright) {
            color[0]=color[1]=color[2]=(byte)tr.identityLightByte;color[3]=255;
        } else if(useBatch){
            if(!C1M_ShadeBatchVertex(&batch,world,n,color))return qfalse;
        } else if(!C1M_Shade(&frame->world,frame->prepared.remainingSky,frame->prepared.lights,
            frame->prepared.weights,frame->prepared.count,world,n,roundMode,
            c1ModelContext.sine,color))return qfalse;
    }
    return qtrue;
}
qboolean R_Cod1PrepareEntityLighting(const trRefdef_t *refdef,trRefEntity_t *ent) {
    c1ModelLightCandidates candidates;float center[3],direction[3]={0,0,0};int i,k;
    if(!refdef || !ent)return qfalse;ent->cod1ModelLighting.valid=0;
    if(!tr.world || !tr.world->isCod1 || !c1ModelContext.valid || (refdef->rdflags&RDF_NOWORLDMODEL))return qfalse;
    VectorCopy((ent->e.renderfx&RF_LIGHTING_ORIGIN)?ent->e.lightingOrigin:ent->e.origin,center);
    if(!R_Cod1ModelCandidates(center,&candidates,1) || !R_Cod1ModelPrepare(center,&candidates,refdef,&ent->cod1ModelLighting))return qfalse;
    /* Other RGB generators retain bounded, defined compatibility inputs.
     * Evaluate their ambient/direct pair through the same finite shader at
     * the query point, including point attenuation and cone acceptance. */
    for(i=0;i<ent->cod1ModelLighting.prepared.count;i++) {
        const c1mLight *l=&ent->cod1ModelLighting.prepared.lights[i];
        float d[3];double intensity;
        for(k=0;k<3;k++)d[k]=l->positionW?l->position[k]-center[k]:l->position[k];
        VectorNormalize(d);
        if(C1M_PrepareAttenuation(l,center,0,&intensity))
            VectorMA(direction,(float)(intensity*ent->cod1ModelLighting.prepared.weights[i]),d,direction);
    }
    VectorNormalize(direction);
    {
        byte ambient[4],lit[4];float zero[3]={0,0,0};
        if(!R_Cod1ShadePreparedVertex(&ent->cod1ModelLighting,center,zero,ambient) ||
           !R_Cod1ShadePreparedVertex(&ent->cod1ModelLighting,center,direction,lit)) {
            ent->cod1ModelLighting.valid=0;return qfalse;
        }
        for(k=0;k<3;k++) {
            ent->ambientLight[k]=ambient[k];
            ent->directedLight[k]=(float)lit[k]-ambient[k];
            ((byte*)&ent->ambientLightInt)[k]=ambient[k];
            ent->lightDir[k]=DotProduct(direction,ent->e.axis[k]);
        }
    }
    VectorNormalize(ent->lightDir);
    ((byte*)&ent->ambientLightInt)[3]=255;ent->lightingCalculated=qtrue;return qtrue;
}
#endif
