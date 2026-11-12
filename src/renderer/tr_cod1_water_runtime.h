/* R50 ordinary fixed-function water executor, included ONCE by tr_shade.c.
 * All operations and coefficients come from the immutable source program.
 * New path is opt-in. NV/ATI selected programs are rejected by the compiler.
 * No per-map, texture-family, foliage or brightness workarounds. */
#include "tr_cod1_water_shared.h"
#include "cod1_water_runtime_cpu.h"
/* A single-TU inclusion avoids changing legacy MSVC/Makefile source lists.
 * R49 remains the canonical CPU implementation; this is not a rewritten copy. */
#include "cod1_watermap_core.c"
#ifndef GL_TEXTURE_CUBE_MAP_ARB
#define GL_TEXTURE_CUBE_MAP_ARB 0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x8515
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812f
#endif
#ifndef GL_COMBINE_ARB
#define GL_COMBINE_ARB 0x8570
#define GL_COMBINE_RGB_ARB 0x8571
#define GL_COMBINE_ALPHA_ARB 0x8572
#define GL_RGB_SCALE_ARB 0x8573
#define GL_ADD_SIGNED_ARB 0x8574
#define GL_INTERPOLATE_ARB 0x8575
#define GL_CONSTANT_ARB 0x8576
#define GL_PRIMARY_COLOR_ARB 0x8577
#define GL_PREVIOUS_ARB 0x8578
#define GL_SOURCE0_RGB_ARB 0x8580
#define GL_SOURCE0_ALPHA_ARB 0x8588
#define GL_OPERAND0_RGB_ARB 0x8590
#define GL_OPERAND0_ALPHA_ARB 0x8598
#endif
#ifndef GL_DOT3_RGB_ARB
#define GL_DOT3_RGB_ARB 0x86ae
#define GL_DOT3_RGBA_ARB 0x86af
#endif
static c1wr_pool cod1WaterPool;
/* Metadata addresses stay stable while their reserved texture numbers exist. */
static image_t cod1WaterImages[C1WR_RESOURCES],cod1RenormalizeImage;
static int cod1WaterGpuReady[C1WR_RESOURCES],cod1RenormalizeReady;
static unsigned char cod1RenormalizePixels[6*64*64*4];
static float cod1WaterCoords[2][SHADER_MAX_VERTEXES][3];
static byte cod1WaterColors[SHADER_MAX_VERTEXES][4];
static int cod1WaterTraceDraws;
/* Force both selectors because higher-unit setup uses raw GL, not RTCW's
 * two-unit cache. Merely calling R_C1WR_Select(0) after raw unit7 is unsafe. */
static void R_C1WR_Select(int unit) {
    qglActiveTextureARB(GL_TEXTURE0_ARB+unit);qglClientActiveTextureARB(GL_TEXTURE0_ARB+unit);
    glState.currenttmu=unit;
}
static int R_C1WR_Rand(void *unused){(void)unused;return rand()&32767;}
static void R_C1WR_Delete(image_t *im) {
    if(im->texnum){if(qglDeleteTextures)qglDeleteTextures(1,&im->texnum);R_ReleaseCod1WaterTexnum(im);}
    memset(im,0,sizeof(*im));
}
void R_Cod1WaterRuntimeShutdown(void) {
    int i;
    for(i=0;i<C1WR_RESOURCES;i++)R_C1WR_Delete(&cod1WaterImages[i]);
    R_C1WR_Delete(&cod1RenormalizeImage);C1WR_Reset(&cod1WaterPool);
    memset(cod1WaterGpuReady,0,sizeof(cod1WaterGpuReady));cod1RenormalizeReady=0;cod1WaterTraceDraws=0;
    memset(glState.currenttextures,0,sizeof(glState.currenttextures));
}
static void R_C1WR_TextureParams(GLenum target,int clamp) {
    qglTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    qglTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    qglTexParameteri(target,GL_TEXTURE_WRAP_S,clamp?GL_CLAMP_TO_EDGE:GL_REPEAT);
    qglTexParameteri(target,GL_TEXTURE_WRAP_T,clamp?GL_CLAMP_TO_EDGE:GL_REPEAT);
}
/* R50A: retain the exact driver operation/error in the console before the
 * ordinary failure cleanup. No error is treated as a successful upload. */
static int R_C1WR_CheckUpload(const char *operation,GLenum target,GLuint name,int face) {
    GLenum error=qglGetError();
    if(error==GL_NO_ERROR)return 1;
    ri.Printf(PRINT_WARNING,"CoD1 R50A water GL error: operation=%s target=0x%x texture=%u face=%d error=0x%x\n",
        operation,(unsigned)target,(unsigned)name,face,(unsigned)error);
    return 0;
}
/* Caller has preserved GL state. Upload raw normal data directly: no gamma,
 * overbright, picmip, texture compression or color-image cache transforms. */
static int R_C1WR_EnsureMap(int id,int updated) {
    image_t *im=&cod1WaterImages[id];c1wr_resource *r=&cod1WaterPool.item[id];
    if(!cod1WaterGpuReady[id]) {
        if(!im->texnum)R_FindFreeTexnum(im);
        qglBindTexture(GL_TEXTURE_2D,im->texnum);
        if(!R_C1WR_CheckUpload("waterMap-bind",GL_TEXTURE_2D,im->texnum,-1)){R_C1WR_Delete(im);return 0;}
        R_C1WR_TextureParams(GL_TEXTURE_2D,0);
        if(!R_C1WR_CheckUpload("waterMap-parameters",GL_TEXTURE_2D,im->texnum,-1)){R_C1WR_Delete(im);return 0;}
        qglTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,r->desc.width,r->desc.height,0,GL_RGBA,GL_UNSIGNED_BYTE,r->rgba);
        if(!R_C1WR_CheckUpload("waterMap-image",GL_TEXTURE_2D,im->texnum,-1)){R_C1WR_Delete(im);return 0;}
        cod1WaterGpuReady[id]=1;
    } else if(updated) {
        qglBindTexture(GL_TEXTURE_2D,im->texnum);
        if(!R_C1WR_CheckUpload("waterMap-update-bind",GL_TEXTURE_2D,im->texnum,-1)){R_C1WR_Delete(im);cod1WaterGpuReady[id]=0;return 0;}
        qglTexSubImage2D(GL_TEXTURE_2D,0,0,0,r->desc.width,r->desc.height,GL_RGBA,GL_UNSIGNED_BYTE,r->rgba);
        if(!R_C1WR_CheckUpload("waterMap-subimage",GL_TEXTURE_2D,im->texnum,-1)){R_C1WR_Delete(im);cod1WaterGpuReady[id]=0;return 0;}
    }
    im->frameUsed=tr.frameCount;return 1;
}
static int R_C1WR_EnsureCube(void) {
    int f;if(cod1RenormalizeReady)return 1;
    if(!C1WR_Renormalize(cod1RenormalizePixels,sizeof(cod1RenormalizePixels))){
        ri.Printf(PRINT_WARNING,"CoD1 R50A renormalize CPU generation rejected; rounding=%d expected=%d\n",fegetround(),FE_TONEAREST);
        return 0;
    }
    if(!cod1RenormalizeImage.texnum)R_FindFreeTexnum(&cod1RenormalizeImage);
    qglBindTexture(GL_TEXTURE_CUBE_MAP_ARB,cod1RenormalizeImage.texnum);
    if(!R_C1WR_CheckUpload("cube-bind",GL_TEXTURE_CUBE_MAP_ARB,cod1RenormalizeImage.texnum,-1))goto failed;
    R_C1WR_TextureParams(GL_TEXTURE_CUBE_MAP_ARB,1);
    if(!R_C1WR_CheckUpload("cube-parameters",GL_TEXTURE_CUBE_MAP_ARB,cod1RenormalizeImage.texnum,-1))goto failed;
    for(f=0;f<6;f++){
        qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB+f,0,GL_RGBA,64,64,0,
            GL_RGBA,GL_UNSIGNED_BYTE,cod1RenormalizePixels+f*64*64*4);
        if(!R_C1WR_CheckUpload("cube-face",GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB+f,cod1RenormalizeImage.texnum,f))goto failed;
    }
    cod1RenormalizeReady=1;return 1;
failed:
    R_C1WR_Delete(&cod1RenormalizeImage);return 0;
}
static GLenum R_C1WR_Op(int op) {
    switch(op){case C1WP_REPLACE:return GL_REPLACE;case C1WP_ADD:return GL_ADD;
      case C1WP_INTERPOLATE:return GL_INTERPOLATE_ARB;case C1WP_DOT3_RGB:return GL_DOT3_RGB_ARB;
      case C1WP_DOT3_RGBA:return GL_DOT3_RGBA_ARB;default:return GL_MODULATE;}
}
static void R_C1WR_Expr(const c1wp_expr *x,int alpha) {
    int i;GLenum source,operand;
    qglTexEnvi(GL_TEXTURE_ENV,alpha?GL_COMBINE_ALPHA_ARB:GL_COMBINE_RGB_ARB,R_C1WR_Op(x->op));
    for(i=0;i<x->count;i++) {
        const c1wp_arg *a=&x->arg[i];source=a->source==C1WP_PREVIOUS?GL_PREVIOUS_ARB:a->source==C1WP_TEXTURE?GL_TEXTURE:GL_CONSTANT_ARB;
        operand=a->alpha?(a->invert?GL_ONE_MINUS_SRC_ALPHA:GL_SRC_ALPHA):(a->invert?GL_ONE_MINUS_SRC_COLOR:GL_SRC_COLOR);
        qglTexEnvi(GL_TEXTURE_ENV,(alpha?GL_SOURCE0_ALPHA_ARB:GL_SOURCE0_RGB_ARB)+i,source);
        qglTexEnvi(GL_TEXTURE_ENV,(alpha?GL_OPERAND0_ALPHA_ARB:GL_OPERAND0_RGB_ARB)+i,operand);
    }
}
static void R_C1WR_Combine(const c1wp_combine *c) {
    int k;GLfloat color[4];
    if(!c->enabled){qglTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);return;}
    qglTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
    qglTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,1);qglTexEnvi(GL_TEXTURE_ENV,GL_ALPHA_SCALE,1);
    R_C1WR_Expr(&c->rgb,0);
    if(c->rgb.op!=C1WP_DOT3_RGBA)R_C1WR_Expr(&c->alpha,1);
    /* Original 0x4e7b7d..0x4e7bae scales RGB ONLY. Alpha .2 remains .2. */
    for(k=0;k<4;k++)color[k]=c->constant[k]*((c->constantLighting&&k<3)?tr.identityLight:1.0f);
    qglTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,color);
}
static byte R_C1WR_Color(float v){if(!(v>0))return 0;if(v>=255)return 255;return (byte)v;}
static void R_C1WR_Colors(const c1wp_stage *s) {
    int i,k;float scale;for(i=0;i<tess.numVertexes;i++) {
        for(k=0;k<3;k++) {
            switch(s->rgb) {
             case C1WP_VERTEX:cod1WaterColors[i][k]=R_C1WR_Color(tess.vertexColors[i][k]*tr.identityLight);break;
             case C1WP_EXACT_VERTEX:cod1WaterColors[i][k]=tess.vertexColors[i][k];break;
             case C1WP_CONST:case C1WP_CONST_LIGHTING:
                scale=s->rgb==C1WP_CONST_LIGHTING?tr.identityLight:1;cod1WaterColors[i][k]=R_C1WR_Color(s->color[k]*scale*255);break;
             case C1WP_IDENTITY_LIGHTING:cod1WaterColors[i][k]=R_C1WR_Color(tr.identityLight*255);break;
             default:cod1WaterColors[i][k]=255;break;
            }
        }
        cod1WaterColors[i][3]=s->alpha==C1WP_ALPHA_VERTEX?tess.vertexColors[i][3]:s->alpha==C1WP_ALPHA_DOT?
           C1WR_AlphaDot(backEnd.or.viewOrigin,tess.xyz[i],tess.normal[i]):255;
    }
}
static int R_C1WR_Coords(const c1wp_bundle *b,int unit) {
    int i,k,m;float scroll[2];
    for(i=0;i<tess.numVertexes;i++) {
        float *out=cod1WaterCoords[unit][i];out[2]=0;
        if(b->tcgen==C1WP_SUN_HALF){C1WR_SunHalf(backEnd.or.viewOrigin,tess.xyz[i],tr.cod1WaterSunDirection,out);
          if(!isfinite(out[0])||!isfinite(out[1])||!isfinite(out[2]))return 0;continue;}
        if(b->tcgen==C1WP_VECTOR)for(k=0;k<2;k++)out[k]=DotProduct(tess.xyz[i],b->vectors[k]);
        else {int channel=b->tcgen==C1WP_LIGHTMAP_UV?1:0;
            out[0]=tess.texCoords[i][channel][0];out[1]=tess.texCoords[i][channel][1];}
        for(m=0;m<b->numMods;m++) {
            const c1wp_mod *mod=&b->mod[m];
            if(mod->kind==C1WP_SCROLL) {
                for(k=0;k<2;k++){scroll[k]=mod->value[k]*tess.shaderTime;
                    scroll[k]-=(float)floor(scroll[k]);out[k]+=scroll[k];}
            } else if(mod->kind==C1WP_SCALE){out[0]*=mod->value[0];out[1]*=mod->value[1];}
            else return 0; /* compiler never publishes unimplemented modifiers */
        }
        if(!isfinite(out[0])||!isfinite(out[1]))return 0;
    }return 1;
}
void RB_StageIteratorCod1Water(void) {
    const cod1WaterMaterial_t *mat=tess.shader->cod1WaterRuntime;const c1wp_plan *p;
    glstate_t oldState;int oldFog,oldMatrix,stage,b,i,ok=1;GLuint handles[C1WP_STAGES][2];
    const char *why="none";GLenum err;unsigned frame=(unsigned)tr.frameCount;
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
    vec4_t oldFogColor;qboolean oldFogValid,oldFogBlack;
#endif
    if(!mat)return;p=&mat->plan;
    if(!qglActiveTextureARB||!qglClientActiveTextureARB||!qglPushAttrib||!qglPushClientAttrib||!qglDrawElements||!qglGetError){ri.Error(ERR_DROP,"R50 water: incomplete GL dispatch");return;}
    if(tess.numVertexes<=0||tess.numVertexes>SHADER_MAX_VERTEXES||tess.numIndexes<0||tess.numIndexes>SHADER_MAX_INDEXES||!isfinite(tess.shaderTime)){ri.Error(ERR_DROP,"R50 water: invalid batch/time");return;}
    if(p->needsSun&&!tr.cod1WaterSunValid){ri.Error(ERR_DROP,"R50 water: no valid world sun direction for %s",tess.shader->name);return;}
    for(i=0;i<tess.numIndexes;i++)if(tess.indexes[i]>=(unsigned)tess.numVertexes){ri.Error(ERR_DROP,"R50 water: index outside batch");return;}
    /* R52 diagnostic boundary: no GL state-changing call has happened in this
     * invocation yet. This does NOT identify the producer of an entry error. */
    for(i=0;i<16&&(err=qglGetError())!=GL_NO_ERROR;i++)
        ri.Printf(PRINT_WARNING,"CoD1 R52 water entry GL error: 0x%x before state setup\n",err);
    if(i==16){ri.Error(ERR_DROP,"R52 water: persistent entry GL error; state untouched");return;}
    oldState=glState;oldFog=fogIsOn;
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
    memcpy(oldFogColor,cod1StageFogSceneColor,sizeof(oldFogColor));oldFogValid=cod1StageFogSceneValid;oldFogBlack=cod1StageFogBlack;
#endif
    /* Preserve actual driver state AND the renderer's cached state. This
     * conservative research boundary avoids leaking DOT3, cubemap enables,
     * 3-component arrays, alpha-test or blend state into the next draw. */
    qglPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_POLYGON_BIT|GL_FOG_BIT|GL_CURRENT_BIT|GL_PIXEL_MODE_BIT);
    qglPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT|GL_CLIENT_PIXEL_STORE_BIT);
    qglGetIntegerv(GL_MATRIX_MODE,&oldMatrix);
    for(b=0;b<2;b++){R_C1WR_Select(b);qglMatrixMode(GL_TEXTURE);qglPushMatrix();qglLoadIdentity();}
    qglMatrixMode(oldMatrix);R_C1WR_Select(0);
    /* R52: errors here arose in the state-setup interval above. Keep this
     * diagnostic separate from entry errors and from subsequent uploads.
     * This preserves R51's bounded drain behaviour; it is NOT a fix for 0x501. */
    for(i=0;i<16&&(err=qglGetError())!=GL_NO_ERROR;i++)
        ri.Printf(PRINT_WARNING,"CoD1 R52 water setup GL error: 0x%x in attrib/matrix setup\n",err);
    if(i==16){ok=0;why="persistent setup GL error";goto cleanup;}
    /* Procedural uploads must ignore arbitrary client unpack settings too. */
    qglPixelStorei(GL_UNPACK_ALIGNMENT,1);qglPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    qglPixelStorei(GL_UNPACK_SKIP_PIXELS,0);qglPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    qglPixelTransferf(GL_RED_SCALE,1);qglPixelTransferf(GL_GREEN_SCALE,1);
    qglPixelTransferf(GL_BLUE_SCALE,1);qglPixelTransferf(GL_ALPHA_SCALE,1);
    qglPixelTransferf(GL_RED_BIAS,0);qglPixelTransferf(GL_GREEN_BIAS,0);
    qglPixelTransferf(GL_BLUE_BIAS,0);qglPixelTransferf(GL_ALPHA_BIAS,0);
    qglPixelTransferi(GL_MAP_COLOR,GL_FALSE);
    for(stage=0;stage<p->numStages;stage++)for(b=0;b<p->stage[stage].numBundles;b++) {
        const c1wp_bundle *bundle=&p->stage[stage].bundle[b];
        if(bundle->imageKind==C1WP_WATERMAP) {
            int id=C1WR_Get(&cod1WaterPool,&bundle->water,R_C1WR_Rand,NULL),updated;
            if(id<0){ok=0;why="waterMap allocation/spectrum/cache capacity";goto cleanup;}
            updated=C1WR_Update(&cod1WaterPool,id,frame,tess.shaderTime);
            if(updated<0||!R_C1WR_EnsureMap(id,updated)){ok=0;why="waterMap update/upload";goto cleanup;}
            handles[stage][b]=cod1WaterImages[id].texnum;
        } else if(bundle->imageKind==C1WP_RENORMALIZE) {
            if(!R_C1WR_EnsureCube()){ok=0;why="renormalize cubemap upload";goto cleanup;}
            handles[stage][b]=cod1RenormalizeImage.texnum;
        } else {if(!mat->image[stage][b]){ok=0;why="unbound selected image";goto cleanup;}
            handles[stage][b]=mat->image[stage][b]->texnum;mat->image[stage][b]->frameUsed=tr.frameCount;}
    }
    if(!cod1WaterPool.tablesReady){C1W_Tables(&cod1WaterPool.tables);cod1WaterPool.tablesReady=1;}
    if(!C1WR_Wave(p,&cod1WaterPool.tables,tess.shaderTime,(float*)tess.xyz,(float*)tess.normal,tess.numVertexes,4)){ok=0;why="invalid wave phase/geometry";goto cleanup;}
    /* Validate ALL transformed coordinates before first pass. */
    for(stage=0;stage<p->numStages;stage++)for(b=0;b<p->stage[stage].numBundles;b++)
        if(!R_C1WR_Coords(&p->stage[stage].bundle[b],b)){ok=0;why="non-finite/unsupported coordinates";goto cleanup;}
    SetIteratorFog();GL_Cull(tess.shader->cullType);qglDisable(GL_LIGHTING);
    for(b=0;b<2;b++){R_C1WR_Select(b);qglDisable(GL_TEXTURE_GEN_S);qglDisable(GL_TEXTURE_GEN_T);qglDisable(GL_TEXTURE_GEN_R);qglDisable(GL_TEXTURE_GEN_Q);}
    if(p->polygonOffset){qglEnable(GL_POLYGON_OFFSET_FILL);qglPolygonOffset(r_offsetFactor->value,r_offsetUnits->value);}
    /* This executor uses two units; exclude stale enabled higher units while
     * drawing, without changing the separate source capability snapshot. */
    for(b=2;b<glConfig.maxActiveTextures;b++){qglActiveTextureARB(GL_TEXTURE0_ARB+b);qglDisable(GL_TEXTURE_2D);qglDisable(GL_TEXTURE_CUBE_MAP_ARB);}
    R_C1WR_Select(0);qglEnableClientState(GL_VERTEX_ARRAY);qglDisableClientState(GL_NORMAL_ARRAY);
    qglVertexPointer(3,GL_FLOAT,16,tess.xyz);qglEnableClientState(GL_COLOR_ARRAY);
    qglColorPointer(4,GL_UNSIGNED_BYTE,0,cod1WaterColors);
    for(stage=0;stage<p->numStages;stage++) {
        const c1wp_stage *s=&p->stage[stage];unsigned long bits=R_Cod1WaterState(s);
        R_C1WR_Colors(s);GL_State(bits);
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
        Cod1StageFogForDraw(tess.shader->stages[stage],bits);
#endif
        for(b=0;b<2;b++) {
            R_C1WR_Select(b);qglDisable(GL_TEXTURE_2D);qglDisable(GL_TEXTURE_CUBE_MAP_ARB);
            if(b>=s->numBundles){qglDisableClientState(GL_TEXTURE_COORD_ARRAY);continue;}
            {const c1wp_bundle *bundle=&s->bundle[b];GLenum target=bundle->imageKind==C1WP_RENORMALIZE?GL_TEXTURE_CUBE_MAP_ARB:GL_TEXTURE_2D;
             R_C1WR_Coords(bundle,b);qglEnable(target);qglBindTexture(target,handles[stage][b]);R_C1WR_Combine(&bundle->combine);
             qglEnableClientState(GL_TEXTURE_COORD_ARRAY);qglTexCoordPointer(bundle->tcgen==C1WP_SUN_HALF?3:2,GL_FLOAT,3*sizeof(float),cod1WaterCoords[b]);}
        }
        R_C1WR_Select(0);qglDrawElements(GL_TRIANGLES,tess.numIndexes,GL_UNSIGNED_INT,tess.indexes);
        if(qglGetError()!=GL_NO_ERROR){ok=0;why="GL draw/combiner state rejected";break;}
    }
    if(ok&&cod1WaterTraceDraws<16&&ri.Cvar_Get("r_cod1MaterialTrace","0",0)->integer){
      ri.Printf(PRINT_ALL,"CoD1 R50 water draw: material=%s source=%08x frame=%u passes=%d wave=%d normalMaps=%d time=%.6g\n",
       tess.shader->name,p->sourceHash,frame,p->numStages,p->hasWave,cod1WaterPool.count,tess.shaderTime);cod1WaterTraceDraws++;}
cleanup:
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
    Cod1StageFogRestore();
#endif
    for(b=1;b>=0;b--){R_C1WR_Select(b);qglMatrixMode(GL_TEXTURE);qglPopMatrix();}
    qglMatrixMode(oldMatrix);qglPopClientAttrib();qglPopAttrib();glState=oldState;fogIsOn=oldFog;
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
    memcpy(cod1StageFogSceneColor,oldFogColor,sizeof(oldFogColor));cod1StageFogSceneValid=oldFogValid;cod1StageFogBlack=oldFogBlack;
#endif
    if(!ok)ri.Error(ERR_DROP,"R50 water %s: %s (state restored; no silent two-pass substitution)",tess.shader->name,why);
}
