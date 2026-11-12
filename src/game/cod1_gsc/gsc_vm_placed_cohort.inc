/* A same-allocation continuation witness, not a serialization or a scheduler.
 * Only real canonical INIT/pop + released entry + synchronous native return
 * can publish it. Generic entry, factory and final nochild gates stay separate. */
#include "gsc_postcohort_private.h"
#include "gsc_vm_cohort_storage.inc"
enum {PLACED_COHORT_READY=1,PLACED_COHORT_AITYPED,PLACED_COHORT_BUSY,PLACED_COHORT_FAILED,PLACED_COHORT_POST,PLACED_COHORT_LATE,PLACED_COHORT_LATE_DONE};
typedef struct {int slot;Task*allocation;unsigned char*task_bytes;unsigned char*results_bytes;} PlacedCohortMember;
struct PlacedCohortWitness {
 int phase,count,checking,now,bucket,cadence,frame_open,frame_target,start_draining,frame_budget,late_call;
 uint32_t row;uint64_t owner,instance,program_id,catalog,next_serial,insertion,frame_generation,time_root;
 const GscProgram*program;GscMapExecutionCertificate map,anim_map;
 GscEntityRef entity;GscEntityExecutionCertificate certificate;
 uint64_t aitype_serial;PlacedCohortMember*members;PlacedCohortWitness*pending;PlacedCohortStorage*storage;
};
struct PlacedCohortPost {
 GscRuntime*r;PlacedCohortWitness*prior,*candidate;int active,checking;
 const GscPlacedInitRun*run;GscPlacedInitRun seal;
 GscEntityExecutionCertificate certificate;GscPlacedCohortPostHost host;
 const GscPlacedCohortPostHost*input;const void*authority;
};
static void placed_cohort_free(GscRuntime*r,PlacedCohortWitness*w){int n;if(!w)return;
 placed_cohort_free(r,w->pending);w->pending=NULL;cohort_storage_free(r,w->storage);w->storage=NULL;
 for(n=0;n<w->count;n++){GSC_RUNTIME_FREE(w->members[n].task_bytes);GSC_RUNTIME_FREE(w->members[n].results_bytes);}
 GSC_RUNTIME_FREE(w->members);GSC_RUNTIME_FREE(w);
}
static int placed_cohort_destructing(GscRuntime*r){
 if(r->placed_cohort_post||(r->placed_cohort&&(r->placed_cohort->phase==PLACED_COHORT_BUSY||r->placed_cohort->phase==PLACED_COHORT_LATE))){
  rfail(r,0,"placed cohort runtime destruction during transaction");return 1;}return 0;
}
static int placed_cohort_witness_private(const PlacedCohortWitness*w,const void*p,size_t size){int n;
 if(!w)return 0;if(cohort_storage_private(w->storage,p,size)||Cod1ModelStringOverlap(p,size,w,sizeof(*w))||
  Cod1ModelStringOverlap(p,size,w->members,(size_t)w->count*sizeof(*w->members)))return 1;
 for(n=0;n<w->count;n++)if(Cod1ModelStringOverlap(p,size,w->members[n].allocation,sizeof(Task))||
  (w->members[n].task_bytes&&((const Task*)w->members[n].task_bytes)->results&&Cod1ModelStringOverlap(p,size,((const Task*)w->members[n].task_bytes)->results,sizeof(ResultsTask)))||
  Cod1ModelStringOverlap(p,size,w->members[n].task_bytes,sizeof(Task))||
  (w->members[n].results_bytes&&Cod1ModelStringOverlap(p,size,w->members[n].results_bytes,sizeof(ResultsTask))))return 1;
 return placed_cohort_witness_private(w->pending,p,size);
}
static int placed_cohort_private(const GscRuntime*r,const void*p,size_t size){return r&&
 (placed_cohort_witness_private(r->placed_cohort,p,size)||
  (r->placed_cohort_post&&Cod1ModelStringOverlap(p,size,r->placed_cohort_post,sizeof(*r->placed_cohort_post))));}
static int placed_cohort_quiet(GscRuntime*r){return r&&r->started&&!r->error[0]&&!r->bsp_construction_only&&r->lifetime_enabled&&
 !r->advancing&&!r->executing_task&&!r->entity_entry_depth&&!r->execution_budget&&!r->entity_sequence_busy&&
 !r->entity_prepare_busy&&!r->entity_certificate_busy&&!r->entity_inheritance_busy&&!r->entity_field_busy&&!r->entity_method_busy&&
 !r->r243_controller_enabled&&!r->placed_return&&!r->placed_init_initial_scope;}
static void placed_cohort_clock(GscRuntime*r,PlacedCohortWitness*w){
 w->instance=r->array_token;w->program=r->program;w->program_id=gsc_runtime_program_identity(r);w->catalog=r->animation_namespace.identity;
 w->now=r->now_ms;w->bucket=r237_bucket(r);w->cadence=r->cadence_drain_ms;w->next_serial=r->next_serial;
 w->insertion=r->r237_next_insertion;w->frame_generation=r->r237_frame_generation;w->time_root=r->r237_frame_time_root;
 w->frame_open=r->r237_frame_open;w->frame_target=r->r237_frame_target_ms;w->start_draining=r->r237_start_draining;w->frame_budget=r->r237_frame_budget;
 w->anim_map=r->anim_data;
}
/* No foreign callbacks here. Compare Task first; a retired task may have freed
 * its Results allocation. Never dereference a saved Results pointer. */
static int placed_cohort_same(GscRuntime*r,const PlacedCohortWitness*w,int extras,int births){int n,k,live=0;
 if(!r||!w||r->error[0]||(w->storage&&!cohort_storage_same(r,w->storage))||r->array_token!=w->instance||r->program!=w->program||gsc_runtime_program_identity(r)!=w->program_id||
  r->animation_namespace.identity!=w->catalog||r->now_ms!=w->now||r237_bucket(r)!=w->bucket||r->cadence_drain_ms!=w->cadence||
  r->r237_frame_generation!=w->frame_generation||r->r237_frame_time_root!=w->time_root||r->r237_frame_open!=w->frame_open||
  r->r237_frame_target_ms!=w->frame_target||r->r237_start_draining!=w->start_draining||r->r237_frame_budget!=w->frame_budget||
  memcmp(&r->anim_data,&w->anim_map,sizeof(w->anim_map))||
  (births&&(r->next_serial!=w->next_serial||r->r237_next_insertion!=w->insertion))||
  r->task_count<0||r->task_count>GSC_LIFETIME_TASK_CAPACITY)return 0;
 for(n=0;n<w->count;n++){const PlacedCohortMember*m=&w->members[n];Task*t;
  if(m->slot<0||m->slot>=r->task_count||(t=task_at(r,m->slot))!=m->allocation||!t||
   memcmp(t,m->task_bytes,sizeof(*t))||t->status==TASK_DEAD)return 0;
  if(t->results&&(!m->results_bytes||memcmp(t->results,m->results_bytes,sizeof(*t->results))))return 0;
 }
 for(n=0;n<r->task_count;n++){Task*t=task_at(r,n);if(!t)return 0;if(t->status==TASK_DEAD)continue;live++;
  for(k=0;k<w->count;k++)if(w->members[k].slot==n&&w->members[k].allocation==t)break;
  if(k==w->count&&(!extras||t->serial<w->next_serial))return 0;
 }
 return extras?live>=w->count:live==w->count;
}
static PlacedCohortWitness*placed_cohort_snapshot(GscRuntime*r,uint64_t owner,uint32_t row,const GscEntityExecutionCertificate*c,const GscMapExecutionCertificate*map){
 PlacedCohortWitness*w;int n,k=0,count=gsc_active_threads(r);
 if(count<0||count>GSC_LIFETIME_TASK_CAPACITY||r->next_serial==UINT64_MAX)return NULL;
 w=GSC_RUNTIME_CALLOC(1,sizeof(*w));if(!w)return NULL;w->count=count;w->owner=owner;w->row=row;w->entity=c->owner;w->certificate=*c;w->map=*map;
 placed_cohort_clock(r,w);
 if(count){w->members=GSC_RUNTIME_CALLOC((size_t)count,sizeof(*w->members));if(!w->members){w->count=0;placed_cohort_free(r,w);return NULL;}}
 for(n=0;n<r->task_count;n++){Task*t=task_at(r,n);PlacedCohortMember*m;if(t->status==TASK_DEAD)continue;
  if(k>=count)goto fail;m=&w->members[k++];m->slot=n;m->allocation=t;m->task_bytes=GSC_RUNTIME_CALLOC(1,sizeof(*t));if(!m->task_bytes)goto fail;
  memcpy(m->task_bytes,t,sizeof(*t));if(t->results){m->results_bytes=GSC_RUNTIME_CALLOC(1,sizeof(*t->results));if(!m->results_bytes)goto fail;memcpy(m->results_bytes,t->results,sizeof(*t->results));}
 }
 if(k!=count||!placed_cohort_same(r,w,0,1))goto fail;return w;
fail:placed_cohort_free(r,w);return NULL;
}
/* Every retained allocation remains live, even when suspended task bytes did
 * not change. Foreign validation is fenced by the complete same-allocation
 * witness; retirement is terminal, never an expected-set refresh. */
static int placed_cohort_liveness(GscRuntime*r,PlacedCohortWitness*w,int extras,int births){int n,k,ok=1,seen;uint64_t serial,insertion;
 if(!placed_cohort_same(r,w,extras,births))return 0;
 if(w->checking||r->entity_certificate_busy)return rfail(r,0,"placed cohort allocation query reentered");
 w->checking=1;serial=r->next_serial;insertion=r->r237_next_insertion;
 r->entity_certificate_busy=1;ok=w->map.current&&w->map.current(w->map.receipt)==1&&r->entity_certificate_busy==1;r->entity_certificate_busy=0;
 ok=ok&&placed_cohort_same(r,w,extras,births)&&r->next_serial==serial&&r->r237_next_insertion==insertion;
 /* Equal map/allocation certificates name the same owner, regardless of how
  * many suspended children retain them. Validate every DISTINCT authority,
  * with the whole task witness fenced around each foreign invocation. */
 for(n=0;ok&&n<w->count;n++){const Task*saved=(const Task*)w->members[n].task_bytes;GscEntityExecutionCertificate cert=saved->certificate,seal=cert;GscMapExecutionCertificate map=saved->map_certificate;
  seen=!memcmp(&map,&w->map,sizeof(map));
  for(k=0;!seen&&k<n;k++){const Task*old=(const Task*)w->members[k].task_bytes;seen=!memcmp(&map,&old->map_certificate,sizeof(map));}
  if(map.receipt&&!seen){r->entity_certificate_busy=1;ok=map.current&&map.current(map.receipt)==1&&r->entity_certificate_busy==1;r->entity_certificate_busy=0;
   ok=ok&&placed_cohort_same(r,w,extras,births)&&r->next_serial==serial&&r->r237_next_insertion==insertion;}
  seen=0;for(k=0;!seen&&k<n;k++){const Task*old=(const Task*)w->members[k].task_bytes;seen=!memcmp(&cert,&old->certificate,sizeof(cert));}
  if(ok&&cert.serial&&!seen){r->entity_certificate_busy=1;ok=cert.current&&cert.current(&cert.owner,cert.serial)==1&&r->entity_certificate_busy==1;r->entity_certificate_busy=0;
   ok=ok&&!memcmp(&cert,&seal,sizeof(cert))&&placed_cohort_same(r,w,extras,births)&&r->next_serial==serial&&r->r237_next_insertion==insertion;}
 }
 w->checking=0;if(!ok)return rfail(r,0,"placed cohort retained allocation retired or callback mutated tasks");return placed_cohort_same(r,w,extras,births);
}
static int placed_cohort_buffers(GscRuntime*r,const void**p,const size_t*s,int count){int i,j;
 for(i=0;i<count;i++){if(s[i]&&(!p[i]||!gsc_runtime_storage_separate(r,p[i],s[i])))return 0;
  for(j=0;j<i;j++)if(Cod1ModelStringOverlap(p[i],s[i],p[j],s[j]))return 0;}return 1;
}
static int placed_cohort_fail(GscRuntime*r,char*error,size_t cap,const char*why){
 if(r&&r->placed_cohort)r->placed_cohort->phase=PLACED_COHORT_FAILED;
 if(r&&!r->error[0])rfail(r,0,"%s",why);
 return inherited_entry_error(error,cap,r&&r->error[0]?r->error:why);
}
int gsc_placed_cohort_current(GscRuntime*r,uint64_t owner){PlacedCohortWitness*w=r?r->placed_cohort:NULL;
 if(!w||!owner||owner!=w->owner)return 0;
 if(w->phase==PLACED_COHORT_POST)return 0;
 if(w->phase==PLACED_COHORT_BUSY)return w->pending?placed_cohort_same(r,w->pending,0,1):placed_cohort_same(r,w,1,0);
 return (w->phase==PLACED_COHORT_READY||w->phase==PLACED_COHORT_AITYPED||w->phase==PLACED_COHORT_LATE||w->phase==PLACED_COHORT_LATE_DONE)&&placed_cohort_quiet(r)&&placed_cohort_same(r,w,0,1);
}
int gsc_execute_placed_cohort_aitype(GscRuntime*r,const void*authorityGrant,uint64_t owner,uint32_t row,const GscEntityScriptEntry*input,const GscEntityExecutionCertificate*authority,GscEntityScriptRun*out,char*error,size_t cap){
 PlacedCohortWitness*w=r?r->placed_cohort:NULL;GscEntityScriptEntry entry;GscEntityExecutionCertificate cert;GscEntityScriptRun result;uint64_t birth;int ok;
 const void*p[]={input,authority,out,error};size_t sizes[]={sizeof(*input),sizeof(*authority),sizeof(*out),cap};
 if(!r||!input||!authority||!out||!placed_cohort_buffers(r,p,sizes,4))return 0;
 if(!w||w->phase!=PLACED_COHORT_READY||w->owner!=owner||w->row==UINT32_MAX||row!=w->row+1||
  !placed_cohort_quiet(r)||!placed_cohort_same(r,w,0,1)||!w->next_serial||w->next_serial>=UINT64_MAX-1)
  return placed_cohort_fail(r,error,cap,"next placed aitype lacks exact prior task witness");
 if(!gsc_session_placed_cohort_admit(authorityGrant,r,owner,row,2,input,authority,NULL,NULL,NULL)||!placed_cohort_same(r,w,0,1))
  return placed_cohort_fail(r,error,cap,"next placed aitype has no actual Session row grant");
 entry=*input;cert=*authority;birth=w->next_serial;
 if(!cert.serial||!cert.current||!state_ref_equal(&entry.entity,&cert.owner)||state_ref_equal(&entry.entity,&w->entity))return placed_cohort_fail(r,error,cap,"next placed aitype allocation invalid");
 w->phase=PLACED_COHORT_BUSY;if(!placed_cohort_liveness(r,w,0,1))return placed_cohort_fail(r,error,cap,"prior placed task owner retired");memset(&result,0,sizeof(result));
 ok=execute_entity_values_owned_map_r243(r,&entry,&cert,&w->map,NULL,0,&result,NULL,NULL);*out=result;
 if(!ok||result.status!=GSC_ENTITY_ENTRY_RETURNED||result.task_serial!=birth||r->next_serial!=birth+1||
  !placed_cohort_quiet(r)||!placed_cohort_same(r,w,0,0)||!placed_cohort_liveness(r,w,0,0)||
  !gsc_session_placed_cohort_scope(authorityGrant,r,owner,row,2)||r->next_serial!=birth+1||r->r237_next_insertion!=w->insertion||
  memcmp(input,&entry,sizeof(entry))||memcmp(authority,&cert,sizeof(cert)))
  return placed_cohort_fail(r,error,cap,"next placed aitype changed retained tasks or birth counter");
 w->next_serial=r->next_serial;w->row=row;w->entity=entry.entity;w->certificate=cert;w->aitype_serial=result.task_serial;w->phase=PLACED_COHORT_AITYPED;return 1;
}
static int placed_cohort_post_scope(PlacedCohortPost*l){GscRuntime*r=l?l->r:NULL;return r&&l->active&&r->placed_cohort_post==l&&
 r->placed_cohort==l->prior&&l->prior->phase==PLACED_COHORT_BUSY&&l->prior->pending==l->candidate&&
 l->candidate->phase==PLACED_COHORT_POST&&gsc_session_placed_cohort_scope(l->authority,r,l->candidate->owner,l->candidate->row,1)&&placed_cohort_quiet(r)&&placed_cohort_same(r,l->candidate,0,1)&&
 l->run&&!memcmp(l->run,&l->seal,sizeof(l->seal))&&l->input&&!memcmp(l->input,&l->host,sizeof(l->host));}
static int placed_cohort_post_current(void*ctx){PlacedCohortPost*l=ctx;int ok,n,covered=0;
 if(!placed_cohort_post_scope(l))return 0;if(l->checking)return rfail(l->r,0,"placed cohort post-entry current reentered");l->checking=1;
 ok=placed_cohort_liveness(l->r,l->candidate,0,1)&&placed_cohort_post_scope(l);
 for(n=0;ok&&n<l->candidate->count;n++){const Task*saved=(const Task*)l->candidate->members[n].task_bytes;
  if(!memcmp(&saved->certificate,&l->certificate,sizeof(l->certificate))){covered=1;break;}}
 if(ok&&!covered){GscEntityExecutionCertificate q=l->certificate;l->r->entity_certificate_busy=1;
  ok=q.current&&q.current(&q.owner,q.serial)==1&&l->r->entity_certificate_busy==1;l->r->entity_certificate_busy=0;
  ok=ok&&!memcmp(&q,&l->certificate,sizeof(q))&&placed_cohort_post_scope(l);}
 l->checking=0;return ok&&placed_cohort_post_scope(l);
}
int gsc_execute_placed_cohort_init(GscRuntime*r,const void*authorityGrant,uint64_t owner,uint32_t row,const GscEntityRef*input,const GscEntityExecutionCertificate*authority,const GscMapExecutionCertificate*mapInput,const GscPlacedInitReturnHost*popHost,const GscPlacedCohortPostHost*postHost,GscPlacedInitRun*out,char*error,size_t cap){
 PlacedCohortWitness*w=r?r->placed_cohort:NULL,*candidate=NULL;PlacedCohortPost post;GscEntityRef entity;GscEntityExecutionCertificate cert;GscMapExecutionCertificate map;
 GscPlacedInitReturnHost popCopy;GscPlacedCohortPostHost postCopy;GscPlacedInitRun result;int ok;
 const void*p[]={input,authority,mapInput,popHost,postHost,out,error};size_t sizes[]={sizeof(*input),sizeof(*authority),sizeof(*mapInput),sizeof(*popHost),sizeof(*postHost),sizeof(*out),cap};
 if(!r||!owner||!input||!authority||!mapInput||!popHost||!postHost||!out||!placed_cohort_buffers(r,p,sizes,7))return 0;
 if(!placed_cohort_quiet(r)||r->placed_cohort_post||!popHost->invoke||!postHost->invoke||!mapInput->receipt||!mapInput->current||
  !authority->serial||!authority->current||!state_ref_equal(input,&authority->owner))return placed_cohort_fail(r,error,cap,"placed cohort INIT scope or providers invalid");
 entity=*input;cert=*authority;map=*mapInput;popCopy=*popHost;postCopy=*postHost;memset(&result,0,sizeof(result));*out=result;
 if(!w){
  if(row||gsc_active_threads(r))return placed_cohort_fail(r,error,cap,"first cohort INIT must precede retained children");
  w=placed_cohort_snapshot(r,owner,row,&cert,&map);if(!w)return placed_cohort_fail(r,error,cap,"placed cohort initial witness allocation failed");r->placed_cohort=w;
 }else if(w->phase!=PLACED_COHORT_AITYPED||w->owner!=owner||w->row!=row||!w->aitype_serial||
  !state_ref_equal(&w->entity,&entity)||memcmp(&w->certificate,&cert,sizeof(cert))||memcmp(&w->map,&map,sizeof(map))||!placed_cohort_same(r,w,0,1))
  return placed_cohort_fail(r,error,cap,"placed cohort INIT lacks its actual ordered aitype");
 if(!gsc_session_placed_cohort_admit(authorityGrant,r,owner,row,1,NULL,&cert,&map,popHost,postHost))return placed_cohort_fail(r,error,cap,"cohort INIT has no actual Session native grant");
 w->phase=PLACED_COHORT_BUSY;
 if(!placed_cohort_liveness(r,w,0,1))return placed_cohort_fail(r,error,cap,"placed INIT prior allocation retired");
 ok=execute_placed_init_selected(r,&entity,&cert,&map,&popCopy,1,&result,error,cap);*out=result;
 if(!ok||!result.root_pop_observed||!result.entry_scope_released||result.entry.status!=GSC_ENTITY_ENTRY_RETURNED||
  result.entry.task_serial!=w->next_serial||r->next_serial<=w->next_serial||!placed_cohort_quiet(r)||!placed_cohort_same(r,w,1,0)||!placed_cohort_liveness(r,w,1,0))
  return placed_cohort_fail(r,error,cap,"placed cohort INIT lost prior task continuation or genuine pop");
 candidate=placed_cohort_snapshot(r,owner,row,&cert,&map);if(!candidate)return placed_cohort_fail(r,error,cap,"placed cohort extension allocation failed");
 candidate->phase=PLACED_COHORT_POST;w->pending=candidate;
 memset(&post,0,sizeof(post));post.r=r;post.prior=w;post.candidate=candidate;post.active=1;post.run=&result;post.seal=result;post.certificate=cert;post.host=postCopy;post.input=postHost;post.authority=authorityGrant;r->placed_cohort_post=&post;
 ok=placed_cohort_post_current(&post);
 if(ok)ok=postCopy.invoke(postCopy.user,&result,error,cap,placed_cohort_post_current,&post);
 if(error&&cap)error[cap-1]=0;
 if(!placed_cohort_post_current(&post)||memcmp(input,&entity,sizeof(entity))||memcmp(authority,&cert,sizeof(cert))||
  memcmp(mapInput,&map,sizeof(map))||memcmp(popHost,&popCopy,sizeof(popCopy))||memcmp(postHost,&postCopy,sizeof(postCopy)))ok=0;
 post.active=0;r->placed_cohort_post=NULL;w->pending=NULL;
 if(ok!=1){placed_cohort_free(r,candidate);return placed_cohort_fail(r,error,cap,"placed cohort native return or post-entry owner failed");}
 candidate->phase=PLACED_COHORT_READY;r->placed_cohort=candidate;placed_cohort_free(r,w);
 result.currentstate_completed=1;*out=result;return 1;
}

#include "gsc_vm_postcohort.inc"
