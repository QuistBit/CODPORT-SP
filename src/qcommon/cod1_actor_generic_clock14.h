#ifndef COD1_ACTOR_GENERIC_CLOCK14_H
#define COD1_ACTOR_GENERIC_CLOCK14_H
#include "cod1_actor_generic_flagged14.h"
#include "cod1_actor_clock_ranges46.h"
/* Original server47CCD0 closure. Search and advance deliberately differ in
 * float materialization. Sync descendants share the root's phase and cycle;
 * the event order is native preorder. VM delivery remains with the caller. */
#define C1AGC14_MAX_EVENTS 1024u
typedef struct {unsigned node,leaf;uint16_t notify,payload;} c1agc14Event_t;
typedef struct {unsigned count,resume;float usedDt;c1agc14Event_t event[C1AGC14_MAX_EVENTS];} c1agc14Step_t;
typedef struct {
 const c1agt14Definition_t*d;const c1agf14Topology_t*t;
 const c1anm14Metadata_t*const*m;const float*frequency;
 c1atl14Node_t*n;c1afl14State_t*f;c1agc14Step_t*out;int owner,ok;
} c1agc14Context_t;
typedef struct {float current,old,next,delta;unsigned cycle,oldCycle;} c1agc14Search_t;
static int C1AGC14_Cycles(unsigned a,unsigned b){return (int)(int16_t)a-(int)(int16_t)b;}
static double C1AGC14_Fraction(const c1agc14Search_t*s,float note){
 int cycles=C1AGC14_Cycles(s->cycle,s->oldCycle);
 if(s->current==1)return 1;
 if(s->next<s->current){
  if(s->next>note)cycles++;
  else if(s->current>note)return 1;
 }else if((s->next<=note&&s->next!=1)||s->current>note)return 1;
 return (((double)note-s->old)+cycles)/s->delta;
}
static double C1AGC14_NamedFraction(c1agc14Context_t*c,unsigned i,const c1agc14Search_t*s){
 unsigned leaf;float note=1;
 if(!c->owner||!c->f[i].token)return 1;
 leaf=c->d[i].children?c->f[i].descendant:i;
 if(leaf&&c->f[i].cursor>=0)note=c->m[leaf]->notes[c->f[i].cursor].phase;
 return C1AGC14_Fraction(s,note);
}
static double C1AGC14_SyncSearch(c1agc14Context_t*c,unsigned root,const c1agc14Search_t*s){
 float best=(float)C1AGC14_NamedFraction(c,root,s);unsigned child;
 for(child=c->t->first[root];child!=C1AGF14_NONE;child=c->t->next[child])
  if(c->n[child].currentWeight>0&&c->n[child].goalWeight>0){
   double value=C1AGC14_SyncSearch(c,child,s);if(value<best)best=(float)value;
  }
 return best;
}
static double C1AGC14_Search(c1agc14Context_t*c,unsigned root,float dt){
 const c1atl14Node_t*n=c->n+root;unsigned child,cycle;double delta,phase;int branch=c->d[root].children!=0,loop;
 c1agc14Search_t search;
 if(!n->currentWeight||!n->goalWeight)return 1;
 if(branch&&!(c->d[root].flags&3)){
  float scaled=(float)((double)dt*n->rate),best=1;
  if(!scaled)return 1;if(!C1ATL14_Finite(scaled)){c->ok=0;return 1;}
  for(child=c->t->first[root];child!=C1AGF14_NONE;child=c->t->next[child]){
   double f=C1AGC14_Search(c,child,scaled);if(f<best)best=(float)f;
  }return best;
 }
 delta=branch?C1AGF14_Recurse(c->t,c->n,c->frequency,root):c->frequency[root];
 delta*=n->rate;delta*=dt;
 if(!isfinite(delta)||delta>65535){c->ok=0;return 1;}
 if(delta==0)return 1;
 search.delta=(float)delta;phase=(double)search.delta+n->oldPhase;cycle=n->oldCycle;
 loop=branch?!(c->d[root].flags&2):c->m[root]->loop;
 if(loop){while(phase>=1){phase-=1;cycle=(cycle+1)&65535u;}}
 else if(phase>=1)phase=1;
 if((double)n->phase-phase>C1AGC14_Cycles(cycle,n->cycle))return 1;
 search.current=n->phase;search.old=n->oldPhase;search.next=(float)phase;search.cycle=n->cycle;search.oldCycle=n->oldCycle;
 return branch?C1AGC14_SyncSearch(c,root,&search):C1AGC14_NamedFraction(c,root,&search);
}
static int C1AGC14_Emit(c1agc14Context_t*c,unsigned i,unsigned leaf,unsigned payload){
 c1agc14Event_t*e;if(c->out->count==C1AGC14_MAX_EVENTS){c->ok=0;return 0;}
 e=c->out->event+c->out->count++;e->node=i;e->leaf=leaf;e->notify=c->f[i].token;e->payload=(uint16_t)payload;return 1;
}
static void C1AGC14_Notify(c1agc14Context_t*c,unsigned i,float old,float next){
 unsigned leaf,j,cursor;const c1anm14Metadata_t*m;
 if(!c->owner||!c->f[i].token||!c->ok)return;
 leaf=c->d[i].children?c->f[i].descendant:i;
 if(old==1){C1AGC14_Emit(c,i,0,0);return;}
 if(!leaf){if(next<old||next==1)C1AGC14_Emit(c,i,0,0);return;}
 m=c->m[leaf];cursor=(unsigned)c->f[i].cursor;
 if(cursor>=m->count){c->ok=0;return;}
 if(next<old){
  if(next>m->notes[cursor].phase){
   for(j=cursor;j<m->count&&next>m->notes[j].phase;j++)if(!C1AGC14_Emit(c,i,leaf,m->notes[j].token))return;
  }else if(old<=m->notes[cursor].phase){
   for(j=cursor;j<m->count;j++)if(!C1AGC14_Emit(c,i,leaf,m->notes[j].token))return;
   for(j=0;j<m->count&&next>m->notes[j].phase;j++)if(!C1AGC14_Emit(c,i,leaf,m->notes[j].token))return;
  }else return;
  c->f[i].cursor=(int16_t)C1ANC14_Cursor(m->notes,m->count,next);
 }else if(next==1){
  if(old<=m->notes[cursor].phase)for(j=cursor;j<m->count;j++)if(!C1AGC14_Emit(c,i,leaf,m->notes[j].token))return;
 }else if(next>m->notes[cursor].phase&&old<=m->notes[cursor].phase){
  for(j=cursor;j<m->count&&next>m->notes[j].phase;j++)if(!C1AGC14_Emit(c,i,leaf,m->notes[j].token))return;
  c->f[i].cursor=(int16_t)C1ANC14_Cursor(m->notes,m->count,next);
 }
}
static void C1AGC14_SyncNotify(c1agc14Context_t*c,unsigned root,float old,float next){
 unsigned child;C1AGC14_Notify(c,root,old,next);
 for(child=c->t->first[root];child!=C1AGF14_NONE;child=c->t->next[child])
  if(c->n[child].currentWeight>0&&c->n[child].goalWeight>0)C1AGC14_SyncNotify(c,child,old,next);
}
static void C1AGC14_SyncStore(c1agc14Context_t*c,unsigned root,float phase,unsigned cycle){
 unsigned child;c->n[root].phase=phase;c->n[root].cycle=cycle;
 for(child=c->t->first[root];child!=C1AGF14_NONE;child=c->t->next[child])C1AGC14_SyncStore(c,child,phase,cycle);
}
static void C1AGC14_AdvanceNode(c1agc14Context_t*c,unsigned root,float dt,int notify){
 c1atl14Node_t*n=c->n+root;double delta,phase;unsigned cycle,child;int branch=c->d[root].children!=0,loop;float rounded;
 if(!c->ok||!n->currentWeight)return;if(!n->goalWeight)notify=0;
 if(branch&&!(c->d[root].flags&3)){
  float scaled=(float)((double)dt*n->rate);if(!C1ATL14_Finite(scaled)){c->ok=0;return;}
  for(child=c->t->first[root];child!=C1AGF14_NONE;child=c->t->next[child])C1AGC14_AdvanceNode(c,child,scaled,notify);
  return;
 }
 delta=branch?C1AGF14_Recurse(c->t,c->n,c->frequency,root):c->frequency[root];delta*=n->rate;delta*=dt;
 if(!isfinite(delta)||delta>65535){c->ok=0;return;}
 phase=delta+n->oldPhase;cycle=n->oldCycle;loop=branch?!(c->d[root].flags&2):c->m[root]->loop;
 if(loop){while(phase>=1){phase-=1;cycle=(cycle+1)&65535u;}}
 else if(phase>=1)phase=1;else if(phase<=0)return;
 if((double)n->phase-phase>C1AGC14_Cycles(cycle,n->cycle))return;
 rounded=(float)phase;
 if(branch){if(notify)C1AGC14_SyncNotify(c,root,n->phase,rounded);C1AGC14_SyncStore(c,root,rounded,cycle);}
 else{if(notify)C1AGC14_Notify(c,root,n->phase,rounded);n->phase=rounded;n->cycle=cycle;}
}
/* A validation ledger belongs to one synchronous admission only. Nothing
 * is retained across clock steps, actor changes or VM callbacks. Each
 * source ordinal is still checked on first use in the original order. */
static int C1AGC46_MetadataOnce(const c1anm14Metadata_t*const*m,unsigned leaf,uint32_t*seen){
 uint32_t bit=(uint32_t)1u<<(leaf%32u);unsigned word=leaf/32u;
 if(!m[leaf])return 0;
 if(seen[word]&bit)return 1;
 if(!C1AGM14_Valid(m[leaf]))return 0;
 seen[word]|=bit;return 1;
}
static int C1AGC46_ValidateClock(const c1agc14Context_t*c,float dt,int notify){
 unsigned i,count=c->t->count;
 uint32_t seen[(C1AGF14_MAX+31u)/32u]={0};
 if(!C1AGT14_Valid(c->d,c->n,count)||!C1ATL14_Finite(dt)||dt<0||dt>60||(notify!=0&&notify!=1)||(c->owner!=0&&c->owner!=1))return 0;
 for(i=0;i<count;i++){
  unsigned leaf=c->d[i].children?c->f[i].descendant:i;
  if(c->n[i].rate<0||c->n[i].rate>100||!C1ATL14_Finite(c->frequency[i])||c->frequency[i]<0||c->frequency[i]>120||
     c->f[i].descendant>=count||c->f[i].cursor<-1||c->f[i].cursor>255)return 0;
  if(c->f[i].descendant&&(!c->d[i].children||c->d[leaf].children||!C1AGT14_Descendant(c->d,leaf,i)))return 0;
  if(!c->d[i].children&&c->n[i].currentWeight>0&&!C1AGC46_MetadataOnce(c->m,i,seen))return 0;
  if(c->f[i].token&&leaf){
   if(!C1AGC46_MetadataOnce(c->m,leaf,seen)||c->f[i].cursor>=(int)c->m[leaf]->count)return 0;
  }
 }
 return 1;
}
static int C1AGC14_Inner(c1agc14Context_t*c,float dt,int notify){
 float used=dt;
 if(!C1AGC46_ValidateClock(c,dt,notify))return 0;
 memset(c->out,0,sizeof(*c->out));c->ok=1;
 if(notify){double fraction=C1AGC14_Search(c,0,dt);
  if(!c->ok||!isfinite(fraction)||fraction<0)return 0;
  if(fraction!=1){double chosen=fraction*dt+(double)0.001f;if(chosen<=dt){used=(float)chosen;c->out->resume=1;}}
 }
 c->out->usedDt=used;C1AGC14_AdvanceNode(c,0,used,notify);return c->ok;
}
static int C1AGC14_AdvanceStorageIndexed(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,
 const c1agf14Topology_t*t,const c1anm14Metadata_t*const*m,const float*frequency,
 float dt,int owner,int notify,c1agc14Step_t*out,int disposable,const c1agc46Ranges_t*ranges){
 c1agc14Context_t c;c1agc14Step_t*step=NULL;c1atl14Node_t*stage=NULL;c1afl14State_t*flags=NULL;
 unsigned i,count;int ok=0,indexed;size_t nb,fb,db,mb,rb;
 if(!d||!n||!f||!t||!m||!frequency||!out||!t->count||t->count>C1AGF14_MAX)return 0;
 count=t->count;nb=count*sizeof(*n);fb=count*sizeof(*f);db=count*sizeof(*d);mb=count*sizeof(*m);rb=count*sizeof(*frequency);
 if(C1ANC14_Overlap(n,nb,f,fb)||C1ANC14_Overlap(n,nb,out,sizeof(*out))||C1ANC14_Overlap(f,fb,out,sizeof(*out)))return 0;
#define C1AGC14_RO(p,z) (C1ANC14_Overlap(n,nb,p,z)||C1ANC14_Overlap(f,fb,p,z)||C1ANC14_Overlap(out,sizeof(*out),p,z))
 if(C1AGC14_RO(d,db)||C1AGC14_RO(t,sizeof(*t))||C1AGC14_RO(m,mb)||C1AGC14_RO(frequency,rb))return 0;
 indexed=C1AGC46_Admit(ranges,m,count,n,nb,f,fb,out,sizeof(*out));
 if(!indexed)return 0;
 if(indexed<0)for(i=0;i<count;i++)if(m[i]&&C1AGC14_RO(m[i],sizeof(*m[i])))return 0;
#undef C1AGC14_RO
 if(disposable){stage=n;flags=f;step=out;}
 else {stage=malloc(nb);flags=malloc(fb);step=malloc(sizeof(*step));if(!stage||!flags||!step)goto done;
 memcpy(stage,n,nb);memcpy(flags,f,fb);}
 memset(&c,0,sizeof(c));c.d=d;c.t=t;c.m=m;c.frequency=frequency;c.n=stage;c.f=flags;c.out=step;c.owner=owner;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1AGC14_Inner(&c,dt,notify);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1AGC14_Inner(&c,dt,notify);
#endif
 if(ok&&!disposable){memcpy(n,stage,nb);memcpy(f,flags,fb);*out=*step;}
done:if(!disposable){free(stage);free(flags);free(step);}return ok;
}
static int C1AGC14_AdvanceStorage(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,
 const c1agf14Topology_t*t,const c1anm14Metadata_t*const*m,const float*frequency,
 float dt,int owner,int notify,c1agc14Step_t*out,int disposable){
 return C1AGC14_AdvanceStorageIndexed(d,n,f,t,m,frequency,dt,owner,notify,out,disposable,NULL);
}
static int C1AGC14_Advance(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,
 const c1agf14Topology_t*t,const c1anm14Metadata_t*const*m,const float*frequency,
 float dt,int owner,int notify,c1agc14Step_t*out){return C1AGC14_AdvanceStorage(d,n,f,t,m,frequency,dt,owner,notify,out,0);}
/* Writes may be partial on refusal: only pass an unpublished, disposable actor transaction. */
static int C1AGC14_AdvanceDisposable(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,
 const c1agf14Topology_t*t,const c1anm14Metadata_t*const*m,const float*frequency,
 float dt,int owner,int notify,c1agc14Step_t*out){return C1AGC14_AdvanceStorage(d,n,f,t,m,frequency,dt,owner,notify,out,1);}
/* Internal map-owned entry. All mutable state validation and native clock
 * math remain in the same kernel; only metadata alias admission is indexed. */
static int C1AGC14_AdvanceDisposablePrepared(const c1agt14Definition_t*d,c1atl14Node_t*n,c1afl14State_t*f,
 const c1agf14Topology_t*t,const c1anm14Metadata_t*const*m,const float*frequency,
 float dt,int owner,int notify,c1agc14Step_t*out,const c1agc46Ranges_t*ranges){
 return C1AGC14_AdvanceStorageIndexed(d,n,f,t,m,frequency,dt,owner,notify,out,1,ranges);
}
#endif
