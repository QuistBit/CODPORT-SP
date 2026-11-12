/* R60: source-shaped static projection with an explicit perLight omission.
 * Native perLight is stage flags+0 bit 0x80 (0x4edc8f), replayed by separate
 * dynamic and prepared-static light loops. This header DOES NOT implement
 * those loops. Success means partial_perlight, never complete material parity.
 * No material names, filesystem/image IO, renderer state or GL calls live here.
 */
#ifndef COD1_PERLIGHT_STATIC_H
#define COD1_PERLIGHT_STATIC_H
#include <stdio.h>
#include "cod1_material_stype.h"

typedef struct {
    unsigned hash, selected, excluded, emitted, omitted;
    unsigned nativeSurfaceFlags;
    /* Compact emitted-stage authorization survives stripped requires. */
    unsigned bundleAuth[8];
    unsigned multiplyIdentityMask;
    int ignoredSurfaceParms, ignoredRuntimeHints;
    int stages, omittedStages, line, partialPerlight;
    char reason[160];
} cod1PlStaticResult_t;

static int Cod1PL_Why(cod1PlStaticResult_t *r,int line,const char *why) {
    r->line=line;Cod1MP_Copy(r->reason,sizeof(r->reason),why);return 0;
}

/* Bounded native two/three-bundle perLight forms. The three-bundle form
 * retains the ordinary file mask before $dlight and must carry its own native
 * requires authorization. This validates the omitted pass; it does not draw
 * or approximate the native light replay. */
static int Cod1PL_OmittedStage(const cod1MaterialProbe_t *ir,
                              const cod1MpStage_t *s,char reason[80]) {
    unsigned seen=0;int j,last=s->numBundles-1;
    if((s->numBundles!=2 && s->numBundles!=3) || s->mapCount[0]!=1 ||
       s->mapCount[last]!=1 || s->tcModCount[0] || s->tcModCount[last]) {
        strcpy(reason,"perlight bundle shape unsupported");return 0;
    }
    if(s->numBundles==3 &&
       (!(Cod1ST_BundleAuth(ir,s)&COD1_ST_AUTH_THIRD) ||
        s->mapCount[1]!=1 || s->tcModCount[1]>4)) {
        strcpy(reason,"perlight third bundle authorization/shape unsupported");return 0;
    }
    for(j=0;j<s->numOps;++j) {
        const cod1MpOp_t *op=&s->op[j];cod1MpLexer_t lx;
        cod1MpToken_t cmd,arg;char line[512];unsigned bit=0;
        if(!Cod1ME_Line(ir,op,&lx,line)) {
            strcpy(reason,"perlight nested/long operation unsupported");return 0;
        }
        cmd=Cod1MP_Next(&lx);arg=Cod1MP_Next(&lx);
        if(Cod1MP_Equal(cmd.text,"requires"))continue;
        if(op->bundle==0 && Cod1MP_Equal(cmd.text,"perlight") && !arg.text[0])bit=1u;
        else if(Cod1MP_Equal(cmd.text,"map")) {
            int axes=C1MAP_REPEAT;unsigned loadFlags=0;
            while(C1Map_Option(arg.text,&axes,&loadFlags))arg=Cod1MP_Next(&lx);
            if(op->bundle==0 && arg.text[0] && C1Map_Source(arg.text)==C1MAP_FILE)bit=2u;
            else if(op->bundle==last && Cod1MP_Equal(arg.text,"$dlight"))bit=16u;
            else if(s->numBundles==3 && op->bundle==1 && arg.text[0] &&
                    C1Map_Source(arg.text)==C1MAP_FILE)bit=512u;
        } else if(op->bundle==0 && Cod1MP_Equal(cmd.text,"rgbGen") &&
                  Cod1MP_Equal(arg.text,"exactVertex"))bit=4u;
        else if(op->bundle==0 && Cod1MP_Equal(cmd.text,"blendFunc")) {
            if(Cod1MP_Equal(arg.text,"add"))bit=8u;
            else if(Cod1MP_Equal(arg.text,"GL_SRC_ALPHA")) {
                arg=Cod1MP_Next(&lx);if(Cod1MP_Equal(arg.text,"GL_ONE"))bit=8u;
            }
        } else if(op->bundle==last && Cod1MP_Equal(cmd.text,"blendFunc") &&
                  Cod1MP_Equal(arg.text,"filter"))bit=32u;
        else if(s->numBundles==3 && op->bundle==1 && Cod1MP_Equal(cmd.text,"blendFunc") &&
                Cod1MP_Equal(arg.text,"filter"))bit=1024u;
        else if(Cod1MP_Equal(cmd.text,"nextbundle") && !arg.text[0]) {
            if(op->bundle==0)bit=64u;
            else if(s->numBundles==3 && op->bundle==1)bit=256u;
        } else if(op->bundle==0 && Cod1MP_Equal(cmd.text,"alphaFunc") &&
                  Cod1MP_Equal(arg.text,"GE128"))bit=128u;
        else if(s->numBundles==3 && op->bundle==1 &&
                Cod1ME_OneOf(cmd.text,"tcGen tcMod")) {
            cod1MpOp_t primary=*op;
            if(!(seen&512u)) {
                strcpy(reason,"perlight mask coordinates before image");return 0;
            }
            primary.bundle=0;reason[0]=0;
            Cod1ME_StageOp(ir,s,&primary,reason);
            if(reason[0])return 0;
            continue;
        }
        if(!bit || (seen&bit) || !Cod1ME_End(&lx)) {
            strcpy(reason,"perlight operation/arguments unsupported");return 0;
        }
        seen|=bit;
    }
    if((seen&95u)!=95u || (s->numBundles==3 && (seen&768u)!=768u)) {
        strcpy(reason,"perlight required operation missing");return 0;
    }
    return 1;
}

static int Cod1PL_HasPerlight(const cod1MaterialProbe_t *ir,
                             const cod1MpStage_t *s) {
    int j;
    for(j=0;j<s->numOps;++j) {
        cod1MpLexer_t lx;cod1MpToken_t cmd;char line[512];
        if(!Cod1ME_Line(ir,&s->op[j],&lx,line))continue;
        cmd=Cod1MP_Next(&lx);
        if(Cod1MP_Equal(cmd.text,"perlight"))return 1;
    }
    return 0;
}

/* R65 bounded image producer: exactly one identityLighting RGB multiplier
 * before the sole file map. The image loader, not vertex rgbGen, performs
 * this operation on decoded pixels before the normal upload pipeline. */
static int Cod1PL_CheckStaticStage(const cod1MaterialProbe_t *ir,
                                  const cod1MpStage_t *s,int units,
                                  int *multiplyIdentity,char reason[80]) {
    cod1MpStage_t *checked;
    int j,n=0,loaded=0,multiply=0,ok;
    *multiplyIdentity=0;
    checked=(cod1MpStage_t*)malloc(sizeof(*checked));
    if(!checked) {strcpy(reason,"static stage allocation failed");return 0;}
    *checked=*s;
    for(j=0;j<s->numOps;++j) {
        cod1MpLexer_t lx;cod1MpToken_t cmd,arg;char line[512];
        if(!Cod1ME_Line(ir,&s->op[j],&lx,line)) {
            strcpy(reason,"invalid static stage operation");free(checked);return 0;
        }
        cmd=Cod1MP_Next(&lx);
        if(Cod1MP_Equal(cmd.text,"multiplyImage")) {
            arg=Cod1MP_Next(&lx);
            if(multiply || loaded || s->numBundles!=1 || s->op[j].bundle ||
               !Cod1MP_Equal(arg.text,"identityLighting") || !Cod1ME_End(&lx)) {
                strcpy(reason,"image multiplier shape/operand unsupported");free(checked);return 0;
            }
            multiply=1;continue;
        }
        if(Cod1ME_OneOf(cmd.text,"map clampmap animMap nextbundle"))loaded=1;
        checked->op[n++]=s->op[j];
    }
    checked->numOps=n;
    if(multiply && (s->mapCount[0]!=1 || !loaded)) {
        strcpy(reason,"image multiplier requires sole file map");free(checked);return 0;
    }
    ok=Cod1ST_CheckStageAuth(ir,checked,units,Cod1ST_BundleAuth(ir,s),1,reason);
    if(ok && multiply) {
        for(j=0;j<checked->numOps;++j) {
            cod1MpLexer_t lx;cod1MpToken_t cmd,arg;char line[512];
            Cod1ME_Line(ir,&checked->op[j],&lx,line);cmd=Cod1MP_Next(&lx);
            if(Cod1ME_OneOf(cmd.text,"map clampmap")) {
                int axes=C1MAP_REPEAT;unsigned flags=0;
                if(!Cod1MP_Equal(cmd.text,"map")) {
                    strcpy(reason,"image multiplier requires native map operation");ok=0;break;
                }
                arg=Cod1MP_Next(&lx);
                if(Cod1MP_Equal(cmd.text,"map"))
                    while(C1Map_Option(arg.text,&axes,&flags))arg=Cod1MP_Next(&lx);
                if(C1Map_Source(arg.text)!=C1MAP_FILE) {
                    strcpy(reason,"image multiplier requires file image");ok=0;break;
                }
            }
        }
    }
    if(ok)*multiplyIdentity=multiply;
    free(checked);return ok;
}

/* Native 0x4ef7d0..0x4efb00; transcribed R48 hash-locked sort audit. */
static int Cod1PL_NativeSort(const char *name,int *value) {
    static const struct {const char *name;int value;} rows[]={
        {"portal",1},{"sky",2},{"ocean",3},{"boathull",4},{"opaque",5},
        {"decal",6},{"seeThrough",7},{"banner",8},{"underwater",10},
        {"water",11},{"corona",12},{"innerBlend",13},{"outerBlend",14},
        {"blend",15},{"blend2",16},{"blend3",17},{"blend4",18},
        {"additive",19},{"nearest",22}
    };
    unsigned i;
    for(i=0;i<sizeof(rows)/sizeof(rows[0]);++i)
        if(Cod1MP_Equal(name,rows[i].name)) {*value=rows[i].value;return 1;}
    return 0;
}

/* Entire output is private until all selected static and omitted stages have
 * passed validation. Surface tags use the native renderer-only table rather
 * than rewriting BSP collision flags. The caller must apply returned native
 * flags and publish an explicit partial route after backend/image success.
 */
static int Cod1PL_Prepare(const char *body,const cod1MpCaps_t *caps,
                         char *out,size_t cap,cod1PlStaticResult_t *r) {
    cod1MaterialProbe_t *ir=NULL,*checked=NULL;
    size_t used=0;int i,j,selected=0,deforms=0,units,ok=0;
    char reason[80],line[512];cod1MpLexer_t lx;cod1MpToken_t cmd,arg;
    if(out && cap)out[0]=0;
    if(!r)return 0;
    memset(r,0,sizeof(*r));
    if(!body || !caps || !out || !cap || caps->units<1) {
        return Cod1PL_Why(r,0,"invalid perlight projection input/profile");
    }
    ir=(cod1MaterialProbe_t*)calloc(1,sizeof(*ir));
    checked=(cod1MaterialProbe_t*)calloc(1,sizeof(*checked));
    if(!ir || !checked) {Cod1PL_Why(r,0,"projection allocation failed");goto done;}
    if(!Cod1MP_Parse(body,caps,ir)) {Cod1PL_Why(r,0,ir->error);goto done;}
    r->hash=ir->sourceHash;r->selected=ir->selectedMask;
    units=caps->units>2?2:caps->units;
    for(i=0;i<ir->numStages;++i) {
        const cod1MpStage_t *s=&ir->stage[i];
        if(s->selected==COD1_MP_FALSE) {r->excluded|=1u<<i;continue;}
        if(s->selected!=COD1_MP_TRUE || s->syntaxUnknown) {
            Cod1PL_Why(r,s->line,"requires unknown/syntax unsupported");goto done;
        }
        if(++selected>8) {Cod1PL_Why(r,s->line,"too many selected native stages");goto done;}
        reason[0]=0;
        if(Cod1PL_HasPerlight(ir,s)) {
            if(!Cod1PL_OmittedStage(ir,s,reason)) {
                Cod1PL_Why(r,s->line,reason);goto done;
            }
            r->omitted|=1u<<i;++r->omittedStages;
        } else {
            int multiplyIdentity;
            if(!Cod1PL_CheckStaticStage(ir,s,units,&multiplyIdentity,reason)) {
                Cod1PL_Why(r,s->line,reason);goto done;
            }
            r->bundleAuth[r->stages]=Cod1ST_BundleAuth(ir,s);
            if(multiplyIdentity)r->multiplyIdentityMask|=1u<<r->stages;
            r->emitted|=1u<<i;++r->stages;
        }
    }
    if(!r->omittedStages) {Cod1PL_Why(r,0,"no selected perlight pass");goto done;}
    if(!r->stages) {Cod1PL_Why(r,0,"no supported static passes");goto done;}
    if(!Cod1ST_Append(out,cap,&used,"{\n",2))goto full;
    for(j=0;j<ir->topOps;++j) {
        const cod1MpOp_t *op=&ir->topOp[j];
        reason[0]=0;
        if(!Cod1ST_Top(ir,op,reason)) {Cod1PL_Why(r,0,reason);goto done;}
        Cod1ME_Line(ir,op,&lx,line);cmd=Cod1MP_Next(&lx);
        if(Cod1MP_Equal(cmd.text,"deformVertexes") && ++deforms>3) {
            Cod1PL_Why(r,0,"too many selected deforms");goto done;
        }
        if(Cod1MP_Equal(cmd.text,"surfaceparm")) {
            int known;arg=Cod1MP_Next(&lx);
            r->nativeSurfaceFlags|=Cod1ST_SurfaceFlag(arg.text,&known);
            if(!known)++r->ignoredSurfaceParms;
            continue;
        }
        if(Cod1MP_Equal(cmd.text,"radialNormals")) {++r->ignoredRuntimeHints;continue;}
        if(Cod1MP_Equal(cmd.text,"sort")) {
            int value;char number[48];
            arg=Cod1MP_Next(&lx);
            if(Cod1PL_NativeSort(arg.text,&value)) {
                /* Native label -> numeric source sort avoids ordinary RTCW's
                 * narrower named-sort table. Values come from R48 evidence. */
                sprintf(number,"sort %d\n",value);
                if(!Cod1ST_Append(out,cap,&used,number,strlen(number)))goto full;
                continue;
            }
        }
        if(!Cod1ST_Append(out,cap,&used,line,strlen(line)) ||
           !Cod1ST_Append(out,cap,&used,"\n",1))goto full;
    }
    for(i=0;i<ir->numStages;++i)if(r->emitted&(1u<<i)) {
        const cod1MpStage_t *s=&ir->stage[i];
        if(!Cod1ST_Append(out,cap,&used,"{\n",2))goto full;
        for(j=0;j<s->numOps;++j) {
            if(!Cod1ME_Line(ir,&s->op[j],&lx,line)) {
                Cod1PL_Why(r,s->line,"static operation copy failed");goto done;
            }
            cmd=Cod1MP_Next(&lx);
            if(Cod1ME_OneOf(cmd.text,"requires multiplyImage"))continue;
            if(!Cod1ST_Append(out,cap,&used,line,strlen(line)) ||
               !Cod1ST_Append(out,cap,&used,"\n",1))goto full;
        }
        if(!Cod1ST_Append(out,cap,&used,"}\n",2))goto full;
    }
    if(!Cod1ST_Append(out,cap,&used,"}\n",2))goto full;
    if(!Cod1MP_Parse(out,caps,checked)) {Cod1PL_Why(r,0,checked->error);goto done;}
    if(checked->numStages!=r->stages || checked->unknownMask) {
        Cod1PL_Why(r,0,"projected source verification failed");goto done;
    }
    r->partialPerlight=1;
    Cod1MP_Copy(r->reason,sizeof(r->reason),"partial_perlight: validated static passes; native perlight replay deferred");
    ok=1;goto done;
full: Cod1PL_Why(r,0,"projected source exceeds limit");
done:
    if(!ok) {out[0]=0;r->partialPerlight=0;}
    free(ir);free(checked);return ok;
}
#endif
