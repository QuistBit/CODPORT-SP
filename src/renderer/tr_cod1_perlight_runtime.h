/* Authored dynamic perLight replay, separate from RTCW generated overlay.
 * Local dynamic-light adapter and generated normal basis are the bounded
 * R62 subset, with R64 native CPU-soup normal availability; optimized native
 * cached TBN and prepared-static lists remain open. */
#include "cod1_perlight_projection.h"
#include "cod1_perlight_basis_r64.h"
#ifdef _MSC_VER
#include <float.h>
#else
#include <fenv.h>
#endif
static int RB_Cod1PerlightRoundMode(void) {
#ifdef _MSC_VER
    unsigned mode=_controlfp(0,0)&_MCW_RC;
    if(mode==_RC_NEAR)return 0;
    if(mode==_RC_DOWN)return 1;
    if(mode==_RC_UP)return 2;
    if(mode==_RC_CHOP)return 3;
#else
    int mode=fegetround();
    if(mode==FE_TONEAREST)return 0;
    if(mode==FE_DOWNWARD)return 1;
    if(mode==FE_UPWARD)return 2;
    if(mode==FE_TOWARDZERO)return 3;
#endif
    return -1;
}
static qboolean RB_Cod1PerlightDynamic(void) {
    static cod1PLProjectionVertex_t projected[SHADER_MAX_VERTEXES];
    static unsigned int original[SHADER_MAX_INDEXES],filtered[SHADER_MAX_INDEXES];
    static unsigned char clip[SHADER_MAX_VERTEXES];
    shaderStage_t *runtimeStages[MAX_SHADER_STAGES];
    shaderStage_t **savedStages;
    cod1PerlightRuntime_t *program=tess.shader->cod1PerlightRuntime;
    int l,i,s,roundMode,originalCount,useBasis,count,valid;
    if(!program)return qfalse;
    /* An authored program owns this path even if no eligible light remains.
     * Never replace zero contribution with RTCW's generated additive pass. */
    if(!tess.dlightBits || !(tess.shader->sort>=-FLT_MAX && tess.shader->sort<=5.0f) ||
       r_lightmap->integer || r_vertexLight->integer || tess.fogNum ||
       (tess.shader->surfaceFlags&(SURF_NODLIGHT|SURF_SKY)) ||
       backEnd.refdef.num_dlights<0 || backEnd.refdef.num_dlights>32 ||
       tess.numVertexes<1 || tess.numVertexes>SHADER_MAX_VERTEXES ||
       tess.numIndexes<0 || tess.numIndexes>SHADER_MAX_INDEXES ||
       tess.numIndexes%3 || program->numStages<1 || program->numStages>MAX_SHADER_STAGES)
        return qtrue;
    /* The bounded CPU adapter requires the geometry producer's copied
     * normal array whenever native quality requests it. Never downgrade a
     * missing array to plain XY; quality0 does not read normals. */
    if(program->quality && program->quality->integer && !tess.shader->needsNormal)
        return qtrue;
    roundMode=RB_Cod1PerlightRoundMode();if(roundMode<0)return qtrue;
    for(s=0;s<program->numStages;++s) {
        int units=program->dlightBundle[s]+1;
        if(units<2 || units>NUM_TEXTURE_BUNDLES || glConfig.maxActiveTextures<units ||
           !qglActiveTextureARB || !qglClientActiveTextureARB)return qtrue;
        runtimeStages[s]=&program->stage[s];
    }
    originalCount=tess.numIndexes;
    for(i=0;i<originalCount;++i)original[i]=tess.indexes[i];
    savedStages=tess.xstages;tess.xstages=runtimeStages;
    /* Native CPU soup submission (0x50343f) supplies normals when either
     * the material requests them or quality and the aggregate light mask
     * are nonzero. The projector reads that availability, not just the
     * original material flag. Valid BSP59 CPU programs retain needsNormal
     * and indexed/chunked TRIANGLES copy their authored normals unchanged. */
    useBasis=Cod1PLR64_ProjectionUsesBasis(program->quality?program->quality->integer:0,
        tess.shader->needsNormal && Cod1PLR64_TriangleNormalsRequired(program->needsBasis,
            program->quality?program->quality->integer:0,tess.dlightBits));
    for(l=0;l<backEnd.refdef.num_dlights;++l) {
        cod1PLProjectionLight_t light;
        dlight_t *source=&backEnd.refdef.dlights[l];
        if(!(tess.dlightBits&(1u<<l)))continue;
        light.radius=source->radius;
        for(i=0;i<3;++i) {light.origin[i]=source->transformed[i];light.color[i]=source->color[i];}
        valid=1;
        for(i=0;i<tess.numVertexes;++i) {
            if(!Cod1PL_ProjectVertex(&light,tess.xyz[i],tess.normal[i],useBasis,
                                     (cod1PLRoundMode_t)roundMode,&projected[i])) {valid=0;break;}
            clip[i]=projected[i].clip;
        }
        if(!valid)continue;
        count=Cod1PL_FilterTriangles(original,originalCount,clip,tess.numVertexes,
                                     filtered,SHADER_MAX_INDEXES);
        if(count<=0)continue;
        for(i=0;i<count;++i)tess.indexes[i]=filtered[i];
        tess.numIndexes=count;
        for(s=0;s<program->numStages;++s) {
            shaderStage_t *stage=runtimeStages[s];
            int slot=program->dlightBundle[s];
            /* Native exactVertex consumes projectionRGBA directly. Applying
             * baked RGB or RTCW software fog here would alter that contract. */
            for(i=0;i<tess.numVertexes;++i)
                memcpy(tess.svars.colors[i],projected[i].rgba,4);
            ComputeTexCoords(stage);
            for(i=0;i<tess.numVertexes;++i) {
                tess.svars.texcoords[slot][i][0]=projected[i].st[0];
                tess.svars.texcoords[slot][i][1]=projected[i].st[1];
            }
            qglEnableClientState(GL_COLOR_ARRAY);
            qglColorPointer(4,GL_UNSIGNED_BYTE,0,tess.svars.colors);
            DrawMultitextured(&tess,s);
            backEnd.pc.c_dlightIndexes+=count;
        }
        backEnd.pc.c_dlightVertexes+=tess.numVertexes;
    }
    for(i=0;i<originalCount;++i)tess.indexes[i]=original[i];
    tess.numIndexes=originalCount;tess.xstages=savedStages;
    qglClientActiveTextureARB(GL_TEXTURE1_ARB);
    qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
    qglClientActiveTextureARB(GL_TEXTURE0_ARB);
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
    Cod1StageFogRestore();
#endif
    return qtrue;
}
