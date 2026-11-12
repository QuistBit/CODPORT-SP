#ifndef BG_COD1_SOUND_NOTIFY_H
#define BG_COD1_SOUND_NOTIFY_H
/* R227 bounded generation/Session-owned counterpart of native GAME
 * 200466A0 (pending sound),20016E50 (duration report),2001EF10 (frame notify).
 * Native unknown-duration fallback is5000ms. This is not its packed ABI. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#define C1N_FALLBACK_MS 5000
#define C1N_MAX_DURATION_MS 120000
typedef struct {uint64_t epoch,session,sequence;uint32_t entity,generation;} C1NIdentity;
typedef struct {C1NIdentity identity;char notify[128];int64_t start,last;int duration,mode,pending,busy,cancelled;} C1NPending;
typedef struct {void *user;int (*current)(void *,const C1NIdentity *);int (*notify)(void *,const C1NIdentity *,const char *);} C1NOps;
static int C1N_Identity(const C1NIdentity *r){return r&&r->epoch&&r->session&&r->sequence&&r->entity<1024&&r->generation;}
static int C1N_Text(const char *p){size_t i;if(!p)return 0;for(i=0;i<128;i++){unsigned c=(unsigned char)p[i];if(!c)return i>0;if(c<32||c>=127)return 0;}return 0;}
static int C1N_SameOwner(const C1NIdentity*a,const C1NIdentity*b){return a->epoch==b->epoch&&a->session==b->session&&a->entity==b->entity&&a->generation==b->generation;}
static int C1N_Current(const C1NOps *ops,const C1NIdentity *id){C1NIdentity copy=*id;return ops->current(ops->user,&copy)==1&&!memcmp(&copy,id,sizeof(copy));}
static void C1N_Reset(C1NPending *s){if(!s)return;if(s->busy){s->pending=0;s->cancelled=1;}else memset(s,0,sizeof(*s));}
static int C1N_BeginResult(C1NPending *s,const C1NIdentity *id,const char *notify,int mode,int64_t now,const C1NOps *ops,int *deliveredConflict){
 C1NIdentity next,old;char name[128],oldName[128];int oldMode,ok=0;
 if(!s||s->busy||!C1N_Identity(id)||!C1N_Text(notify)||!ops||!ops->current||!ops->notify||now<0||now>INT_MAX||(mode!=0&&mode!=1))return 0;
 next=*id;strcpy(name,notify);s->busy=1;s->cancelled=0;
 if(s->identity.sequence&&(!C1N_SameOwner(&s->identity,&next)||next.sequence<=s->identity.sequence||now<s->last))goto done;
 if(!C1N_Current(ops,&next)||s->cancelled)goto done;
 if(s->pending){
  old=s->identity;strcpy(oldName,s->notify);oldMode=s->mode;
  if(!C1N_SameOwner(&old,&next)||next.sequence<=old.sequence||now<s->last)goto done;
  /* Native replacement first notifies the older waiter. Clear it before
   * crossing that callback; a cancelled scope cannot commit a replacement. */
  s->pending=0;
  if(!ops->notify(ops->user,&old,oldName)||s->cancelled||!C1N_Current(ops,&next))goto done;
  if(!oldMode||!mode){
   if(deliveredConflict)*deliveredConflict=1;
   goto done;
  }
 }
 if(s->cancelled||!C1N_Current(ops,&next))goto done;
 s->identity=next;strcpy(s->notify,name);s->mode=mode;s->start=s->last=now;s->duration=-1;s->pending=1;ok=1;
done:s->busy=0;return ok;
}
static int C1N_Begin(C1NPending *s,const C1NIdentity *id,const char *notify,int mode,int64_t now,const C1NOps *ops){
 return C1N_BeginResult(s,id,notify,mode,now,ops,NULL);
}
static int C1N_Report(C1NPending*s,const C1NIdentity*id,int milliseconds,int64_t now,const C1NOps *ops){
 C1NIdentity copy;int ok=0;
 if(!s||s->busy||!s->pending||!C1N_Identity(id)||milliseconds<0||milliseconds>C1N_MAX_DURATION_MS||now<s->last||now>INT_MAX||!ops||!ops->current)return 0;
 copy=*id;if(!C1N_SameOwner(&s->identity,&copy)||s->identity.sequence!=copy.sequence)return 0;
 s->busy=1;s->cancelled=0;
 if(C1N_Current(ops,&copy)&&!s->cancelled){if(milliseconds>s->duration)s->duration=milliseconds;s->last=now;ok=1;}
 s->busy=0;return ok;
}
static int C1N_Frame(C1NPending*s,int64_t now,const C1NOps *ops){
 C1NIdentity copy;char name[128];int duration,ok=0;
 if(!s||s->busy||!ops||!ops->current||!ops->notify||now<0||now>INT_MAX)return 0;
 if(!s->pending)return 1;
 if(now<s->last)return 0;copy=s->identity;strcpy(name,s->notify);s->busy=1;s->cancelled=0;
 if(!C1N_Current(ops,&copy)||s->cancelled){s->pending=0;goto done;}
 s->last=now;duration=s->duration<0?C1N_FALLBACK_MS:s->duration;
 if(now-s->start<duration){ok=1;goto done;}
 s->pending=0;ok=ops->notify(ops->user,&copy,name)==1&&!s->cancelled;
done:s->busy=0;return ok;
}
#endif
