/* Private storage policy only. Slots never move relative to their indices,
 * stale owners are never reclaimed, and capacity is not native pool parity. */
#include "gsc_vm_state_bound55.inc"
static uint32_t state_fields_limit(const GscRuntime*r){return r->state_field_limit?r->state_field_limit:GSC_STATE_FIELD_CAPACITY;}
static size_t state_fields_used(const GscRuntime*r){int n;size_t used=0;for(n=0;n<r->state_field_capacity;n++)used+=r->state_fields[n].used!=0;return used;}
/* Count only the free slots needed by the caller. Search the unused tail
 * first; no dense-prefix assumption, persistent count or allocation. */
static size_t state_fields_unused_up_to(const GscRuntime*r,size_t wanted){
 int n;size_t unused=0;
 for(n=r->state_field_capacity;n>0&&unused<wanted;){--n;unused+=r->state_fields[n].used==0;}
 return unused;
}
static int state_fields_can_fit(const GscRuntime*r,size_t empty){
 size_t used,limit=state_fields_limit(r),spare;
 if(r->state_field_capacity>=0&&(size_t)r->state_field_capacity<=limit&&
    (!r->state_field_capacity||r->state_fields)){
  if(empty>limit)return 0;
  spare=limit-(size_t)r->state_field_capacity;
  if(empty<=spare)return 1;
  return state_fields_unused_up_to(r,empty-spare)==empty-spare;
 }
 used=state_fields_used(r);return used<=limit&&empty<=limit-used;
}
/* Allocator hooks receive no VM loan. Guard reentry/destruction just like
 * foreign certificate boundaries, including callbacks in failure/free hooks. */
static void*state_storage_allocate(GscRuntime*r,size_t count,size_t width,int line){
 const GscProgram*program;StateField*fields;uint64_t instance;uint32_t limit;int capacity,ok;void*p;
 if(!r||r->error[0])return NULL;
 if(r->state_arena_busy||r->entity_certificate_busy){rfail(r,line,"state arena allocation reentered");return NULL;}
 if(!count||!width||count>SIZE_MAX/width){rfail(r,line,"state arena allocation size overflow");return NULL;}
 program=r->program;fields=r->state_fields;capacity=r->state_field_capacity;limit=r->state_field_limit;instance=r->array_token;
 r->state_arena_busy=1;r->entity_certificate_busy=1;p=GSC_RUNTIME_CALLOC(count,width);
 ok=r->state_arena_busy==1&&r->entity_certificate_busy==1&&r->program==program&&r->array_token==instance&&
  r->state_fields==fields&&r->state_field_capacity==capacity&&r->state_field_limit==limit&&!r->error[0];
 if(!ok||!p){if(p)GSC_RUNTIME_FREE(p);p=NULL;if(!r->error[0])rfail(r,line,ok?"state arena allocation failed":"state arena allocation context changed");}
 r->entity_certificate_busy=0;r->state_arena_busy=0;return p;
}
static int state_storage_free(GscRuntime*r,void*p){int ok;if(!p)return 1;
 if(r->state_arena_busy||r->entity_certificate_busy)return rfail(r,0,"state arena free reentered");
 r->state_arena_busy=1;r->entity_certificate_busy=1;GSC_RUNTIME_FREE(p);
 ok=r->state_arena_busy==1&&r->entity_certificate_busy==1&&!r->error[0];r->entity_certificate_busy=0;r->state_arena_busy=0;return ok;
}
static int state_fields_reserve_free(GscRuntime*r,size_t empty,int line){
 StateField*candidate,*old;size_t used,needed,capacity,limit,unused;
 if(!r||r->error[0])return 0;
 if(r->state_arena_busy)return rfail(r,line,"state arena reserve reentered");
 if(r->state_field_capacity<0||r->state_field_capacity>GSC_STATE_FIELD_OWNED_LIMIT||
  (r->state_field_capacity&&!r->state_fields))return rfail(r,line,"state arena ownership invalid");
 limit=state_fields_limit(r);
 if((size_t)r->state_field_capacity<=limit&&empty<=limit){
  unused=state_fields_unused_up_to(r,empty);
  if(unused==empty)return 1;
  /* The search exhausted the arena, so this is the exact used count. */
  used=(size_t)r->state_field_capacity-unused;
 }else used=state_fields_used(r);
 if(used>limit||empty>limit-used)return rfail(r,line,"state field limit");
 needed=used+empty;if(needed<=(size_t)r->state_field_capacity)return 1;
 capacity=r->state_field_capacity?(size_t)r->state_field_capacity:GSC_STATE_FIELD_CAPACITY;
 while(capacity<needed){if(capacity>limit/2){capacity=limit;break;}capacity*=2;}
 if(capacity<needed||capacity>limit||capacity>INT_MAX||capacity>SIZE_MAX/sizeof(*candidate))return rfail(r,line,"state arena capacity overflow");
 candidate=state_storage_allocate(r,capacity,sizeof(*candidate),line);if(!candidate)return 0;
 if(r->state_field_capacity)memcpy(candidate,r->state_fields,(size_t)r->state_field_capacity*sizeof(*candidate));
 old=r->state_fields;
 if(!r->state_field_capacity)r->state_field_scan_end_plus1=1;
 else if(r->state_field_scan_end_plus1<0||r->state_field_scan_end_plus1-1>r->state_field_capacity)r->state_field_scan_end_plus1=0;
 r->state_fields=candidate;r->state_field_capacity=(int)capacity;
 /* Capacity may remain enlarged after a failing free hook. All old records
  * are retained; no new used field/array value has been published. */
 if(!state_storage_free(r,old))return 0;return !r->error[0];
}
static void state_fields_release(GscRuntime*r){StateField*old=r->state_fields;r->state_fields=NULL;r->state_field_capacity=0;r->state_field_scan_end_plus1=0;if(old)(void)state_storage_free(r,old);}
int gsc_runtime_select_owned_state_arena(GscRuntime*r,uint32_t limit){
 if(!r)return 0;if(r->state_arena_busy)return rfail(r,0,"state arena selection reentered");
 if(!r->program||!r->state_enabled||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
  r->entity_sequence_busy||r->entity_prepare_busy||r->entity_certificate_busy||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy||r->state_scratch||
  limit<GSC_STATE_FIELD_CAPACITY||limit>GSC_STATE_FIELD_OWNED_LIMIT||limit>SIZE_MAX/sizeof(*r->state_fields)||
  (r->state_field_limit&&r->state_field_limit!=limit)||(uint32_t)r->state_field_capacity>limit)return 0;
 r->state_field_limit=limit;return 1;
}
