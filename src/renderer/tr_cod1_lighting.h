/* Static vertex lighting using the decoded native visibility cache.
 * R54 restores worldspawn input defaults/normalization. The existing isotropic
 * sky and local-light shading remain incomplete. R56 fills absent visibility
 * records through the native collision service during static model baking.
 * R70 supplies the separately authored world ambient to the baked diffuse
 * stream. Native 4cc480..4cc56a consumes it independently of precalc RGB and
 * visibility; the dedicated lightingPrecalc stream remains unchanged. */
#include "cod1_lightcache.h"
#include "cod1_worldspawn_light.h"
#include "cod1_light_visibility.h"
static c1LightCache c1Cache;
static int c1Lighting, c1SunIndex, c1LitVerts, c1UncachedVerts;
static vec3_t c1SunDir,c1SunColor,c1SkyColor,c1Ambient;
static int c1LightTrace, c1StaticLightDebug, c1PointLighting;
static const char *c1LightInitReason = "not-initialized";
static c1WorldLightParams c1WorldLight;
static c1VisibilityRuntime *c1Visibility;

static void R_Cod1LightingFinish(void) {
    if(c1Visibility && c1LightTrace)ri.Printf(PRINT_ALL,
        "CoD1 R56 visibility: generated=%u rejected=%u unavailable=%u memoHits=%u rays=%u contents=%u; immutable BSP cache retained; prepared model cache retained\n",
        c1Visibility->generated,c1Visibility->rejected,c1Visibility->unavailable,
        c1Visibility->memoHits,c1Visibility->rays,c1Visibility->contentsQueries);
    /* R72 owns this cache until a synchronized map reset. */
    c1Visibility=NULL;
}
static int R_Cod1VisibilityTrace(void *context,const float *start,const float *end,c1VisibilityHit *out) {
    trace_t hit;(void)context;
    if(!ri.CM_Cod1LightTrace || !ri.CM_Cod1LightTrace(&hit,start,end))return 0;
    out->fraction=hit.fraction;out->surfaceFlags=(unsigned)hit.surfaceFlags;
    out->contents=(unsigned)hit.contents;out->startsolid=hit.startsolid;return 1;
}
static int R_Cod1VisibilityContents(void *context,const float *point,unsigned int *out) {
    int flags;(void)context;
    if(!ri.CM_PointContentsV59 || !ri.CM_PointContentsV59(point,0,&flags))return 0;
    *out=(unsigned)flags;return 1;
}
#include "tr_cod1_model_lighting.h"

static int R_Cod1VisibilitySample(const float *point,float *sun,float *sky,float *coverage) {
    return C1V_Sample(&c1Cache,point,c1SunIndex,c1ModelContext.quality?c1ModelContext.quality:3,c1SunDir,c1Visibility,
        R_Cod1VisibilityTrace,R_Cod1VisibilityContents,NULL,sun,sky,coverage);
}

static void R_Cod1LightFieldReport(void) {
    const float *sky=(c1WorldLight.present&C1WL_SKYCOLOR)?c1WorldLight.skyColor:c1WorldLight.sunColor;
    if(!c1LightTrace)return;
    ri.Printf(PRINT_ALL,
        "CoD1 R54 light params: seen=%02x defaults=%02x rejected=%02x legacyAmbient=%d; ambient=%.9g sunlight=%.9g diffusefraction=%.9g; sun=(%.9g %.9g %.9g) sky=(%.9g %.9g %.9g) skyInherited=%d; minlight-not-ambient\n",
        c1WorldLight.present,C1WL_ALL&~c1WorldLight.present,c1WorldLight.invalid,c1WorldLight.legacyAmbient,
        c1WorldLight.ambient,c1WorldLight.sunlight,c1WorldLight.fraction,
        c1WorldLight.sunColor[0],c1WorldLight.sunColor[1],c1WorldLight.sunColor[2],
        sky[0],sky[1],sky[2],!(c1WorldLight.present&C1WL_SKYCOLOR));
}

static void R_Cod1LightingInit(const cod1Bsp_t *bsp) {
    int len,k,closed=0;const byte *p;char *text,*copy,key[MAX_TOKEN_CHARS],*t;
    unsigned int n;
    R_Cod1LightingFinish();
    R_Cod1ModelLightingReset();
    c1Lighting=0;c1SunIndex=-1;c1LitVerts=c1UncachedVerts=0;
    C1WL_Defaults(&c1WorldLight);
    VectorClear(c1SunDir);VectorClear(c1SunColor);VectorClear(c1SkyColor);VectorClear(c1Ambient);
    c1LightTrace=ri.Cvar_Get("r_cod1MaterialTrace","0",0)->integer;
    c1StaticLightDebug=ri.Cvar_Get("r_cod1StaticLightDebug","0",0)->integer==1;
    c1PointLighting=ri.Cvar_Get("r_cod1PointLighting","0",0)->integer==1;
    if(c1LightTrace)ri.Printf(PRINT_ALL,"CoD1 R54 static light input: native worldspawn defaults and color normalization; cache solver unchanged; debugWhite=%d\n",c1StaticLightDebug);
    c1LightInitReason="disabled";
    if(c1StaticLightDebug)ri.Printf(PRINT_WARNING,
        "CoD1 R46 static debug: baked vertex RGB forced to 255; DIAGNOSTIC ONLY, not an original lighting implementation\n");
    if(!ri.Cvar_Get("r_cod1StaticLighting","1",CVAR_ARCHIVE)->integer){
        if(c1LightTrace)ri.Printf(PRINT_ALL,"CoD1 static lighting: reason=disabled; retaining lightingPrecalc\n");
        return;
    }
    c1LightInitReason="visibility-cache";
    if(!C1_Init(&c1Cache,bsp->base,bsp->length))goto unavailable;
    c1LightInitReason="entities-lump";
    p=COD1_BSP_LumpData(bsp,COD1_BSP_LUMP_ENTITIES,&len);
    if(!p||len<2||p[len-1])goto unavailable;
    c1LightInitReason="entity-allocation";
    copy=(char*)malloc(len);if(!copy)goto unavailable;
    memcpy(copy,p,len);text=copy;
    c1LightInitReason="worldspawn-open";
    t=COM_ParseExt(&text,qtrue);
    if(strcmp(t,"{")){free(copy);goto unavailable;}
    while(text){
        t=COM_ParseExt(&text,qtrue);
        if(!strcmp(t,"}")){closed=1;break;}
        if(!t[0])break;
        Q_strncpyz(key,t,sizeof(key));t=COM_ParseExt(&text,qtrue);
        if(!text || !strcmp(t,"{") || !strcmp(t,"}"))break;
        if(c1LightTrace && (C1WL_Field(key) || !Q_stricmp(key,"minlight") ||
            !Q_stricmp(key,"minlightcolor") || !Q_stricmp(key,"sundirection")))
            ri.Printf(PRINT_ALL,"CoD1 light source: key=%s value=\"%s\"\n",key,t);
        if(C1WL_Set(&c1WorldLight,key,t)<0 && c1LightTrace)
            ri.Printf(PRINT_WARNING,"CoD1 R54 light input rejected: key=%s value=\"%s\" (malformed/nonfinite or unrepresentable)\n",key,t);
    }
    free(copy);R_Cod1LightFieldReport();
    c1LightInitReason="worldspawn-structure";if(!closed)goto unavailable;
    c1LightInitReason="worldspawn-invalid-value";if(c1WorldLight.invalid)goto unavailable;
    c1LightInitReason="worldspawn-coefficient-range";
    if(!C1WL_Coefficients(&c1WorldLight,c1SunColor,c1SkyColor,c1Ambient))goto unavailable;
    c1LightInitReason="light-lump";
    p=C1_Lump(bsp->base,bsp->length,30,&n);if(!p||n%72)goto unavailable;
    /* The primary native sun is the last type1 record; sunless maps can
     * still prepare local sources and separately authored world ambient. */
    c1SunIndex=-1;
    for(k=0;k<(int)(n/72);k++)if(C1_U32(p+k*72)==1)c1SunIndex=k;
    if(c1SunIndex>=0)for(k=0;k<3;k++)c1SunDir[k]=C1_F32(p+c1SunIndex*72+28+k*4);
    c1LightInitReason="prepared-model-context";
    if(!R_Cod1ModelLightingInit(bsp))goto unavailable;
    c1Lighting=1;c1LightInitReason="native-prepared-model";
    if(c1LightTrace)ri.Printf(PRINT_ALL,
        "CoD1 R72 model lighting: shared prepared records; sun=%d visibilitySteps=%d sunQuality=%d maxLights=%d minIntensity=%.6g cutoff=%.6g; local types=2/3/4/5/7; pointExperiment=%d (separate legacy path)\n",
        c1SunIndex,c1ModelContext.quality,c1ModelContext.settings.diffuseSunQuality,
        c1ModelContext.settings.maxLights,c1ModelContext.settings.minIntensity,
        c1ModelContext.settings.tailCutoff,c1PointLighting);
    return;
unavailable:
    R_Cod1ModelLightingReset();
    ri.Printf(PRINT_ALL,"CoD1 static lighting: unsupported/missing data; retaining lightingPrecalc; reason=%s\n",c1LightInitReason);
}
static void R_Cod1LightVertex(drawVert_t*v,const vec3_t base,const float*fallback){
 float sun,sky,coverage,d,local[3]={0,0,0};int k,visible=1;
 if(!c1Lighting)return;
 if(R_Cod1VisibilitySample(v->xyz,&sun,&sky,&coverage))c1LitVerts++;
 else if(fallback && fallback[2]>0){sun=fallback[0];sky=fallback[1];c1UncachedVerts++;}
 else {c1UncachedVerts++;sun=sky=0;visible=0;}
 /* World ambient is not a visibility weight or the instance's packed
  * lightingPrecalc. An unavailable vertex/center sample must not discard it.
  * Keep the previously supported directional/local inputs conservative. */
 if(visible && c1PointLighting)C1V_PointRGB(&c1Cache,v->xyz,v->normal,c1SunDir,c1SunIndex,c1Visibility,
     R_Cod1VisibilityTrace,R_Cod1VisibilityContents,NULL,local);
 d=DotProduct(v->normal,c1SunDir);if(d<0)d=0;if(d>1)d=1;
 for(k=0;k<3;k++){float color=base[k]+sun*d*c1SunColor[k]+sky*c1SkyColor[k]+local[k]+c1Ambient[k];if(color<0)color=0;if(color>1)color=1;v->color[k]=(byte)(255*color);}
}
