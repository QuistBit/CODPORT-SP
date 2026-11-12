/* Source-level CoD1 shader probe, independent of renderer and resource IO.
 * Diagnostic only. No GL, printf, allocation, file access or material names.
 * A selected stage is an authored condition match, not executable native GL.
 */
#ifndef COD1_MATERIAL_PROBE_H
#define COD1_MATERIAL_PROBE_H
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define COD1_MP_MAX_STAGES 32
#define COD1_MP_MAX_BUNDLES 8
#define COD1_MP_MAX_OPS 32
#define COD1_MP_MAX_TOP_OPS 32
#define COD1_MP_MAX_BYTES (128u*1024u)

typedef enum { COD1_MP_FALSE=0, COD1_MP_TRUE=1, COD1_MP_UNKNOWN=2 } cod1MpTruth_t;
typedef cod1MpTruth_t (*cod1MpExtensionFn)(const char *name,void *user);
typedef int (*cod1MpCvarFn)(const char *name,double *value,void *user);
typedef struct {
    int units; /* Implemented profile, -1 means unknown. Not retail GPU proof. */
    cod1MpExtensionFn extension;
    cod1MpCvarFn cvar;
    void *user;
} cod1MpCaps_t;
typedef struct {
    size_t start,end; /* byte span in the original source */
    unsigned char bundle,nested;
} cod1MpOp_t;
typedef struct {
    size_t start,end; /* byte offsets relative to source, which may begin before '{' */
    int line, numBundles, numOps, numRequires, syntaxUnknown;
    int mapCount[COD1_MP_MAX_BUNDLES],tcModCount[COD1_MP_MAX_BUNDLES];
    cod1MpTruth_t selected;
    char blockingOpcode[48];
    cod1MpOp_t op[COD1_MP_MAX_OPS];
} cod1MpStage_t;
typedef struct {
    const char *source; /* borrowed shaderText pointer, never owned */
    size_t bytes;
    unsigned sourceHash,selectedMask,unknownMask;
    int numStages,topOps;
    char sortToken[48],topBlockingOpcode[48],error[96];
    cod1MpOp_t topOp[COD1_MP_MAX_TOP_OPS];
    cod1MpStage_t stage[COD1_MP_MAX_STAGES];
} cod1MaterialProbe_t;
typedef enum { COD1_MP_IMAGE_NONE=0,COD1_MP_MAP,COD1_MP_CLAMPMAP,COD1_MP_NEXTBUNDLE } cod1MpImageKind_t;
typedef struct {
    cod1MpImageKind_t kind;
    int bundle;
    int clamp; /* clampmap or CoD's `map clamp path` */
    char path[128]; /* empty for nextbundle */
} cod1MpImageEvent_t;

typedef struct { char text[128];const char *start,*end;int line; } cod1MpToken_t;
typedef struct {
    const char *base,*cursor;
    int line,failed,hasPeek;
    cod1MpToken_t peek;
} cod1MpLexer_t;
static int Cod1MP_Equal(const char *a,const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a)!=tolower((unsigned char)*b)) return 0;
        ++a;++b;
    }
    return !*a && !*b;
}
static void Cod1MP_Copy(char *dst,size_t size,const char *src) {
    size_t n=strlen(src);
    if (n>=size) n=size-1;
    memcpy(dst,src,n);dst[n]=0;
}
static void Cod1MP_Fail(cod1MaterialProbe_t *out,cod1MpLexer_t *lx,const char *reason) {
    lx->failed=1;Cod1MP_Copy(out->error,sizeof(out->error),reason);
}
static cod1MpToken_t Cod1MP_Raw(cod1MpLexer_t *lx) {
    cod1MpToken_t t;const char *p=lx->cursor;size_t n=0;
    memset(&t,0,sizeof(t));
    while (*p && (size_t)(p-lx->base)<COD1_MP_MAX_BYTES) {
        if (isspace((unsigned char)*p)) { if (*p=='\n') ++lx->line; ++p;continue; }
        if (p[0]=='/' && p[1]=='/') { p+=2;while (*p && *p!='\n') ++p;continue; }
        if (p[0]=='/' && p[1]=='*') {
            p+=2;
            while (*p && !(p[0]=='*' && p[1]=='/')) {
                if (*p=='\n') ++lx->line;
                ++p;
            }
            if (!*p) { lx->failed=1;lx->cursor=p;return t; }
            p+=2;continue;
        }
        break;
    }
    t.line=lx->line;t.start=p;
    if ((size_t)(p-lx->base)>=COD1_MP_MAX_BYTES) lx->failed=1;
    if (!*p || lx->failed) { lx->cursor=p;t.end=p;return t; }
    if (strchr("{}()!<>=|&",*p)) {
        t.text[n++]=*p++;
        if ((strchr("!<>=",t.text[0]) && *p=='=') ||
            (t.text[0]=='|' && *p=='|') || (t.text[0]=='&' && *p=='&'))
            t.text[n++]=*p++;
    } else if (*p=='"') {
        t.text[n++]=*p++;
        while (*p && *p!='"' && n+2<sizeof(t.text)) {
            if (*p=='\\' && p[1]) { t.text[n++]=*p++; }
            t.text[n++]=*p++;
        }
        if (*p=='"') t.text[n++]=*p++;
        else lx->failed=1;
    } else {
        while (*p && !isspace((unsigned char)*p) && !strchr("{}()!<>=|&",*p) &&
               !(p[0]=='/' && (p[1]=='/' || p[1]=='*'))) {
            if (n+1>=sizeof(t.text)) { lx->failed=1;break; }
            t.text[n++]=*p++;
        }
    }
    t.text[n]=0;t.end=p;lx->cursor=p;
    return t;
}
static cod1MpToken_t Cod1MP_Next(cod1MpLexer_t *lx) {
    if (lx->hasPeek) { lx->hasPeek=0;return lx->peek; }
    return Cod1MP_Raw(lx);
}
static cod1MpToken_t Cod1MP_Peek(cod1MpLexer_t *lx) {
    if (!lx->hasPeek) { lx->peek=Cod1MP_Raw(lx);lx->hasPeek=1; }
    return lx->peek;
}
static void Cod1MP_SkipLine(cod1MpLexer_t *lx,int line) {
    while (!lx->failed && Cod1MP_Peek(lx).text[0] && Cod1MP_Peek(lx).line==line) {
        cod1MpToken_t t=Cod1MP_Next(lx);
        /* This bounded dialect expects braces on separate lines. An inline
         * block must fail closed rather than hide a stage boundary. */
        if (!strcmp(t.text,"{") || !strcmp(t.text,"}")) lx->failed=1;
    }
}
static cod1MpTruth_t Cod1MP_Not(cod1MpTruth_t x) {
    return x==COD1_MP_UNKNOWN?x:(x==COD1_MP_TRUE?COD1_MP_FALSE:COD1_MP_TRUE);
}
static cod1MpTruth_t Cod1MP_Or(cod1MpTruth_t a,cod1MpTruth_t b) {
    if (a==COD1_MP_TRUE || b==COD1_MP_TRUE) return COD1_MP_TRUE;
    if (a==COD1_MP_UNKNOWN || b==COD1_MP_UNKNOWN) return COD1_MP_UNKNOWN;
    return COD1_MP_FALSE;
}
static cod1MpTruth_t Cod1MP_And(cod1MpTruth_t a,cod1MpTruth_t b) {
    if (a==COD1_MP_FALSE || b==COD1_MP_FALSE) return COD1_MP_FALSE;
    if (a==COD1_MP_UNKNOWN || b==COD1_MP_UNKNOWN) return COD1_MP_UNKNOWN;
    return COD1_MP_TRUE;
}
static int Cod1MP_Compare(double left,const char *op,double right) {
    if (!strcmp(op,"<")) return left<right;
    if (!strcmp(op,"<=")) return left<=right;
    if (!strcmp(op,">")) return left>right;
    if (!strcmp(op,">=")) return left>=right;
    if (!strcmp(op,"==")) return left==right;
    if (!strcmp(op,"!=")) return left!=right;
    return -1;
}
static int Cod1MP_Number(const char *s,double *out) {
    char *end;
    if (!s[0]) return 0;
    *out=strtod(s,&end);
    return end!=s && !*end;
}
static cod1MpTruth_t Cod1MP_Atom(cod1MpLexer_t *lx,const cod1MpCaps_t *caps,int line,int *ok) {
    cod1MpToken_t t,op,n;
    cod1MpTruth_t v;
    int negate=0,comparison;
    double left,right;
    *ok=1;t=Cod1MP_Next(lx);
    if (!strcmp(t.text,"!")) { negate=1;t=Cod1MP_Next(lx); }
    if (t.line!=line || !t.text[0]) { *ok=0;return COD1_MP_UNKNOWN; }
    if (Cod1MP_Equal(t.text,"GL_MAX_TEXTURE_UNITS_ARB") || Cod1MP_Equal(t.text,"cvar")) {
        int isCvar=Cod1MP_Equal(t.text,"cvar");
        cod1MpToken_t name;
        if (negate) { *ok=0;return COD1_MP_UNKNOWN; }
        if (isCvar) { name=Cod1MP_Next(lx); if (name.line!=line) { *ok=0;return COD1_MP_UNKNOWN; } }
        op=Cod1MP_Next(lx);n=Cod1MP_Next(lx);
        if (op.line!=line || n.line!=line || !Cod1MP_Number(n.text,&right) ||
            Cod1MP_Compare(0,op.text,0)<0) { *ok=0;return COD1_MP_UNKNOWN; }
        if (isCvar) {
            if (!caps->cvar || !caps->cvar(name.text,&left,caps->user)) return COD1_MP_UNKNOWN;
        } else {
            if (caps->units<0) return COD1_MP_UNKNOWN;
            left=caps->units;
        }
        comparison=Cod1MP_Compare(left,op.text,right);
        return comparison?COD1_MP_TRUE:COD1_MP_FALSE;
    }
    if (strncmp(t.text,"GL_",3)) { *ok=0;return COD1_MP_UNKNOWN; }
    v=caps->extension?caps->extension(t.text,caps->user):COD1_MP_UNKNOWN;
    return negate?Cod1MP_Not(v):v;
}
/* Only || is supported within a line. Malformed syntax poisons the entire
 * line even if a preceding term was true; another false line can still veto.
 */
static cod1MpTruth_t Cod1MP_Require(cod1MpLexer_t *lx,const cod1MpCaps_t *caps,
                                     int line,int *syntaxUnknown) {
    cod1MpTruth_t result=COD1_MP_FALSE,v;
    cod1MpToken_t separator;
    int terms=0,ok;
    while (!lx->failed && Cod1MP_Peek(lx).text[0] && Cod1MP_Peek(lx).line==line) {
        if (++terms>16) break;
        v=Cod1MP_Atom(lx,caps,line,&ok);
        if (!ok) break;
        result=Cod1MP_Or(result,v);
        if (Cod1MP_Peek(lx).text[0] && Cod1MP_Peek(lx).line==line) {
            separator=Cod1MP_Next(lx);
            if (strcmp(separator.text,"||")) break;
            if (!Cod1MP_Peek(lx).text[0] || Cod1MP_Peek(lx).line!=line) break;
        } else return result;
    }
    *syntaxUnknown=1;Cod1MP_SkipLine(lx,line);
    return COD1_MP_UNKNOWN;
}
static void Cod1MP_Block(char *dst,const char *opcode) {
    if (!dst[0]) Cod1MP_Copy(dst,48,opcode);
}
static void Cod1MP_Operation(cod1MpLexer_t *lx,cod1MpOp_t *op,
                              const char *begin,const char *end,int bundle,int nested) {
    if (!nested) {
        end=begin;
        while ((size_t)(end-lx->base)<COD1_MP_MAX_BYTES && *end && *end!='\n') ++end;
        while (end>begin && isspace((unsigned char)end[-1])) --end;
    }
    op->start=(size_t)(begin-lx->base);
    op->end=(size_t)(end-lx->base);
    op->bundle=(unsigned char)bundle;
    op->nested=(unsigned char)nested;
}
static int Cod1MP_Nested(cod1MpLexer_t *lx) {
    cod1MpToken_t t;int depth=1;
    while (!lx->failed && (t=Cod1MP_Next(lx)).text[0]) {
        if (!strcmp(t.text,"{")) { if (++depth>32) return 0; }
        else if (!strcmp(t.text,"}") && --depth==0) return 1;
    }
    return 0;
}
static int Cod1MP_Stage(cod1MpLexer_t *lx,const cod1MpCaps_t *caps,
                         cod1MpStage_t *stage,cod1MpToken_t opening) {
    cod1MpToken_t t,arg;
    int bundle=0,unknown;
    stage->start=(size_t)(opening.start-lx->base);
    stage->line=opening.line;
    stage->numBundles=1;
    stage->selected=COD1_MP_TRUE;
    while (!lx->failed && (t=Cod1MP_Next(lx)).text[0]) {
        if (!strcmp(t.text,"}")) { stage->end=(size_t)(t.end-lx->base);return 1; }
        if (!strcmp(t.text,"{")) {
            Cod1MP_Block(stage->blockingOpcode,"nested_block");
            if (!Cod1MP_Nested(lx)) return 0;
            if (stage->numOps>=COD1_MP_MAX_OPS) return 0;
            Cod1MP_Operation(lx,&stage->op[stage->numOps++],t.start,lx->cursor,bundle,1);
            continue;
        }
        if (stage->numOps>=COD1_MP_MAX_OPS) return 0;
        Cod1MP_Operation(lx,&stage->op[stage->numOps],t.start,t.end,bundle,0);
        ++stage->numOps;
        if (Cod1MP_Equal(t.text,"requires")) {
            if (++stage->numRequires>16) return 0;
            unknown=0;
            stage->selected=Cod1MP_And(stage->selected,Cod1MP_Require(lx,caps,t.line,&unknown));
            stage->syntaxUnknown|=unknown;
            continue;
        }
        if (Cod1MP_Equal(t.text,"nextbundle")) {
            if (++bundle>=COD1_MP_MAX_BUNDLES) return 0;
            stage->numBundles=bundle+1;Cod1MP_SkipLine(lx,t.line);continue;
        }
        if (Cod1MP_Equal(t.text,"map") || Cod1MP_Equal(t.text,"clampmap"))
            ++stage->mapCount[bundle];
        if (Cod1MP_Equal(t.text,"tcmod")) {
            ++stage->tcModCount[bundle];
            arg=Cod1MP_Peek(lx);
            if (arg.line==t.line && Cod1MP_Equal(arg.text,"turb"))
                Cod1MP_Block(stage->blockingOpcode,"tcMod turb");
        } else if (Cod1MP_Equal(t.text,"rgbgen")) {
            arg=Cod1MP_Peek(lx);
            if (arg.line==t.line && Cod1MP_Equal(arg.text,"constlighting"))
                Cod1MP_Block(stage->blockingOpcode,"rgbGen constlighting");
        } else if (Cod1MP_Equal(t.text,"multiplyimage"))
            Cod1MP_Block(stage->blockingOpcode,"multiplyImage");
        else if (Cod1MP_Equal(t.text,"perlight"))
            Cod1MP_Block(stage->blockingOpcode,"perlight");
        else if (Cod1MP_Equal(t.text,"watermap") || Cod1MP_Equal(t.text,"cubemap") ||
                 Cod1MP_Equal(t.text,"nvtexshader") || Cod1MP_Equal(t.text,"nvregcombiners") ||
                 Cod1MP_Equal(t.text,"atifragmentshader"))
            Cod1MP_Block(stage->blockingOpcode,t.text);
        else if (!Cod1MP_Equal(t.text,"map") && !Cod1MP_Equal(t.text,"clampmap") &&
                 !Cod1MP_Equal(t.text,"rgbgen") && !Cod1MP_Equal(t.text,"tcmod") &&
                 !Cod1MP_Equal(t.text,"blendfunc") && !Cod1MP_Equal(t.text,"alphagen") &&
                 !Cod1MP_Equal(t.text,"tcgen") && !Cod1MP_Equal(t.text,"texgen") &&
                 !Cod1MP_Equal(t.text,"alphafunc") && !Cod1MP_Equal(t.text,"depthfunc") &&
                 !Cod1MP_Equal(t.text,"depthwrite") && !Cod1MP_Equal(t.text,"detail") &&
                 !Cod1MP_Equal(t.text,"fog") && !Cod1MP_Equal(t.text,"animmap"))
            Cod1MP_Block(stage->blockingOpcode,t.text);
        Cod1MP_SkipLine(lx,t.line);
    }
    return 0;
}
static int Cod1MP_Parse(const char *body,const cod1MpCaps_t *caps,cod1MaterialProbe_t *out) {
    cod1MpLexer_t lx;cod1MpToken_t t,arg;int i;
    unsigned hash=2166136261u;
    memset(out,0,sizeof(*out));
    if (!body || !caps) { Cod1MP_Copy(out->error,sizeof(out->error),"missing body/profile");return 0; }
    out->source=body;
    memset(&lx,0,sizeof(lx));lx.base=body;lx.cursor=body;lx.line=1;
    t=Cod1MP_Next(&lx);
    if (strcmp(t.text,"{")) { Cod1MP_Fail(out,&lx,"expected material opening brace");return 0; }
    while (!lx.failed && (t=Cod1MP_Next(&lx)).text[0]) {
        if (!strcmp(t.text,"}")) {
            out->bytes=(size_t)(t.end-body);
            for (i=0;i<(int)out->bytes;++i) hash=(hash^(unsigned char)body[i])*16777619u;
            out->sourceHash=hash;
            for (i=0;i<out->numStages;++i) {
                if (out->stage[i].selected==COD1_MP_TRUE) out->selectedMask|=1u<<i;
                else if (out->stage[i].selected==COD1_MP_UNKNOWN) out->unknownMask|=1u<<i;
            }
            return 1;
        }
        if (!strcmp(t.text,"{")) {
            if (out->numStages>=COD1_MP_MAX_STAGES ||
                !Cod1MP_Stage(&lx,caps,&out->stage[out->numStages],t)) {
                Cod1MP_Fail(out,&lx,"unbalanced/oversized stage");return 0;
            }
            ++out->numStages;continue;
        }
        if (out->topOps>=COD1_MP_MAX_TOP_OPS) {
            Cod1MP_Fail(out,&lx,"too many material directives");return 0;
        }
        Cod1MP_Operation(&lx,&out->topOp[out->topOps],t.start,t.end,0,0);
        ++out->topOps;
        if (Cod1MP_Equal(t.text,"sort")) {
            arg=Cod1MP_Peek(&lx);
            if (arg.line==t.line) {
                Cod1MP_Copy(out->sortToken,sizeof(out->sortToken),arg.text);
                if (Cod1MP_Equal(arg.text,"water"))
                    Cod1MP_Block(out->topBlockingOpcode,"sort water");
            }
        }
        Cod1MP_SkipLine(&lx,t.line);
    }
    Cod1MP_Fail(out,&lx,"unbalanced/oversized material");
    return 0;
}
/* Reads a single source operation without touching image state. The caller
 * decides whether its stage was selected; do not inspect vendor/unselected
 * stages just to drive resource IO. Return 1 for an image operation, 0 for
 * another directive and -1 for an incomplete image directive. */
static int Cod1MP_ImageEvent(const cod1MaterialProbe_t *ir,const cod1MpOp_t *op,
                              cod1MpImageEvent_t *event) {
    cod1MpLexer_t lx;
    cod1MpToken_t t,arg;
    memset(event,0,sizeof(*event));
    if (op->nested || op->start>=op->end || op->end>ir->bytes) return 0;
    memset(&lx,0,sizeof(lx));
    lx.base=ir->source+op->start;lx.cursor=lx.base;lx.line=1;
    t=Cod1MP_Next(&lx);
    if (Cod1MP_Equal(t.text,"map")) event->kind=COD1_MP_MAP;
    else if (Cod1MP_Equal(t.text,"clampmap")) {
        event->kind=COD1_MP_CLAMPMAP;event->clamp=1;
    }
    else if (Cod1MP_Equal(t.text,"nextbundle")) event->kind=COD1_MP_NEXTBUNDLE;
    else return 0;
    event->bundle=op->bundle;
    if (event->kind==COD1_MP_NEXTBUNDLE) return 1;
    arg=Cod1MP_Next(&lx);
    if (arg.line!=t.line || !arg.text[0]) return -1;
    if (event->kind==COD1_MP_MAP && Cod1MP_Equal(arg.text,"clamp")) {
        event->clamp=1;
        arg=Cod1MP_Next(&lx);
        if (arg.line!=t.line || !arg.text[0]) return -1;
    }
    Cod1MP_Copy(event->path,sizeof(event->path),arg.text);
    return 1;
}
#endif /* COD1_MATERIAL_PROBE_H */
