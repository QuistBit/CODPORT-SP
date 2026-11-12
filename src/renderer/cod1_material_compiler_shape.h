/* Narrow, fail-closed source IR -> fixed-function plan. This header has no
 * renderer, image, GL or material-name dependency. It intentionally accepts
 * only the authored one-stage/two-bundle waterfall source shape. */
#ifndef COD1_MATERIAL_COMPILER_SHAPE_H
#define COD1_MATERIAL_COMPILER_SHAPE_H

#include <float.h>

typedef struct {
    char image[128];
    float scroll[2];
} cod1McPlan_t;

static int Cod1MC_Float(const char *text,float *value) {
    double parsed;
    if (!Cod1MP_Number(text,&parsed) || parsed!=parsed ||
        parsed>FLT_MAX || parsed< -FLT_MAX) return 0;
    *value=(float)parsed;
    return 1;
}

static int Cod1MC_Plan(const cod1MaterialProbe_t *ir,
                       const cod1MeResult_t *eligible,cod1McPlan_t *plan) {
    const cod1MpStage_t *stage;
    int i,topMask=0,stageMask=0;
    cod1MpLexer_t lx;
    cod1MpToken_t op,arg;
    char line[512];
    memset(plan,0,sizeof(*plan));
    if (!ir || !eligible || ir->numStages!=1 || ir->topOps!=5 ||
        ir->selectedMask!=1u || ir->unknownMask ||
        eligible->candidateMask!=1u || eligible->blockedMask ||
        eligible->unknownMask || eligible->globalBlock[0]) return 0;
    stage=&ir->stage[0];
    if (stage->selected!=COD1_MP_TRUE || stage->syntaxUnknown ||
        stage->numRequires || stage->numBundles!=2 || stage->numOps!=6 ||
        stage->mapCount[0]!=1 || stage->mapCount[1]!=1 ||
        stage->tcModCount[0]!=1 || stage->tcModCount[1]) return 0;
    for (i=0;i<ir->topOps;++i) {
        int bit=0;
        if (!Cod1ME_Line(ir,&ir->topOp[i],&lx,line)) return 0;
        op=Cod1MP_Next(&lx);arg=Cod1MP_Next(&lx);
        if (Cod1MP_Equal(op.text,"sort") && Cod1MP_Equal(arg.text,"seeThrough")) bit=16;
        else if (Cod1MP_Equal(op.text,"surfaceParm")) {
            if (Cod1MP_Equal(arg.text,"nonsolid")) bit=1;
            else if (Cod1MP_Equal(arg.text,"noimpact")) bit=2;
            else if (Cod1MP_Equal(arg.text,"nodlight")) bit=4;
            else if (Cod1MP_Equal(arg.text,"water")) bit=8;
        }
        if (!bit || topMask&bit || !Cod1ME_End(&lx)) return 0;
        topMask|=bit;
    }
    if (topMask!=31) return 0;
    for (i=0;i<stage->numOps;++i) {
        const cod1MpOp_t *sourceOp=&stage->op[i];
        int bit=0;
        if (!Cod1ME_Line(ir,sourceOp,&lx,line)) return 0;
        op=Cod1MP_Next(&lx);arg=Cod1MP_Next(&lx);
        if (!sourceOp->bundle && Cod1MP_Equal(op.text,"map") &&
            arg.text[0] && arg.text[0]!='$' && arg.text[0]!='"' &&
            arg.text[0]!='/' && !strstr(arg.text,"..") &&
            strlen(arg.text)<sizeof(plan->image)) {
            bit=1;Cod1MP_Copy(plan->image,sizeof(plan->image),arg.text);
        } else if (!sourceOp->bundle && Cod1MP_Equal(op.text,"tcMod") &&
                   Cod1MP_Equal(arg.text,"scroll")) {
            cod1MpToken_t s=Cod1MP_Next(&lx),t=Cod1MP_Next(&lx);
            if (Cod1MC_Float(s.text,&plan->scroll[0]) &&
                Cod1MC_Float(t.text,&plan->scroll[1])) bit=2;
        } else if (!sourceOp->bundle && Cod1MP_Equal(op.text,"rgbGen") &&
                   Cod1MP_Equal(arg.text,"identity")) bit=4;
        else if (!sourceOp->bundle && Cod1MP_Equal(op.text,"blendFunc") &&
                 Cod1MP_Equal(arg.text,"blend")) bit=8;
        else if (!sourceOp->bundle && Cod1MP_Equal(op.text,"nextbundle") &&
                 !arg.text[0]) bit=16;
        else if (sourceOp->bundle==1 && Cod1MP_Equal(op.text,"map") &&
                 Cod1MP_Equal(arg.text,"$lightmap")) bit=32;
        if (!bit || stageMask&bit || !Cod1ME_End(&lx)) return 0;
        stageMask|=bit;
    }
    return stageMask==63;
}
#endif /* COD1_MATERIAL_COMPILER_SHAPE_H */
