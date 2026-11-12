/* Private R62 compiler. Ordinary passes retain their established storage;
 * authored dynamic perLight passes have independently owned permanent mods.
 * The native prepared-static producer remains absent: status stays partial. */
#include "cod1_perlight_runtime_plan.h"
static cod1PerlightRuntime_t cod1PerlightWorking;
static texModInfo_t cod1PerlightWorkingMods[MAX_SHADER_STAGES][NUM_TEXTURE_BUNDLES][TR_MAX_TEXMODS];
typedef struct {
    shader_t shader;
    shaderStage_t stages[MAX_SHADER_STAGES];
    texModInfo_t mods[MAX_SHADER_STAGES][NUM_TEXTURE_BUNDLES][TR_MAX_TEXMODS];
} cod1PerlightSnapshot_t;

static qboolean R_Cod1SelectedPerlight(const char *body,const cod1MpCaps_t *caps) {
    cod1MaterialProbe_t *ir=(cod1MaterialProbe_t*)calloc(1,sizeof(*ir));
    int i,found=0;
    if(!ir)return qfalse;
    Cod1MP_Parse(body,caps,ir);
    for(i=0;i<ir->numStages;++i)
        if(ir->stage[i].selected!=COD1_MP_FALSE && Cod1PL_HasPerlight(ir,&ir->stage[i]))found=1;
    free(ir);return found?qtrue:qfalse;
}

static qboolean R_Cod1PerlightPreflight(const char *body,const cod1MpCaps_t *caps) {
    cod1PlRuntimePlan_t plan;
    char *out=(char*)malloc(COD1_ST_BODY);
    int ok;
    if(!out)return qfalse;
    ok=Cod1PLRuntime_Prepare(body,caps,out,COD1_ST_BODY,&plan);
    if(!ok)ri.Printf(PRINT_WARNING,"CoD1 R62 perLight preflight: %s line=%d reason=%s\n",shader.name,plan.line,plan.reason);
    free(out);return ok?qtrue:qfalse;
}

static qboolean R_Cod1CompilePerlightRuntime(const char *body,const cod1MpCaps_t *caps) {
    cod1PlRuntimePlan_t plan;
    cod1StResult_t auth;
    cod1PerlightSnapshot_t *saved;
    char *projected,*cursor;
    int i,b,ok=0;
    projected=(char*)malloc(COD1_ST_BODY);
    saved=(cod1PerlightSnapshot_t*)malloc(sizeof(*saved));
    if(!projected || !saved) {free(projected);free(saved);return qfalse;}
    if(!Cod1PLRuntime_Prepare(body,caps,projected,COD1_ST_BODY,&plan)) {
        ri.Printf(PRINT_WARNING,"CoD1 R62 perLight plan: %s line=%d reason=%s\n",shader.name,plan.line,plan.reason);
        free(projected);free(saved);return qfalse;
    }
    if(!qglActiveTextureARB || !qglClientActiveTextureARB || !tr.cod1DlightImage) {
        free(projected);free(saved);return qfalse;
    }
    saved->shader=shader;memcpy(saved->stages,stages,sizeof(stages));
    for(i=0;i<MAX_SHADER_STAGES;++i) {
        memcpy(saved->mods[i][0],texMods[i],sizeof(texMods[i]));
        memcpy(saved->mods[i][1],cod1Bundle1TexMods[i],sizeof(texMods[i]));
        memcpy(saved->mods[i][2],cod1Bundle2TexMods[i],sizeof(texMods[i]));
    }
    memset(stages,0,sizeof(stages));memset(&auth,0,sizeof(auth));
    memset(&cod1PerlightWorking,0,sizeof(cod1PerlightWorking));
    memset(cod1PerlightWorkingMods,0,sizeof(cod1PerlightWorkingMods));
    shader.multitextureEnv=0;
    for(i=0;i<MAX_SHADER_STAGES;++i) {
        stages[i].bundle[0].texMods=texMods[i];
        stages[i].bundle[1].texMods=cod1Bundle1TexMods[i];
        stages[i].bundle[2].texMods=cod1Bundle2TexMods[i];
        auth.bundleAuth[i]=plan.bundleAuth[i];
    }
    R_Cod1SetBundleAuth(&auth,1);cod1PerlightCompiling=qtrue;cursor=projected;
#if defined(COD1_BSP59_EXACT_VERTEX_ALPHA) && COD1_BSP59_EXACT_VERTEX_ALPHA
    ok=ParseShaderForSource(&cursor,qtrue);
#else
    ok=ParseShader(&cursor);
#endif
    cod1PerlightCompiling=qfalse;R_Cod1ClearBundleAuth();
    for(i=0;ok && i<plan.stages;++i) {
        shaderStage_t *stage=&stages[i];
        int slot=plan.dlightBundle[i];
        if(!stage->active || stage->rgbGen!=CGEN_EXACT_VERTEX ||
           (stage->alphaGen!=AGEN_VERTEX && stage->alphaGen!=AGEN_IDENTITY && stage->alphaGen!=AGEN_SKIP) ||
           slot<1 || slot>=NUM_TEXTURE_BUNDLES || !tr.cod1DlightImage ||
           stage->bundle[slot].image[0]!=tr.cod1DlightImage) {ok=0;break;}
        stage->bundle[slot].tcGen=TCGEN_LIGHTMAP;
        if(stage->bundle[0].tcGen==TCGEN_BAD)stage->bundle[0].tcGen=TCGEN_TEXTURE;
        /* Native implicit alpha-test promotion applies to the author blend. */
        if(!(stage->stateBits&GLS_ATEST_BITS) &&
           (stage->stateBits&(GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS))==
           (GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE))stage->stateBits|=GLS_ATEST_GT_0;
        cod1PerlightWorking.stage[i]=*stage;
        cod1PerlightWorking.dlightBundle[i]=slot;
        for(b=0;b<NUM_TEXTURE_BUNDLES;++b) {
            textureBundle_t *bundle=&cod1PerlightWorking.stage[i].bundle[b];
            memcpy(cod1PerlightWorkingMods[i][b],stage->bundle[b].texMods,
                   sizeof(cod1PerlightWorkingMods[i][b]));
            bundle->texMods=cod1PerlightWorkingMods[i][b];
        }
    }
    shader=saved->shader;memcpy(stages,saved->stages,sizeof(stages));
    for(i=0;i<MAX_SHADER_STAGES;++i) {
        memcpy(texMods[i],saved->mods[i][0],sizeof(texMods[i]));
        memcpy(cod1Bundle1TexMods[i],saved->mods[i][1],sizeof(texMods[i]));
        memcpy(cod1Bundle2TexMods[i],saved->mods[i][2],sizeof(texMods[i]));
    }
    if(ok) {
        cod1PerlightWorking.sourceHash=plan.hash;
        cod1PerlightWorking.sourceStageMask=plan.omittedMask;
        cod1PerlightWorking.numStages=plan.stages;
        cod1PerlightWorking.needsBasis=plan.needsBasis;
        cod1PerlightWorking.quality=ri.Cvar_Get("r_dlightQuality","1",0);
        shader.cod1PerlightRuntime=&cod1PerlightWorking;
        ri.Printf(PRINT_ALL,"CoD1 R62 perLight: %s dynamicStages=%d basis=%d preparedStatic=deferred status=partial_perlight\n",
                  shader.name,plan.stages,plan.needsBasis);
    }
    free(projected);free(saved);return ok?qtrue:qfalse;
}
