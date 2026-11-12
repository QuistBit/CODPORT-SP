/* SPDX-License-Identifier: GPL-3.0-or-later
 * NATIVE_EFX_R1: bounded, engine-independent reader for the inspected CoD1 EFX
 * subset. This is new code, not recovered proprietary engine source.
 * Unrecognised execution-affecting input fails the whole definition; it is
 * never silently discarded. No original assets are embedded here.
 */
#ifndef CG_COD1_EFX_PARSE_H
#define CG_COD1_EFX_PARSE_H
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#define CEFX_PATH 96
#define CEFX_PRIMS 48
#define CEFX_MEDIA 8
#define CEFX_FILE_LIMIT 131072
#define CEFX_TOKEN 192
#define CEFX_VALUES 24

typedef struct { float a, b; } ceRange;
typedef struct {
    ceRange start[3], end[3];
    float parm[2]; int parmCount;
    int linear, random, clamp, nonlinear;
} ceCurve;
enum { CE_PARTICLE=1, CE_LIGHT, CE_RUNNER, CE_EMITTER, CE_DECAL, CE_TAIL, CE_LINE, CE_SOUND, CE_ORIENTED };
enum { CE_ALPHA=1, CE_PHYSICS=2, CE_IMPACT_KILLS=4, CE_EMIT=8, CE_RELATIVE=16,CE_DEPTHHACK=32, CE_MODEL=64, CE_IMPACTFX=128, CE_SHADER_TIME=256, CE_DEATHFX=512 };
enum { CE_CHEAP=1, CE_ABS_VEL=2, CE_ABS_ACCEL=4, CE_SPHERE=8,
    CE_CYLINDER=16, CE_AXIS=32, CE_EVEN=64, CE_RGB_LINK=128, CE_RAND_FWD=256 };
typedef struct {
    int type, flags, spawn, nonUniform;
    ceRange count, life, delay, cull, radius, height, rotation, rotationDelta;
    ceRange origin[3], origin2[3], velocity[3], acceleration[3], gravity, density, variance;
    /* BRASS candidate: accepted only by mode 3, never by prior FX modes. */
    ceRange angle[3], angleDelta[3], bounce;
    ceCurve rgb, alpha, size, size2, length;
    char shaders[CEFX_MEDIA][CEFX_PATH]; int shaderCount;
    char play[CEFX_MEDIA][CEFX_PATH]; int playCount;
    char emit[CEFX_MEDIA][CEFX_PATH]; int emitCount;
    char models[CEFX_MEDIA][CEFX_PATH]; int modelCount;
    char sounds[CEFX_MEDIA][CEFX_PATH]; int soundCount;
    char impact[CEFX_MEDIA][CEFX_PATH]; int impactCount;
    char death[CEFX_MEDIA][CEFX_PATH]; int deathCount;
} cePrimitive;
typedef struct { int count; cePrimitive prim[CEFX_PRIMS]; } ceDefinition;
typedef struct {
    const char *data; size_t size, pos; int line, tokenLine, kind, muzzle;
    char token[CEFX_TOKEN], error[256];
} ceLexer;
enum { CE_EOF, CE_WORD, CE_NL, CE_OPEN, CE_CLOSE, CE_LIST, CE_ENDLIST };

static int ceEqual(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a++) != tolower((unsigned char)*b++)) return 0;
    }
    return !*a && !*b;
}
static ceRange ceFixed(float x) { ceRange r; r.a=r.b=x; return r; }
static int ceError(ceLexer *p, const char *why) {
    if (!p->error[0]) snprintf(p->error,sizeof(p->error),"line %d: %s [%.100s]",
        p->tokenLine,why,p->token);
    return 0;
}
/* Virtual paths only. Parent traversal, drive names, double separators and
 * alternate extensions cannot accidentally reach unrelated local files. */
static int cePath(const char *in, char out[CEFX_PATH], int effect) {
    size_t i,n; char tmp[CEFX_PATH];
    if (!in || !(n=strlen(in)) || n>=sizeof(tmp)-5 || in[0]=='/' || in[0]=='\\') return 0;
    for(i=0;i<n;++i) {
        unsigned char c=(unsigned char)in[i];
        if (!(isalnum(c) || c=='/' || c=='\\' || c=='_' || c=='-' || c=='.')) return 0;
        tmp[i]=(char)(c=='\\' ? '/' : tolower(c));
    }
    tmp[n]=0;
    if (strstr(tmp,"..") || strstr(tmp,"//") || strstr(tmp,"/./") || tmp[n-1]=='/') return 0;
    if (effect == 1) {
        if (strncmp(tmp,"fx/",3)) {
            if(n+3>=sizeof(tmp)-5) return 0;
            memmove(tmp+3,tmp,n+1); memcpy(tmp,"fx/",3); n+=3;
        }
        if(n<4 || strcmp(tmp+n-4,".efx")) {
            if(strchr(strrchr(tmp,'/')+1,'.')) return 0;
            memcpy(tmp+n,".efx",5);
        }
    } else if (effect == 2) {
        if (strncmp(tmp,"xmodel/",7) || !tmp[7]) return 0;
    } else if (effect == 3) {
        /* An alias identifier, NOT a filename to pass to the filesystem. */
        for(i=0;tmp[i];i++) if(!(isalnum((unsigned char)tmp[i]) || tmp[i]=='_' || tmp[i]=='-')) return 0;
    } else if (strncmp(tmp,"gfx/",4)) return 0;
    strcpy(out,tmp); return 1;
}
static int ceNext(ceLexer *p) {
    size_t k=0; char c;
    p->token[0]=0;
    for (;;) {
        if(p->pos>=p->size) { p->kind=CE_EOF; return 1; }
        c=p->data[p->pos];
        if(c==' '||c=='\t'||c=='\r') { ++p->pos; continue; }
        if(c=='\n') { ++p->pos; p->tokenLine=p->line++; p->kind=CE_NL; return 1; }
        if(c=='#' || (c=='/' && p->pos+1<p->size && p->data[p->pos+1]=='/')) {
            while(p->pos<p->size && p->data[p->pos]!='\n') ++p->pos;
            continue;
        }
        if(c=='/' && p->pos+1<p->size && p->data[p->pos+1]=='*') {
            int newline=0,closed=0; p->pos+=2;
            while(p->pos<p->size) {
                if(p->data[p->pos]=='\n') { ++p->line; newline=1; }
                if(p->data[p->pos]=='*' && p->pos+1<p->size && p->data[p->pos+1]=='/') {
                    p->pos+=2; closed=1; break;
                }
                ++p->pos;
            }
            if(!closed) return ceError(p,"unterminated comment");
            if(newline) { p->tokenLine=p->line; p->kind=CE_NL; return 1; }
            continue;
        }
        break;
    }
    p->tokenLine=p->line;
    c=p->data[p->pos++];
    if(c=='{'||c=='}'||c=='['||c==']') {
        p->token[0]=c;p->token[1]=0;
        p->kind=c=='{'?CE_OPEN:c=='}'?CE_CLOSE:c=='['?CE_LIST:CE_ENDLIST; return 1;
    }
    p->kind=CE_WORD;
    if(c=='"') {
        int closed=0;
        while(p->pos<p->size) {
            c=p->data[p->pos++];
            if(c=='"') { closed=1;break; }
            if(c=='\n'||c=='\r') return ceError(p,"newline in quoted value");
            if(k+1>=sizeof(p->token)) return ceError(p,"token too long");
            p->token[k++]=c;
        }
        if(!closed) return ceError(p,"unterminated quote");
    } else {
        p->token[k++]=c;
        while(p->pos<p->size) {
            c=p->data[p->pos];
            if(isspace((unsigned char)c)||c=='{'||c=='}'||c=='['||c==']'||c=='#') break;
            if(c=='/' && p->pos+1<p->size && (p->data[p->pos+1]=='/'||p->data[p->pos+1]=='*')) break;
            if(k+1>=sizeof(p->token)) return ceError(p,"token too long");
            p->token[k++]=c; ++p->pos;
        }
    }
    p->token[k]=0; return 1;
}
static int ceSkipLines(ceLexer *p) {
    while(p->kind==CE_NL) if(!ceNext(p)) return 0;
    return 1;
}
static void ceCurveInit(ceCurve *c, float x) {
    int i; memset(c,0,sizeof(*c));
    for(i=0;i<3;++i) c->start[i]=c->end[i]=ceFixed(x);
}
static void ceDefaults(cePrimitive *e,int type) {
    memset(e,0,sizeof(*e)); e->type=type;
    e->count=ceFixed(1);e->life=ceFixed(50);
    e->radius=e->height=ceFixed(10); e->density=ceFixed(10);e->variance=ceFixed(1);
    ceCurveInit(&e->rgb,1);ceCurveInit(&e->alpha,1);ceCurveInit(&e->size,1);
    ceCurveInit(&e->size2,1);ceCurveInit(&e->length,1);
}
static int ceValues(ceLexer *p,char vals[CEFX_VALUES][CEFX_TOKEN],int *count) {
    *count=0;
    while(p->kind==CE_WORD) {
        if(*count>=CEFX_VALUES) return ceError(p,"too many values");
        strcpy(vals[(*count)++],p->token);
        if(!ceNext(p)) return 0;
    }
    return 1;
}
static int ceNumbers(ceLexer *p,char vals[CEFX_VALUES][CEFX_TOKEN],int n,float *f) {
    int i; char *end; double x;
    for(i=0;i<n;++i) {
        errno=0; x=strtod(vals[i],&end);
        if(end==vals[i]||*end||errno==ERANGE||!(x>=-1000000 && x<=1000000))
            return ceError(p,"invalid or out-of-bounds number");
        f[i]=(float)x;
    }
    return 1;
}
static int ceRanges(ceLexer *p,ceRange *dst,int components,
    char vals[CEFX_VALUES][CEFX_TOKEN],int n) {
    float f[CEFX_VALUES];int i;
    if(n!=components && n!=components*2) return ceError(p,"wrong number of range endpoints");
    if(!ceNumbers(p,vals,n,f)) return 0;
    for(i=0;i<components;++i) { dst[i].a=f[i];dst[i].b=f[n==components?i:components+i]; }
    return 1;
}
static int ceFlags(ceLexer *p,char v[CEFX_VALUES][CEFX_TOKEN],int n,int spawn,int *out) {
    static const char *flags[]={"useAlpha","usePhysics","impactKills","emitFx","relative","depthHack","useModel","impactFx","setShaderTime","deathFx"};
    static const char *spawns[]={"cheapOrgCalc","absoluteVel","absoluteAccel","orgOnSphere",
        "orgOnCylinder","axisFromSphere","evenDistribution","rgbComponentInterpolation","randrotaroundfwd"};
    int extended=p->muzzle==4 || p->muzzle==6;
    int i,j,limit=spawn?(extended?9:8):(extended?10:(p->muzzle==3?8:(p->muzzle?6:4))); const char **names=spawn?spawns:flags;
    for(i=0;i<n;++i) {
        for(j=0;j<limit;++j) if(ceEqual(v[i],names[j])) break;
        if(j==limit) { strncpy(p->token,v[i],sizeof(p->token)-1);return ceError(p,"unsupported flag"); }
        *out|=1<<j;
    }
    return 1;
}
static int ceParseCurve(ceLexer *p,ceCurve *c,int components,int allowNonlinear) {
    char key[CEFX_TOKEN],v[CEFX_VALUES][CEFX_TOKEN];int n,keyLine,valueLine;
    if(!ceSkipLines(p)||p->kind!=CE_OPEN) return ceError(p,"expected curve '{'");
    if(!ceNext(p)) return 0;
    for(;;) {
        if(!ceSkipLines(p)) return 0;
        if(p->kind==CE_CLOSE) {
            if(c->nonlinear&&(c->linear||c->random||c->clamp||c->parmCount))
                return ceError(p,"unsupported mixed/parameterized nonlinear size curve");
            return ceNext(p);
        }
        if(p->kind!=CE_WORD) return ceError(p,"expected curve field or '}'");
        strcpy(key,p->token);keyLine=p->tokenLine;
        if(!ceNext(p)) return 0;
        valueLine=p->tokenLine;
        if(!ceValues(p,v,&n)) return 0;
        if(ceEqual(key,"start")||ceEqual(key,"end")) {
            if(!ceRanges(p,ceEqual(key,"start")?c->start:c->end,components,v,n)) return 0;
        } else if(ceEqual(key,"parm")) {
            float f[2];
            /* CoD EFX stores one/two curve parameters even on linear curves.
             * For the inspected rocket blast files the interpolation flag is
             * linear, so these values are metadata rather than a different
             * easing mode. Preserve them for later non-linear/wave support. */
            if(n<1 || n>2 || !ceNumbers(p,v,n,f)) return ceError(p,"bad curve parm");
            c->parm[0]=f[0]; c->parm[1]=n>1?f[1]:0.0f; c->parmCount=n;
        } else if(ceEqual(key,"flags")) {
            int i;
            for(i=0;i<n;++i) {
                if(ceEqual(v[i],"linear")) c->linear=1;
                else if(allowNonlinear && ceEqual(v[i],"nonlinear")) c->nonlinear=1;
                else if(p->muzzle && ceEqual(v[i],"random")) c->random=1;
                else if((p->muzzle==4 || p->muzzle==6) && ceEqual(v[i],"clamp")) c->clamp=1;
                else {
                    /* ceValues consumed the line terminator; report the
                     * unsupported value instead of that empty token. */
                    strcpy(p->token,v[i]);p->tokenLine=valueLine;
                    return ceError(p,"unsupported curve interpolation");
                }
            }
        } else {
            strcpy(p->token,key);p->tokenLine=keyLine;
            return ceError(p,"unsupported curve field");
        }
    }
}
static int ceParseList(ceLexer *p,char out[CEFX_MEDIA][CEFX_PATH],int *n,int effect) {
    if(!ceSkipLines(p)||p->kind!=CE_LIST) return ceError(p,"expected '['");
    if(!ceNext(p)) return 0;
    for(;;) {
        if(!ceSkipLines(p)) return 0;
        if(p->kind==CE_ENDLIST) return ceNext(p);
        if(p->kind!=CE_WORD || *n>=CEFX_MEDIA) return ceError(p,"bad or oversized media list");
        if(!cePath(p->token,out[*n],effect)) return ceError(p,"unsafe/unsupported virtual resource path");
        ++*n;
        if(!ceNext(p)) return 0;
    }
}
static int ceParsePrimitive(ceLexer *p,cePrimitive *e) {
    char key[CEFX_TOKEN],v[CEFX_VALUES][CEFX_TOKEN];int n,components;
    ceRange *dest;
    if(!ceSkipLines(p)||p->kind!=CE_OPEN) return ceError(p,"expected primitive '{'");
    if(!ceNext(p)) return 0;
    for(;;) {
        ceCurve *curve=NULL;
        if(!ceSkipLines(p)) return 0;
        if(p->kind==CE_CLOSE) break;
        if(p->kind!=CE_WORD) return ceError(p,"expected field or closing '}'");
        strcpy(key,p->token);
        if(p->muzzle==3) {
            static const char *modelKeys[]={"name","flags","count","life","delay","cullrange",
                "bounce","angle","angledelta","origin","velocity","acceleration","gravity",
                "size","rgb","alpha","models","impactfx"};
            static const char *soundKeys[]={"name","count","life","delay","cullrange","sounds"};
            int j,limit=e->type==CE_SOUND?
                (int)(sizeof(soundKeys)/sizeof(soundKeys[0])):
                (int)(sizeof(modelKeys)/sizeof(modelKeys[0]));
            const char *const *keys=e->type==CE_SOUND?soundKeys:modelKeys;
            for(j=0;j<limit;j++)if(ceEqual(key,keys[j]))break;
            if(j==limit)return ceError(p,"field is not executed by narrow brass runtime");
        }
        if(!ceNext(p)) return 0;
        if(ceEqual(key,"rgb")) curve=&e->rgb;
        else if(ceEqual(key,"alpha")) curve=&e->alpha;
        else if(ceEqual(key,"size")) curve=&e->size;
        else if(ceEqual(key,"size2")) curve=&e->size2;
        else if(ceEqual(key,"length")) curve=&e->length;
        if(curve) { if(!ceParseCurve(p,curve,curve==&e->rgb?3:1,
            p->muzzle==6&&e->type==CE_PARTICLE&&curve==&e->size)) return 0;continue; }
        if(ceEqual(key,"shaders")) { if(!ceParseList(p,e->shaders,&e->shaderCount,0)) return 0;continue; }
        if(ceEqual(key,"playfx")) { if(!ceParseList(p,e->play,&e->playCount,1)) return 0;continue; }
        if(ceEqual(key,"emitfx")) { if(!ceParseList(p,e->emit,&e->emitCount,1)) return 0;continue; }
        if((p->muzzle==3||p->muzzle==4||p->muzzle==6) && ceEqual(key,"models")) { if(!ceParseList(p,e->models,&e->modelCount,2)) return 0;continue; }
        if(p->muzzle==3 && ceEqual(key,"sounds")) { if(!ceParseList(p,e->sounds,&e->soundCount,3)) return 0;continue; }
        if((p->muzzle==3||p->muzzle==4) && ceEqual(key,"impactfx")) { if(!ceParseList(p,e->impact,&e->impactCount,1)) return 0;continue; }
        if(p->muzzle==4 && ceEqual(key,"deathfx")) { if(!ceParseList(p,e->death,&e->deathCount,1)) return 0;continue; }
        if(!ceValues(p,v,&n)) return 0;
        if(ceEqual(key,"name")) continue; /* editor label, no execution semantics */
        if(ceEqual(key,"flags")||ceEqual(key,"spawnflags")) {
            int spawn=ceEqual(key,"spawnflags");
            if(!ceFlags(p,v,n,spawn,spawn?&e->spawn:&e->flags)) return 0;
            continue;
        }
        dest=NULL; components=1;
        if(ceEqual(key,"count")) dest=&e->count;
        else if(ceEqual(key,"life")) dest=&e->life;
        else if(ceEqual(key,"delay")) dest=&e->delay;
        else if(ceEqual(key,"cullrange")) dest=&e->cull;
        else if(ceEqual(key,"radius")) dest=&e->radius;
        else if(ceEqual(key,"height")) dest=&e->height;
        else if(ceEqual(key,"rotation")) dest=&e->rotation;
        else if(ceEqual(key,"rotationdelta")) dest=&e->rotationDelta;
        else if(ceEqual(key,"gravity")) dest=&e->gravity;
        else if((p->muzzle==3||p->muzzle==4||p->muzzle==6) && ceEqual(key,"bounce")) dest=&e->bounce;
        else if((p->muzzle==3||p->muzzle==4||p->muzzle==6) && ceEqual(key,"angle")) {dest=e->angle;components=3;}
        else if((p->muzzle==3||p->muzzle==4||p->muzzle==6) && ceEqual(key,"angledelta")) {dest=e->angleDelta;components=3;}
        else if(ceEqual(key,"density")) dest=&e->density;
        else if(ceEqual(key,"variance")) dest=&e->variance;
        else if(ceEqual(key,"origin")) { dest=e->origin;components=3; }
        else if(p->muzzle && e->type==CE_LINE && ceEqual(key,"origin2")) { dest=e->origin2;components=3; }
        else if(ceEqual(key,"velocity")) { dest=e->velocity;components=3; }
        else if(ceEqual(key,"acceleration")) { dest=e->acceleration;components=3; }
        else if(ceEqual(key,"nonuniformscale")) {
            if(n!=1 || (strcmp(v[0],"0")&&strcmp(v[0],"1"))) return ceError(p,"nonUniformScale must be 0/1");
            e->nonUniform=v[0][0]=='1';continue;
        }
        if(!dest) { strcpy(p->token,key);return ceError(p,"unsupported EFX field"); }
        if(!ceRanges(p,dest,components,v,n)) return 0;
    }
    if(e->count.a<0||e->count.b<0||e->count.a>128||e->count.b>128)
        return ceError(p,"count outside 0..128");
    if(e->life.a<=0||e->life.b<=0||e->life.a>60000||e->life.b>60000)
        return ceError(p,"life outside 1..60000ms");
    if(e->delay.a<0||e->delay.b<0||e->delay.a>60000||e->delay.b>60000)
        return ceError(p,"delay outside 0..60000ms");
    if((e->spawn&CE_SPHERE)&&(e->spawn&CE_CYLINDER)) return ceError(p,"conflicting spawn shapes");
    if(e->type==CE_PARTICLE || e->type==CE_ORIENTED || e->type==CE_DECAL || e->type==CE_TAIL || e->type==CE_LINE) {
        if(!e->shaderCount) return ceError(p,"visual primitive has no shader");
    }
    if(e->type==CE_RUNNER && !e->playCount) return ceError(p,"FxRunner has no child effect");
    if(p->muzzle!=3 && p->muzzle!=4 && p->muzzle!=6 && e->type==CE_EMITTER && (!(e->flags&CE_EMIT)||!e->emitCount || e->shaderCount))
        return ceError(p,"R1 supports unmodelled emitFx emitters only");
    if(p->muzzle!=3 && p->muzzle!=4 && p->muzzle!=6 && (e->flags&CE_PHYSICS) && !(e->flags&CE_IMPACT_KILLS))
        return ceError(p,"R1 physics requires impactKills (bounce not implemented)");
    /* Extended semantics are opt-in ONLY for the initial muzzle slice. Do not
     * silently expand the set of accepted grenade/water/snow definitions. */
    if(p->muzzle==1 && ((e->type!=CE_PARTICLE && e->type!=CE_LIGHT && e->type!=CE_TAIL && e->type!=CE_LINE) ||
        (e->flags&~(CE_ALPHA|CE_RELATIVE|CE_DEPTHHACK)) || e->playCount || e->emitCount ||
        ((e->flags&CE_RELATIVE) && (e->spawn || e->gravity.a || e->gravity.b))))
        return ceError(p,"unsupported muzzle composition");
    if(p->muzzle==2 && ((e->flags&(CE_RELATIVE|CE_DEPTHHACK)) || e->type==CE_LINE))
        return ceError(p,"unsupported world-impact attachment/type");
    /* MUZZLE_R3: only the inspected two-endpoint Line. Other origin2 modes,
     * shapes and motion require separate evidence; never turn them into Tail.
     * The original Line updater transforms endpoints, not velocity/physics. */
    if(e->type==CE_LINE) {
        int i;
        if(e->spawn || e->gravity.a || e->gravity.b || e->nonUniform)
            return ceError(p,"unsupported Line spawn/motion/scale");
        for(i=0;i<3;i++) if(e->velocity[i].a || e->velocity[i].b ||
            e->acceleration[i].a || e->acceleration[i].b)
            return ceError(p,"Line motion is not a supported endpoint mode");
    }

    if(p->muzzle==7) {
        /* Current world tag attachment, inspected MG42 Tail/Particle/Light.
         * No local-view depth hack, children, physics or model fallbacks. */
        if((e->type!=CE_TAIL&&e->type!=CE_PARTICLE&&e->type!=CE_LIGHT)||
           (e->flags&~(CE_ALPHA|CE_RELATIVE))||e->playCount||e->emitCount||e->modelCount||e->impactCount||e->deathCount||e->soundCount||e->nonUniform||
           (e->spawn&~CE_CYLINDER)||((e->flags&CE_RELATIVE)&&(e->gravity.a||e->gravity.b)))
            return ceError(p,"unsupported current-world attachment composition");
    }
    if(p->muzzle==6) {
        /* Script world effects share only actually executed primitives.
         * Relative/depth-hack, sound and impact/death children stay unsupported.
         * R200 additionally admits only unparameterized Particle.size nonlinear
         * curves using the documented port smoothstep; weapon modes stay strict. */
        int allowed=CE_ALPHA|CE_PHYSICS|CE_IMPACT_KILLS|CE_EMIT|CE_MODEL|CE_SHADER_TIME;
        if(e->flags&~allowed || e->impactCount || e->deathCount || e->soundCount)
            return ceError(p,"unsupported script effect flags/resources");
        if((e->spawn&CE_RAND_FWD) && e->type!=CE_RUNNER)
            return ceError(p,"script randrotaroundfwd requires FxRunner");
        if(e->type==CE_EMITTER) {
            if(e->flags&CE_MODEL) {
                if(!(e->flags&CE_PHYSICS) || !e->modelCount || e->shaderCount ||
                   e->playCount || e->emitCount || (e->flags&(CE_ALPHA|CE_EMIT|CE_SHADER_TIME)))
                    return ceError(p,"unsupported script model emitter composition");
            } else if(!(e->flags&CE_EMIT) || !e->emitCount || e->modelCount ||
                      e->shaderCount || e->playCount ||
                      (e->flags&~(CE_EMIT|CE_PHYSICS|CE_IMPACT_KILLS)))
                return ceError(p,"unsupported script child emitter composition");
        } else if(e->modelCount || (e->flags&(CE_MODEL|CE_EMIT)) || e->emitCount)
            return ceError(p,"script model/emit resources require Emitter");
        if(e->type!=CE_RUNNER && e->playCount)
            return ceError(p,"script play resources require FxRunner");
        if(e->type==CE_RUNNER && (e->flags || e->shaderCount || e->nonUniform))
            return ceError(p,"unsupported script FxRunner composition");
        if((e->type==CE_LIGHT || e->type==CE_DECAL) && e->flags)
            return ceError(p,"unsupported script light/decal flags");
        if(e->type==CE_LINE && (e->flags&~(CE_ALPHA|CE_SHADER_TIME)))
            return ceError(p,"unsupported script Line flags");
        if((e->flags&CE_PHYSICS) && !(e->flags&CE_IMPACT_KILLS) &&
           !(e->type==CE_EMITTER && (e->flags&CE_MODEL)))
            return ceError(p,"script nonmodel physics requires impactKills");
        if((e->flags&CE_IMPACT_KILLS) && !(e->flags&CE_PHYSICS))
            return ceError(p,"script impactKills requires physics");
    }

    if(p->muzzle==4) {
        if((e->spawn&CE_RAND_FWD) && e->type!=CE_RUNNER)
            return ceError(p,"randrotaroundfwd supported only on rocket FxRunner");
        /* Rocket-impact mode: narrow extension for the inspected v_blast1*
         * family. Model emitters may bounce or die on impact; all other
         * primitive types continue through the shared execution rules. */
        if(e->type==CE_EMITTER && (e->flags&CE_MODEL)) {
            if(!(e->flags&CE_PHYSICS) || !e->modelCount)
                return ceError(p,"rocket model emitter requires physics/model");
            if((e->flags&CE_IMPACTFX) != (e->impactCount?CE_IMPACTFX:0))
                return ceError(p,"rocket impactFx flag/resource mismatch");
        } else if(e->modelCount || e->impactCount || (e->flags&(CE_MODEL|CE_IMPACTFX))) {
            return ceError(p,"rocket model/impact resources require emitter");
        }
        /* Narrow deathFx support proven for the snow rocket-impact particle:
         * natural particle expiry spawns one child effect at its final point.
         * Physics-driven death semantics remain deliberately unsupported. */
        if((e->flags&CE_DEATHFX) != (e->deathCount?CE_DEATHFX:0))
            return ceError(p,"rocket deathFx flag/resource mismatch");
        if(e->deathCount && (e->type!=CE_PARTICLE || (e->flags&CE_PHYSICS)))
            return ceError(p,"rocket deathFx supported only on nonphysical Particle");
        if(e->flags&~(CE_ALPHA|CE_PHYSICS|CE_IMPACT_KILLS|CE_EMIT|CE_MODEL|CE_IMPACTFX|CE_SHADER_TIME|CE_DEATHFX))
            return ceError(p,"unsupported rocket-impact flags");
    }

    if(p->muzzle==3) {
        /* Narrow executable contract for the FOUR inspected brass resources.
         * No approximate sphere, emit, relative, bbox or glass support. */
        if(e->spawn || e->nonUniform || e->playCount || e->emitCount || e->shaderCount)
            return ceError(p,"unsupported brass composition");
        if(e->type==CE_EMITTER) {
            if(e->flags&~(CE_PHYSICS|CE_IMPACT_KILLS|CE_MODEL|CE_IMPACTFX))
                return ceError(p,"unsupported brass flags");
            if(!(e->flags&CE_PHYSICS) || e->soundCount)
                return ceError(p,"brass emitter requires physical motion");
            if(!!(e->flags&CE_MODEL)!=!!e->modelCount ||
               !!(e->flags&CE_IMPACTFX)!=!!e->impactCount)
                return ceError(p,"model/impact flag and resource list disagree");
            if(!(e->flags&CE_MODEL) && !(e->flags&CE_IMPACTFX))
                return ceError(p,"invisible brass emitter has no action");
        } else if(e->type==CE_SOUND) {
            if(!e->soundCount || e->flags || e->modelCount || e->impactCount)
                return ceError(p,"unsupported Sound composition");
        } else return ceError(p,"unsupported brass type");
        if(e->cull.a!=e->cull.b || e->cull.a<0 || e->cull.a>32767 ||
           floorf(e->cull.a)!=e->cull.a)
            return ceError(p,"brass cullrange must be a bounded integer");
        if(e->rgb.random || e->alpha.random || e->size.random || e->size2.random || e->length.random)
            return ceError(p,"random brass curves not yet executed");
    }
    return ceNext(p);
}
static int ceParseMode(const char *data,size_t size,ceDefinition *out,char error[256],int muzzle) {
    ceLexer p; int type,ok=0;
    memset(&p,0,sizeof(p));memset(out,0,sizeof(*out));p.data=data;p.size=size;p.line=p.tokenLine=1;p.muzzle=muzzle;
    if(!data||!size||size>CEFX_FILE_LIMIT||memchr(data,0,size)) {
        snprintf(error,256,"invalid file size or embedded NUL");return 0;
    }
    if(size>=3 && (unsigned char)data[0]==239 && (unsigned char)data[1]==187 && (unsigned char)data[2]==191) p.pos=3;
    if(!ceNext(&p)) goto finish;
    for(;;) {
        if(!ceSkipLines(&p)) break;
        if(p.kind==CE_EOF) { ok=out->count>0; if(!ok) ceError(&p,"empty definition");break; }
        type=ceEqual(p.token,"particle")?CE_PARTICLE:ceEqual(p.token,"light")?CE_LIGHT:
             ceEqual(p.token,"fxrunner")?CE_RUNNER:ceEqual(p.token,"emitter")?CE_EMITTER:
             ceEqual(p.token,"decal")?CE_DECAL:
             (muzzle && ceEqual(p.token,"tail"))?CE_TAIL:
             (muzzle && ceEqual(p.token,"line"))?CE_LINE:
             (muzzle==3 && ceEqual(p.token,"sound"))?CE_SOUND:
             ((muzzle==4 || muzzle==6) && ceEqual(p.token,"orientedparticle"))?CE_ORIENTED:0;
        if(p.kind!=CE_WORD||!type) { ceError(&p,"unsupported primitive");break; }
        if(out->count>=CEFX_PRIMS) { ceError(&p,"too many primitives");break; }
        ceDefaults(&out->prim[out->count],type);
        if(!ceNext(&p)||!ceParsePrimitive(&p,&out->prim[out->count])) break;
        ++out->count;
    }
finish:
    snprintf(error,256,"%s",p.error);
    if(!ok) out->count=0;
    return ok;
}
static int ceParse(const char *data,size_t size,ceDefinition *out,char error[256]) {
    return ceParseMode(data,size,out,error,0);
}
#endif
