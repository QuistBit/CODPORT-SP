/* R93 private bounded containers. Shared opaque handles live until runtime
 * teardown; no recursive script evaluator, engine pointers, refcount or GC. */
int gsc_runtime_select_sparse_numeric_arrays(GscRuntime *r) {
    if(!r||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       !r->arrays_enabled||r->sparse_numeric_arrays_enabled||r->array_count||r->array_entry_count||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||
       r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
    r->sparse_numeric_arrays_enabled=1;return 1;
}
/* A separately selected bounded slice of native numeric unlink. Shared
 * containers retain their explicit COW refusal; global persistence is held. */
int gsc_program_select_r233_owned_numeric_delete(GscProgram*g){
 if(!g||g->invalid||g->linked||g->module_count||g->owned_numeric_delete_selected)return 0;
 g->owned_numeric_delete_selected=1;return 1;
}
int gsc_runtime_select_r233_owned_numeric_delete(GscRuntime*r){
 if(!r||!r->program->owned_numeric_delete_selected||!r->sparse_numeric_arrays_enabled||r->owned_numeric_delete_enabled||
    r->started||r->advancing||r->executing_task||r->task_count||r->array_count||r->array_entry_count||r->error[0]||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy)return 0;
 r->owned_numeric_delete_enabled=1;return 1;
}
static int array_value_valid(const GscRuntime *r,uint32_t handle) {
    return r&&r->arrays_enabled&&handle&&handle<=(uint32_t)r->array_count;
}
static int array_token_new(uint64_t *out) {
    static uint64_t serial;
    if(serial==UINT64_MAX)return 0;
    *out=++serial;return 1;
}
static int array_new(GscRuntime *r,int line,Value *out) {
    if(!r->arrays_enabled||r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY)
        return rfail(r,line,"array container limit");
    memset(out,0,sizeof(*out));out->kind=V_ARRAY;out->array=(uint32_t)++r->array_count;
    if(r->game_array_enabled)out->object_token=r->array_token;
    memset(&r->arrays[out->array-1],0,sizeof(r->arrays[0]));return 1;
}
static int array_key(GscRuntime *r,const Value *value,int line,ArrayKey *out) {
    memset(out,0,sizeof(*out));
    if(value->kind==V_NUMBER) {
        int maxKey=r->node_queries_enabled?GSC_NODE_QUERY_CAPACITY-1:
            r->vehicle_nodes_enabled?GSC_VEHICLE_NODE_CAPACITY-1:GSC_ARRAY_NUMERIC_KEY_MAX;
        if(!isfinite(value->number)||value->number<0||
           value->number>maxKey||truncf(value->number)!=value->number)
            return rfail(r,line,"array numeric key must be an integer 0..%d",maxKey);
        out->kind=V_NUMBER;out->number=(int)value->number;return 1;
    }
    if(value->kind!=V_STRING||!memchr(value->str,0,sizeof(value->str)))
        return rfail(r,line,"array key requires a number or string");
    out->kind=V_STRING;str_copy(out->string,sizeof(out->string),value->str);return 1;
}
static int array_key_equal(const ArrayKey *a,const ArrayKey *b) {
    return a->kind==b->kind&&(a->kind==V_NUMBER?a->number==b->number:!strcmp(a->string,b->string));
}
#include "gsc_vm_array_lookup55.inc"
static int array_read_key(GscRuntime *r,uint32_t owner,const ArrayKey *key,int line,Value *out) {
    int index;
    if(!array_value_valid(r,owner))return rfail(r,line,"invalid array handle");
    index=array_entry_find(r,owner,key);memset(out,0,sizeof(*out));
    if(index>=0)*out=r->array_entries[index].value;
    return 1;
}
static int array_index_read(GscRuntime *r,const Value *base,const Value *key,int line,Value *out) {
    ArrayKey converted;
    if(base->kind==V_STRING){
        int32_t index;
        char character;
        if(key->kind!=V_NUMBER || !key->integer)return rfail(r,line,"string index requires an integer");
        if(!scalar_get_integer(r,key,line,&index))return 0;
        if(index<0 || (size_t)index>=strlen(base->str))return rfail(r,line,"string index %d out of range",index);
        /* ENGINE 45FEAA copies one byte into a new ordinary string. */
        character=base->str[index];
        memset(out,0,sizeof(*out));out->kind=V_STRING;out->str[0]=character;
        return 1;
    }
    if(base->kind==V_VECTOR) {
        if(key->kind!=V_NUMBER||!isfinite(key->number)||key->number<0||key->number>2||
           truncf(key->number)!=key->number)return rfail(r,line,"vector read index must be 0..2");
        memset(out,0,sizeof(*out));out->kind=V_NUMBER;
        out->number=base->vector[(int)key->number];return 1;
    }
    if(base->kind!=V_ARRAY)return rfail(r,line,"index read requires an array or vector");
    if(!array_key(r,key,line,&converted))return 0;
    return array_read_key(r,base->array,&converted,line,out);
}
static int string_size_field(const Value *base,const char *name){
    return base->kind==V_STRING && control_field_equal(name,"size");
}
static int array_field_read(GscRuntime *r,const Value *base,const char *name,int line,Value *out) {
    if(string_size_field(base,name)){
        scalar_integer(out,(int32_t)strlen(base->str));return 1;
    }
    if(array_value_valid(r,base->array)&&r->keybinding_enabled&&r->arrays[base->array-1].keybinding&&
       (!strcmp(name,"key1")||!strcmp(name,"key2")||!strcmp(name,"count"))) {
        ArrayKey key;memset(&key,0,sizeof(key));key.kind=V_STRING;str_copy(key.string,sizeof(key.string),name);
        return array_read_key(r,base->array,&key,line,out);
    }
    if(!array_value_valid(r,base->array)||!control_field_equal(name,"size"))
        return rfail(r,line,"array field read permits only size");
    memset(out,0,sizeof(*out));out->kind=V_NUMBER;out->integer=1;
    out->number=(float)r->arrays[base->array-1].count;
    if(r->keybinding_enabled&&r->arrays[base->array-1].keybinding){out->integer_exact=1;out->integer_value=r->arrays[base->array-1].count;}
    return 1;
}
/* Cycles are refused by an iterative bounded walk. Shared DAGs are admitted.
 * A mutation to an existing handle must not make it reachable from its RHS. */
static int array_reaches(GscRuntime *r,uint32_t start,uint32_t target,int line) {
    uint32_t stack[GSC_ARRAY_CONTAINER_CAPACITY];unsigned char seen[GSC_ARRAY_CONTAINER_CAPACITY];
    int head[GSC_ARRAY_CONTAINER_CAPACITY],links[GSC_ARRAY_ENTRY_CAPACITY];
    int top=0,n,indexed=0,walked=0;memset(seen,0,sizeof(seen));
    if(!array_value_valid(r,start))return rfail(r,line,"invalid array value handle");
    stack[top++]=start;seen[start-1]=1;
    while(top) {
        uint32_t current=stack[--top];
        if(current==target)return 1;
        /* The first container uses the original scan. Build an operation-local
         * directory only if a second container actually needs expansion.
         * No callback or mutation occurs on the successful traversal.
         * Ignore unrelated invalid owners, just as the original scan did. */
        if(!indexed&&walked&&r->array_count>=0&&r->array_count<=GSC_ARRAY_CONTAINER_CAPACITY&&
           r->array_entry_count>=0&&r->array_entry_count<=GSC_ARRAY_ENTRY_CAPACITY) {
            for(n=0;n<r->array_count;n++)head[n]=-1;
            for(n=r->array_entry_count-1;n>=0;n--) {
                int owner=r->array_entries[n].owner;
                if(owner<1||owner>r->array_count||r->array_entries[n].value.kind!=V_ARRAY)continue;
                links[n]=head[owner-1];head[owner-1]=n;
            }
            indexed=1;
        }
        walked=1;
        for(n=indexed?head[current-1]:0;indexed?n>=0:n<r->array_entry_count;
            n=indexed?links[n]:n+1)if(r->array_entries[n].owner==(int)current&&
           r->array_entries[n].value.kind==V_ARRAY) {
            uint32_t next=r->array_entries[n].value.array;
            if(!array_value_valid(r,next)){rfail(r,line,"invalid nested array handle");return -1;}
            if(!seen[next-1]){seen[next-1]=1;stack[top++]=next;}
        }
    }
    return 0;
}
static int array_lvalue_valid(const GscProgram *g,int index) {
    int depth=0;const Expr *e;
    while(index>=0&&index<g->expr_count&&(e=&g->expr[index])->kind==E_INDEX) {
        if(++depth>48||e->a<0||e->a>=index||e->b<0||e->b>=index)return 0;
        index=e->a;
    }
    if(index<0||index>=g->expr_count)return 0;
    e=&g->expr[index];
    if(e->kind==E_VAR)return e->name[0]&&!literal_name(e->name)&&
        strcmp(e->name,"self")&&strcmp(e->name,"level")&&strlen(e->name)<64;
    if(e->kind!=E_FIELD||e->a<0||e->a>=index||!e->name[0])return 0;
    /* A selected structure can own an ordinary metadata-named field. Its
     * receiver kind is checked at runtime; entity metadata remains immutable. */
    if(!g->struct_mode&&control_metadata_name(e->name)&&
       !(g->expr[e->a].kind==E_VAR&&!strcmp(g->expr[e->a].name,"level")))return 0;
    return 1;
}
typedef struct {
    int root, count; Value owner, base;
    ArrayKey keys[48];
} ArrayLvalue;
static int array_sparse_unshared(GscRuntime *,uint32_t,const ArrayLvalue *);
static int r238_array_value_valid(const GscRuntime *,const Value *);
static int r238_array_cow_commit(GscRuntime *,Task *,Frame *,const ArrayLvalue *,const Value *,int,int,int,int);
static int array_lvalue_capture(GscRuntime *r,Frame *f,int lhs,int line,ArrayLvalue *out,Value *current) {
    int chain[48],count=0,n,index=lhs;Value key,next;const Expr *root;
    memset(out,0,sizeof(*out));
    if(!array_lvalue_valid(r->program,lhs))return rfail(r,line,"unsupported array lvalue");
    while(r->program->expr[index].kind==E_INDEX) {
        chain[count++]=index;index=r->program->expr[index].a;
    }
    out->root=index;out->count=count;root=&r->program->expr[index];
    if(root->kind==E_VAR) {
        if(!eval(r,f,index,0,&out->base,line))return 0;
    } else {
        if(!eval(r,f,root->a,0,&out->owner,line))return 0;
        if(out->owner.kind==V_ARRAY)return rfail(r,line,"array size field is read-only");
        if(!state_field_read(r,&out->owner,root->name,line,&out->base))return 0;
    }
    *current=out->base;
    for(n=0;n<count;n++) {
        if(current->kind!=V_UNDEFINED&&current->kind!=V_ARRAY)
            return rfail(r,line,"indexed write requires array containers");
        if(!eval(r,f,r->program->expr[chain[count-1-n]].b,0,&key,line)||
           !array_key(r,&key,line,&out->keys[n]))return 0;
        /* A key callback can retire a destination independent of active self.
         * Reject it before evaluating another key or the RHS. */
        if(root->kind==E_FIELD&&!state_receiver(r,&out->owner,line))return 0;
        if(r->executing_task&&r->executing_task->status==TASK_DEAD)return 0;
        if(current->kind==V_UNDEFINED) {
            if(!r->sparse_numeric_arrays_enabled&&out->keys[n].kind==V_NUMBER&&out->keys[n].number!=0)
                return rfail(r,line,"sparse numeric array writes are unsupported");
            continue;
        }
        if(!array_value_valid(r,current->array))return rfail(r,line,"invalid array handle");
        if(!r->sparse_numeric_arrays_enabled&&out->keys[n].kind==V_NUMBER&&out->keys[n].number>r->arrays[current->array-1].numeric_count)
            return rfail(r,line,"sparse numeric array writes are unsupported");
        if(!array_read_key(r,current->array,&out->keys[n],line,&next))return 0;
        *current=next;
    }
    return 1;
}
/* Caller already validated the current state owner/native-field domain.
 * No allocation or foreign callback occurs while unlinking. The captured root
 * must still be the actual destination after arbitrary yielding RHS effects. */
static int array_owned_numeric_delete(GscRuntime*r,Frame*f,const ArrayLvalue*path,int state_index,int local_index,int global_root,int line){
 const Expr*root=&r->program->expr[path->root];Value live,current=path->base;uint32_t owner;int n,index;
 if(!r->owned_numeric_delete_enabled||global_root)return rfail(r,line,"owned numeric deletion does not admit persistent game arrays");
 if(path->count<1||path->keys[path->count-1].kind!=V_NUMBER)return rfail(r,line,"owned numeric deletion requires a final numeric key");
 memset(&live,0,sizeof(live));
 if(root->kind==E_FIELD){if(state_index>=0&&r->state_fields[state_index].used)live=r->state_fields[state_index].value;}
 else if(local_index>=0)live=f->locals[local_index].value;
 if(current.kind!=V_ARRAY||live.kind!=V_ARRAY||current.array!=live.array||current.object_token!=live.object_token)
  return rfail(r,line,"owned numeric deletion root changed or is absent");
 for(n=0;n<path->count;n++){
  if(current.kind!=V_ARRAY||!array_value_valid(r,current.array))return rfail(r,line,"owned numeric deletion requires existing array parents");
  owner=current.array;if(!array_sparse_unshared(r,owner,path))return rfail(r,line,"owned numeric deletion shared container requires copy-on-write");
  index=array_entry_find(r,owner,path->keys+n);
  if(n==path->count-1){
   if(index<0)return 1; /* Native missing-key deletion is a true no-op. */
   if(r->arrays[owner-1].count<=0||r->arrays[owner-1].numeric_count<=0)return rfail(r,line,"owned numeric deletion count invariant failed");
   r->arrays[owner-1].count--;r->arrays[owner-1].numeric_count--;
   memmove(r->array_entries+index,r->array_entries+index+1,(size_t)(r->array_entry_count-index-1)*sizeof(*r->array_entries));
   r->array_entry_count--;memset(r->array_entries+r->array_entry_count,0,sizeof(*r->array_entries));array_index_rebuild55(r);return 1;
  }
  if(index<0)return rfail(r,line,"owned numeric deletion missing intermediate parent");current=r->array_entries[index].value;
 }
 return rfail(r,line,"owned numeric deletion invalid path");
}
/* No callbacks occur after final owner validation. Missing nodes and entries
 * are staged locally, capacities/keys/cycles checked in full, then published
 * together. RHS allocations/effects are not rolled back. */
/* Original4584C0 chooses CLEAR only for a syntactic undefined literal;
 * a runtime type0 delivered by an ordinary RHS still reaches store opcode21.
 * Program AST is immutable; Results retains the source rhs/op across waits. */
static int array_assignment_clear(const GscProgram *g,int rhs,int op) {
    return !op && known_value_kind(g,rhs)==V_UNDEFINED;
}
static int array_lvalue_commit(GscRuntime *r,Task *task,Frame *f,const ArrayLvalue *path,
                               const Value *value,int clear,int line) {
    ArrayEntry staged[48];ArrayContainer fresh[48];Value root_value=path->base,current;
    Value stored_value=*value;value=&stored_value;
    int staged_count=0,new_count=0,n,index,existing_final=-1,state_index=-1,local_index=-1;
    uint32_t owner=0, ancestors[48];const Expr *root=&r->program->expr[path->root];
    int global_root=root->kind==E_VAR&&game_array_selected(r,root->name);
    if(animation_namespace_kind(value->kind)&&!animation_namespace_value_valid(r,value))return rfail(r,line,"invalid animation metadata array member");
    if(value->kind==V_ANIMATION&&!animation_value_valid(r,value))return rfail(r,line,"invalid animation reference array member");
    if(value->kind==V_FUNCTION&&!function_value_valid(r,value))return rfail(r,line,"invalid function reference array member");
    if(value->kind==V_STRUCT&&!struct_value_valid(r,value))return rfail(r,line,"invalid structure array member");
    if(value->kind==V_HUDELEM&&!hud_value_valid(r,value))return rfail(r,line,"invalid HUD element array member");
    if(value->kind==V_VEHICLE_NODE&&!vehicle_node_value_valid(r,value))return rfail(r,line,"invalid vehicle-node array identity");
    if(value->kind==V_NODE&&!node_value_valid(r,value))return rfail(r,line,"invalid node reference array member");
    if(value->kind==V_UNDEFINED&&!r->owned_numeric_delete_enabled)return rfail(r,line,"indexed undefined writes are unsupported");
    if(clear&&value->kind!=V_UNDEFINED)return rfail(r,line,"literal undefined clear result changed");
    if(value->kind==V_ANIM)return rfail(r,line,"whole anim owner array member is unsupported");
    if(value->kind==V_LEVEL)return rfail(r,line,"whole level values cannot be array members");
    if(value->kind==V_ARRAY&&!array_value_valid(r,value->array))return rfail(r,line,"invalid array member handle");
    if(!refresh_state_context(r,task))return 0;
    if(task->status==TASK_DEAD)return 1;
    if(root->kind==E_FIELD) {
        if(!state_receiver(r,&path->owner,line))return 0;
        if(task->status==TASK_DEAD)return 1;
        if(path->owner.kind==V_ENTITY) {
            int native=entity_field_classify(r,&path->owner.entity,root->name,line);
            if(native<0)return task->status==TASK_DEAD&&!r->error[0];
            if(native)return rfail(r,line,"native entity fields are not mutable private arrays");
        }
        if(r->controls_enabled&&path->owner.kind==V_ENTITY&&control_metadata_name(root->name))
            return rfail(r,line,"entity target/targetname metadata is read-only");
        if(path->owner.kind==V_ENTITY&&control_property_descriptor(r->program,root->name)>=0)
            return rfail(r,line,"native vector properties are not mutable arrays");
        state_index=state_field_index(r,&path->owner,root->name);

    } else if(global_root) {
        if(!game_array_root_matches(r,&path->base,line))return 0;
    } else {
        local_index=frame_local_find55(f,root->name,0);
        if(local_index<0&&f->local_count>=MAX_LOCALS)return rfail(r,line,"local limit");
    }
    if(clear)return array_owned_numeric_delete(r,f,path,state_index,local_index,global_root,line);
    if(value->kind==V_UNDEFINED&&global_root)return rfail(r,line,"ordinary undefined persistent game writes are unsupported");
    if(r->r238_array_value_cow_enabled)return r238_array_cow_commit(r,task,f,path,value,state_index,local_index,global_root,line);
    memset(fresh,0,sizeof(fresh));current=root_value;
    for(n=0;n<path->count;n++) {
        int count,numeric;ArrayKey key=path->keys[n];Value next;
        if(current.kind==V_UNDEFINED) {
            if(r->array_count+new_count>=GSC_ARRAY_CONTAINER_CAPACITY)return rfail(r,line,"array container limit");
            memset(&current,0,sizeof(current));current.kind=V_ARRAY;
            current.array=(uint32_t)(r->array_count+(++new_count));
            if(r->game_array_enabled)current.object_token=r->array_token;
            if(n==0)root_value=current;
            else staged[staged_count-1].value=current;
        }
        if(current.kind!=V_ARRAY)return rfail(r,line,"indexed write requires array containers");
        owner=current.array;ancestors[n]=owner;
        if(r->sparse_numeric_arrays_enabled&&owner<=(uint32_t)r->array_count&&
           !array_sparse_unshared(r,owner,path))
            return rfail(r,line,"shared array mutation requires unsupported original copy-on-write");
        if(owner<=(uint32_t)r->array_count) {
            index=array_entry_find(r,owner,&key);count=r->arrays[owner-1].count;
            numeric=r->arrays[owner-1].numeric_count;
        } else {index=-1;count=0;numeric=0;}
        if(index>=0)next=r->array_entries[index].value;
        else memset(&next,0,sizeof(next));
        if((!r->sparse_numeric_arrays_enabled||global_root)&&index<0&&key.kind==V_NUMBER&&key.number!=numeric)
            return rfail(r,line,"sparse numeric array writes are unsupported");
        if(n==path->count-1&&index>=0)existing_final=index;
        if(index<0) {
            if(r->array_entry_count+staged_count>=GSC_ARRAY_ENTRY_CAPACITY)return rfail(r,line,"array entry limit");
            memset(&staged[staged_count],0,sizeof(staged[0]));staged[staged_count].owner=(int)owner;
            staged[staged_count].key=key;staged_count++;
            if(owner>(uint32_t)r->array_count) {
                fresh[owner-r->array_count-1].count=count+1;
                fresh[owner-r->array_count-1].numeric_count=numeric+(key.kind==V_NUMBER);
            }
        }
        if(n==path->count-1) {
            if(value->kind==V_ARRAY) {
                int j;
                for(j=0;j<=n;j++)if(ancestors[j]<=(uint32_t)r->array_count) {
                    int cyclic=array_reaches(r,value->array,ancestors[j],line);
                    if(cyclic<0)return 0;
                    if(cyclic)return rfail(r,line,"array cycles are unsupported");
                }
            }
            if(index<0)staged[staged_count-1].value=*value;
        } else {
            if(index>=0&&next.kind!=V_ARRAY)
                return rfail(r,line,"nested array path requires an array or absent member");
            current=next;
        }
    }
    /* Validate member identity without treating arrays as engine entities. */
    if(value->kind==V_ENTITY) {
        int alive=state_entity_live(r,&value->entity);
        if(!entity_task_current(r))return !r->error[0];
        if(alive!=1)return rfail(r,line,"array member entity is stale");
    }
    if(!refresh_state_context(r,task))return 0;
    if(task->status==TASK_DEAD)return 1;
    if(root->kind==E_FIELD&&!state_receiver(r,&path->owner,line))return 0;
    if(task->status==TASK_DEAD)return 1;
    if(global_root&&!game_array_root_matches(r,&path->base,line))return 0;
    if(root->kind==E_FIELD){Value live;memset(&live,0,sizeof(live));
     state_index=state_field_index(r,&path->owner,root->name);
     if(state_index>=0)live=r->state_fields[state_index].value;
     if(!(path->base.kind==V_UNDEFINED?live.kind==V_UNDEFINED:
       path->base.kind==V_ARRAY&&live.kind==V_ARRAY&&path->base.array==live.array&&path->base.object_token==live.object_token))return rfail(r,line,"array captured root changed");
     if(state_index<0){if(!state_fields_reserve_free(r,1,line))return 0;
      for(n=0;n<r->state_field_capacity;n++)if(!r->state_fields[n].used){state_index=n;break;}
      if(state_index<0)return rfail(r,line,"array state reservation unavailable");}
    }
    for(n=0;n<new_count;n++)r->arrays[r->array_count+n]=fresh[n];
    r->array_count+=new_count;
    for(n=0;n<staged_count;n++) {
        ArrayEntry entry=staged[n];
        if(entry.owner<=r->array_count-new_count) {
            r->arrays[entry.owner-1].count++;
            if(entry.key.kind==V_NUMBER)r->arrays[entry.owner-1].numeric_count++;
        }
        array_entry_append55(r,&entry);
    }
    if(existing_final>=0)r->array_entries[existing_final].value=*value;
    if(root->kind==E_VAR) {
        if(!global_root) {
            if(local_index<0){local_index=f->local_count++;str_copy(f->locals[local_index].name,64,root->name);}
            f->locals[local_index].value=root_value;
        }
    } else {
        StateField field;memset(&field,0,sizeof(field));field.used=1;field.owner_kind=path->owner.kind;
        if(path->owner.kind==V_STRUCT||path->owner.kind==V_ANIM)field.struct_owner=path->owner.object;
        field.owner=path->owner.entity;str_copy(field.name,sizeof(field.name),root->name);field.value=root_value;
        state_field_publish(r,state_index,&field);
    }
    return 1;
}
static int array_assign(GscRuntime *r,Task *task,Frame *f,int lhs,int rhs,int op,int line) {
    ArrayLvalue path;Value current,arg,result;int clear=array_assignment_clear(r->program,rhs,op);
    memset(&current,0,sizeof(current));
    if(!array_lvalue_capture(r,f,lhs,line,&path,&current))return 0;
    memset(&arg,0,sizeof(arg));arg.kind=V_NUMBER;arg.integer=1;arg.number=1;
    if(rhs>=0&&!eval(r,f,rhs,0,&arg,line))return 0;
    result=arg;
    if(r->integer_bitwise_enabled&&integer_bitwise_mutation_op(op)) {
        if(!integer_bitwise_mutation(r,op,&current,&arg,line,&result))return 0;
    } else if(op==GSC_MUT_ADD&&string_vector_concat_pair(r,&current,&arg)) {
        if(!string_vector_concat(r,&current,&arg,line,&result))return 0;
    } else if(op&&r->vector_arithmetic_enabled&&(current.kind==V_VECTOR||arg.kind==V_VECTOR)) {
        if(!vector_arithmetic_mutation(r,op,&current,&arg,line,&result))return 0;
    } else if(op) {
        if(!(r->scalar_switch_enabled&&op==GSC_MUT_ADD&&
             current.kind==V_STRING&&arg.kind==V_STRING)&&
           !(op==GSC_MUT_ADD&&string_int_concat_pair(r,&current,&arg))&&
           (current.kind!=V_NUMBER||arg.kind!=V_NUMBER||!isfinite(current.number)||!isfinite(arg.number)))
            return rfail(r,line,"indexed numeric mutation requires finite numbers");
        if(r->scalar_switch_enabled) {
            if(!scalar_mutation(r,op,&current,&arg,line,&result))return 0;
        } else {
        memset(&result,0,sizeof(result));result.kind=V_NUMBER;result.integer=current.integer&&arg.integer;
        switch(op) {
          case GSC_MUT_ADD:case GSC_MUT_INC:result.number=current.number+arg.number;break;
          case GSC_MUT_SUB:case GSC_MUT_DEC:result.number=current.number-arg.number;break;
          case GSC_MUT_MUL:result.number=current.number*arg.number;break;
          case GSC_MUT_DIV:
            if(arg.number==0)return rfail(r,line,"division by zero");
            result.number=current.number/arg.number;result.integer=0;break;
          default:return rfail(r,line,"invalid indexed mutation");
        }
        if(!isfinite(result.number))return rfail(r,line,"nonfinite indexed mutation");
        }
    }
    return array_lvalue_commit(r,task,f,&path,&result,clear,line);
}
static int array_getentarray(GscRuntime *r,const Value *args,int argc,int line,Value *out) {
    GscEntityRef refs[GSC_ARRAY_CONTAINER_CAPACITY];size_t count=(size_t)-1,n;
    char error[256];Value result;int ok,old_arrays=r->array_count,old_entries=r->array_entry_count;
    if(argc!=2||args[0].kind!=V_STRING||args[1].kind!=V_STRING||!r->collections.lookup_array)
        return rfail(r,line,"getentarray requires two strings and collection capability");
    memset(refs,0,sizeof(refs));error[0]=0;
    ok=r->collections.lookup_array(r->collections.user,args[0].str,args[1].str,
        refs,GSC_ARRAY_CONTAINER_CAPACITY,&count,error,sizeof(error));
    error[sizeof(error)-1]=0;
    if(!entity_task_current(r))return 0;
    if(!ok)return rfail(r,line,"getentarray collection host: %s",error);
    if(r->error[0])return 0;
    if(count>GSC_ARRAY_CONTAINER_CAPACITY)return rfail(r,line,"getentarray host overflow");
    if(r->executing_task&&!refresh_state_context(r,r->executing_task))return 0;
    if(r->executing_task&&r->executing_task->status==TASK_DEAD)return 0;
    for(n=0;n<count;n++) {
        int alive=state_entity_live(r,&refs[n]);
        if(!entity_task_current(r))return 0;
        if(alive!=1)return rfail(r,line,"getentarray member is stale");
        if(n&&refs[n].epoch!=refs[0].epoch)return rfail(r,line,"getentarray mixed map epochs");
    }
    if(r->executing_task&&!refresh_state_context(r,r->executing_task))return 0;
    if(r->executing_task&&r->executing_task->status==TASK_DEAD)return 0;
    if(r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY||
       count>(size_t)(GSC_ARRAY_ENTRY_CAPACITY-r->array_entry_count))
        return rfail(r,line,"getentarray arena limit");
    if(!array_new(r,line,&result))return 0;
    for(n=0;n<count;n++) {
        ArrayEntry entry;memset(&entry,0,sizeof(entry));entry.owner=(int)result.array;
        entry.key.kind=V_NUMBER;entry.key.number=(int)n;entry.value.kind=V_ENTITY;entry.value.entity=refs[n];
        array_entry_append55(r,&entry);
    }
    r->arrays[result.array-1].count=r->arrays[result.array-1].numeric_count=(int)count;
    /* No result reaches script state before this second full validation. */
    for(n=0;n<count;n++) {
        int alive=state_entity_live(r,&refs[n]);
        if(!entity_task_current(r))goto retired;
        if(alive!=1)goto invalid;
    }
    if(r->executing_task&&!refresh_state_context(r,r->executing_task))goto invalid;
    if(r->executing_task&&r->executing_task->status==TASK_DEAD)goto retired;
    *out=result;return 1;
invalid:
    rfail(r,line,"getentarray member changed during publication");
retired:
    memset(&r->arrays[old_arrays],0,(size_t)(r->array_count-old_arrays)*sizeof(r->arrays[0]));
    memset(&r->array_entries[old_entries],0,(size_t)(r->array_entry_count-old_entries)*sizeof(r->array_entries[0]));
    r->array_count=old_arrays;r->array_entry_count=old_entries;array_index_rebuild55(r);return 0;
}
static int array_preflight_error(char *error,size_t size,int line,const char *why) {
    if(error&&size)snprintf(error,size,"line %d: %s",line,why);
    return 0;
}
static int array_runtime_preflight(const GscProgram *g,const GscEntityCollectionHost *collections,
                                    char *error,size_t size) {
    int n;
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];
        if(e->kind==E_NATIVE_CALL&&e->call_binding==GSC_EXPR_TARGET_ENTITY_NATIVE&&
           e->target==ENTITY_GLOBAL_GETENTARRAY&&(!collections||!collections->lookup_array))
            return array_preflight_error(error,size,e->line,"getentarray requires collection capability");
        if(e->kind==E_INDEX) {
            int type=known_value_kind(g,e->b);const Expr *key=&g->expr[e->b];
            if((type>=0&&type!=V_NUMBER&&type!=V_STRING)||
               (key->kind==E_NUMBER&&(key->number<0||key->number>(g->node_queries_selected?GSC_NODE_QUERY_CAPACITY-1:g->vehicle_nodes_selected?GSC_VEHICLE_NODE_CAPACITY-1:GSC_ARRAY_NUMERIC_KEY_MAX)||
                truncf(key->number)!=key->number))||
               (key->kind==E_UNARY&&key->op=='-'&&g->expr[key->a].kind==E_NUMBER&&
                g->expr[key->a].number!=0))
                return array_preflight_error(error,size,e->line,"unsupported literal array key");
        }
    }
    for(n=0;n<g->code_count;n++) {
        const Instr *i=&g->code[n];
        if(i->kind==I_GAME_SET&&g->expr[i->dest].kind==E_INDEX) {
            if(!array_lvalue_valid(g,i->dest))return array_preflight_error(error,size,i->line,"unsupported indexed lvalue");
            if(known_value_kind(g,i->expr)==V_UNDEFINED&&!g->owned_numeric_delete_selected)
                return array_preflight_error(error,size,i->line,"indexed undefined writes are unsupported");
            if(known_value_kind(g,i->expr)==V_UNDEFINED&&g->owned_numeric_delete_selected&&
               known_value_kind(g,g->expr[i->dest].b)==V_STRING)
                return array_preflight_error(error,size,i->line,"owned numeric deletion does not admit string keys");
        }
        if(i->kind==I_GAME_SET&&g->expr[i->dest].kind==E_FIELD&&
           control_field_equal(g->expr[i->dest].name,"size")&&
           g->expr[g->expr[i->dest].a].kind==E_EMPTY_ARRAY)
            return array_preflight_error(error,size,i->line,"array size field is read-only");
    }
    for(n=0;n<g->loop_count;n++) {
        const ForNode *loop=&g->loops[n];int targets[2],values[2],j;
        targets[0]=loop->init_target;values[0]=loop->init_expr;
        targets[1]=loop->step_kind==FOR_ASSIGN?loop->step_target:-1;values[1]=loop->step_expr;
        for(j=0;j<2;j++)if(targets[j]>=0&&g->expr[targets[j]].kind==E_INDEX&&
           known_value_kind(g,values[j])==V_UNDEFINED&&!g->owned_numeric_delete_selected)
            return array_preflight_error(error,size,loop->line,"indexed undefined writes are unsupported");
        for(j=0;j<2;j++)if(targets[j]>=0&&g->expr[targets[j]].kind==E_INDEX&&
           known_value_kind(g,values[j])==V_UNDEFINED&&g->owned_numeric_delete_selected&&
           known_value_kind(g,g->expr[targets[j]].b)==V_STRING)
            return array_preflight_error(error,size,loop->line,"owned numeric deletion does not admit string keys");
    }
    return 1;
}
