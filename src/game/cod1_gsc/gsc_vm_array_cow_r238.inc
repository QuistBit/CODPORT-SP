/* Original 4603C0/45F1D0: indexed lvalue detaches a referenced array by
 * recursively copying every nested array, including sibling/DAG occurrences.
 * Values denoting nonarray engine/script objects retain their identity. Native
 * proof: work/r237-native-array-cow/NATIVE-COW-RESULT.json. This port keeps
 * bounded arenas and performs a callback-free staged commit; it does not add
 * native-field lvalues, undefined deletion, or unbounded native heap parity. */
int gsc_program_select_r238_array_value_cow(GscProgram*g){
 if(!g||g->invalid||g->linked||g->r238_array_value_cow_selected)return 0;
 g->r238_array_value_cow_selected=1;return 1;
}
int gsc_runtime_select_r238_array_value_cow(GscRuntime*r){
 if(!r||!r->program||!r->program->r238_array_value_cow_selected||
    r->r238_array_value_cow_enabled||!r->arrays_enabled||!r->sparse_numeric_arrays_enabled||
    r->started||r->advancing||r->executing_task||r->task_count||r->array_count||
    r->array_entry_count||r->error[0]||r->entity_entry_depth||r->hud_elements_busy||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
    r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
 r->r238_array_value_cow_enabled=1;return 1;
}
int gsc_runtime_r238_array_value_cow_selected(const GscRuntime*r){
 return r&&r->program&&r->program->r238_array_value_cow_selected&&
        r->r238_array_value_cow_enabled&&!r->error[0];
}
typedef struct {
 int arrays,entries;
 ArrayContainer containers[GSC_ARRAY_CONTAINER_CAPACITY];
 ArrayEntry members[GSC_ARRAY_ENTRY_CAPACITY];
} R238ArrayStage;
static int r238_array_value_valid(const GscRuntime*r,const Value*v){
 return r&&v&&v->kind==V_ARRAY&&r238_array_reference_valid(r,v->array,v->object_token);
}
static int r238_cow_same(const Value*a,const Value*b){
 return a->kind==b->kind&&(a->kind==V_UNDEFINED||
        (a->kind==V_ARRAY&&a->array==b->array&&a->object_token==b->object_token));
}
static int r238_cow_same_ancestor(const Value *value,const ResultsAncestor *ref) {
    return results_same_ancestor(value,ref) &&
        (value->kind!=V_ARRAY || value->object_token==ref->object_token);
}
/* Results guards also run before expression resumption. Repeat only their
 * captured-array comparison after the final foreign validation, with no host
 * calls: an adversarial callback cannot replace a nested cell underneath us.
 * Direct C evaluation does not own these continuation captures; public hosts
 * have no capability to modify its private arrays or reenter the evaluator. */
static int r238_cow_captured_ancestors(GscRuntime*r,const Task*task,const ArrayLvalue*path,int line){
 int frame,k;const ResultsTask*s=task->results;
 if(!s)return 1;
 for(frame=0;frame<task->depth;frame++){
  const ResultsPending*p=s->pending+frame;Value current=path->base;
  if(!p->active||&p->path!=path)continue;
  if(!p->path_ready||p->key_index!=path->count)return rfail(r,line,"R238 array COW capture incomplete");
  for(k=0;k<path->count-1;k++){
   Value next;memset(&next,0,sizeof(next));
   if(current.kind==V_ARRAY&&!array_read_key(r,current.array,path->keys+k,line,&next))return 0;
   if(!r238_cow_same_ancestor(&next,p->ancestors+k))return rfail(r,line,"R238 array COW captured ancestor changed during validation");
   current=next;
  }
  return 1;
 }
 return rfail(r,line,"R238 array COW pending capture unavailable");
}
/* Refcounts concern reachable owning values, not garbage still present in the
 * bounded arena. The current indexed-lvalue captures borrow their destination;
 * they are retained for GC, but are not extra native value copies. */
static int r238_cow_references_reference(GscRuntime*r,const ArrayLvalue*path,int*refs,int line){
 ArrayCollection c;int n,k,f,j;Task*t;ResultsTask*s;
 int first[GSC_ARRAY_CONTAINER_CAPACITY],last[GSC_ARRAY_CONTAINER_CAPACITY],next[GSC_ARRAY_ENTRY_CAPACITY];
 if(r->array_count<0||r->array_count>GSC_ARRAY_CONTAINER_CAPACITY||
    r->array_entry_count<0||r->array_entry_count>GSC_ARRAY_ENTRY_CAPACITY)
  return rfail(r,line,"R238 array COW arena invalid");
 /* Build an operation-local directory in the same ascending entry order.
  * All owner checks still precede root traversal, even for unreachable data.
  * From here through the reference count there are no callbacks or mutation;
  * visiting the matching entries once replaces repeated whole-arena scans. */
 memset(first,0xff,sizeof(first));memset(last,0xff,sizeof(last));
 for(n=0;n<r->array_entry_count;n++){
  int owner=r->array_entries[n].owner;
  if(owner<1||owner>r->array_count)return rfail(r,line,"R238 array COW entry owner invalid");
  owner--;next[n]=-1;if(last[owner]>=0)next[last[owner]]=n;else first[owner]=n;last[owner]=n;
 }
 memset(&c,0,sizeof(c));memset(refs,0,GSC_ARRAY_CONTAINER_CAPACITY*sizeof(*refs));
 array_collect_roots(r,&c);
 for(k=0;k<c.count&&!c.failed;k++)for(n=first[c.queue[k]-1];n>=0;n=next[n])
  array_collect_value(r,&c,&r->array_entries[n].value);
 if(c.failed)return rfail(r,line,"R238 array COW live graph invalid");
#define R238_COUNT(qp) do{if((qp)->kind==V_ARRAY){if(!r238_array_reference_valid(r,(qp)->array,(qp)->object_token))return rfail(r,line,"R238 array COW root identity invalid");if(refs[(qp)->array-1]<2)refs[(qp)->array-1]++;}}while(0)
 if(r->game_array_enabled)R238_COUNT(&r->game_array_root);
 for(n=0;n<r->state_field_capacity;n++)if(r->state_fields[n].used)R238_COUNT(&r->state_fields[n].value);
 for(n=0;n<r->array_entry_count;n++)if(c.marked[r->array_entries[n].owner-1])R238_COUNT(&r->array_entries[n].value);
 for(n=0;n<r->task_count;n++){
  t=task_at(r,n);if(t->status==TASK_DEAD)continue;R238_COUNT(&t->initial_self);
  for(k=0;k<t->event_filter_count;k++)R238_COUNT(&t->event_filters[k]);
  for(f=0;f<t->depth;f++){R238_COUNT(&t->frames[f].self);for(k=0;k<t->frames[f].local_count;k++)R238_COUNT(&t->frames[f].locals[k].value);}
  s=t->results;if(!s)continue;
  for(k=0;k<s->node_count;k++){
   ResultsNode*node=s->nodes+k;R238_COUNT(&node->a);R238_COUNT(&node->b);R238_COUNT(&node->receiver);
   for(j=0;j<MAX_PARAMS;j++)R238_COUNT(&node->args[j]);
  }
  for(f=0;f<t->depth;f++)if(s->pending[f].active){
   ResultsPending*p=s->pending+f;R238_COUNT(&p->owner);R238_COUNT(&p->receiver);
   R238_COUNT(&p->value);R238_COUNT(&p->path.owner);
   if(&p->path!=path){R238_COUNT(&p->current);R238_COUNT(&p->path.base);for(j=0;j<48;j++)R238_COUNT(&p->ancestors[j]);}
   for(j=0;j<MAX_PARAMS;j++)R238_COUNT(&p->args[j]);
  }
 }
#undef R238_COUNT
 return 1;
}
static int r238_cow_references(GscRuntime*r,const ArrayLvalue*path,int*refs,int line){
 ArrayCollection c;int n,k,f,j;Task*t;ResultsTask*s;
 int first[GSC_ARRAY_CONTAINER_CAPACITY],last[GSC_ARRAY_CONTAINER_CAPACITY],next[GSC_ARRAY_ENTRY_CAPACITY];
 if(r->array_count<0||r->array_count>GSC_ARRAY_CONTAINER_CAPACITY||
    r->array_entry_count<0||r->array_entry_count>GSC_ARRAY_ENTRY_CAPACITY)
  return rfail(r,line,"R238 array COW arena invalid");
 /* Build an operation-local directory in the same ascending entry order.
  * All owner checks still precede root traversal, even for unreachable data.
  * From here through the reference count there are no callbacks or mutation;
  * visiting the matching entries once replaces repeated whole-arena scans. */
 memset(first,0xff,sizeof(first));memset(last,0xff,sizeof(last));
 for(n=0;n<r->array_entry_count;n++){
  int owner=r->array_entries[n].owner;
  if(owner<1||owner>r->array_count)return rfail(r,line,"R238 array COW entry owner invalid");
  owner--;next[n]=-1;if(last[owner]>=0)next[last[owner]]=n;else first[owner]=n;last[owner]=n;
 }
 memset(&c,0,sizeof(c));memset(refs,0,GSC_ARRAY_CONTAINER_CAPACITY*sizeof(*refs));
 /* For the selected COW lane each root can mark reachability and count
  * its value reference during the same visit. Captured lvalues stay roots
  * even when their references are intentionally excluded. No host callbacks
  * or VM mutations occur in this traversal. Use the original refusal path
  * on invalid data to preserve the complete failure result. */
#define R600_VISIT(qp,include) do { if((qp)->kind==V_ARRAY){array_collect_reference(r,&c,&(qp)->array,&(qp)->object_token);if(c.failed)goto original;if((include)&&refs[(qp)->array-1]<2)++refs[(qp)->array-1];} }while(0)
#define R238_COUNT(qp) R600_VISIT(qp,1)
 if(!r->r238_array_value_cow_enabled)goto original;
 if(r->game_array_enabled)R238_COUNT(&r->game_array_root);
 for(n=0;n<r->state_field_capacity;n++)if(r->state_fields[n].used)R238_COUNT(&r->state_fields[n].value);

 for(n=0;n<r->task_count;n++){
  t=task_at(r,n);if(t->status==TASK_DEAD)continue;R238_COUNT(&t->initial_self);
  for(k=0;k<t->event_filter_count;k++)R238_COUNT(&t->event_filters[k]);
  for(f=0;f<t->depth;f++){R238_COUNT(&t->frames[f].self);for(k=0;k<t->frames[f].local_count;k++)R238_COUNT(&t->frames[f].locals[k].value);}
  s=t->results;if(!s)continue;
  for(k=0;k<s->node_count;k++){
   ResultsNode*node=s->nodes+k;R238_COUNT(&node->a);R238_COUNT(&node->b);R238_COUNT(&node->receiver);
   for(j=0;j<MAX_PARAMS;j++)R238_COUNT(&node->args[j]);
  }
  for(f=0;f<t->depth;f++)if(s->pending[f].active){
   ResultsPending*p=s->pending+f;R238_COUNT(&p->owner);R238_COUNT(&p->receiver);
   R238_COUNT(&p->value);R238_COUNT(&p->path.owner);
   {int count_capture=&p->path!=path;R600_VISIT(&p->current,count_capture);R600_VISIT(&p->path.base,count_capture);for(j=0;j<48;j++)R600_VISIT(&p->ancestors[j],count_capture);}
   for(j=0;j<MAX_PARAMS;j++)R238_COUNT(&p->args[j]);
  }
 }
 for(k=0;k<c.count;k++)for(n=first[c.queue[k]-1];n>=0;n=next[n])R238_COUNT(&r->array_entries[n].value);
#undef R600_VISIT
#undef R238_COUNT
 return 1;
original:
 return r238_cow_references_reference(r,path,refs,line);
}
static int r238_stage_find(const R238ArrayStage*s,uint32_t owner,const ArrayKey*key){
 int n;for(n=0;n<s->entries;n++)if(s->members[n].owner==(int)owner&&array_key_equal(&s->members[n].key,key))return n;
 return -1;
}
static int r238_stage_new(GscRuntime*r,R238ArrayStage*s,int line,uint32_t*out){
 if(s->arrays>=GSC_ARRAY_CONTAINER_CAPACITY)return rfail(r,line,"R238 array COW container capacity");
 *out=(uint32_t)++s->arrays;memset(s->containers+*out-1,0,sizeof(s->containers[0]));return 1;
}
/* Iterative DFS deliberately does not memoize repeated array IDs: original
 * CopyArray duplicates a shared nested DAG once for each copied parent entry. */
static int r238_stage_clone(GscRuntime*r,R238ArrayStage*s,uint32_t source,int line,uint32_t*out){
 struct{uint32_t old_id,new_id;int next;}stack[GSC_ARRAY_CONTAINER_CAPACITY];int top=0,n,k,index;
 int heads[GSC_ARRAY_CONTAINER_CAPACITY],links[GSC_ARRAY_ENTRY_CAPACITY],indexed=0;
 if(!array_value_valid(r,source)||!r238_stage_new(r,s,line,out))return 0;
 s->containers[*out-1].keybinding=r->arrays[source-1].keybinding;
 stack[0].old_id=source;stack[0].new_id=*out;stack[0].next=0;top=1;
 while(top){
  int frame=top-1;uint32_t old=stack[frame].old_id,fresh=stack[frame].new_id;
  if(indexed)n=stack[frame].next;
  else for(n=stack[frame].next;n<r->array_entry_count;n++)if(r->array_entries[n].owner==(int)old)break;
  if(indexed?n<0:n==r->array_entry_count){
   if(s->containers[fresh-1].count!=r->arrays[old-1].count||
      s->containers[fresh-1].numeric_count!=r->arrays[old-1].numeric_count)return rfail(r,line,"R238 array COW source counts invalid");
   top--;continue;
  }
  stack[frame].next=indexed?links[n]:n+1;
  if(s->entries>=GSC_ARRAY_ENTRY_CAPACITY)return rfail(r,line,"R238 array COW entry capacity");
  index=s->entries++;s->members[index]=r->array_entries[n];s->members[index].owner=(int)fresh;
  s->containers[fresh-1].count++;if(s->members[index].key.kind==V_NUMBER)s->containers[fresh-1].numeric_count++;
  if(s->members[index].value.kind==V_ARRAY){
   uint32_t child=s->members[index].value.array,copy;
   if(!array_value_valid(r,child))return rfail(r,line,"R238 array COW nested handle invalid");
   for(k=0;k<top;k++)if(stack[k].old_id==child)return rfail(r,line,"R238 array COW cycle refused");
   if(top>=GSC_ARRAY_CONTAINER_CAPACITY||!r238_stage_new(r,s,line,&copy))return 0;
   s->containers[copy-1].keybinding=r->arrays[child-1].keybinding;
   /* Flat clones keep their original one-pass scan. At the first nested
    * child, index the immutable source arena in source-entry order. The
    * parent is still the only stack frame. Repeated DAG occurrences retain
    * separate copies and the same depth-first insertion order. Invalid
    * unrelated owners are ignored here, as in the original clone scan. */
   if(!indexed&&top==1&&r->array_count>0&&r->array_count<=GSC_ARRAY_CONTAINER_CAPACITY&&
      r->array_entry_count>=0&&r->array_entry_count<=GSC_ARRAY_ENTRY_CAPACITY){
    int i,owner;
    for(i=0;i<r->array_count;i++)heads[i]=-1;
    for(i=r->array_entry_count-1;i>=0;i--){
     owner=r->array_entries[i].owner;
     if(owner<1||owner>r->array_count)continue;
     links[i]=heads[owner-1];heads[owner-1]=i;
    }
    stack[frame].next=links[n];indexed=1;
   }
   s->members[index].value.array=copy;stack[top].old_id=child;stack[top].new_id=copy;
   stack[top].next=indexed?heads[child-1]:0;top++;
  }
 }
 return 1;
}
static int r238_stage_reaches(GscRuntime*r,const R238ArrayStage*s,uint32_t start,uint32_t target,int line){
 uint32_t stack[GSC_ARRAY_CONTAINER_CAPACITY];unsigned char seen[GSC_ARRAY_CONTAINER_CAPACITY]={0};int top=0,n;
 if(!start||start>(uint32_t)s->arrays)return rfail(r,line,"R238 array COW RHS handle invalid");
 stack[top++]=start;seen[start-1]=1;
 while(top){uint32_t id=stack[--top];if(id==target)return 1;
  for(n=0;n<s->entries;n++)if(s->members[n].owner==(int)id&&s->members[n].value.kind==V_ARRAY){
   uint32_t child=s->members[n].value.array;
   if(!child||child>(uint32_t)s->arrays){rfail(r,line,"R238 array COW nested RHS handle invalid");return -1;}
   if(!seen[child-1]){seen[child-1]=1;stack[top++]=child;}
  }
 }
 return 0;
}
static int r238_array_cow_commit(GscRuntime*r,Task*task,Frame*f,const ArrayLvalue*path,
 const Value*value,int state_index,int local_index,int global_root,int line){
 Value stored_value=*value;value=&stored_value;
 R238ArrayStage*s;Value root_value=path->base,current,live;const Expr*root=r->program->expr+path->root;
 int refs[GSC_ARRAY_CONTAINER_CAPACITY],n,index,parent=-1,ok=0;uint32_t ancestors[48];
 int oldEntries,touched[48],touchedCount=0;
 /* Caller retains the R233 gate for ordinary type0 stores and diverts
  * literal CLEAR before this path. A type0 member owns an entry, not an
  * array reference; existing deep-copy/count/owner checks still apply. */
 if(path->count<1||path->count>48)return rfail(r,line,"R238 array COW path domain refused");
 if(value->kind==V_ARRAY&&!r238_array_value_valid(r,value))return rfail(r,line,"R238 array COW RHS identity invalid");
 /* Finish every potentially foreign validation BEFORE allocating a candidate.
  * A callback or yielding RHS that replaced the owning cell fails closed. */
 if(value->kind==V_ENTITY){int alive=state_entity_live(r,&value->entity);if(!entity_task_current(r))return !r->error[0];if(alive!=1)return rfail(r,line,"array member entity is stale");}
 if(!refresh_state_context(r,task))return 0;if(task->status==TASK_DEAD)return 1;
 if(root->kind==E_FIELD&&!state_receiver(r,&path->owner,line))return 0;if(task->status==TASK_DEAD)return 1;
 memset(&live,0,sizeof(live));
 if(global_root){if(!game_array_root_matches(r,&path->base,line))return 0;live=r->game_array_root;}
 else if(root->kind==E_FIELD){state_index=state_field_index(r,&path->owner,root->name);if(state_index>=0&&r->state_fields[state_index].used)live=r->state_fields[state_index].value;}
 else if(local_index>=0)live=f->locals[local_index].value;
 if(!r238_cow_same(&live,&path->base))return rfail(r,line,"R238 array COW captured root changed");
 if(!r238_cow_captured_ancestors(r,task,path,line))return 0;
 /* Preserve the existing self-insertion frontier. A native compiler may
  * arrange detachment before RHS evaluation; this port's deferred lvalue
  * contract does not claim that ordering, even if a staged copy breaks a cycle. */
 if(value->kind==V_ARRAY){Value ancestor=path->base;int k;
  for(k=0;k<path->count&&ancestor.kind==V_ARRAY;k++){
   Value next;int cyclic=array_reaches(r,value->array,ancestor.array,line);
   if(cyclic<0)return 0;if(cyclic)return rfail(r,line,"array cycles/self-insertion are unsupported");
   if(!array_read_key(r,ancestor.array,path->keys+k,line,&next))return 0;ancestor=next;
  }
 }
 if(!r238_cow_references(r,path,refs,line))return 0;
 if(root->kind==E_FIELD&&state_index<0){if(!state_fields_reserve_free(r,1,line))return 0;
  for(n=0;n<r->state_field_capacity;n++)if(!r->state_fields[n].used){state_index=n;break;}
  if(state_index<0)return rfail(r,line,"COW state reservation unavailable");}
 if(r->state_scratch)return rfail(r,line,"R238 array COW scratch is already owned");
 /* The private stage does not escape this callback-free transaction. Reuse
  * its allocation, not its values: both used prefixes are overwritten below,
  * and every newly appended container/member is initialized before access.
  * No cached entry participates in GC, reference counts or script identity. */
 if(r->state_arena_busy||r->entity_certificate_busy||r->error[0])return rfail(r,line,"R329 COW scratch acquisition reentered");
 if(r->r329_cow_scratch){
  if(r->r329_cow_scratch_bytes!=sizeof(*s))return rfail(r,line,"R329 COW scratch ownership invalid");
  s=r->r329_cow_scratch;
 }else{
  s=state_storage_allocate(r,1,sizeof(*s),line);if(!s)return 0;
  r->r329_cow_scratch=s;r->r329_cow_scratch_bytes=sizeof(*s);
 }
 r->state_scratch=s;r->state_scratch_bytes=sizeof(*s);
 s->arrays=r->array_count;s->entries=r->array_entry_count;oldEntries=s->entries;
 memcpy(s->containers,r->arrays,(size_t)s->arrays*sizeof(*s->containers));
 memcpy(s->members,r->array_entries,(size_t)s->entries*sizeof(*s->members));current=root_value;
 for(n=0;n<path->count;n++){
  uint32_t owner;ArrayKey key=path->keys[n];
  if(current.kind==V_UNDEFINED){
   memset(&current,0,sizeof(current));current.kind=V_ARRAY;
   if(!r238_stage_new(r,s,line,&current.array))goto done;
   if(r->game_array_enabled)current.object_token=r->array_token;
   if(parent<0)root_value=current;else{s->members[parent].value=current;if(parent<oldEntries)touched[touchedCount++]=parent;}
  }
  if(current.kind!=V_ARRAY||!current.array||current.array>(uint32_t)s->arrays){rfail(r,line,"R238 array COW requires array parents");goto done;}
  owner=current.array;
  if(owner<=(uint32_t)r->array_count&&refs[owner-1]>1){
   if(!r238_stage_clone(r,s,owner,line,&current.array))goto done;
   if(parent<0)root_value=current;else{s->members[parent].value=current;if(parent<oldEntries)touched[touchedCount++]=parent;}owner=current.array;
  }
  ancestors[n]=owner;index=r238_stage_find(s,owner,&key);
  if(index<0){
   if(global_root&&key.kind==V_NUMBER&&key.number!=s->containers[owner-1].numeric_count){rfail(r,line,"sparse numeric persistent game writes are unsupported");goto done;}
   if(s->entries>=GSC_ARRAY_ENTRY_CAPACITY){rfail(r,line,"R238 array COW entry capacity");goto done;}
   index=s->entries++;memset(s->members+index,0,sizeof(s->members[0]));s->members[index].owner=(int)owner;s->members[index].key=key;
   s->containers[owner-1].count++;if(key.kind==V_NUMBER)s->containers[owner-1].numeric_count++;
  }
  if(n==path->count-1){
   if(value->kind==V_ARRAY){int k;for(k=0;k<=n;k++){int cyclic=r238_stage_reaches(r,s,value->array,ancestors[k],line);if(cyclic<0)goto done;if(cyclic){rfail(r,line,"array cycles are unsupported");goto done;}}}
   s->members[index].value=*value;if(index<oldEntries)touched[touchedCount++]=index;
  }else{
   parent=index;current=s->members[index].value;
   if(current.kind!=V_UNDEFINED&&current.kind!=V_ARRAY){rfail(r,line,"nested array path requires an array or absent member");goto done;}
  }
 }
 /* The entire new arena and owning-cell replacement publish together. There
  * are no host calls, allocations or validation failures after this point. */
 memcpy(r->arrays,s->containers,(size_t)s->arrays*sizeof(*r->arrays));
 /* Staging remains a complete private arena and every guard above remains.
  * Only a path parent or the final lvalue can modify an existing member;
  * deep clone/new-member writes append after oldEntries. There is at most
  * one parent write per non-root path level plus the final write (<=48).
  * Publish their final bytes, then all appended bytes, with no callbacks,
  * allocation or possible refusal between these stores and the root commit. */
 for(n=0;n<touchedCount;n++)r->array_entries[touched[n]]=s->members[touched[n]];
 if(s->entries>oldEntries)memcpy(r->array_entries+oldEntries,s->members+oldEntries,
     (size_t)(s->entries-oldEntries)*sizeof(*r->array_entries));
 r->array_count=s->arrays;r->array_entry_count=s->entries;
 if(global_root)r->game_array_root=root_value;
 else if(root->kind==E_VAR){if(local_index<0){local_index=f->local_count++;str_copy(f->locals[local_index].name,64,root->name);}f->locals[local_index].value=root_value;}
 else{StateField field;memset(&field,0,sizeof(field));field.used=1;field.owner_kind=path->owner.kind;
  if(path->owner.kind==V_STRUCT||path->owner.kind==V_ANIM)field.struct_owner=path->owner.object;
  field.owner=path->owner.entity;str_copy(field.name,sizeof(field.name),root->name);field.value=root_value;r->state_fields[state_index]=field;}
 ok=1;
done:r->state_scratch=NULL;r->state_scratch_bytes=0;return ok&&!r->error[0];
}
