/* A bounded source-shape plan for CoD's opaque glass with static glow.
 * The 2-TMU authored branch has ordinary fixed-function blend operations;
 * the per-light bundle remains outside this plan until its CoD semantics
 * are implemented. No material names, image IO or GL state live here. */
#ifndef COD1_MATERIAL_WINDOW_GLOW_H
#define COD1_MATERIAL_WINDOW_GLOW_H

typedef struct {
    char base[128];
    char glow[128];
} cod1WgPlan_t;

static int Cod1WG_Path(const char *path) {
    return path[0] && path[0]!='$' && path[0]!='/' &&
           path[0]!='"' && !strstr(path,"..") && strlen(path)<MAX_QPATH;
}

static int Cod1WG_Op(const cod1MaterialProbe_t *ir,const cod1MpOp_t *source,
                     int stage,cod1WgPlan_t *plan) {
    cod1MpLexer_t lx;
    cod1MpToken_t op,arg;
    char line[512];
    int bit=0;
    if (!Cod1ME_Line(ir,source,&lx,line)) return 0;
    op=Cod1MP_Next(&lx);arg=Cod1MP_Next(&lx);
    if (Cod1MP_Equal(op.text,"requires")) {
        cod1MpToken_t cmp=Cod1MP_Next(&lx),value=Cod1MP_Next(&lx);
        if (!Cod1MP_Equal(arg.text,"GL_MAX_TEXTURE_UNITS_ARB") ||
            strcmp(value.text,"3")) return 0;
        bit=(stage==2 && !strcmp(cmp.text,"<"))?1:
            (stage==3 && !strcmp(cmp.text,">="))?1:0;
    } else if (Cod1MP_Equal(op.text,"map")) {
        if (stage==0 || (stage==4 && source->bundle==0)) {
            if (!Cod1WG_Path(arg.text)) return 0;
            if (stage==0) Cod1MP_Copy(plan->base,sizeof(plan->base),arg.text);
            else if (!Cod1MP_Equal(plan->base,arg.text)) return 0;
            bit=stage==4?2:1;
        } else if (stage==1 && Cod1MP_Equal(arg.text,"$lightmap")) bit=1;
        else if (stage==2 || stage==3) {
            if (!Cod1WG_Path(arg.text)) return 0;
            if (stage==2) Cod1MP_Copy(plan->glow,sizeof(plan->glow),arg.text);
            else if (!Cod1MP_Equal(plan->glow,arg.text)) return 0;
            bit=2;
        } else if (stage==4 && source->bundle==1 &&
                   Cod1MP_Equal(arg.text,"$dlight")) bit=16;
    } else if (Cod1MP_Equal(op.text,"blendFunc")) {
        if (stage==1 && Cod1MP_Equal(arg.text,"filter")) bit=2;
        else if ((stage==2 || stage==3) && Cod1MP_Equal(arg.text,"add")) bit=4;
        else if (stage==4 && source->bundle==0 &&
                 Cod1MP_Equal(arg.text,"add")) bit=8;
        else if (stage==4 && source->bundle==1 &&
                 Cod1MP_Equal(arg.text,"filter")) bit=32;
    } else if (Cod1MP_Equal(op.text,"rgbGen")) {
        if (stage==0 && Cod1MP_Equal(arg.text,"identity")) bit=2;
        else if (stage==1 && Cod1MP_Equal(arg.text,"identity")) bit=4;
        else if (stage==2 && Cod1MP_Equal(arg.text,"identityLighting")) bit=8;
        else if (stage==3 && Cod1MP_Equal(arg.text,"identity")) bit=8;
        else if (stage==4 && source->bundle==0 &&
                 Cod1MP_Equal(arg.text,"exactVertex")) bit=4;
    } else if (stage==3 && Cod1MP_Equal(op.text,"multiplyImage") &&
               Cod1MP_Equal(arg.text,"identityLighting")) bit=16;
    else if (stage==4 && source->bundle==0 &&
             Cod1MP_Equal(op.text,"perlight") && !arg.text[0]) bit=1;
    else if (stage==4 && source->bundle==0 &&
             Cod1MP_Equal(op.text,"nextbundle") && !arg.text[0]) bit=64;
    if (!bit || !Cod1ME_End(&lx)) return 0;
    return bit;
}

static int Cod1WG_Plan(const cod1MaterialProbe_t *ir,cod1WgPlan_t *plan) {
    static const int wanted[5]={3,7,15,31,127};
    int i,j,opaque=0;
    memset(plan,0,sizeof(*plan));
    if (!ir || ir->numStages!=5 || ir->unknownMask || ir->selectedMask!=0x17u ||
        ir->topBlockingOpcode[0]) return 0;
    for (i=0;i<ir->topOps;++i) {
        cod1MpLexer_t lx;
        cod1MpToken_t op,arg;
        char line[512];
        if (!Cod1ME_Line(ir,&ir->topOp[i],&lx,line)) return 0;
        op=Cod1MP_Next(&lx);arg=Cod1MP_Next(&lx);
        if (Cod1MP_Equal(op.text,"surfaceParm") &&
            Cod1MP_Equal(arg.text,"opaqueglass") && Cod1ME_End(&lx)) {
            if (++opaque!=1) return 0;
        } else if (Cod1MP_Equal(op.text,"q3map_lightimage") ||
                   Cod1MP_Equal(op.text,"qer_editorimage") ||
                   Cod1MP_Equal(op.text,"q3map_surfacelight")) {
            if (!arg.text[0] || !Cod1ME_End(&lx)) return 0;
        } else return 0;
    }
    if (opaque!=1) return 0;
    for (i=0;i<5;++i) {
        const cod1MpStage_t *s=&ir->stage[i];
        int mask=0;
        if (s->syntaxUnknown || s->numBundles!=(i==4?2:1) ||
            s->selected!=(i==3?COD1_MP_FALSE:COD1_MP_TRUE) ||
            s->numRequires!=((i==2 || i==3)?1:0) ||
            s->mapCount[0]!=1 || s->mapCount[1]!=(i==4?1:0) ||
            s->tcModCount[0] || s->tcModCount[1]) return 0;
        for (j=0;j<s->numOps;++j) {
            int bit=Cod1WG_Op(ir,&s->op[j],i,plan);
            if (!bit || (mask&bit)) return 0;
            mask|=bit;
        }
        if (mask!=wanted[i]) return 0;
    }
    return plan->base[0] && plan->glow[0];
}
#endif
