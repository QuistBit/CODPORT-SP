#ifndef BG_COD1_DROP_FRAME_H
#define BG_COD1_DROP_FRAME_H
/* Native RunItem2001CBC0 ordinary weapon lane; entity IDs/masks here are
 * explicit native data, never port enums. Services must validate lifetime
 * before/after foreign work. Unsupported/failing services return refusal. */
#include "bg_cod1_drop_motion.h"
#include "bg_cod1_drop_bounce.h"
#include "bg_cod1_drop_surface.h"
#include <string.h>
typedef struct {float fraction,end[3],normal[3];int ground,startsolid;} c1dfTrace;
typedef struct {
 int alive,kind,ground,time,angularKind,angularTime,owner,number,weapon;
 unsigned flags,mask;float base[3],delta[3],origin[3],angles[3],angularBase[3],angularDelta[3],mins[3],maxs[3],bounce;
} c1dfState;
typedef struct {
 void*user;int(*current)(void*);
 int(*trace)(void*,int,const float[3],const float[3],const float[3],const float[3],int,unsigned,c1dfTrace*);
 int(*link)(void*,const c1dfState*);int(*think)(void*,c1dfState*);
 int(*contents)(void*,const float[3],int,unsigned,int*);
 int(*remove)(void*,c1dfState*);int(*draw_random)(void*,int*);
} c1dfServices;
static int C1DF_Current(const c1dfServices*h){return h&&h->current&&h->current(h->user);}
static int C1DF_TraceValid(const c1dfTrace*t){unsigned i;if(!t||!isfinite(t->fraction)||t->fraction<0||t->fraction>1||t->ground<0||t->ground>2047)return 0;for(i=0;i<3;i++)if(!isfinite(t->end[i])||!isfinite(t->normal[i]))return 0;return t->startsolid==0||t->startsolid==1;}
static int C1DF_Link(c1dfState*s,const c1dfServices*h){return C1DF_Current(h)&&h->link&&h->link(h->user,s)&&C1DF_Current(h);}
static int C1DF_Run(c1dfState*s,int now,int prev,const c1dfServices*h){
 float target[3],velocity[3],angular[3];c1dfTrace tr;int hit,contents,r;unsigned i;
 if(!s||!s->alive||now<0||prev<0||now<prev||!C1DF_Current(h))return 0;
 if(s->ground==2047&&s->kind!=5){s->kind=5;s->time=now;}
 if(s->kind==0||s->kind==8)return h->think&&h->think(h->user,s)&&C1DF_Current(h);
 if(s->kind!=2&&s->kind!=5)return 0;
 if(!C1DM_Evaluate(s->kind==5,s->time,now,s->base,s->delta,target,velocity))return 0;
 if(!h->trace||!h->trace(h->user,(s->flags&16)!=0,s->origin,s->mins,s->maxs,target,s->owner,s->mask?s->mask:0x491,&tr)||!C1DF_Current(h)||!C1DF_TraceValid(&tr))return 0;
 for(i=0;i<3;i++)s->origin[i]=tr.end[i];if(tr.startsolid)tr.fraction=0;
 if(!C1DF_Link(s,h)||!h->think||!h->think(h->user,s)||!C1DF_Current(h))return 0;
 if(!s->alive||tr.fraction==1)return 1;
 if(!h->contents||!h->contents(h->user,s->origin,-1,0x80000000u,&contents)||!C1DF_Current(h))return 0;
 if(contents)return h->remove&&h->remove(h->user,s)&&C1DF_Current(h);
 /* Native ftol truncation of frame interval times fraction. */
 hit=prev+(int)((double)(float)(now-prev)*tr.fraction);
 if(!C1DM_Evaluate(s->kind==5,s->time,hit,s->base,s->delta,target,velocity)||!C1DB_Reflect(velocity,tr.normal,s->bounce,s->delta))return 0;
 if(tr.startsolid){for(i=0;i<3;i++){s->delta[i]=0;target[i]=s->origin[i];}target[2]-=128.0f;
  if(!h->trace(h->user,(s->flags&16)!=0,s->origin,s->mins,s->maxs,target,s->number,0x411,&tr)||!C1DF_Current(h)||!C1DF_TraceValid(&tr))return 0;
 }
 if(tr.normal[2]>0&&s->delta[2]<40){
  if(!h->draw_random||!h->draw_random(h->user,&r)||!C1DF_Current(h)||r<0||r>32767)return 0;
  tr.end[2]=(float)((double)(float)(((double)r/32768.0+1.0)*0.5)+tr.end[2]);
  for(i=0;i<3;i++){s->origin[i]=s->base[i]=tr.end[i];s->delta[i]=0;}s->kind=0;s->time=0;s->ground=tr.ground;
  if(!C1DS_Surface(s->angles,tr.normal,s->weapon,angular))return 0;
  for(i=0;i<3;i++){s->angles[i]=s->angularBase[i]=angular[i];s->angularDelta[i]=0;}s->angularKind=0;s->angularTime=0;
  return C1DF_Link(s,h);
 }
 for(i=0;i<3;i++)s->base[i]=s->origin[i]=(float)((double)s->origin[i]+tr.normal[i]);s->time=now;return 1;
}
#endif
