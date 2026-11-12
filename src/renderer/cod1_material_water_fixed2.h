/* Experimental source shape for the fixed two-bundle water path. The caller
 * must provide measured runtime capabilities and explicitly opt in. This
 * planner performs no image IO and does not claim retail pixel parity. */
#ifndef COD1_MATERIAL_WATER_FIXED2_H
#define COD1_MATERIAL_WATER_FIXED2_H

#include <float.h>

typedef struct {
    char image[128];
    float scroll[2],scale[2],turbSource[3];
    int hasTurbSource;
} cod1MwBundle_t;
typedef struct {
    cod1MwBundle_t base,overlay[2];
    float constLighting[3];
    unsigned sourceHash;
} cod1MwPlan_t;

/* All four extension operands must have a known value, even when an earlier
 * `units < 4` term would short-circuit the authored `requires` expression.
 * The renderer needs two backend bundles; `caps->units` is the separate,
 * actual GL capability used only to evaluate the source requirements. */
static int Cod1MW_ProfileKnown(const cod1MpCaps_t *caps) {
    static const char *names[]={"GL_NV_texture_shader","GL_NV_register_combiners",
                                "GL_ATI_fragment_shader","GL_ARB_texture_cube_map"};
    int i;
    if (!caps || caps->units<2 || !caps->extension) return 0;
    for (i=0;i<4;++i) {
        cod1MpTruth_t value=caps->extension(names[i],caps->user);
        if (value!=COD1_MP_TRUE && value!=COD1_MP_FALSE) return 0;
    }
    return 1;
}

static int Cod1MW_Float(const char *s,float *value) {
    double d;
    if (!Cod1MP_Number(s,&d) || d!=d || d>FLT_MAX || d< -FLT_MAX) return 0;
    *value=(float)d;
    return 1;
}
static int Cod1MW_Arg(cod1MpLexer_t *lx,const char *wanted) {
    cod1MpToken_t token=Cod1MP_Next(lx);
    return Cod1MP_Equal(token.text,wanted);
}
static int Cod1MW_Floats(cod1MpLexer_t *lx,float *v,int count) {
    int i;
    for (i=0;i<count;++i) {
        cod1MpToken_t token=Cod1MP_Next(lx);
        if (!Cod1MW_Float(token.text,&v[i])) return 0;
    }
    return Cod1ME_End(lx);
}
static int Cod1MW_Image(cod1MpLexer_t *lx,char *path,size_t size) {
    cod1MpToken_t token=Cod1MP_Next(lx);
    const char *s=token.text;
    if (!*s || *s=='/' || *s=='$' || *s=='"' || strchr(s,'\\') ||
        strstr(s,"..") || strlen(s)>=size || !Cod1ME_End(lx)) return 0;
    Cod1MP_Copy(path,size,s);
    return 1;
}
static int Cod1MW_Match(cod1MpLexer_t *lx,const char **expected,int count) {
    int i;
    for (i=0;i<count;++i) {
        cod1MpToken_t t=Cod1MP_Next(lx);
        if (!Cod1MP_Equal(t.text,expected[i])) return 0;
    }
    return Cod1ME_End(lx);
}
static int Cod1MW_Requires(cod1MpLexer_t *lx,int index) {
    static const char *first[]={"GL_MAX_TEXTURE_UNITS_ARB","<","4","||","!",
                                "GL_NV_texture_shader","||","!","GL_NV_register_combiners"};
    static const char *second[]={"GL_MAX_TEXTURE_UNITS_ARB","<","4","||","!",
                                 "GL_ATI_fragment_shader"};
    const char **expected=index?second:first;
    return Cod1MW_Match(lx,expected,index?6:9);
}
static int Cod1MW_Excluded(const cod1MaterialProbe_t *ir) {
    static const char *nv0[]={"GL_MAX_TEXTURE_UNITS_ARB",">=","4"};
    static const char *nv1[]={"GL_NV_texture_shader"};
    static const char *cube[]={"GL_ARB_texture_cube_map"};
    static const char *nv3[]={"GL_NV_register_combiners"};
    static const char *ati0[]={"!","GL_NV_texture_shader","||","!","GL_NV_register_combiners"};
    static const char *ati2[]={"GL_ATI_fragment_shader"};
    const char **expected[2][4]={{nv0,nv1,cube,nv3},{ati0,nv0,ati2,cube}};
    const int counts[2][4]={{3,1,1,1},{5,3,1,1}};
    cod1MpLexer_t lx;
    cod1MpToken_t t;
    char line[512];
    int s,i;
    for (s=0;s<2;++s) {
        const cod1MpStage_t *stage=&ir->stage[s+2];
        if (stage->numRequires!=4 || stage->numOps<4 || stage->syntaxUnknown)
            return 0;
        for (i=0;i<4;++i) {
            const cod1MpOp_t *op=&stage->op[i];
            if (op->nested || op->bundle || !Cod1ME_Line(ir,op,&lx,line)) return 0;
            t=Cod1MP_Next(&lx);
            if (!Cod1MP_Equal(t.text,"requires") ||
                !Cod1MW_Match(&lx,expected[s][i],counts[s][i])) return 0;
        }
    }
    return 1;
}
static int Cod1MW_Top(const cod1MaterialProbe_t *ir) {
    int i,flags=0,edit=0,tess=0,global=0,sort=0;
    cod1MpLexer_t lx;
    cod1MpToken_t t,arg;
    char line[512];float value;
    /* noshadow is authored surface metadata, not a prerequisite for the
     * selected fixed-function color stages. */
    if (ir->topOps!=8 && ir->topOps!=9) return 0;
    for (i=0;i<ir->topOps;++i) {
        if (!Cod1ME_Line(ir,&ir->topOp[i],&lx,line)) return 0;
        t=Cod1MP_Next(&lx);arg=Cod1MP_Next(&lx);
        if (Cod1MP_Equal(t.text,"qer_editorimage")) {
            if (++edit!=1 || !*arg.text || !Cod1ME_End(&lx)) return 0;
        } else if (Cod1MP_Equal(t.text,"tessSize")) {
            if (++tess!=1 || !Cod1MW_Float(arg.text,&value) || value!=512.0f ||
                !Cod1ME_End(&lx)) return 0;
        } else if (Cod1MP_Equal(t.text,"q3map_globaltexture")) {
            if (++global!=1 || *arg.text) return 0;
        } else if (Cod1MP_Equal(t.text,"sort")) {
            if (++sort!=1 || !Cod1MP_Equal(arg.text,"water") || !Cod1ME_End(&lx)) return 0;
        } else if (Cod1MP_Equal(t.text,"surfaceParm")) {
            int bit=0;
            if (Cod1MP_Equal(arg.text,"trans")) bit=1;
            else if (Cod1MP_Equal(arg.text,"nonsolid")) bit=2;
            else if (Cod1MP_Equal(arg.text,"water")) bit=4;
            else if (Cod1MP_Equal(arg.text,"nolightmap")) bit=8;
            else if (Cod1MP_Equal(arg.text,"noshadow")) bit=16;
            if (!bit || (flags&bit) || !Cod1ME_End(&lx)) return 0;
            flags|=bit;
        } else return 0;
    }
    return (flags==15 || flags==31) && edit==1 && tess==1 && global==1 && sort==1;
}
/* Each selected stage is checked in authored order. 'requires' is resolved by
 * Cod1MP_Parse using the caller's profile; unfamiliar syntax remains unknown.
 * Three-value turb lines are retained as evidence and deliberately omitted
 * from executable mods (live CoDSP asks for a fourth value before commit). */
static int Cod1MW_Stage(const cod1MaterialProbe_t *ir,int index,cod1MwPlan_t *plan) {
    const cod1MpStage_t *s=&ir->stage[index];
    static const char *baseOps[]={"requires","requires","map","tcMod","tcMod","rgbGen"};
    static const char *overOps[]={"requires","requires","map","tcMod","tcMod","tcMod",
                                  "rgbGen","blendFunc","nextbundle","map","tcMod","tcMod","tcMod"};
    const char **ops=index?overOps:baseOps;
    int count=index?13:6,i,bundle=0,phase=0;
    cod1MpLexer_t lx;
    cod1MpToken_t op,arg;
    cod1MwBundle_t *b=index?&plan->overlay[0]:&plan->base;
    char line[512];
    if (s->numOps!=count || s->numRequires!=2 || s->syntaxUnknown ||
        s->numBundles!=(index?2:1) || s->mapCount[0]!=1 ||
        (index && s->mapCount[1]!=1)) return 0;
    for (i=0;i<count;++i) {
        const cod1MpOp_t *source=&s->op[i];
        if (source->nested || source->bundle!=bundle ||
            !Cod1ME_Line(ir,source,&lx,line)) return 0;
        op=Cod1MP_Next(&lx);
        if (!Cod1MP_Equal(op.text,ops[i])) return 0;
        if (i<2) {
            if (!Cod1MW_Requires(&lx,i)) return 0;
            continue;
        }
        if (Cod1MP_Equal(op.text,"nextbundle")) {
            if (!index || !Cod1ME_End(&lx)) return 0;
            bundle=1;phase=0;b=&plan->overlay[1];
            continue;
        }
        if (Cod1MP_Equal(op.text,"map")) {
            if (phase++ || !Cod1MW_Image(&lx,b->image,sizeof(b->image))) return 0;
            continue;
        }
        arg=Cod1MP_Next(&lx);
        if (Cod1MP_Equal(op.text,"tcMod")) {
            if (Cod1MP_Equal(arg.text,"scroll")) {
                if (phase++!=1 || !Cod1MW_Floats(&lx,b->scroll,2)) return 0;
            } else if (Cod1MP_Equal(arg.text,"scale")) {
                if (phase++!=2 || !Cod1MW_Floats(&lx,b->scale,2)) return 0;
            } else if (Cod1MP_Equal(arg.text,"turb")) {
                if (!index || phase++!=3 || !Cod1MW_Floats(&lx,b->turbSource,3)) return 0;
                b->hasTurbSource=1;
            } else return 0;
        } else if (Cod1MP_Equal(op.text,"rgbGen")) {
            if (index) {
                if (phase!=4 || !Cod1MP_Equal(arg.text,"exactVertex") || !Cod1ME_End(&lx)) return 0;
            } else {
                int j;
                if (phase!=3 || !Cod1MP_Equal(arg.text,"constLighting") ||
                    !Cod1MW_Arg(&lx,"(")) return 0;
                for (j=0;j<3;++j) {
                    cod1MpToken_t value=Cod1MP_Next(&lx);
                    if (!Cod1MW_Float(value.text,&plan->constLighting[j]) ||
                        plan->constLighting[j]<0.0f || plan->constLighting[j]>1.0f) return 0;
                }
                if (!Cod1MW_Arg(&lx,")") || !Cod1ME_End(&lx)) return 0;
            }
        } else if (Cod1MP_Equal(op.text,"blendFunc")) {
            if (!index || phase!=4 || !Cod1MP_Equal(arg.text,"add") ||
                !Cod1ME_End(&lx)) return 0;
        } else return 0;
    }
    return index ?
        (plan->overlay[0].hasTurbSource && plan->overlay[1].hasTurbSource &&
         plan->overlay[1].image[0] &&
         !strcmp(plan->overlay[0].image,plan->overlay[1].image)) :
        plan->base.image[0]!=0;
}
static int Cod1MW_Plan(const cod1MaterialProbe_t *ir,cod1MwPlan_t *plan) {
    if (!ir || !plan) return 0;
    memset(plan,0,sizeof(*plan));
    if (ir->numStages!=4 || ir->selectedMask!=3u || ir->unknownMask ||
        ir->stage[0].selected!=COD1_MP_TRUE || ir->stage[1].selected!=COD1_MP_TRUE ||
        ir->stage[2].selected!=COD1_MP_FALSE || ir->stage[3].selected!=COD1_MP_FALSE ||
        !Cod1MW_Excluded(ir) || !Cod1MW_Top(ir) || !Cod1MW_Stage(ir,0,plan) ||
        !Cod1MW_Stage(ir,1,plan)) return 0;
    plan->sourceHash=ir->sourceHash;
    return 1;
}
#endif
