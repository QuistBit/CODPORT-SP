/* R64 native sunfile subset. Included by tr_sky.c only, after tr_local.h.
 * Native address proofs and explicit port limits: research/sun_runtime_r63
 * and research/sun_rgba_r64.
 * No map-name aliases, generated replacement images, or cached-shader edits.
 */
#ifndef TR_COD1_SUN_RUNTIME_H
#define TR_COD1_SUN_RUNTIME_H
#ifdef __FAST_MATH__
#error CoD1 sun runtime requires strict FP; disable fast-math
#endif
#include "cod1_sun_config.h"
#include "cod1_sun_effect.h"
#include "cod1_sun_visibility.h"
#ifdef _MSC_VER
#include <float.h>
#else
#include <fenv.h>
#endif

typedef struct cod1SunRuntime_s {
    cod1SunParams_t params;
    cod1SunEffectCache_t cache;
    cod1SunEffectState_t effect;
    shader_t sprite, flare;
    shaderStage_t spriteStages[MAX_SHADER_STAGES], flareStages[MAX_SHADER_STAGES];
    qboolean valid, spriteValid, flareValid, queueExists;
    cod1SunVisibilityPoint_t point;
    float half, intensity, flareFraction;
    int queueFrame, queueScene, flareTime;
} cod1SunRuntime_t;

static cvar_t *cod1SunCvars[C1SUN_COUNT], *cod1SunTest;
/* The normal sky iterator's input must survive a nested generic disk draw,
 * including optional debug geometry. Backend execution is serial here. */
static shaderCommands_t cod1SunSavedTess;

static int R_Cod1SunRoundMode(void) {
#ifdef _MSC_VER
    unsigned mode;
    mode=_controlfp(0,0)&_MCW_RC;
    if(mode==_RC_NEAR)return 0;
    if(mode==_RC_DOWN)return 1;
    if(mode==_RC_UP)return 2;
    if(mode==_RC_CHOP)return 3;
#else
    int mode;
    mode=fegetround();
    if(mode==FE_TONEAREST)return 0;
    if(mode==FE_DOWNWARD)return 1;
    if(mode==FE_UPWARD)return 2;
    if(mode==FE_TOWARDZERO)return 3;
#endif
    return -1;
}

void R_Cod1SunWorldReset(void) {
    Cod1Sun_ResetName(tr.cod1SunFile);
    tr.cod1SunWorldDirectionValid=qfalse;
    tr.cod1SunPrimaryValid=qfalse;
    if(tr.cod1SunRuntime)memset(tr.cod1SunRuntime,0,sizeof(*tr.cod1SunRuntime));
}

void R_Cod1SunInit(void) {
    int i;
    for(i=0;i<C1SUN_COUNT;i++)
        cod1SunCvars[i]=ri.Cvar_Get(cod1SunKeys[i],cod1SunDefaults[i],0);
    cod1SunTest=ri.Cvar_Get("r_suntest","0",CVAR_CHEAT);
    tr.cod1SunRuntime=ri.Hunk_Alloc(sizeof(cod1SunRuntime_t),h_low);
    R_Cod1SunWorldReset();
}

static void R_Cod1SunSet(void *context,int key,const char *value) {
    (void)context;
    ri.Cvar_Set(cod1SunKeys[key],value);
}
static void R_Cod1SunUnknown(void *context,const char *key,unsigned line) {
    (void)context;
    ri.Printf(PRINT_WARNING,"CoD1 sunfile: unknown '%s' at line %u\n",key,line);
}

/* Native uses R_FindShader(name,-3,0,4). Authored sun shaders in A1 have
 * no mipmaps/picmip. Preserve source stages; clear disk depth writes in a
 * private copy. Vertex-dependent disk colors are admitted here, then gated
 * against their proven BSP CPU sky producer at each draw. Normal-dependent
 * color generators remain unresolved.
 */
static qboolean R_Cod1SunClone(const char *name,qboolean disk,
    shader_t *out,shaderStage_t storage[MAX_SHADER_STAGES]) {
    shader_t *source;
    shaderStage_t *stage;
    int i,b;
    if(!name[0])return qfalse;
    source=R_FindShader(name,LIGHTMAP_BY_VERTEX,qfalse);
    if(!source || source->defaultShader || source->isSky || source->numDeforms ||
       source->numUnfoggedPasses<1 || source->numUnfoggedPasses>MAX_SHADER_STAGES ||
       source->cod1PerlightRuntime)return qfalse;
    for(i=0;i<source->numUnfoggedPasses;i++) {
        stage=source->stages[i];
        if(!stage || !stage->active)return qfalse;
        if(disk && stage->rgbGen!=CGEN_IDENTITY &&
           stage->rgbGen!=CGEN_IDENTITY_LIGHTING && stage->rgbGen!=CGEN_CONST &&
           stage->rgbGen!=CGEN_VERTEX && stage->rgbGen!=CGEN_EXACT_VERTEX)
            return qfalse;
        if(disk && stage->alphaGen!=AGEN_IDENTITY && stage->alphaGen!=AGEN_SKIP &&
           stage->alphaGen!=AGEN_CONST && stage->alphaGen!=AGEN_VERTEX)return qfalse;
        if(!disk && stage->rgbGen!=CGEN_VERTEX && stage->rgbGen!=CGEN_EXACT_VERTEX &&
           stage->rgbGen!=CGEN_IDENTITY && stage->rgbGen!=CGEN_IDENTITY_LIGHTING &&
           stage->rgbGen!=CGEN_CONST)return qfalse;
        if(!disk && stage->alphaGen!=AGEN_VERTEX && stage->alphaGen!=AGEN_IDENTITY &&
           stage->alphaGen!=AGEN_SKIP && stage->alphaGen!=AGEN_CONST)return qfalse;
        for(b=0;b<NUM_TEXTURE_BUNDLES;b++) {
            if(b && stage->bundle[b].image[0])return qfalse;
            if(stage->bundle[b].tcGen!=TCGEN_TEXTURE &&
               stage->bundle[b].tcGen!=TCGEN_BAD)return qfalse;
        }
    }
    *out=*source;
    out->cod1SunDirectional=disk;
    out->optimalStageIteratorFunc=RB_StageIteratorGeneric;
    out->remappedShader=NULL;
    for(i=0;i<MAX_SHADER_STAGES;i++) {
        out->stages[i]=NULL;
        if(i>=source->numUnfoggedPasses)continue;
        storage[i]=*source->stages[i];
        if(disk)storage[i].stateBits&=~GLS_DEPTHMASK_TRUE;
        out->stages[i]=&storage[i];
    }
    return qtrue;
}

static qboolean R_Cod1SunNeedsVertexColors(const shader_t *shader) {
    int i;
    for(i=0;i<shader->numUnfoggedPasses;i++) {
        const shaderStage_t *stage=shader->stages[i];
        if(stage->rgbGen==CGEN_VERTEX || stage->rgbGen==CGEN_EXACT_VERTEX ||
           stage->alphaGen==AGEN_VERTEX)return qtrue;
    }
    return qfalse;
}

/* Native zero-stage BSP skies cannot enter its optimized surface routes.
 * CPU soup 24 copies all authored vertex RGBA, and clipping / outer sky /
 * BeginSurface leave that input array intact before the sun disk consumes it.
 * Cloud/VBO producers and batches with fewer than four defined inputs are
 * intentionally outside this contract; never substitute white for them.
 */
static qboolean RB_Cod1SunSkyColors(byte colors[4][4]) {
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
    if(!tess.shader || !tess.shader->isSky ||
       !tess.shader->cod1Bsp59WorldMaterialSource ||
       tess.shader->numUnfoggedPasses!=0 || tess.numVertexes<4)return qfalse;
    memcpy(colors,tess.vertexColors,4*4);
    return qtrue;
#else
    (void)colors;
    return qfalse;
#endif
}

static qboolean R_Cod1SunBuild(void) {
    cod1SunRuntime_t *runtime;
    cod1SunParams_t params;
    cod1SunEffectCache_t cache;
    const char *strings[C1SUN_COUNT];
    float values[C1SUN_COUNT];
    int i,mode;
    runtime=tr.cod1SunRuntime;
    if(!runtime)return qfalse;
    for(i=0;i<C1SUN_COUNT;i++) {
        if(!cod1SunCvars[i])return qfalse;
        strings[i]=cod1SunCvars[i]->string;values[i]=cod1SunCvars[i]->value;
    }
    mode=R_Cod1SunRoundMode();
    if(mode<0 || !Cod1Sun_Snapshot(strings,values,&params) ||
       !Cod1SunEffect_CacheBuild(&params,(cod1SunRoundMode_t)mode,&cache)) {
        runtime->valid=qfalse;
        return qfalse;
    }
    runtime->params=params;runtime->cache=cache;
    runtime->spriteValid=R_Cod1SunClone(params.spriteShader,qtrue,
        &runtime->sprite,runtime->spriteStages);
    runtime->flareValid=R_Cod1SunClone(params.flareShader,qfalse,
        &runtime->flare,runtime->flareStages);
    runtime->valid=qtrue;
    return qtrue;
}

void R_Cod1SunLoadWorld(void) {
    char path[96];
    void *buffer;
    int length;
    cod1SunApplyResult_t applied;
    if(!tr.cod1SunRuntime || !tr.cod1SunFile[0])return;
    Com_sprintf(path,sizeof(path),"scripts/%s.sun",tr.cod1SunFile);
    buffer=NULL;length=ri.FS_ReadFile(path,&buffer);
    if(length<0 || !buffer) {
        ri.Printf(PRINT_WARNING,"CoD1 sunfile: missing exact resource %s\n",path);
        return;
    }
    if(Cod1Sun_Apply(buffer,(size_t)length,R_Cod1SunSet,R_Cod1SunUnknown,NULL,&applied))
        R_Cod1SunBuild();
    else ri.Printf(PRINT_WARNING,"CoD1 sunfile: incomplete %s mask=%x; cvar assignments retained\n",
        path,applied.seenMask);
    ri.FS_FreeFile(buffer);
    ri.Printf(PRINT_ALL,"CoD1 R64 sunfile: %s cache=%d disk=%d flare=%d\n",path,
        tr.cod1SunRuntime->valid,tr.cod1SunRuntime->spriteValid,tr.cod1SunRuntime->flareValid);
}

static qboolean RB_Cod1SunView(void) {
    return cod1Bsp59MapActive && tr.world && tr.world->isCod1 &&
        !(backEnd.refdef.rdflags&(RDF_NOWORLDMODEL|RDF_SKYBOXPORTAL)) &&
        !backEnd.viewParms.isPortal && !glConfig.stereoEnabled &&
        backEnd.viewParms.viewportX==0 && backEnd.viewParms.viewportY==0 &&
        backEnd.viewParms.viewportWidth>=65 && backEnd.viewParms.viewportHeight>=65 &&
        backEnd.viewParms.viewportWidth<=16384 && backEnd.viewParms.viewportHeight<=16384;
}

/* Native 4c9c00 stores m10=-1,m11=-1,m15=0,m14=-2*znear.
 * Retain infinite far for this W0 pass, also when the base view is finite;
 * x/y/w and the original matrix
 * remain unchanged. New zero-radius records fail its strict far clip gate. */
static void RB_Cod1SunProjection(float projection[16]) {
    memcpy(projection,backEnd.viewParms.projectionMatrix,16*sizeof(float));
    projection[10]=-1.0f;
}

static void RB_Cod1SunQuad(shader_t *shader,const cod1SunQuad_t *quad,
    const byte color[4],const byte vertexColors[4][4],qboolean disk) {
    static const unsigned int indexes[6]={0,1,2,0,2,3};
    static const float diskST[4][2]={{1,1},{0,1},{0,0},{1,0}};
    static const float flareST[4][2]={{0,1},{0,0},{1,0},{1,1}};
    int i;
    RB_BeginSurface(shader,0);
    tess.numVertexes=4;tess.numIndexes=6;tess.dlightBits=0;
    for(i=0;i<4;i++) {
        VectorCopy(quad->xyz[i],tess.xyz[i]);tess.xyz[i][3]=disk?0.0f:1.0f;
        memcpy(tess.texCoords[i][0],disk?diskST[i]:flareST[i],2*sizeof(float));
        memcpy(tess.vertexColors[i],vertexColors?vertexColors[i]:color,4);
    }
    memcpy(tess.indexes,indexes,sizeof(indexes));
    RB_EndSurface();
}

void RB_Cod1SunSky(void) {
    cod1SunRuntime_t *runtime;
    cod1SunQuad_t quad;
    cod1SunVisibilityPoint_t point;
    float dot,projection[16],half,intensity;
    byte color[4],skyColors[4][4];
    int radius,mode,oldState,oldCull,oldTmu;
    qboolean oldFog,matchingQueue,vertexColors;
    if(!RB_Cod1SunView() || !r_drawSun->integer || !tr.cod1SunWorldDirectionValid || !tr.cod1SunPrimaryValid)return;
    runtime=tr.cod1SunRuntime;
    if(!runtime)return;
    /* R_FindShader may synchronize the render thread on an uncached name.
     * Debug rebuilds are bounded to the serial renderer; cached normal sun
     * remains usable with SMP. Never make the render thread wait on itself. */
    if(cod1SunTest && cod1SunTest->integer && !r_smp->integer)R_Cod1SunBuild();
    if(!runtime->valid || !Cod1SunEffect_Dot(tr.cod1SunWorldDirection,
       backEnd.viewParms.or.axis[0],&dot) || dot<=0.0f)return;
    RB_Cod1SunProjection(projection);
    vertexColors=runtime->spriteValid && R_Cod1SunNeedsVertexColors(&runtime->sprite);
    if(runtime->spriteValid && (!vertexColors || RB_Cod1SunSkyColors(skyColors)) &&
       Cod1SunEffect_DiskQuad(tr.cod1SunWorldDirection,
       runtime->params.value[C1SUN_SPRITE_SIZE],&quad)) {
        memcpy(&cod1SunSavedTess,&tess,sizeof(tess));
        oldState=glState.glStateBits;oldCull=glState.faceCulling;
        oldTmu=glState.currenttmu;oldFog=fogIsOn;GL_SelectTexture(0);
        qglMatrixMode(GL_PROJECTION);qglPushMatrix();qglLoadMatrixf(projection);
        qglMatrixMode(GL_MODELVIEW);qglPushMatrix();
        qglLoadMatrixf(backEnd.viewParms.world.modelMatrix);
        memset(color,255,sizeof(color));
        RB_Cod1SunQuad(&runtime->sprite,&quad,color,vertexColors?skyColors:NULL,qtrue);
        qglPopMatrix();qglMatrixMode(GL_PROJECTION);qglPopMatrix();qglMatrixMode(GL_MODELVIEW);
        GL_State(oldState);if(oldCull>=0)GL_Cull(oldCull);GL_SelectTexture(oldTmu);
        if(oldFog)R_FogOn();else R_FogOff();
        memcpy(&tess,&cod1SunSavedTess,sizeof(tess));
    }
    mode=R_Cod1SunRoundMode();
    matchingQueue=runtime->queueExists && runtime->queueScene==backEnd.viewParms.frameSceneNum;
    if(!runtime->flareValid || mode<0 ||
       Cod1SunEffect_Flare(&runtime->cache,dot,&half,&intensity)!=1 ||
       !Cod1SunVisibility_Radius(backEnd.viewParms.viewportWidth,
         runtime->params.value[C1SUN_SPRITE_SIZE],mode,&radius) ||
       !Cod1SunVisibility_ProjectDirection(tr.cod1SunPrimaryDirection,
         backEnd.viewParms.world.modelMatrix,projection,backEnd.viewParms.viewportWidth,
         backEnd.viewParms.viewportHeight,0,0,radius,matchingQueue,&point))return;
    if(!matchingQueue) {
        runtime->flareFraction=0.0f;
        /* Avoid signed overflow in the adapter's native now-10 sentinel. */
        runtime->flareTime=backEnd.refdef.time>=INT_MIN+10?backEnd.refdef.time-10:backEnd.refdef.time;
    }
    runtime->point=point;runtime->half=half;runtime->intensity=intensity;
    runtime->queueFrame=backEnd.viewParms.frameCount;
    runtime->queueScene=backEnd.viewParms.frameSceneNum;runtime->queueExists=qtrue;
}

qboolean RB_Cod1SunEffects(void) {
    cod1SunRuntime_t *runtime;
    cod1SunVisibilityPlan_t plan;
    cod1SunQuad_t quad;
    float depths[65*65],raw,visible,dot,blind,glare,pixelHalf;
    byte color[4];
    double elapsed;
    int dt,i,count,oldState,oldCull,oldTmu;
    qboolean current,matchingQueue,oldFog;
    if(!cod1Bsp59MapActive)return qfalse;
    if(!RB_Cod1SunView())return qtrue;
    runtime=tr.cod1SunRuntime;
    if(!runtime || !runtime->valid || !tr.cod1SunWorldDirectionValid || !tr.cod1SunPrimaryValid ||
       !r_flares->integer || !Cod1SunEffect_Dot(tr.cod1SunWorldDirection,
       backEnd.viewParms.or.axis[0],&dot))return qtrue;
    raw=visible=0.0f;
    matchingQueue=runtime->queueExists && runtime->queueScene==backEnd.viewParms.frameSceneNum;
    current=matchingQueue && runtime->queueFrame==backEnd.viewParms.frameCount;
    if(current && Cod1SunVisibility_PlanDepth(&runtime->point,
       backEnd.viewParms.viewportWidth,backEnd.viewParms.viewportHeight,&plan)) {
        count=plan.readWidth*plan.readHeight;
        /* A failed GL read cannot reuse preceding frame's depth values. */
        for(i=0;i<count;i++)depths[i]=0.0f;
        if(count)qglReadPixels(plan.readX,plan.readY,plan.readWidth,plan.readHeight,
            GL_DEPTH_COMPONENT,GL_FLOAT,depths);
        if(Cod1SunVisibility_Ratio(&plan,depths,count,&visible))raw=visible;
    }
    if(matchingQueue) {
        elapsed=(double)backEnd.refdef.time-runtime->flareTime;
        if(elapsed<INT_MIN || elapsed>INT_MAX)return qtrue;
        dt=(int)elapsed;runtime->flareTime=backEnd.refdef.time;
        if(!Cod1SunEffect_Approach(runtime->flareFraction,visible,dt,
           runtime->cache.flareInMs,runtime->cache.flareOutMs,&runtime->flareFraction))return qtrue;
        if(runtime->flareFraction==0.0f)runtime->queueExists=qfalse;
    }
    if(!Cod1SunEffect_Postprocess(&runtime->cache,&runtime->effect,backEnd.refdef.time,
       dot,raw,&blind,&glare))return qtrue;
    oldState=glState.glStateBits;oldCull=glState.faceCulling;oldTmu=glState.currenttmu;
    oldFog=fogIsOn;R_FogOff();GL_SelectTexture(0);
    qglMatrixMode(GL_PROJECTION);qglPushMatrix();qglLoadIdentity();
    qglOrtho(0,backEnd.viewParms.viewportWidth,0,backEnd.viewParms.viewportHeight,-99999,99999);
    qglMatrixMode(GL_MODELVIEW);qglPushMatrix();qglLoadIdentity();
    if((blind!=0.0f || glare!=0.0f) && Cod1SunEffect_ComposeColor(blind,glare,color)) {
        R_FogOff();GL_SelectTexture(0);GL_Bind(tr.whiteImage);GL_Cull(CT_TWO_SIDED);
        GL_State(GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA|GLS_DEPTHTEST_DISABLE);
        qglColor4ubv(color);qglBegin(GL_QUADS);
        qglTexCoord2f(0,0);qglVertex2f(0,0);
        qglTexCoord2f(1,0);qglVertex2f(backEnd.viewParms.viewportWidth,0);
        qglTexCoord2f(1,1);qglVertex2f(backEnd.viewParms.viewportWidth,backEnd.viewParms.viewportHeight);
        qglTexCoord2f(0,1);qglVertex2f(0,backEnd.viewParms.viewportHeight);qglEnd();
    }
    if(matchingQueue && runtime->queueExists && runtime->flareValid &&
       Cod1SunEffect_PixelHalfSize(runtime->half,backEnd.viewParms.viewportWidth,&pixelHalf) &&
       Cod1SunEffect_ScreenQuad(runtime->point.windowX,runtime->point.windowY,pixelHalf,r_znear->value,&quad) &&
       Cod1SunEffect_FlareColor(runtime->intensity,runtime->flareFraction,tr.identityLight,color))
        RB_Cod1SunQuad(&runtime->flare,&quad,color,NULL,qfalse);
    qglPopMatrix();qglMatrixMode(GL_PROJECTION);qglPopMatrix();qglMatrixMode(GL_MODELVIEW);
    GL_State(oldState);if(oldCull>=0)GL_Cull(oldCull);GL_SelectTexture(oldTmu);
    if(oldFog)R_FogOn();else R_FogOff();
    return qtrue;
}
#endif
