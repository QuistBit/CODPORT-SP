/* SPDX-License-Identifier: GPL-3.0-or-later
 * BRASS_R1 — bounded detached model emitter runtime.
 * Bounded model-emitter + Sound scheduler. No renderer/audio/prediction ABI is
 * invented here: adapters are supplied explicitly by the caller. Existing
 * grenade/muzzle/impact executors and their random streams are not used.
 * Physical arithmetic follows inspected CoDSP 0x482f80/0x4833a0. startsolid
 * mapping is an explicit safety policy; render/audio fidelity requires game tests.
 */
#ifndef CG_COD1_BRASS_RUNTIME_H
#define CG_COD1_BRASS_RUNTIME_H
#include "cg_cod1_efx_parse.h"
#include <limits.h>
#define CB_DEFS 16
#define CB_PARTS 256
#define CB_QUEUE 32
#define CB_DEPTH 8
#define CB_BUDGET 512

typedef float cbVec[3];
typedef struct { float fraction; cbVec end, normal; int startSolid, allSolid; } cbTrace;
typedef struct { cbVec origin,axis[3]; } cbTag;
typedef struct {
    void *user;
    /* read returns allocated bytes, released by freeText; no IO after preload. */
    int (*read)(void *,const char *,char **);
    void (*freeText)(void *,char *);
    int (*model)(void *,const char *);
    int (*sound)(void *,const char *); /* alias registration, not WAV pathname */
    void (*trace)(void *,const cbVec,const cbVec,cbTrace *);
    void (*playSound)(void *,int,const cbVec,int);
    void (*drawModel)(void *,int,const cbVec,const cbVec *,const float *);
    void (*diagnostic)(void *,const char *);
} cbIO;
typedef struct {
    char path[CEFX_PATH]; int state;
    ceDefinition *definition;
    int models[CEFX_PRIMS][CEFX_MEDIA],sounds[CEFX_PRIMS][CEFX_MEDIA];
    int impacts[CEFX_PRIMS][CEFX_MEDIA];
} cbDefinition;
typedef struct {
    int used,defId,primId,flags,depth,model,sound;
    double start,end,last;
    cbVec pos,vel,acc,angles,spin;
    float bounce,size0,size1,rgb0[3],rgb1[3],alpha0,alpha1;
} cbPart;
typedef struct { int effect,key,when; } cbPending;
typedef struct {
    cbIO io; cbDefinition defs[CB_DEFS]; cbPart part[CB_PARTS];
    cbPending pending[CB_QUEUE]; int definitions,pendingCount;
    int clock,clockValid,budget,drawClock; cbVec camera;
    unsigned rng,spawned,dropped,sounds,collisions,stopped,played,events,discarded;
} cbRuntime;
static void cbCopy(const cbVec a,cbVec b){memcpy(b,a,sizeof(cbVec));}
static void cbAxesIdentity(cbVec a[3]){memset(a,0,3*sizeof(cbVec));a[0][0]=a[1][1]=a[2][2]=1;}
static void cbTransform(const cbVec a,const cbVec axis[3],cbVec out){int i;for(i=0;i<3;i++)out[i]=a[0]*axis[0][i]+a[1]*axis[1][i]+a[2]*axis[2][i];}
static int cbFinite(const cbVec v){return isfinite(v[0])&&isfinite(v[1])&&isfinite(v[2]);}
static unsigned cbRandom(cbRuntime *r){r->rng=r->rng*1664525u+1013904223u;return r->rng;}
static float cbUnit(cbRuntime *r){return (cbRandom(r)>>8)*(1.0f/16777216.0f);}
static float cbSample(cbRuntime *r,ceRange x){return x.a+(x.b-x.a)*cbUnit(r);}
static void cbSampleVec(cbRuntime *r,const ceRange x[3],cbVec v){int i;for(i=0;i<3;i++)v[i]=cbSample(r,x[i]);}
static void cbNote(cbRuntime *r,const char *s){if(r->io.diagnostic)r->io.diagnostic(r->io.user,s);}
static void cbClear(cbRuntime *r){memset(r->part,0,sizeof(r->part));r->pendingCount=0;r->budget=CB_BUDGET;}
static void cbInit(cbRuntime *r,const cbIO *io){memset(r,0,sizeof(*r));r->io=*io;r->rng=0x13bb47a1u;r->budget=CB_BUDGET;r->drawClock=INT_MIN;}
static void cbDestroy(cbRuntime *r){int i;for(i=0;i<r->definitions;i++)free(r->defs[i].definition);memset(r,0,sizeof(*r));}
static void cbClock(cbRuntime *r,int time,const cbVec camera){
    if(r->clockValid && time<r->clock)cbClear(r);
    if(!r->clockValid || time!=r->clock)r->budget=CB_BUDGET;
    r->clock=time;r->clockValid=1;cbCopy(camera,r->camera);
}
static int cbRegister(cbRuntime *r,const char *name,int depth){
    char path[CEFX_PATH],error[256],*text=NULL;cbDefinition *d;int i,j,id,n,ok=1;
    if(depth>CB_DEPTH || !cePath(name,path,1))return 0;
    for(i=0;i<r->definitions;i++)if(!strcmp(path,r->defs[i].path))return r->defs[i].state==2?i+1:0;
    if(r->definitions>=CB_DEFS || !r->io.read || !r->io.freeText)return 0;
    id=++r->definitions;d=&r->defs[id-1];strcpy(d->path,path);d->state=1;
    n=r->io.read(r->io.user,path,&text);
    if(n<=0 || n>CEFX_FILE_LIMIT || !text){if(text)r->io.freeText(r->io.user,text);d->state=-1;cbNote(r,path);return 0;}
    d->definition=(ceDefinition *)malloc(sizeof(*d->definition));
    if(!d->definition || !ceParseMode(text,(size_t)n,d->definition,error,3)){
        cbNote(r,d->definition?error:"brass definition allocation failed");
        free(d->definition);d->definition=NULL;r->io.freeText(r->io.user,text);d->state=-1;return 0;
    }
    r->io.freeText(r->io.user,text);
    for(i=0;i<d->definition->count;i++){
        cePrimitive *p=&d->definition->prim[i];
        for(j=0;j<p->modelCount;j++){d->models[i][j]=r->io.model?r->io.model(r->io.user,p->models[j]):0;if(!d->models[i][j])ok=0;}
        for(j=0;j<p->soundCount;j++){d->sounds[i][j]=r->io.sound?r->io.sound(r->io.user,p->sounds[j]):0;if(!d->sounds[i][j])ok=0;}
        for(j=0;j<p->impactCount;j++){
            int child,k;d->impacts[i][j]=child=cbRegister(r,p->impact[j],depth+1);
            if(!child)ok=0;
            else for(k=0;k<r->defs[child-1].definition->count;k++)
                if(r->defs[child-1].definition->prim[k].type!=CE_SOUND)ok=0;
        }
    }
    d->state=ok?2:-1;if(!ok)cbNote(r,path);return ok?id:0;
}
static cbPart *cbAlloc(cbRuntime *r){
    int i;if(r->budget<=0){r->dropped++;return NULL;}r->budget--;
    for(i=0;i<CB_PARTS;i++)if(!r->part[i].used){cbPart *p=&r->part[i];memset(p,0,sizeof(*p));p->used=1;r->spawned++;return p;}
    r->dropped++;return NULL;
}
static void cbSpawn(cbRuntime *r,int id,const cbTag *tag,double at,int depth){
    cbDefinition *d;int i,j,k,count;
    if(id<1 || id>r->definitions || depth>CB_DEPTH || !cbFinite(tag->origin)){r->dropped++;return;}
    d=&r->defs[id-1];if(d->state!=2)return;
    for(i=0;i<d->definition->count;i++){
        const cePrimitive *p=&d->definition->prim[i];
        float dist=0;for(k=0;k<3;k++){float delta=tag->origin[k]-r->camera[k];dist+=delta*delta;}
        /* Gate per component, never for the entire shell effect. */
        if(p->cull.a>0 && dist>p->cull.a*p->cull.a)continue;
        count=(int)floorf(cbSample(r,p->count)+.5f);
        for(j=0;j<count;j++){
            cbPart *l;cbVec tmp;double birth=at+cbSample(r,p->delay);float life=cbSample(r,p->life);
            if(birth+life<=r->clock)continue;
            l=cbAlloc(r);if(!l)return;
            l->defId=id;l->primId=i;l->depth=depth;l->flags=p->flags;
            l->start=l->last=birth;l->end=birth+life;
            cbSampleVec(r,p->origin,tmp);cbTransform(tmp,tag->axis,l->pos);
            for(k=0;k<3;k++)l->pos[k]+=tag->origin[k];
            if(p->type==CE_SOUND){
                l->sound=d->sounds[i][cbRandom(r)%(unsigned)p->soundCount];
                /* Execute once when due, no persistent rendering component. */
                if(birth<=r->clock){int h=l->sound;l->used=0;
                    if(r->io.playSound && h){r->io.playSound(r->io.user,h,l->pos,(int)birth);r->sounds++;}}
                continue;
            }
            cbSampleVec(r,p->velocity,tmp);cbTransform(tmp,tag->axis,l->vel);
            cbSampleVec(r,p->acceleration,tmp);cbTransform(tmp,tag->axis,l->acc);l->acc[2]+=cbSample(r,p->gravity);
            cbSampleVec(r,p->angle,l->angles);cbSampleVec(r,p->angleDelta,l->spin);l->bounce=cbSample(r,p->bounce);
            l->size0=cbSample(r,p->size.start[0]);l->size1=cbSample(r,p->size.end[0]);
            for(k=0;k<3;k++){l->rgb0[k]=cbSample(r,p->rgb.start[k]);l->rgb1[k]=cbSample(r,p->rgb.end[k]);}
            l->alpha0=cbSample(r,p->alpha.start[0]);l->alpha1=cbSample(r,p->alpha.end[0]);
            if(p->modelCount)l->model=d->models[i][cbRandom(r)%(unsigned)p->modelCount];
        }
    }
}
/* Inspected no-attachment branch. One point trace per simulation update, not
 * a new rigid body or copied server-grenade solver. Fraction/interpenetration
 * semantics are kept explicit for comparison with the original trace adapter. */
static void cbIntegrate(cbRuntime *r,cbPart *l,double now){
    cbTrace tr;cbVec old,end;float ms=(float)(now-l->last),dt=ms*.001f;int k;
    if(ms<=0)return;
    cbCopy(l->pos,old);
    for(k=0;k<3;k++){l->vel[k]+=l->acc[k]*dt;end[k]=old[k]+l->vel[k]*dt+.5f*l->acc[k]*dt*dt;}
    memset(&tr,0,sizeof(tr));tr.fraction=1;cbCopy(end,tr.end);
    if((l->flags&CE_PHYSICS) && r->io.trace)r->io.trace(r->io.user,old,end,&tr);
    if(!isfinite(tr.fraction)||tr.fraction<0||tr.fraction>1||!cbFinite(tr.normal)||!cbFinite(tr.end)){
        l->used=0;r->dropped++;cbNote(r,"invalid cosmetic trace result");return;
    }
    /* CoDSP detached-shell semantics: a startsolid/allsolid trace is not an
     * impact.  Do not manufacture impact FX/bounce and do not retire the
     * particle; advance it to the already integrated endpoint so the next
     * update can escape a boundary/interpenetration condition. */
    if(tr.startSolid || tr.allSolid){
        cbCopy(end,l->pos);
        if(l->pos[0]==old[0]&&l->pos[1]==old[1]&&l->pos[2]==old[2])
            for(k=0;k<3;k++)l->spin[k]*=.7f;
        for(k=0;k<3;k++)l->angles[k]+=ms*.01f*l->spin[k];
        l->last=now;
        if(!cbFinite(l->pos)||!cbFinite(l->vel)||!cbFinite(l->angles)){l->used=0;r->dropped++;}
        return;
    }
    if(tr.fraction<1){
        const cePrimitive *p=&r->defs[l->defId-1].definition->prim[l->primId];
        float dot=0;cbTag hit;
        r->collisions++;
        if((l->flags&CE_IMPACTFX) && p->impactCount){
            int child=r->defs[l->defId-1].impacts[l->primId][cbRandom(r)%(unsigned)p->impactCount];
            cbCopy(tr.end,hit.origin);cbAxesIdentity(hit.axis);
            /* Stock child here is Sound with zero origin/motion, so its axes
             * cannot affect the result. General impact orientation is NOT supported. */
            cbSpawn(r,child,&hit,now,l->depth+1);
        }
        if(l->flags&CE_IMPACT_KILLS){l->used=0;return;}
        for(k=0;k<3;k++){l->vel[k]+=l->acc[k]*(dt*tr.fraction);dot+=l->vel[k]*tr.normal[k];}
        for(k=0;k<3;k++)l->vel[k]=(l->vel[k]-2*dot*tr.normal[k])*l->bounce;
        /* CoDSP compares upward normal > 0, reflected vertical speed < 4. */
        if(tr.normal[2]>0 && l->vel[2]<4){
            memset(l->vel,0,sizeof(cbVec));memset(l->acc,0,sizeof(cbVec));l->flags&=~CE_PHYSICS;r->stopped++;
        }
        for(k=0;k<3;k++)l->pos[k]=old[k]+(end[k]-old[k])*tr.fraction;
    } else cbCopy(end,l->pos);
    /* No-motion damping is evaluated after the positional update. */
    if(l->pos[0]==old[0]&&l->pos[1]==old[1]&&l->pos[2]==old[2])for(k=0;k<3;k++)l->spin[k]*=.7f;
    for(k=0;k<3;k++)l->angles[k]+=ms*.01f*l->spin[k];
    l->last=now;
    if(!cbFinite(l->pos)||!cbFinite(l->vel)||!cbFinite(l->angles)){l->used=0;r->dropped++;}
}
static void cbAdvance(cbRuntime *r){
    int i;for(i=0;i<CB_PARTS;i++){
        cbPart *l=&r->part[i];int kind;
        if(!l->used || r->clock<l->start)continue;
        if(r->clock>=l->end){l->used=0;continue;}
        kind=r->defs[l->defId-1].definition->prim[l->primId].type;
        if(kind==CE_SOUND){int h=l->sound;l->used=0;
            if(h && r->io.playSound){r->io.playSound(r->io.user,h,l->pos,(int)l->start);r->sounds++;}continue;}
        cbIntegrate(r,l,r->clock);
    }
}
/* Same basis convention as RTCW AnglesToAxis (forward, -right, up). Built
 * from angles afresh on every draw: previous scaled matrices are never reused. */
static void cbModelAxes(const cbVec angles,float scale,cbVec axis[3]){
    const float rad=0.01745329251994329577f;
    float p=angles[0]*rad,y=angles[1]*rad,r=angles[2]*rad;
    float sp=sinf(p),cp=cosf(p),sy=sinf(y),cy=cosf(y),sr=sinf(r),cr=cosf(r);
    axis[0][0]=cp*cy;axis[0][1]=cp*sy;axis[0][2]=-sp;
    axis[1][0]=sr*sp*cy-cr*sy;axis[1][1]=sr*sp*sy+cr*cy;axis[1][2]=sr*cp;
    axis[2][0]=cr*sp*cy+sr*sy;axis[2][1]=cr*sp*sy-sr*cy;axis[2][2]=cr*cp;
    {int i,j;for(i=0;i<3;i++)for(j=0;j<3;j++)axis[i][j]*=scale;}
}
static void cbRender(cbRuntime *r){
    int i,k;for(i=0;i<CB_PARTS;i++){
        const cbPart *l=&r->part[i];const cePrimitive *p;cbVec axis[3];float size,t,color[4];
        if(!l->used || !l->model || r->clock<l->start || r->clock>=l->end)continue;
        p=&r->defs[l->defId-1].definition->prim[l->primId];
        t=(float)(((double)r->clock-l->start)/(l->end-l->start));
        size=l->size0+(l->size1-l->size0)*(p->size.linear?t:0);
        if(!(size>0) || size>8192)continue;
        cbModelAxes(l->angles,size,axis);
        for(k=0;k<3;k++)color[k]=l->rgb0[k]+(l->rgb1[k]-l->rgb0[k])*(p->rgb.linear?t:0);
        color[3]=l->alpha0+(l->alpha1-l->alpha0)*(p->alpha.linear?t:0);
        if(r->io.drawModel)r->io.drawModel(r->io.user,l->model,l->pos,axis,color);
    }
}
/* Feed this queue ONLY from the existing prediction-deduplicated event path.
 * Queueing itself does not invent a new event ID or dedup using timestamps.
 * One timestamp may legitimately contain two distinct fire events. */
static int cbQueue(cbRuntime *r,int effect,int key,int when){
    cbPending *q;
    if(effect<1||effect>r->definitions||r->defs[effect-1].state!=2||key<=0)return 0;
    r->events++;
    if(r->pendingCount==CB_QUEUE){r->discarded++;return 0;}
    q=&r->pending[r->pendingCount++];q->effect=effect;q->key=key;q->when=when;return 1;
}
static void cbSubmitTag(cbRuntime *r,int key,const cbTag *tag){
    int i;
    if(!tag || !cbFinite(tag->origin))return;
    for(i=0;i<r->pendingCount;i++){
        const cbPending *q=&r->pending[i];double age=(double)r->clock-q->when;
        if(q->key!=key || age<0 || age>150){r->discarded++;continue;}
        cbSpawn(r,q->effect,tag,r->clock,0);r->played++;
    }
    /* Actual spawn uses the now-visible pose; no fictitious past trajectory
     * is back-extrapolated from a later model frame. Event-age choice is a port
     * policy to be checked in prediction integration, not original equivalence. */
    r->pendingCount=0;
}
static void cbEndView(cbRuntime *r){r->discarded+=(unsigned)r->pendingCount;r->pendingCount=0;}
#endif
