/* R47: source-driven shader type preparation, no material-family table.
 * Native routing: CoDSP af73842d..., 0x4f1310 and 0x4f1170.
 * This is a deliberately bounded fixed-function subset, NOT the full CoD
 * shader language. Unknown selected operations fail before image IO.
 */
#ifndef COD1_MATERIAL_STYPE_H
#define COD1_MATERIAL_STYPE_H
#include "cod1_material_probe.h"
#include "cod1_material_eligibility.h"
#include "cod1_stype_native_rules.h"
#include "cod1_material_map.h"
#include <float.h>

#define COD1_ST_PATH 128
#define COD1_ST_BODY (128u*1024u)
#define COD1_ST_AUTH_THIRD 1u
#define COD1_ST_AUTH_ENV_ADD 2u
#if defined(COD1_STAGE_BACKEND_INFRA) && COD1_STAGE_BACKEND_INFRA
#define COD1_ST_STORAGE_BUNDLES 3
#else
#define COD1_ST_STORAGE_BUNDLES 2
#endif
typedef enum { COD1_ST_MODEL, COD1_ST_WORLD, COD1_ST_2D } cod1StUsage_t;
typedef struct {
    char path[COD1_ST_PATH];
    unsigned hash, selected, excluded;
    unsigned nativeSurfaceFlags;
    unsigned bundleAuth[8];
    int ignoredSurfaceParms, ignoredRuntimeHints;
    int stages, line;
    char reason[160];
} cod1StResult_t;

/* Return 0: ordinary name, 1: type request, -1: invalid request. The caller
 * supplies material usage explicitly; a static model's baked-vertex input
 * does not change its native model/ shader-type namespace to world/. */
static int Cod1ST_Path(const char *name, cod1StUsage_t usage, char *out, size_t cap) {
    const char *at, *p, *base, *scope;
    size_t n, prefix;
    if (!name || !out || !cap) return -1;
    out[0]=0; at=strchr(name,'@');
    if (!at) return 0;
    if (!*name || strlen(name)>=64 || name[0]=='/' || name[0]=='\\') return -1;
    base=name;
    for (p=name;*p;++p) {
        unsigned char c=(unsigned char)*p;
        if (c<33 || c>126 || c==':' || c=='"' || c=='$' ||
            c=='{' || c=='}' || c==';') return -1;
        if (*p=='/' || *p=='\\') {
            if (p>at || p==base || (p-base==1 && *base=='.') ||
                (p-base==2 && base[0]=='.' && base[1]=='.')) return -1;
            base=p+1;
        }
    }
    if (at<=base || !at[1] || strchr(at+1,'@')) return -1;
    n=(size_t)(at-base);
    for (p=base;p<at;++p)
        if (!isalnum((unsigned char)*p) && *p!='_' && *p!='-') return -1;
    scope=usage==COD1_ST_MODEL?"model/":usage==COD1_ST_WORLD?"world/":"2d/";
    /* Native @<namespace>/<type> cache key must fit 64 bytes. */
    if (1+strlen(scope)+n>=64) return -1;
    prefix=strlen("shadertypes/")+strlen(scope);
    if (prefix+n+strlen(".stype")+1>cap) return -1;
    strcpy(out,"shadertypes/");strcat(out,scope);
    memcpy(out+prefix,base,n);out[prefix+n]=0;strcat(out,".stype");
    return 1;
}
static int Cod1ST_Append(char *out,size_t cap,size_t *used,const char *text,size_t n) {
    if (*used>=cap || n>=cap-*used) return 0;
    memcpy(out+*used,text,n);*used+=n;out[*used]=0;return 1;
}
static int Cod1ST_Why(cod1StResult_t *r,int line,const char *why) {
    r->line=line;Cod1MP_Copy(r->reason,sizeof(r->reason),why);return 0;
}
static int Cod1ST_Top(const cod1MaterialProbe_t *ir,const cod1MpOp_t *op,
                       char reason[80]) {
    cod1MpLexer_t lx;cod1MpToken_t cmd,arg;char line[512];
    if (!Cod1ME_Line(ir,op,&lx,line)) { strcpy(reason,"invalid top operation");return 0; }
    cmd=Cod1MP_Next(&lx);
    if (Cod1MP_Equal(cmd.text,"cull")) {
        arg=Cod1MP_Next(&lx);
        if (!Cod1ME_OneOf(arg.text,"none twosided disable back backsided front") || !Cod1ME_End(&lx))
            strcpy(reason,"cull arguments unsupported");
    } else if(Cod1MP_Equal(cmd.text,"surfaceparm")) {
        arg=Cod1MP_Next(&lx);
        if(!arg.text[0] || !Cod1ME_End(&lx))strcpy(reason,"surfaceparm arguments unsupported");
    } else if(Cod1MP_Equal(cmd.text,"radialNormals")) {
        /* 0x4efd7b..0x4efd97 calls SkipRestOfLine; runtime no-op. */
    } else if(Cod1MP_Equal(cmd.text,"deformVertexes")) {
        /* R59: native parser enum 10 at 0x4ef086, indexed runtime at
         * 0x5098a0. Other deforms retain the strict unsupported boundary. */
        arg=Cod1MP_Next(&lx);
        if(!Cod1MP_Equal(arg.text,"autosprite2") || !Cod1ME_End(&lx))
            strcpy(reason,"deformVertexes arguments unsupported");
    } else if(Cod1MP_Equal(cmd.text,"nofog")) {
        if(!Cod1ME_End(&lx))strcpy(reason,"nofog arguments unsupported");
    } else if(Cod1MP_Equal(cmd.text,"sort")) {
        double value;arg=Cod1MP_Next(&lx);
        if(!arg.text[0] || (!Cod1ME_OneOf(arg.text,
            "portal sky ocean boathull opaque decal seeThrough banner underwater water corona innerBlend outerBlend blend blend2 blend3 blend4 additive nearest") && !Cod1MP_Number(arg.text,&value)) || !Cod1ME_End(&lx))
            strcpy(reason,"sort arguments unsupported");
    } else Cod1ME_TopOp(ir,op,reason);
    return !reason[0];
}
/* This expands the .stype contract only; historical water eligibility stays
 * unchanged. Unlike a loose legacy parser, no selected unknown op is dropped. */
/* Native requirement analyzer 0x4e8100 grants bundle/ADD permission only
 * for a standalone positive atom. OR terms select a stage but authorize
 * nothing. Preserve this proof when projecting away the requires lines. */
static int Cod1ST_AuthNumber(const char *token,double *value);
static int Cod1ST_RequiresPlaced(const cod1MaterialProbe_t *ir,
                                 const cod1MpStage_t *stage,char reason[80]) {
    int j,loaded=0;
    for(j=0;j<stage->numOps;++j) {
        cod1MpLexer_t lx;cod1MpToken_t cmd;char line[512];
        if(!Cod1ME_Line(ir,&stage->op[j],&lx,line))continue;
        cmd=Cod1MP_Next(&lx);
        if(Cod1MP_Equal(cmd.text,"requires") && (loaded || stage->op[j].bundle)) {
            strcpy(reason,"requires after image/nextbundle unsupported");return 0;
        }
        if(Cod1MP_Equal(cmd.text,"requires")) {
            cod1MpToken_t t,op,n;double value;
            while((t=Cod1MP_Next(&lx)).text[0])if(Cod1MP_Equal(t.text,"GL_MAX_TEXTURE_UNITS_ARB")) {
                op=Cod1MP_Next(&lx);n=Cod1MP_Next(&lx);(void)op;
                if(!Cod1ST_AuthNumber(n.text,&value)) {
                    strcpy(reason,"requires numeric operand recovery unsupported");return 0;
                }
            }
        }
        if(Cod1ME_OneOf(cmd.text,"map clampmap animMap nextbundle"))loaded=1;
    }
    return 1;
}
static int Cod1ST_AuthNumber(const char *token,double *value) {
    const char *p=token;
    /* Native type-2 classifier 0x4e8027..0x4e805e: decimal digits/dot,
     * optionally signed. Hex and exponent strings grant no unit proof. */
    if(*p=='+' || *p=='-')++p;
    if(!*p)return 0;
    for(;*p;++p)if((*p<'0' || *p>'9') && *p!='.')return 0;
    return Cod1MP_Number(token,value);
}
static unsigned Cod1ST_BundleAuth(const cod1MaterialProbe_t *ir,
                                  const cod1MpStage_t *stage) {
    int j;unsigned auth=0;char reason[80];
    if(!Cod1ST_RequiresPlaced(ir,stage,reason))return 0;
    for(j=0;j<stage->numOps;++j) {
        cod1MpLexer_t lx;cod1MpToken_t cmd,arg,op,n;char line[512];double v;
        if(!Cod1ME_Line(ir,&stage->op[j],&lx,line))continue;
        cmd=Cod1MP_Next(&lx);if(!Cod1MP_Equal(cmd.text,"requires"))continue;
        arg=Cod1MP_Next(&lx);
        if(Cod1MP_Equal(arg.text,"GL_ARB_texture_env_add") && Cod1ME_End(&lx))
            auth|=COD1_ST_AUTH_ENV_ADD;
        else if(Cod1MP_Equal(arg.text,"GL_MAX_TEXTURE_UNITS_ARB")) {
            op=Cod1MP_Next(&lx);n=Cod1MP_Next(&lx);
            if(!Cod1ST_AuthNumber(n.text,&v) || !Cod1ME_End(&lx) ||
               !(v>=0 && v<=8) || v!=(int)v)continue;
            if((!strcmp(op.text,">") && v>=2 && v<=7) ||
               ((!strcmp(op.text,">=") || !strcmp(op.text,"==")) && v>=3))
                auth|=COD1_ST_AUTH_THIRD;
        }
    }
    return auth;
}
static int Cod1ST_CheckStageAuth(const cod1MaterialProbe_t *ir,const cod1MpStage_t *stage,
                            int units,unsigned auth,int strictAuth,char reason[80]) {
    int j,maps[3]={0,0,0},mods[3]={0,0,0};
    if(stage->numBundles<1 || stage->numBundles>units || stage->numBundles>COD1_ST_STORAGE_BUNDLES) {
        strcpy(reason,"too many bundles for backend");return 0;
    }
    if(stage->numBundles>2 && !(auth&COD1_ST_AUTH_THIRD)) {
        strcpy(reason,"third bundle lacks standalone units authorization");return 0;
    }
    for(j=0;j<stage->numOps;++j) {
        const cod1MpOp_t *op=&stage->op[j];cod1MpLexer_t lx;cod1MpToken_t cmd,arg,next;
        char line[512];int b=op->bundle;
        if(b<0 || b>=stage->numBundles || b>=COD1_ST_STORAGE_BUNDLES) {
            strcpy(reason,"invalid operation bundle");return 0;
        }
        if(!Cod1ME_Line(ir,op,&lx,line)) {strcpy(reason,"nested/long stage operation");return 0;}
        cmd=Cod1MP_Next(&lx);
        if(strictAuth || stage->numBundles>2) {
            if(Cod1ME_OneOf(cmd.text,"tcGen texGen") && !maps[b]) {
                strcpy(reason,"coordinate generation before image unsupported");return 0;
            }
            if(Cod1ME_OneOf(cmd.text,"tcGen texGen tcMod rgbGen alphaGen")) {
                cod1MpLexer_t numbers=lx;cod1MpToken_t value;double v;
                while((value=Cod1MP_Next(&numbers)).text[0])
                    if(Cod1MP_Number(value.text,&v) && !(v>=-FLT_MAX && v<=FLT_MAX)) {
                        strcpy(reason,"nonfinite/out-of-range stage operand unsupported");return 0;
                    }
            }
        }
        if(Cod1ME_OneOf(cmd.text,"map clampmap")) {
            int axes=Cod1MP_Equal(cmd.text,"clampmap")?C1MAP_CLAMP_ST:C1MAP_REPEAT;
            unsigned loadFlags=0;
            arg=Cod1MP_Next(&lx);
            if(Cod1MP_Equal(cmd.text,"map"))
                while(C1Map_Option(arg.text,&axes,&loadFlags))arg=Cod1MP_Next(&lx);
            if(!arg.text[0] || !Cod1ME_End(&lx)) {strcpy(reason,"map arguments unsupported");return 0;}
            if(b<0 || b>=COD1_ST_STORAGE_BUNDLES || ++maps[b]>1) {strcpy(reason,"multiple/invalid image bundle");return 0;}
            if(C1Map_Source(arg.text)!=C1MAP_FILE && C1Map_Source(arg.text)!=C1MAP_LIGHTMAP && C1Map_Source(arg.text)!=C1MAP_WHITE) {
                strcpy(reason,"unknown image symbol");return 0;
            }
            if(b && !Cod1MP_Equal(cmd.text,"map")) {
                strcpy(reason,"ordinary nextbundle image unsupported");return 0;
            }
        } else {
            cod1MpOp_t primary=*op;
            arg=Cod1MP_Next(&lx);
            /* Native incomplete complex blend commits neither factor nor
             * depth (0x4edda4..0x4eddbe). This is a known recovery, not an
             * invented alphaFunc or guessed GL_ONE/GL_ZERO replacement. */
            if(!b && Cod1MP_Equal(cmd.text,"blendFunc") &&
               !Cod1ME_OneOf(arg.text,"add filter blend")) {
                next=Cod1MP_Next(&lx);
                if(!next.text[0] && !lx.failed)continue;
            }
            if(Cod1MP_Equal(cmd.text,"tcMod") && Cod1MP_Equal(arg.text,"turb")) {
                double v;int n=0;
                for(;;) {
                    next=Cod1MP_Next(&lx);if(!next.text[0])break;
                    if(++n>4 || !Cod1MP_Number(next.text,&v))break;
                }
                if(!next.text[0] && !lx.failed && (n==3 || n==4)) {
                    if(n==4 && ++mods[b]>4) {strcpy(reason,"too many bundle texture modifiers");return 0;}
                    continue;
                }
                strcpy(reason,"tcMod turb arguments unsupported");return 0;
            }
            if(b && Cod1MP_Equal(cmd.text,"blendFunc")) {
                if(!Cod1ME_OneOf(arg.text,"filter add blend") || !Cod1ME_End(&lx)) {
                    strcpy(reason,"bundle blendFunc unsupported");return 0;
                }
                if(Cod1MP_Equal(arg.text,"add") && (strictAuth || stage->numBundles>2) && !(auth&COD1_ST_AUTH_ENV_ADD)) {
                    strcpy(reason,"bundle ADD lacks standalone extension authorization");return 0;
                }
                continue;
            }
            if(b && !Cod1ME_OneOf(cmd.text,"nextbundle tcGen tcMod")) {
                strcpy(reason,"bundle-local command unsupported");return 0;
            }
            /* Shared legacy eligibility intentionally retains its old
             * boundary. These independently indexed native bundle commands
             * use the same bounded argument grammar as the first bundle. */
            primary.bundle=0;
            Cod1ME_StageOp(ir,stage,&primary,reason);
            if(reason[0])return 0;
            if(Cod1MP_Equal(cmd.text,"tcMod") && ++mods[b]>4) {
                strcpy(reason,"too many bundle texture modifiers");return 0;
            }
        }
    }
    for(j=0;j<stage->numBundles;++j)if(maps[j]!=1) {
        strcpy(reason,"incomplete image bundle");return 0;
    }
    return 1;
}
static int Cod1ST_CheckStage(const cod1MaterialProbe_t *ir,const cod1MpStage_t *stage,
                            int units,char reason[80]) {
    return Cod1ST_CheckStageAuth(ir,stage,units,Cod1ST_BundleAuth(ir,stage),0,reason);
}
/* Prepare only source-authored selected passes. $texturename refers to the
 * WHOLE material name, including type@, not just the suffix after '@'.
 * Native suffix/image transformations ($texturename : ...) are not yet
 * implemented; they are rejected, never quietly stripped. */
static int Cod1ST_PrepareAuth(const char *body,const char *name,const cod1MpCaps_t *caps,
                         char *out,size_t cap,int strictAuth,cod1StResult_t *result) {
    cod1MaterialProbe_t *ir,*checked;
    int units;
    cod1MpLexer_t lx;cod1MpToken_t cmd,arg;
    char line[512],reason[80];
    size_t used=0;int i,j,count=0,deforms=0,ok=0;
    ir=(cod1MaterialProbe_t*)calloc(1,sizeof(*ir));
    checked=(cod1MaterialProbe_t*)calloc(1,sizeof(*checked));
    if (!ir || !checked) { Cod1ST_Why(result,0,"allocation failed");goto done; }
    if (!body || !name || !out || !cap || strlen(body)>=COD1_ST_BODY) {
        Cod1ST_Why(result,0,"source too large/invalid");goto done;
    }
    out[0]=0;
    result->nativeSurfaceFlags=0;result->ignoredSurfaceParms=0;result->ignoredRuntimeHints=0;
    memset(result->bundleAuth,0,sizeof(result->bundleAuth));
    if (!Cod1MP_Parse(body,caps,ir)) { Cod1ST_Why(result,0,ir->error);goto done; }
    result->hash=ir->sourceHash;result->selected=ir->selectedMask;
    result->excluded=0;
    /* R58: the native .stype loader (0x4f128a) calls the single-material
     * parser. At its outer closing brace, 0x4efc75 branches to 0x4f02c1;
     * success returns without reading the remaining file text. Only the
     * first balanced body is selected. Selected unknown operations inside
     * that body still fail below; script-definition indexing is unchanged. */
    if (!Cod1ST_Append(out,cap,&used,"{\n",2)) goto full;
    for (j=0;j<ir->topOps;++j) {
        reason[0]=0;
        if (!Cod1ST_Top(ir,&ir->topOp[j],reason)) { Cod1ST_Why(result,0,reason);goto done; }
        Cod1ME_Line(ir,&ir->topOp[j],&lx,line);cmd=Cod1MP_Next(&lx);
        if(Cod1MP_Equal(cmd.text,"deformVertexes") && ++deforms>3) {
            Cod1ST_Why(result,0,"too many selected deforms");goto done;
        }
        if(Cod1MP_Equal(cmd.text,"surfaceparm")) {
            int known;arg=Cod1MP_Next(&lx);
            result->nativeSurfaceFlags|=Cod1ST_SurfaceFlag(arg.text,&known);
            if(!known)++result->ignoredSurfaceParms;
            continue;
        }
        if(Cod1MP_Equal(cmd.text,"radialNormals")) {++result->ignoredRuntimeHints;continue;}
        if (!Cod1ST_Append(out,cap,&used,body+ir->topOp[j].start,
                          ir->topOp[j].end-ir->topOp[j].start) ||
            !Cod1ST_Append(out,cap,&used,"\n",1)) goto full;
    }
    for (i=0;i<ir->numStages;++i) {
        const cod1MpStage_t *s=&ir->stage[i];
        if(strictAuth) {
            reason[0]=0;
            if(!Cod1ST_RequiresPlaced(ir,s,reason)) {Cod1ST_Why(result,s->line,reason);goto done;}
        }
        if (s->selected==COD1_MP_FALSE) { result->excluded|=1u<<i;continue; }
        if (s->selected!=COD1_MP_TRUE || s->syntaxUnknown) {
            Cod1ST_Why(result,s->line,"requires unknown/syntax unsupported");goto done;
        }
        reason[0]=0;
        if(!Cod1ST_RequiresPlaced(ir,s,reason)) {Cod1ST_Why(result,s->line,reason);goto done;}
        if (++count>8) { Cod1ST_Why(result,s->line,"too many selected stages");goto done; }
        result->bundleAuth[count-1]=Cod1ST_BundleAuth(ir,s);
        if (!Cod1ST_Append(out,cap,&used,"{\n",2)) goto full;
        for (j=0;j<s->numOps;++j) {
            const cod1MpOp_t *op=&s->op[j];
            if (!Cod1ME_Line(ir,op,&lx,line)) { Cod1ST_Why(result,s->line,"nested/long stage operation");goto done; }
            cmd=Cod1MP_Next(&lx);
            if (Cod1MP_Equal(cmd.text,"requires")) continue;
            if (Cod1ME_OneOf(cmd.text,"map clampmap")) {
                int axes=C1MAP_REPEAT;unsigned loadFlags=0;
                arg=Cod1MP_Next(&lx);
                if(Cod1MP_Equal(cmd.text,"map"))
                    while(C1Map_Option(arg.text,&axes,&loadFlags))arg=Cod1MP_Next(&lx);
                if (Cod1MP_Equal(arg.text,"$texturename")) {
                    if (!Cod1ME_End(&lx)) { Cod1ST_Why(result,s->line,"$texturename suffix/transform unsupported");goto done; }
                    if (!Cod1ST_Append(out,cap,&used,line,(size_t)(arg.start-line)) ||
                        !Cod1ST_Append(out,cap,&used,name,strlen(name)) ||
                        !Cod1ST_Append(out,cap,&used,"\n",1)) goto full;
                    continue;
                }
            }
            if (!Cod1ST_Append(out,cap,&used,line,strlen(line)) ||
                !Cod1ST_Append(out,cap,&used,"\n",1)) goto full;
        }
        if (!Cod1ST_Append(out,cap,&used,"}\n",2)) goto full;
    }
    if (!count) { Cod1ST_Why(result,0,"no selected stages");goto done; }
    if (!Cod1ST_Append(out,cap,&used,"}\n",2)) goto full;
    if (!Cod1MP_Parse(out,caps,checked)) { Cod1ST_Why(result,0,checked->error);goto done; }
    units=caps?caps->units:1;
    for(i=0;i<checked->numStages;++i) {
        reason[0]=0;
        if(!Cod1ST_CheckStageAuth(checked,&checked->stage[i],units,result->bundleAuth[i],strictAuth,reason)) {
            Cod1ST_Why(result,checked->stage[i].line,reason);goto done;
        }
    }
    result->stages=count;result->reason[0]=0;result->line=0;ok=1;goto done;
full: Cod1ST_Why(result,0,"expanded source exceeds limit");
done: free(ir);free(checked);return ok;
}
static int Cod1ST_Prepare(const char *body,const char *name,const cod1MpCaps_t *caps,
                         char *out,size_t cap,cod1StResult_t *result) {
    return Cod1ST_PrepareAuth(body,name,caps,out,cap,0,result);
}
#endif
