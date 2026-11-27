/* One callback-free view of the same queue root. Never retain this view
 * across dispatch, host callbacks or allocation. Physical membership and
 * selected-lane checks remain the original time-root predicates. */
typedef struct {
 uint64_t stamp,next;
 int required,bucket;
} R237QueueView39;
static int r237_queue_view39(const GscRuntime *r,R237QueueView39 *v){
 R243TimeRoot *a;
 if(!r)return 0;
 a=r243_time_root_find(r);v->required=r243_time_root_required_found(r,a);
 if(!v->required){v->stamp=0;v->next=r->r237_next_insertion;v->bucket=r->r237_vm_bucket_ms;return 1;}
 if(!r243_time_root_active_found(r,a))return 0;
 v->stamp=a->incarnation;v->next=a->next_insertion;v->bucket=a->bucket;return 1;
}
static int r237_queue_available(GscRuntime *r,int count,int line){
 R237QueueView39 v;
 if(!r237_queue_view39(r,&v))return rfail(r,line,"R237 required time-root unavailable");
 if(count<0||!v.next||v.bucket<0||v.bucket>INT32_C(0xffffff)||
 (uint64_t)count>UINT64_MAX-v.next)return rfail(r,line,"R237 bucket insertion identity/domain exhausted");
 return 1;
}
static int r237_queue_offer(GscRuntime *r,Task *t,int key,int line){
 R237QueueView39 v;uint64_t insertion;int member;
 if(!r237_queue_view39(r,&v))return rfail(r,line,"R237 queue time-root retired");
 member=!v.required;
 if(!member)member=task_member(r,t);
 if(!member)return rfail(r,line,"R237 queue Task belongs to another namespace");
 if(key<v.bucket||key>INT32_C(0xffffff))return rfail(r,line,"R237 queue key outside nonwrapping cursor domain");
 if(t->r237_enqueued){
  /* Native GetVariable(existing key) does not relink the child. */
  if(t->r243_time_root_incarnation!=v.stamp||t->wake_ms!=key||!t->r237_insertion)return rfail(r,line,"R237 task already owns another queue key");
  return 1;
 }
 /* Recheck availability and claim through the original commit gate. */
 if(!r237_queue_available(r,1,line))return 0;
 insertion=r237_insertion_claim(r,line);if(!insertion)return 0;
 t->wake_ms=key;t->r237_insertion=insertion;t->r243_time_root_incarnation=v.stamp;t->r237_enqueued=1;return 1;
}
