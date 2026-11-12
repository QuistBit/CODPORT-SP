/* R50 source compiler, included in tr_shader.c after error-stage helper.
 * Kept opt-in: existing water/base/packed binaries remain on their R49 path. */
#include "cod1_water_program.h"
#include "tr_cod1_water_shared.h"
typedef struct { int known,units,nvtex,nvcombine,ati,cube,combine,dot3; } c1wr_caps;
static c1wr_caps cod1WaterR50Caps;
static int R_C1W_Ext(const char *all,const char *name) {
    size_t n=strlen(name);const char *p=all;
    if(!all)return 0;
    while((p=strstr(p,name))!=NULL){if((p==all||p[-1]==' ')&&(!p[n]||p[n]==' '))return 1;p+=n;}return 0;
}
void R_Cod1WaterRuntimeCaptureProfile(const char *version,const char *extensions) {
    int major=0,minor=0,core13=0;size_t n=0;memset(&cod1WaterR50Caps,0,sizeof(cod1WaterR50Caps));
    if(!version||!extensions||sscanf(version,"%d.%d",&major,&minor)!=2||major<1||minor<0)return;
    while(n<65536&&extensions[n])n++;if(n==65536)return;
    core13=major>1||(major==1&&minor>=3);
    cod1WaterR50Caps.known=1;cod1WaterR50Caps.units=glConfig.maxActiveTextures;
    cod1WaterR50Caps.nvtex=R_C1W_Ext(extensions,"GL_NV_texture_shader");
    cod1WaterR50Caps.nvcombine=R_C1W_Ext(extensions,"GL_NV_register_combiners");
    cod1WaterR50Caps.ati=R_C1W_Ext(extensions,"GL_ATI_fragment_shader");
    cod1WaterR50Caps.cube=(core13||R_C1W_Ext(extensions,"GL_ARB_texture_cube_map"))&&ri.Cvar_Get("r_arb_texture_cube_map","1",CVAR_LATCH)->integer;
    cod1WaterR50Caps.combine=(core13||R_C1W_Ext(extensions,"GL_ARB_texture_env_combine"))&&ri.Cvar_Get("r_arb_texture_env_combine","1",CVAR_LATCH)->integer;
    cod1WaterR50Caps.dot3=(core13||R_C1W_Ext(extensions,"GL_ARB_texture_env_dot3"))&&ri.Cvar_Get("r_arb_texture_env_dot3","1",CVAR_LATCH)->integer;
    ri.Printf(PRINT_ALL,"CoD1 R50 water profile: GL=%d.%d units=%d NV=%d/%d ATI=%d effective cube/combine/dot3=%d/%d/%d; CPU is read separately, never invented\n",
      major,minor,cod1WaterR50Caps.units,cod1WaterR50Caps.nvtex,cod1WaterR50Caps.nvcombine,cod1WaterR50Caps.ati,cod1WaterR50Caps.cube,cod1WaterR50Caps.combine,cod1WaterR50Caps.dot3);
}
static cod1MpTruth_t R_C1W_Extension(const char *name,void *unused) {
    int n=-1;(void)unused;if(!cod1WaterR50Caps.known)return COD1_MP_UNKNOWN;
    if(!strcmp(name,"GL_NV_texture_shader"))n=cod1WaterR50Caps.nvtex;
    else if(!strcmp(name,"GL_NV_register_combiners"))n=cod1WaterR50Caps.nvcombine;
    else if(!strcmp(name,"GL_ATI_fragment_shader"))n=cod1WaterR50Caps.ati;
    else if(!strcmp(name,"GL_ARB_texture_cube_map"))n=cod1WaterR50Caps.cube;
    else if(!strcmp(name,"GL_ARB_texture_env_combine"))n=cod1WaterR50Caps.combine;
    else if(!strcmp(name,"GL_ARB_texture_env_dot3"))n=cod1WaterR50Caps.dot3;
    return n<0?COD1_MP_UNKNOWN:n?COD1_MP_TRUE:COD1_MP_FALSE;
}
static int R_C1W_Cvar(const char *name,double *v,void *unused) {
    const char *s;(void)unused;
    if(strcmp(name,"sys_cpuMHz"))return 0;
    s=ri.Cvar_Get("sys_cpuMHz","",0)->string;
    /* An empty default is UNKNOWN. A test may set sys_cpuMHz explicitly; the
     * log records that input, not a supposed measurement of the host CPU. */
    return s&&Cod1MP_Number(s,v)&&*v>=0&&*v<=1e8;
}
static image_t *R_C1W_Image(const char *name,int mip,int pic,int clamp) {
    static const char *ext[]={".dds",".tga",".jpg"};char path[MAX_QPATH],stem[MAX_QPATH];
    image_t *im;const char *dot,*slash;int i,wrap=C1Map_Wrap(clamp,GL_REPEAT,GL_CLAMP_TO_EDGE,COD1_IMAGE_CLAMP_X,COD1_IMAGE_CLAMP_Y);
    if(strlen(name)>=MAX_QPATH)return NULL;
    im=R_FindCod1TypeImage(name,mip,pic,qfalse,wrap);if(im)return im;
    Q_strncpyz(stem,name,sizeof(stem));dot=strrchr(stem,'.');slash=strrchr(stem,'/');
    if(dot&&(!slash||dot>slash)&&Cod1ME_OneOf(dot,".dds .tga .jpg"))stem[dot-stem]=0;
    for(i=0;i<3;i++){if(strlen(stem)+strlen(ext[i])>=sizeof(path))continue;
       Com_sprintf(path,sizeof(path),"%s%s",stem,ext[i]);if(!strcmp(path,name))continue;
       im=R_FindCod1TypeImage(path,mip,pic,qfalse,wrap);if(im)return im;}
    return NULL;
}
/* return0: not our source. return1: complete program OR explicit default
 * failure. A recognized unsupported program MUST NOT fall into legacy parser. */
static int R_Cod1CompileWaterRuntime(const char *body) {
    cod1MpCaps_t caps;cod1MaterialProbe_t *ir;cod1WaterMaterial_t candidate,*saved;
    c1wp_error error;int parsed,ok,i,j,hasLightmap=0;const char *why="unknown";
    memset(&caps,0,sizeof(caps));caps.units=cod1WaterR50Caps.known?cod1WaterR50Caps.units:-1;
    caps.extension=R_C1W_Extension;caps.cvar=R_C1W_Cvar;
    ir=(cod1MaterialProbe_t*)calloc(1,sizeof(*ir));if(!ir){ri.Error(ERR_DROP,"R50 water IR allocation");return 1;}
    parsed=Cod1MP_Parse(body,&caps,ir);memset(&candidate,0,sizeof(candidate));
    if(!C1WP_Candidate(ir)||!C1WP_NeedsRuntime(ir)){free(ir);return 0;}
    shader.cod1WaterProgramSource=qtrue;
    shader.cod1WaterProgramLightmapKey=shader.lightmapIndex;
    ok=parsed&&C1WP_Plan(ir,&candidate.plan,&error);
    if(!parsed){memset(&error,0,sizeof(error));Q_strncpyz(error.reason,ir->error,sizeof(error.reason));}
    why=error.reason;
    if(ok&&(!cod1WaterR50Caps.known||caps.units<2||!qglActiveTextureARB)) {ok=0;why="unknown/unavailable multitexture backend";}
    if(ok&&atof(glConfig.version_string)<1.3){ok=0;why="R50 backend currently requires a compatibility GL 1.3+ context";}
    if(ok&&candidate.plan.needsCube&&!cod1WaterR50Caps.cube){ok=0;why="selected operation needs effective cube map";}
    if(ok&&candidate.plan.needsCombine&&!cod1WaterR50Caps.combine){ok=0;why="selected operation needs effective combine";}
    if(ok&&candidate.plan.needsDot3&&!cod1WaterR50Caps.dot3){ok=0;why="selected operation needs effective dot3";}
    if(ok)for(i=0;i<candidate.plan.numStages;i++)for(j=0;j<candidate.plan.stage[i].numBundles;j++)
        if(candidate.plan.stage[i].bundle[j].imageKind==C1WP_LIGHTMAP)hasLightmap=1;
    if(ok&&shader.lightmapIndex>=0&&!hasLightmap){ok=0;why="water program requested with atlas without an authored selected lightmap";}
    /* The accepted R46 frame and recovered constLighting scalar cover 0/1.0.
     * RTCW identityLight at other overbright settings is not proof of CoD scale. */
    if(ok&&(tr.overbrightBits!=0||fabs(tr.identityLight-1.0f)>1e-6f))
        {ok=0;why="effective lighting/overbright profile not covered";}
    /* Validate source-independent blend representability before image IO. */
    for(i=0;ok&&i<candidate.plan.numStages;i++) {
        c1wp_stage *s=&candidate.plan.stage[i];
        if(s->srcBlend>=C1WP_SRC_COLOR||s->dstBlend==C1WP_DST_COLOR||s->dstBlend==C1WP_ONE_MINUS_DST_COLOR){ok=0;why="blend factor outside existing renderer state encoding";}
    }
    for(i=0;ok&&i<candidate.plan.numStages;i++)for(j=0;j<candidate.plan.stage[i].numBundles;j++) {
        const c1wp_bundle *b=&candidate.plan.stage[i].bundle[j];
        if(b->imageKind==C1WP_FILE)candidate.image[i][j]=R_C1W_Image(b->image,!candidate.plan.noMip,!candidate.plan.noPicMip,b->clamp);
        else if(b->imageKind==C1WP_LIGHTMAP) {
            /* Native 0x4e88b0..0x4e891a binds *identityLight for negative LM,
             * NOT an arbitrary atlas and not a deleted bundle. The current
             * runtime's existing zero-overbright gate is retained. */
            candidate.image[i][j]=shader.lightmapIndex<0?tr.identityLightImage:
                shader.lightmapIndex<tr.numLightmaps?tr.lightmaps[shader.lightmapIndex]:NULL;
            if(!candidate.image[i][j]){ok=0;why="selected lightmap input unavailable";break;}
        }
        else candidate.image[i][j]=tr.whiteImage; /* procedural handles owned by backend, created before ANY pass */
        if(!candidate.image[i][j]){ok=0;why="selected image unavailable";break;}
        if(b->imageKind==C1WP_FILE)R_Cod1MapPolicyTrace(b->image,b->mapLoadFlags,candidate.image[i][j]);
    }
    ri.Printf(PRINT_ALL,"CoD1 R50 water program: material=%s source=%08x selected=%08x unknown=%08x status=%s passes=%d wave=%d ignoredUncommittedTurb=%d CPU=\"%s\" reason=%s\n",
      shader.name,ir->sourceHash,ir->selectedMask,ir->unknownMask,ok?"ordinary-compiled":"unsupported",ok?candidate.plan.numStages:0,
      candidate.plan.hasWave,candidate.plan.ignoredTurb3,ri.Cvar_Get("sys_cpuMHz","",0)->string,ok?"none":why);
    free(ir);
    if(!ok){R_Cod1MaterialErrorStages();return 1;}
    for(i=0;i<candidate.plan.numStages;i++)for(j=0;j<candidate.plan.stage[i].numBundles;j++)
        if(candidate.plan.stage[i].bundle[j].imageKind==C1WP_LIGHTMAP)
            ri.Printf(PRINT_ALL,"CoD1 R51 water input: material=%s stage=%d bundle=%d source=$lightmap requestedLm=%d resolved=%s tcGen=%d\n",
                shader.name,i,j,shader.lightmapIndex,candidate.image[i][j]->imgName,candidate.plan.stage[i].bundle[j].tcgen);
    saved=(cod1WaterMaterial_t*)ri.Hunk_Alloc(sizeof(*saved),h_low);*saved=candidate;
    shader.cod1WaterRuntime=saved;shader.explicitlyDefined=qtrue;shader.defaultShader=qfalse;
    shader.sort=(float)candidate.plan.sort;shader.cullType=candidate.plan.cull==2?CT_TWO_SIDED:candidate.plan.cull==1?CT_BACK_SIDED:CT_FRONT_SIDED;
    shader.polygonOffset=candidate.plan.polygonOffset;shader.surfaceFlags=SURF_NONSOLID;
    if(candidate.plan.waterFlags&8)shader.surfaceFlags|=SURF_NOLIGHTMAP;
    if(candidate.plan.waterFlags&32)shader.surfaceFlags|=SURF_NODLIGHT;
    if(candidate.plan.waterFlags&64)shader.surfaceFlags|=SURF_NOMARKS;
    shader.contentFlags=CONTENTS_WATER|CONTENTS_TRANSLUCENT;shader.numDeforms=0;shader.multitextureEnv=0;
    shader.noMipMaps=candidate.plan.noMip;shader.noPicMip=candidate.plan.noPicMip;
    shader.cod1MaterialRoute=COD1_MATERIAL_SCRIPT_IR;
    memset(stages,0,sizeof(stages));
    for(i=0;i<candidate.plan.numStages;i++) {
        stages[i].active=qtrue;stages[i].rgbGen=CGEN_IDENTITY;stages[i].alphaGen=AGEN_IDENTITY;
        stages[i].bundle[0].image[0]=candidate.image[i][0];stages[i].bundle[0].tcGen=TCGEN_TEXTURE;
        for(j=0;j<candidate.plan.stage[i].numBundles;j++)
            stages[i].bundle[j].cod1MapLoadFlags=candidate.plan.stage[i].bundle[j].mapLoadFlags;
        /* Publish genuine lightmap inputs for finalizer/cache/diagnostics;
         * the water iterator continues to use the immutable full plan. */
        for(j=0;j<candidate.plan.stage[i].numBundles;j++)
            if(candidate.plan.stage[i].bundle[j].imageKind==C1WP_LIGHTMAP) {
                stages[i].bundle[j].image[0]=candidate.image[i][j];
                stages[i].bundle[j].isLightmap=qtrue;
                stages[i].bundle[j].tcGen=TCGEN_LIGHTMAP;
            }
        stages[i].bundle[0].texMods=texMods[i];stages[i].stateBits=R_Cod1WaterState(&candidate.plan.stage[i]);
#if defined(COD1_NATIVE_STAGE_FOG_COLOR_WIP) && COD1_NATIVE_STAGE_FOG_COLOR_WIP
        stages[i].cod1NativeFogColor=qtrue;
#endif
    }
    return 1;
}
