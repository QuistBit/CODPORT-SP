/* R50: source-driven ordinary water program. No map/material-name dispatch.
 * Selection is performed by cod1_material_probe BEFORE backend validation.
 * Unknown requirements and unsupported selected operations fail the WHOLE
 * program. This is an explicit, bounded subset, not a full CoD shader VM. */
#ifndef COD1_WATER_PROGRAM_R50_H
#define COD1_WATER_PROGRAM_R50_H
#include <math.h>
#include <float.h>
#include "cod1_material_probe.h"
#include "cod1_material_eligibility.h"
#include "cod1_watermap_core.h"
#include "cod1_water_program_types.h"
#include "cod1_material_map.h"

static int C1WP_Finite(float v) { return v==v && v<=FLT_MAX && v>=-FLT_MAX; }
static int C1WP_Float(const char *s,float *v) {
    double d; if(!Cod1MP_Number(s,&d)||d!=d||d>FLT_MAX||d< -FLT_MAX)return 0;
    *v=(float)d;return 1;
}
static int C1WP_ReadFloat(cod1MpLexer_t *l,float *v) {
    cod1MpToken_t t=Cod1MP_Next(l);return C1WP_Float(t.text,v);
}
static int C1WP_Expect(cod1MpLexer_t *l,const char *s) {
    cod1MpToken_t t=Cod1MP_Next(l);return Cod1MP_Equal(t.text,s);
}
static int C1WP_Vector(cod1MpLexer_t *l,float *v,int n) {
    int i;if(!C1WP_Expect(l,"("))return 0;
    for(i=0;i<n;i++)if(!C1WP_ReadFloat(l,&v[i]))return 0;
    return C1WP_Expect(l,")");
}
static int C1WP_Error(c1wp_error *e,const char *s,int line) {
    if(e){e->line=line;Cod1MP_Copy(e->reason,sizeof(e->reason),s);}return 0;
}
/* Combine has its own punctuation lexer. Unlike COM_Parse this also handles
 * the original files' comma-separated Cp,Ct and 1-Ap operands without spaces. */
typedef struct { const char *p,*end; int bad; char token[128]; } c1wp_lex;
static const char *C1WP_Next(c1wp_lex *l) {
    size_t n=0;const char *p=l->p;
    for(;;) {
        while(p<l->end&&isspace((unsigned char)*p))p++;
        if(p+1<l->end&&p[0]=='/'&&p[1]=='/') {while(p<l->end&&*p!='\n')p++;continue;}
        break;
    }
    if(p>=l->end){l->p=p;l->token[0]=0;return l->token;}
    if(strchr("{}()=,*-",*p))l->token[n++]=*p++;
    else while(p<l->end&&!isspace((unsigned char)*p)&&!strchr("{}()=,*-",*p)) {
        if(n+1>=sizeof(l->token)){l->bad=1;break;}l->token[n++]=*p++;
    }
    l->token[n]=0;l->p=p;return l->token;
}
static int C1WP_Token(c1wp_lex *l,const char *s){return Cod1MP_Equal(C1WP_Next(l),s);}
static int C1WP_Operand(c1wp_lex *l,c1wp_arg *a,int alpha) {
    const char *s=C1WP_Next(l);memset(a,0,sizeof(*a));
    if(!strcmp(s,"1")){a->invert=1;if(!C1WP_Token(l,"-"))return 0;s=C1WP_Next(l);}
    if(strlen(s)!=2 || (s[0]!='C'&&s[0]!='c'&&s[0]!='A'&&s[0]!='a'))return 0;
    a->alpha=tolower((unsigned char)s[0])=='a';
    if(alpha&&!a->alpha)return 0;
    if(tolower((unsigned char)s[1])=='p')a->source=C1WP_PREVIOUS;
    else if(tolower((unsigned char)s[1])=='t')a->source=C1WP_TEXTURE;
    else if(tolower((unsigned char)s[1])=='c')a->source=C1WP_CONSTANT;
    else return 0;
    return 1;
}
static int C1WP_Expression(c1wp_lex *l,c1wp_expr *x,int alpha) {
    const char *op;int i;if(!C1WP_Token(l,"="))return 0;op=C1WP_Next(l);
    if(Cod1MP_Equal(op,"REPLACE")){x->op=C1WP_REPLACE;x->count=1;}
    else if(Cod1MP_Equal(op,"MODULATE")){x->op=C1WP_MODULATE;x->count=2;}
    else if(Cod1MP_Equal(op,"ADD")){x->op=C1WP_ADD;x->count=2;}
    else if(Cod1MP_Equal(op,"INTERPOLATE_ARB")){x->op=C1WP_INTERPOLATE;x->count=3;}
    else if(!alpha&&Cod1MP_Equal(op,"DOT3_RGB_ARB")){x->op=C1WP_DOT3_RGB;x->count=2;}
    else if(!alpha&&Cod1MP_Equal(op,"DOT3_RGBA_ARB")){x->op=C1WP_DOT3_RGBA;x->count=2;}
    else return 0;
    if(!C1WP_Token(l,"("))return 0;
    for(i=0;i<x->count;i++){if(i&&!C1WP_Token(l,","))return 0;
        if(!C1WP_Operand(l,&x->arg[i],alpha))return 0;
        if(x->op>=C1WP_DOT3_RGB&&x->arg[i].alpha)return 0;}
    return C1WP_Token(l,")");
}
static void C1WP_DefaultExpr(c1wp_expr *x,int alpha) {
    memset(x,0,sizeof(*x));x->op=C1WP_MODULATE;x->count=2;
    x->arg[0].source=C1WP_PREVIOUS;x->arg[1].source=C1WP_TEXTURE;
    x->arg[0].alpha=x->arg[1].alpha=alpha;
}
static int C1WP_Combine(const cod1MaterialProbe_t *ir,const cod1MpOp_t *o,c1wp_combine *c) {
    c1wp_lex l;const char *s;int i,seen=0;memset(&l,0,sizeof(l));
    l.p=ir->source+o->start;l.end=ir->source+o->end;
    if(!C1WP_Token(&l,"texEnvCombine")||!C1WP_Token(&l,"{"))return 0;
    c->enabled=1;C1WP_DefaultExpr(&c->rgb,0);C1WP_DefaultExpr(&c->alpha,1);
    while(*(s=C1WP_Next(&l)) && strcmp(s,"}")) {
        if(Cod1MP_Equal(s,"rgb")){if(seen&1)return 0;seen|=1;if(!C1WP_Expression(&l,&c->rgb,0))return 0;}
        else if(Cod1MP_Equal(s,"alpha")){if(seen&2)return 0;seen|=2;if(!C1WP_Expression(&l,&c->alpha,1))return 0;}
        else if(Cod1MP_Equal(s,"const")) {
            if(seen&4)return 0;seen|=4;c->constantPresent=1;
            if(!C1WP_Token(&l,"=")||!C1WP_Token(&l,"("))return 0;
            for(i=0;i<4;i++)if(!C1WP_Float(C1WP_Next(&l),&c->constant[i]))return 0;
            if(!C1WP_Token(&l,")"))return 0;
            {const char *save=l.p;s=C1WP_Next(&l);
             if(!strcmp(s,"*")){if(!C1WP_Token(&l,"identityLighting"))return 0;c->constantLighting=1;}
             else l.p=save;}
        } else return 0;
    }
    /* Native DOT3_RGBA writes both RGB/alpha; a separate alpha equation is
     * rejected by the original parser as well (0x4e7c16..0x4e7c40). */
    return !l.bad && !strcmp(s,"}") && !*C1WP_Next(&l) && (seen&1) &&
           !(c->rgb.op==C1WP_DOT3_RGBA&&(seen&2));
}
static int C1WP_Blend(const char *s) {
    static const char *n[]={"GL_ZERO","GL_ONE","GL_SRC_ALPHA","GL_ONE_MINUS_SRC_ALPHA",
      "GL_DST_COLOR","GL_ONE_MINUS_DST_COLOR","GL_DST_ALPHA","GL_ONE_MINUS_DST_ALPHA",
      "GL_SRC_COLOR","GL_ONE_MINUS_SRC_COLOR"};int i;
    for(i=0;i<10;i++)if(Cod1MP_Equal(s,n[i]))return i;return -1;
}
static int C1WP_Top(const cod1MaterialProbe_t *ir,c1wp_plan *p,c1wp_error *e) {
    int i;cod1MpLexer_t l;cod1MpToken_t t,a;char line[512];p->cull=0;p->sort=11;
    for(i=0;i<ir->topOps;i++) {
        if(!Cod1ME_Line(ir,&ir->topOp[i],&l,line))return C1WP_Error(e,"unsupported top-level block",0);
        t=Cod1MP_Next(&l);a=Cod1MP_Next(&l);
        if(Cod1MP_Equal(t.text,"qer_editorimage")){if(!*a.text||!Cod1ME_End(&l))goto bad;}
        else if(Cod1MP_Equal(t.text,"q3map_globaltexture")){if(*a.text)goto bad;}
        else if(Cod1MP_Equal(t.text,"tessSize")){if(!C1WP_Float(a.text,&p->tessSize)||p->tessSize<=0||!Cod1ME_End(&l))goto bad;}
        else if(Cod1MP_Equal(t.text,"surfaceparm")) {
            int bit=0;
            if(Cod1MP_Equal(a.text,"trans"))bit=1;
            else if(Cod1MP_Equal(a.text,"nonsolid"))bit=2;
            else if(Cod1MP_Equal(a.text,"water"))bit=4;
            else if(Cod1MP_Equal(a.text,"nolightmap"))bit=8;
            else if(Cod1MP_Equal(a.text,"noshadow"))bit=16;
            else if(Cod1MP_Equal(a.text,"nodlight"))bit=32;
            else if(Cod1MP_Equal(a.text,"nomarks"))bit=64;
            if(!bit||!Cod1ME_End(&l))goto bad;p->waterFlags|=bit;
        } else if(Cod1MP_Equal(t.text,"sort")) {
            if(Cod1MP_Equal(a.text,"water"))p->sort=11;
            else if(Cod1MP_Equal(a.text,"ocean"))p->sort=3;
            else goto bad;if(!Cod1ME_End(&l))goto bad;
        } else if(Cod1MP_Equal(t.text,"cull")) {
            if(Cod1ME_OneOf(a.text,"none disable twosided"))p->cull=2;
            else if(Cod1MP_Equal(a.text,"back"))p->cull=1;
            else if(Cod1MP_Equal(a.text,"front"))p->cull=0;
            else goto bad;if(!Cod1ME_End(&l))goto bad;
        } else if(Cod1MP_Equal(t.text,"deformVertexes")) {
            if(p->hasWave||!Cod1MP_Equal(a.text,"wave"))goto bad;
            if(!C1WP_ReadFloat(&l,&p->waveSpread)||p->waveSpread==0||
               !C1WP_Expect(&l,"sin")||!C1WP_ReadFloat(&l,&p->waveBase)||
               !C1WP_ReadFloat(&l,&p->waveAmplitude)||!C1WP_ReadFloat(&l,&p->wavePhase)||
               !C1WP_ReadFloat(&l,&p->waveFrequency)||!Cod1ME_End(&l))goto bad;
            p->waveSpread=1.0f/p->waveSpread;if(!C1WP_Finite(p->waveSpread))goto bad;p->hasWave=1;
        } else if(Cod1MP_Equal(t.text,"polygonOffset")){if(*a.text)goto bad;p->polygonOffset=1;}
        else if(Cod1MP_Equal(t.text,"nomipmaps")){if(*a.text)goto bad;p->noMip=1;}
        else if(Cod1MP_Equal(t.text,"nopicmip")){if(*a.text)goto bad;p->noPicMip=1;}
        else goto bad;
        continue;
    bad:return C1WP_Error(e,t.text,0); /* top-level byte span is not a line number */
    }
    return (p->waterFlags&7)==7?1:C1WP_Error(e,"requires explicit trans/nonsolid/water surface metadata",0);
}
static int C1WP_Stage(const cod1MaterialProbe_t *ir,int index,c1wp_plan *p,c1wp_error *e) {
    const cod1MpStage_t *s=&ir->stage[index];c1wp_stage *d=&p->stage[p->numStages];
    int i,j,rgbSeen=0,blendSeen=0,depthSeen=0,tcSeen[C1WP_BUNDLES]={0};char line[512];
    cod1MpLexer_t l;cod1MpToken_t t,a;c1wp_bundle *b;
    if(s->syntaxUnknown||s->numBundles<1||s->numBundles>C1WP_BUNDLES)return C1WP_Error(e,"unsupported bundle count/syntax",s->line);
    memset(d,0,sizeof(*d));d->sourceStage=index;d->numBundles=s->numBundles;
    d->srcBlend=C1WP_ONE;d->dstBlend=C1WP_ZERO;d->depthWrite=1;
    for(i=0;i<s->numOps;i++) {
        const cod1MpOp_t *o=&s->op[i];if(o->bundle>=C1WP_BUNDLES)return C1WP_Error(e,"too many bundles",s->line);
        b=&d->bundle[o->bundle];
        if(o->nested)return C1WP_Error(e,"unclaimed nested program block",s->line);
        if(!Cod1ME_Line(ir,o,&l,line))return C1WP_Error(e,"oversized operation",s->line);
        t=Cod1MP_Next(&l);a=Cod1MP_Next(&l);
        if(Cod1MP_Equal(t.text,"requires"))continue;
        if(Cod1MP_Equal(t.text,"texEnvCombine")) {
            cod1MpOp_t joined=*o;
            if(*a.text || i+1>=s->numOps || !s->op[i+1].nested ||
               s->op[i+1].bundle!=o->bundle || b->combine.enabled)goto bad;
            joined.end=s->op[++i].end;
            if(!C1WP_Combine(ir,&joined,&b->combine))goto bad;
            p->needsCombine=1;if(b->combine.rgb.op>=C1WP_DOT3_RGB)p->needsDot3=1;continue;
        }
        if(Cod1MP_Equal(t.text,"nextbundle")){if(*a.text)goto bad;continue;}
        if(Cod1MP_Equal(t.text,"map")||Cod1MP_Equal(t.text,"clampmap")) {
            int source;
            if(b->imageKind)goto bad;b->imageKind=C1WP_FILE;
            b->clamp=Cod1MP_Equal(t.text,"clampmap")?C1MAP_CLAMP_ST:C1MAP_REPEAT;
            while(C1Map_Option(a.text,&b->clamp,&b->mapLoadFlags))a=Cod1MP_Next(&l);
            source=C1Map_Source(a.text);
            if(source==C1MAP_WHITE)b->imageKind=C1WP_WHITE;
            else if(source==C1MAP_LIGHTMAP)b->imageKind=C1WP_LIGHTMAP;
            else {if(source!=C1MAP_FILE||!*a.text||*a.text=='/'||strstr(a.text,"..")||strchr(a.text,'\\'))goto bad;
                  Cod1MP_Copy(b->image,sizeof(b->image),a.text);}
            if(!Cod1ME_End(&l))goto bad;
        } else if(Cod1MP_Equal(t.text,"waterMap")) {
            float v[8],length;if(b->imageKind||!C1WP_Float(a.text,&v[0]))goto bad;
            for(j=1;j<8;j++)if(!C1WP_ReadFloat(&l,&v[j]))goto bad;
            if(!Cod1ME_End(&l)||v[0]<4||v[0]>256||v[1]<4||v[1]>256||v[0]!=(int)v[0]||v[1]!=(int)v[1])goto bad;
            b->water.width=(int)v[0];b->water.height=(int)v[1];
            if((b->water.width&(b->water.width-1))||(b->water.height&(b->water.height-1))||b->water.width*b->water.height>32768)goto bad;
            b->water.world_width=v[2];b->water.world_height=v[3];b->water.gravity=800;
            b->water.wind_velocity=v[4];b->water.amplitude=v[7];
            length=(float)sqrt((double)v[5]*v[5]+(double)v[6]*v[6]);
            if(!(v[2]>0&&v[3]>0&&v[4]>0&&v[7]>0&&length>0)||!C1WP_Finite(length))goto bad;
            b->water.wind_x=v[5]/length;b->water.wind_y=v[6]/length;b->imageKind=C1WP_WATERMAP;
        } else if(Cod1MP_Equal(t.text,"cubemap")) {
            if(b->imageKind||!Cod1MP_Equal(a.text,"$renormalize")||!Cod1ME_End(&l))goto bad;
            b->imageKind=C1WP_RENORMALIZE;p->needsCube=1;
        } else if(Cod1MP_Equal(t.text,"tcGen")) {
            tcSeen[o->bundle]=1;
            if(Cod1MP_Equal(a.text,"vector")){b->tcgen=C1WP_VECTOR;
                if(!C1WP_Vector(&l,b->vectors[0],3)||!C1WP_Vector(&l,b->vectors[1],3))goto bad;}
            else if(Cod1MP_Equal(a.text,"sunHalfAngle")){b->tcgen=C1WP_SUN_HALF;p->needsSun=1;}
            else if(Cod1MP_Equal(a.text,"texture"))b->tcgen=C1WP_UV;
            else if(Cod1MP_Equal(a.text,"lightmap"))b->tcgen=C1WP_LIGHTMAP_UV;
            else goto bad;if(!Cod1ME_End(&l))goto bad;
        } else if(Cod1MP_Equal(t.text,"tcMod")) {
            c1wp_mod m;cod1MpToken_t peek;float v[4];int count=0;memset(&m,0,sizeof(m));
            if(Cod1MP_Equal(a.text,"scroll"))m.kind=C1WP_SCROLL;
            else if(Cod1MP_Equal(a.text,"scale"))m.kind=C1WP_SCALE;
            else if(Cod1MP_Equal(a.text,"turb"))m.kind=C1WP_TURB;
            else goto bad;
            /* C89 requires an lvalue for the returned token's array member. */
            for(;;){peek=Cod1MP_Peek(&l);if(!peek.text[0])break;
                if(count==4||!C1WP_ReadFloat(&l,&v[count++]))goto bad;}
            if(m.kind==C1WP_TURB&&count==3){p->ignoredTurb3++;continue;}
            if(m.kind==C1WP_TURB || count!=2 || b->numMods>=C1WP_MODS)goto bad;
            memcpy(m.value,v,count*sizeof(float));b->mod[b->numMods++]=m;
        } else if(Cod1MP_Equal(t.text,"rgbGen")) {
            if(o->bundle||rgbSeen++)goto bad;
            if(Cod1MP_Equal(a.text,"identity"))d->rgb=C1WP_IDENTITY;
            else if(Cod1MP_Equal(a.text,"identityLighting"))d->rgb=C1WP_IDENTITY_LIGHTING;
            else if(Cod1MP_Equal(a.text,"vertex"))d->rgb=C1WP_VERTEX;
            else if(Cod1MP_Equal(a.text,"exactVertex"))d->rgb=C1WP_EXACT_VERTEX;
            else if(Cod1MP_Equal(a.text,"const")||Cod1MP_Equal(a.text,"constLighting")) {
                d->rgb=Cod1MP_Equal(a.text,"const")?C1WP_CONST:C1WP_CONST_LIGHTING;
                if(!C1WP_Vector(&l,d->color,3))goto bad;
                for(j=0;j<3;j++)if(d->color[j]<0||d->color[j]>1)goto bad;
            } else goto bad;if(!Cod1ME_End(&l))goto bad;
        } else if(Cod1MP_Equal(t.text,"alphaGen")) {
            if(o->bundle||d->explicitAlpha++)goto bad;
            if(Cod1MP_Equal(a.text,"dot"))d->alpha=C1WP_ALPHA_DOT;
            else if(Cod1MP_Equal(a.text,"vertex"))d->alpha=C1WP_ALPHA_VERTEX;
            else if(Cod1MP_Equal(a.text,"identity"))d->alpha=C1WP_ALPHA_IDENTITY;
            else goto bad;if(!Cod1ME_End(&l))goto bad;
        } else if(Cod1MP_Equal(t.text,"blendFunc")) {
            if(o->bundle||blendSeen++)goto bad;
            if(Cod1MP_Equal(a.text,"add")){d->srcBlend=C1WP_ONE;d->dstBlend=C1WP_ONE;}
            else if(Cod1MP_Equal(a.text,"blend")){d->srcBlend=C1WP_SRC_ALPHA;d->dstBlend=C1WP_ONE_MINUS_SRC_ALPHA;}
            else if(Cod1MP_Equal(a.text,"filter")){d->srcBlend=C1WP_DST_COLOR;d->dstBlend=C1WP_ZERO;}
            else {d->srcBlend=C1WP_Blend(a.text);a=Cod1MP_Next(&l);d->dstBlend=C1WP_Blend(a.text);}
            if(d->srcBlend<0||d->dstBlend<0||!Cod1ME_End(&l))goto bad;
        } else if(Cod1MP_Equal(t.text,"depthWrite")){if(o->bundle||*a.text||depthSeen++)goto bad;}
        else if(Cod1MP_Equal(t.text,"alphaFunc")) {
            if(o->bundle||d->alphaTest)goto bad;
            if(Cod1MP_Equal(a.text,"GT0"))d->alphaTest=1;
            else if(Cod1MP_Equal(a.text,"LT128"))d->alphaTest=2;
            else if(Cod1MP_Equal(a.text,"GE128"))d->alphaTest=3;
            else goto bad;if(!Cod1ME_End(&l))goto bad;
        } else goto bad;
        continue;
    bad:return C1WP_Error(e,t.text,s->line);
    }
    if(!rgbSeen)return C1WP_Error(e,"implicit rgbGen not reconstructed",s->line);
    for(j=0;j<d->numBundles;j++) {
        b=&d->bundle[j];if(!b->imageKind)return C1WP_Error(e,"bundle without image",s->line);
        if(!tcSeen[j]&&b->imageKind==C1WP_LIGHTMAP)b->tcgen=C1WP_LIGHTMAP_UV;
        if((b->tcgen==C1WP_SUN_HALF)!=(b->imageKind==C1WP_RENORMALIZE)||
           (b->tcgen==C1WP_SUN_HALF&&b->numMods))return C1WP_Error(e,"unsupported coordinate/target combination",s->line);
    }
    if(!d->explicitAlpha&&(d->rgb==C1WP_VERTEX||d->rgb==C1WP_EXACT_VERTEX))d->alpha=C1WP_ALPHA_VERTEX;
    d->depthWrite=depthSeen||!blendSeen||(d->srcBlend==C1WP_ONE&&d->dstBlend==C1WP_ZERO);
    /* Native common finalizer (R48), not material-name-specific. */
    if(!d->alphaTest&&((d->srcBlend==C1WP_SRC_ALPHA&&d->dstBlend==C1WP_ONE_MINUS_SRC_ALPHA)||
       (d->srcBlend==C1WP_SRC_ALPHA&&d->dstBlend==C1WP_ONE)))d->alphaTest=1;
    p->numStages++;return 1;
}
static int C1WP_Candidate(const cod1MaterialProbe_t *ir) {
    int i;cod1MpLexer_t l;char line[512];cod1MpToken_t t,a;
    for(i=0;i<ir->topOps;i++)if(Cod1ME_Line(ir,&ir->topOp[i],&l,line)) {
        t=Cod1MP_Next(&l);a=Cod1MP_Next(&l);
        if((Cod1MP_Equal(t.text,"surfaceparm")&&Cod1MP_Equal(a.text,"water"))||
           (Cod1MP_Equal(t.text,"sort")&&Cod1ME_OneOf(a.text,"water ocean")))return 1;
    }return 0;
}
/* Simple water/lightmap shaders already execute through the existing generic
 * parser. Do not take ownership of them just because they contain WATER.
 * A requirement, procedural/combiner operation or deformation selects this
 * executor, independent of material/map names and number of source stages. */
static int C1WP_NeedsRuntime(const cod1MaterialProbe_t *ir) {
    int i,j;cod1MpLexer_t l;cod1MpToken_t t;
    for(i=0;i<ir->topOps;i++) {
        memset(&l,0,sizeof(l));l.base=ir->source;l.cursor=ir->source+ir->topOp[i].start;
        t=Cod1MP_Next(&l);if(Cod1MP_Equal(t.text,"deformVertexes"))return 1;
    }
    for(i=0;i<ir->numStages;i++) {
        if(ir->stage[i].numRequires)return 1;
        for(j=0;j<ir->stage[i].numOps;j++) {
            memset(&l,0,sizeof(l));l.base=ir->source;l.cursor=ir->source+ir->stage[i].op[j].start;
            t=Cod1MP_Next(&l);
            if(Cod1ME_OneOf(t.text,"waterMap texEnvCombine cubemap"))return 1;
        }
    }
    return 0;
}
static int C1WP_Plan(const cod1MaterialProbe_t *ir,c1wp_plan *p,c1wp_error *e) {
    int i,j,hasLightmap=0;memset(p,0,sizeof(*p));memset(e,0,sizeof(*e));e->candidate=C1WP_Candidate(ir);
    if(!e->candidate)return C1WP_Error(e,"not a water program",0);
    if(ir->error[0]||ir->unknownMask)return C1WP_Error(e,ir->error[0]?ir->error:"unknown requires operand",0);
    if(!C1WP_Top(ir,p,e))return 0;
    p->sourceHash=ir->sourceHash;p->sourceStages=ir->numStages;p->selectedMask=ir->selectedMask;
    for(i=0;i<ir->numStages;i++) {
        if(ir->stage[i].selected==COD1_MP_FALSE){p->excludedMask|=1u<<i;continue;}
        if(p->numStages==C1WP_STAGES)return C1WP_Error(e,"selected stage capacity",ir->stage[i].line);
        if(!C1WP_Stage(ir,i,p,e))return 0;
    }
    for(i=0;i<p->numStages;i++)for(j=0;j<p->stage[i].numBundles;j++)
        if(p->stage[i].bundle[j].imageKind==C1WP_LIGHTMAP)hasLightmap=1;
    /* R60 supports the authored atlas-backed branch, without manufacturing
     * nolightmap. A source lacking both remains outside this bounded subset. */
    if(!(p->waterFlags&8) && !hasLightmap)
        return C1WP_Error(e,"lightmapped water lacks a selected $lightmap bundle",0);
    return p->numStages?1:C1WP_Error(e,"no selected stages",0);
}
#endif
