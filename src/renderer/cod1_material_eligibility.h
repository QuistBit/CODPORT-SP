/* Diagnostic contract for the *implemented* RTCW fixed-function backend.
 * This never compiles or executes a stage. A candidate only passes a narrow
 * source-shape check: it is not proof of retail CoD semantics or pixels.
 * This header depends on cod1_material_probe.h and has no engine/IO/GL API.
 */
#ifndef COD1_MATERIAL_ELIGIBILITY_H
#define COD1_MATERIAL_ELIGIBILITY_H

typedef enum {
    COD1_ME_EXCLUDED=0, COD1_ME_UNKNOWN, COD1_ME_BLOCKED, COD1_ME_CANDIDATE
} cod1MeStatus_t;
typedef struct {
    int maxBundles; /* implemented by THIS backend, not GL hardware units */
} cod1MeBackend_t;
typedef struct {
    cod1MeStatus_t status;
    char reason[80];
} cod1MeStage_t;
typedef struct {
    unsigned excludedMask,unknownMask,blockedMask,candidateMask;
    char globalBlock[80];
    cod1MeStage_t stage[COD1_MP_MAX_STAGES];
} cod1MeResult_t;

static const char *Cod1ME_Status(cod1MeStatus_t s) {
    switch (s) {
    case COD1_ME_EXCLUDED: return "excluded";
    case COD1_ME_UNKNOWN: return "unknown";
    case COD1_ME_BLOCKED: return "blocked";
    case COD1_ME_CANDIDATE: return "candidate";
    }
    return "invalid";
}
static void Cod1ME_Reason(char *dst,const char *text) {
    if (!dst[0]) Cod1MP_Copy(dst,80,text);
}
static int Cod1ME_OneOf(const char *arg,const char *list) {
    const char *p=list;
    size_t n=strlen(arg);
    while (*p) {
        const char *e=strchr(p,' ');
        size_t size=e?(size_t)(e-p):strlen(p);
        if (n==size) {
            size_t k;
            for (k=0;k<n;++k)
                if (tolower((unsigned char)arg[k])!=tolower((unsigned char)p[k])) break;
            if (k==n) return 1;
        }
        if (!e) break;
        p=e+1;
    }
    return 0;
}
static int Cod1ME_StartsWith(const char *text,const char *prefix) {
    while (*prefix) {
        if (!*text || tolower((unsigned char)*text++)!=tolower((unsigned char)*prefix++)) return 0;
    }
    return 1;
}
static void Cod1ME_ReasonArg(char *dst,const char *prefix,const char *arg) {
    size_t n;
    if (dst[0]) return;
    Cod1MP_Copy(dst,80,prefix);
    n=strlen(dst);
    if (n+1<80) { dst[n++]=' ';dst[n]=0; }
    if (n<79) strncat(dst,arg,79-n);
}
/* The IR stores byte spans, but its lexer is NUL terminated. Restrict the
 * operation lexer to this one source line so a missing parameter cannot be
 * borrowed from the following line. */
static int Cod1ME_Line(const cod1MaterialProbe_t *ir,const cod1MpOp_t *op,
                        cod1MpLexer_t *lx,char line[512]) {
    size_t n;
    if (op->nested || op->start>=op->end || op->end>ir->bytes) return 0;
    n=op->end-op->start;
    if (n>=512) return 0;
    memcpy(line,ir->source+op->start,n);line[n]=0;
    memset(lx,0,sizeof(*lx));lx->base=line;lx->cursor=line;lx->line=1;
    return 1;
}
static int Cod1ME_End(cod1MpLexer_t *lx) {
    return !Cod1MP_Next(lx).text[0] && !lx->failed;
}
static int Cod1ME_Numbers(cod1MpLexer_t *lx,int count) {
    cod1MpToken_t t;double number;
    int i;
    for (i=0;i<count;++i) {
        t=Cod1MP_Next(lx);
        if (!Cod1MP_Number(t.text,&number)) return 0;
    }
    return Cod1ME_End(lx);
}
static int Cod1ME_Wave(cod1MpLexer_t *lx) {
    cod1MpToken_t kind=Cod1MP_Next(lx);
    return Cod1ME_OneOf(kind.text,"sin square triangle sawtooth inversesawtooth noise") &&
           Cod1ME_Numbers(lx,4);
}
static int Cod1ME_ThreeVector(cod1MpLexer_t *lx) {
    cod1MpToken_t t;
    double value;
    int i;
    t=Cod1MP_Next(lx);
    if (strcmp(t.text,"(")) return 0;
    for (i=0;i<3;++i) {
        t=Cod1MP_Next(lx);
        if (!Cod1MP_Number(t.text,&value)) return 0;
    }
    t=Cod1MP_Next(lx);
    return !strcmp(t.text,")") && Cod1ME_End(lx);
}
static void Cod1ME_TopOp(const cod1MaterialProbe_t *ir,const cod1MpOp_t *op,
                          char *reason) {
    cod1MpLexer_t lx;
    cod1MpToken_t t,arg;
    double numerical;
    char line[512];
    if (!Cod1ME_Line(ir,op,&lx,line)) { Cod1ME_Reason(reason,"invalid/long material operation");return; }
    t=Cod1MP_Next(&lx);
    /* Offline compiler and editor hints are not runtime stage operations.
     * q3map_sun is an exception: RTCW applies it to global lighting. */
    if (Cod1ME_StartsWith(t.text,"qer") ||
        (Cod1ME_StartsWith(t.text,"q3map") && !Cod1MP_Equal(t.text,"q3map_sun"))) return;
    arg=Cod1MP_Next(&lx);
    if (Cod1MP_Equal(t.text,"sort")) {
        if (!arg.text[0] || (!Cod1ME_OneOf(arg.text,
             "portal sky opaque decal seeThrough banner additive nearest underwater") &&
            !Cod1MP_Number(arg.text,&numerical)) || !Cod1ME_End(&lx))
            Cod1ME_ReasonArg(reason,"sort unsupported",arg.text);
    } else if (Cod1MP_Equal(t.text,"surfaceParm")) {
        /* Only names consumed by the existing ParseSurfaceParm table, for
         * which this probe is willing to classify source stage shape. */
        if (!Cod1ME_OneOf(arg.text,
             "water nonsolid trans noimpact nodlight nomarks nolightmap sky fog detail slick ladder metal metalsteps woodsteps grasssteps") ||
            !Cod1ME_End(&lx))
            Cod1ME_ReasonArg(reason,"surfaceParm unverified",arg.text);
    } else if (Cod1ME_OneOf(t.text,
               "polygonOffset nomipmaps nopicmip picmip2 entityMergable")) {
        if (arg.text[0]) Cod1ME_ReasonArg(reason,"material arguments unsupported",t.text);
    } else {
        Cod1ME_ReasonArg(reason,"material command unverified",t.text);
    }
}
/* Lex each bounded operation independently. Unknown forms fail closed, even
 * if the legacy parser accepts their opcode and silently ignores an argument.
 */
static void Cod1ME_StageOp(const cod1MaterialProbe_t *ir,const cod1MpStage_t *s,
                            const cod1MpOp_t *op,char *reason) {
    cod1MpLexer_t lx;
    cod1MpToken_t t,arg,next;
    cod1MpImageEvent_t ev;
    char line[512];
    int imageOp,ok;
    if (op->nested) { Cod1ME_Reason(reason,"nested program unsupported");return; }
    if (!Cod1ME_Line(ir,op,&lx,line)) { Cod1ME_Reason(reason,"invalid/long stage operation");return; }
    t=Cod1MP_Next(&lx);
    if (Cod1MP_Equal(t.text,"requires")) return; /* already tri-state resolved */
    if (Cod1MP_Equal(t.text,"perlight") || Cod1MP_Equal(t.text,"multiplyImage") ||
        Cod1MP_Equal(t.text,"waterMap") || Cod1MP_Equal(t.text,"cubeMap") ||
        Cod1MP_Equal(t.text,"texEnvCombine") || Cod1MP_Equal(t.text,"nvTexShader") ||
        Cod1MP_Equal(t.text,"nvRegCombiners") || Cod1MP_Equal(t.text,"atiFragmentShader")) {
        Cod1ME_Reason(reason,t.text);return;
    }
    imageOp=Cod1MP_ImageEvent(ir,op,&ev);
    if (imageOp<0) { Cod1ME_Reason(reason,"missing image argument");return; }
    if (imageOp>0) {
        if (ev.kind==COD1_MP_NEXTBUNDLE) {
            if (!Cod1ME_End(&lx)) Cod1ME_Reason(reason,"nextbundle unexpected arguments");
            return;
        }
        arg=Cod1MP_Next(&lx);
        if (ev.kind==COD1_MP_MAP && Cod1MP_Equal(arg.text,"clamp"))
            arg=Cod1MP_Next(&lx);
        if (!arg.text[0] || !Cod1ME_End(&lx))
            Cod1ME_Reason(reason,"map arguments unsupported");
        if (Cod1MP_Equal(ev.path,"$dlight"))
            Cod1ME_Reason(reason,"dynamic $dlight stage unsupported");
        else if (ev.path[0]=='$' && !Cod1ME_OneOf(ev.path,"$lightmap $whiteimage"))
            Cod1ME_Reason(reason,"unknown image symbol");
        return;
    }
    arg=Cod1MP_Next(&lx);
    if (Cod1MP_Equal(t.text,"tcMod")) {
        if (Cod1MP_Equal(arg.text,"turb"))
            Cod1ME_Reason(reason,"tcMod turb: CoD 3-arg semantics unverified");
        else if (op->bundle>0) Cod1ME_Reason(reason,"bundle-specific tcMod unsupported");
        else {
            if (Cod1ME_OneOf(arg.text,"scroll scale")) ok=Cod1ME_Numbers(&lx,2);
            else if (Cod1MP_Equal(arg.text,"rotate")) ok=Cod1ME_Numbers(&lx,1);
            else if (Cod1MP_Equal(arg.text,"transform")) ok=Cod1ME_Numbers(&lx,6);
            else if (Cod1MP_Equal(arg.text,"stretch")) ok=Cod1ME_Wave(&lx);
            else if (Cod1ME_OneOf(arg.text,"swap entityTranslate")) ok=Cod1ME_End(&lx);
            else { Cod1ME_Reason(reason,"tcMod subtype unsupported");return; }
            if (!ok) Cod1ME_Reason(reason,"tcMod arguments unsupported");
        }
    } else if (Cod1MP_Equal(t.text,"rgbGen")) {
        if (Cod1ME_OneOf(arg.text,"const constLighting")) {
            if (!Cod1ME_ThreeVector(&lx))
                Cod1ME_Reason(reason,"rgbGen const arguments unsupported");
        } else if (Cod1MP_Equal(arg.text,"wave")) {
            if (!Cod1ME_Wave(&lx)) Cod1ME_Reason(reason,"rgbGen wave arguments unsupported");
        } else if (!Cod1ME_OneOf(arg.text,
                 "identity identityLighting vertex exactVertex entity oneMinusEntity oneMinusVertex lightingDiffuse lightingPrecalc"))
            Cod1ME_Reason(reason,"rgbGen subtype unsupported");
        else if (!Cod1ME_End(&lx)) Cod1ME_Reason(reason,"rgbGen extra arguments");
    } else if (Cod1MP_Equal(t.text,"alphaGen")) {
        if (Cod1MP_Equal(arg.text,"const")) {
            if (!Cod1ME_Numbers(&lx,1)) Cod1ME_Reason(reason,"alphaGen const arguments unsupported");
        } else if (Cod1MP_Equal(arg.text,"wave")) {
            if (!Cod1ME_Wave(&lx)) Cod1ME_Reason(reason,"alphaGen wave arguments unsupported");
        } else if (!Cod1ME_OneOf(arg.text,"identity vertex entity oneMinusEntity oneMinusVertex"))
            Cod1ME_Reason(reason,"alphaGen subtype unverified");
        else if (!Cod1ME_End(&lx)) Cod1ME_Reason(reason,"alphaGen extra arguments");
    } else if (Cod1MP_Equal(t.text,"blendFunc")) {
        if (op->bundle>0 && !Cod1MP_Equal(arg.text,"filter"))
            Cod1ME_Reason(reason,"bundle blendFunc unsupported");
        else if (Cod1ME_OneOf(arg.text,"add filter blend")) {
            if (!Cod1ME_End(&lx)) Cod1ME_Reason(reason,"blendFunc extra arguments");
        } else if (op->bundle==0 && Cod1ME_OneOf(arg.text,
                  "GL_ONE GL_ZERO GL_DST_COLOR GL_ONE_MINUS_DST_COLOR GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA GL_DST_ALPHA GL_ONE_MINUS_DST_ALPHA GL_SRC_ALPHA_SATURATE")) {
            next=Cod1MP_Next(&lx);
            if (!Cod1ME_OneOf(next.text,
                    "GL_ONE GL_ZERO GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA GL_DST_ALPHA GL_ONE_MINUS_DST_ALPHA GL_SRC_COLOR GL_ONE_MINUS_SRC_COLOR") ||
                !Cod1ME_End(&lx)) Cod1ME_Reason(reason,"blendFunc factors unsupported");
        } else Cod1ME_Reason(reason,"blendFunc subtype unverified");
    } else if (Cod1MP_Equal(t.text,"tcGen") || Cod1MP_Equal(t.text,"texGen")) {
        if (!Cod1ME_OneOf(arg.text,"texture base environment lightmap"))
            Cod1ME_Reason(reason,"tcGen subtype unsupported");
        else if (!Cod1ME_End(&lx)) Cod1ME_Reason(reason,"tcGen extra arguments");
    } else if (Cod1MP_Equal(t.text,"alphaFunc")) {
        if (!Cod1ME_OneOf(arg.text,"GT0 LT128 GE128") || !Cod1ME_End(&lx))
            Cod1ME_Reason(reason,"alphaFunc arguments unsupported");
    } else if (Cod1MP_Equal(t.text,"depthFunc")) {
        if (!Cod1ME_OneOf(arg.text,"lequal equal") || !Cod1ME_End(&lx))
            Cod1ME_Reason(reason,"depthFunc arguments unsupported");
    } else if (Cod1MP_Equal(t.text,"depthWrite") || Cod1MP_Equal(t.text,"detail")) {
        if (arg.text[0]) Cod1ME_Reason(reason,"unexpected stage arguments");
    } else {
        /* Be conservative about every new script command until validated. */
        Cod1ME_Reason(reason,t.text[0]?t.text:"empty stage opcode");
    }
    (void)s;
}
static void Cod1ME_Evaluate(const cod1MaterialProbe_t *ir,const cod1MeBackend_t *backend,
                             cod1MeResult_t *out) {
    int i,j,tailMap;
    memset(out,0,sizeof(*out));
    if (ir->topBlockingOpcode[0])
        Cod1MP_Copy(out->globalBlock,sizeof(out->globalBlock),ir->topBlockingOpcode);
    for (j=0;j<ir->topOps;++j)
        Cod1ME_TopOp(ir,&ir->topOp[j],out->globalBlock);
    for (i=0;i<ir->numStages;++i) {
        const cod1MpStage_t *s=&ir->stage[i];
        cod1MeStage_t *stage=&out->stage[i];
        const unsigned mask=1u<<i;
        if (s->selected==COD1_MP_FALSE) {
            stage->status=COD1_ME_EXCLUDED;
            Cod1ME_Reason(stage->reason,"requires false");
            out->excludedMask|=mask;continue;
        }
        if (s->selected==COD1_MP_UNKNOWN) {
            stage->status=COD1_ME_UNKNOWN;
            Cod1ME_Reason(stage->reason,s->syntaxUnknown?"requires syntax unknown":"requires profile unknown");
            out->unknownMask|=mask;continue;
        }
        if (!backend || backend->maxBundles<1) Cod1ME_Reason(stage->reason,"backend units unknown");
        else if (s->numBundles>backend->maxBundles) Cod1ME_Reason(stage->reason,"too many bundles for backend");
        tailMap=0;
        for (j=0;j<s->numOps;++j) {
            cod1MpImageEvent_t ev;
            int isImage=Cod1MP_ImageEvent(ir,&s->op[j],&ev);
            if (isImage==1 && ev.bundle>0 && ev.kind!=COD1_MP_NEXTBUNDLE) {
                if (ev.kind!=COD1_MP_MAP ||
                    (!Cod1MP_Equal(ev.path,"$lightmap") && !Cod1MP_Equal(ev.path,"$whiteimage")))
                    Cod1ME_Reason(stage->reason,"ordinary nextbundle image unsupported");
                ++tailMap;
            }
            Cod1ME_StageOp(ir,s,&s->op[j],stage->reason);
        }
        if (s->numBundles>1 && (s->numBundles!=2 || tailMap!=1 || s->mapCount[0]!=1 || s->mapCount[1]!=1))
            Cod1ME_Reason(stage->reason,"nextbundle layout unsupported");
        if (!s->mapCount[0]) Cod1ME_Reason(stage->reason,"stage has no base image");
        if (s->mapCount[0]>1) Cod1ME_Reason(stage->reason,"multiple base images unsupported");
        if (out->globalBlock[0]) Cod1ME_Reason(stage->reason,out->globalBlock);
        if (stage->reason[0]) {
            stage->status=COD1_ME_BLOCKED;out->blockedMask|=mask;
        } else {
            stage->status=COD1_ME_CANDIDATE;
            Cod1ME_Reason(stage->reason,"source shape only; pixels unverified");
            out->candidateMask|=mask;
        }
    }
}
#endif /* COD1_MATERIAL_ELIGIBILITY_H */
