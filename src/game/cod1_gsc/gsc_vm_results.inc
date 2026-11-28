/* R94 private continuation machine. Syntax IR is never rewritten. One task
 * owns its bounded node stack and one pending statement per active call frame.
 * No pointer into a C evaluation stack survives a wait or a script call. */
#define RESULTS_EXPR_CAPACITY 48
#define RESULTS_CALL_WAIT 100
#define RESULTS_CALL_READY 101
typedef struct {
    int expression, frame, phase, argument, destination, receiver_guard, function_guard;
    Value a,b,receiver,args[MAX_PARAMS];
} ResultsNode;
/* Each capture is compared as an undefined/array identity. GC and COW
 * also inspect its ownership token and may remap its array number. The
 * complete value returned by an indexed read lives in ResultsPending.current. */
typedef struct {
    int kind;
    uint32_t array;
    uint64_t object_token;
} ResultsAncestor;
static ResultsAncestor results_capture_ancestor(const Value *value) {
    ResultsAncestor ref;
    ref.kind=value->kind;
    memcpy(&ref.array,&value->array,sizeof(ref.array));
    memcpy(&ref.object_token,&value->object_token,sizeof(ref.object_token));
    return ref;
}
static int results_same_ancestor(const Value *value,const ResultsAncestor *ref) {
    return value->kind==ref->kind && (value->kind==V_UNDEFINED ||
        (value->kind==V_ARRAY && value->array==ref->array));
}
typedef struct {
    int active, instruction, stage, argument, expression_active;
    int assignment_active, lhs,rhs,op,assign_phase,key_index,chain[48];
    int owner_guard, receiver_guard, path_ready, boundary, function_guard;
    Value owner,receiver,current,value,args[MAX_PARAMS];
    FlowLoop loop;
    ResultsAncestor ancestors[48];
    ArrayLvalue path;
} ResultsPending;
struct ResultsTask {
    int node_count;
    ResultsNode nodes[RESULTS_EXPR_CAPACITY];
    ResultsPending pending[MAX_FRAMES];
};
static int results_same_value(const Value *a,const Value *b) {
    if(a->kind!=b->kind)return 0;
    return a->kind==V_UNDEFINED?1:a->kind==V_ARRAY?a->array==b->array:0;
}
/* Captured write owners and method receivers remain guards throughout a
 * yielding child call. Validate BEFORE the child can emit its next effect. */
/* Called only with the bound native read-only validator. A refusal goes
 * through state_receiver again for its normal retirement/error handling. */
static int results_readonly_receiver(GscRuntime *r,const Value *v,int *read) {
    GscEntityRef original,borrowed;
    if(v->kind==V_LEVEL)return 1;
    if(v->kind!=V_ENTITY||!v->entity.epoch||!v->entity.generation)return 0;
    original=borrowed=v->entity;
    *read=1;
    return r->entities.validate(r->entities.user,&borrowed)==1 &&
        state_ref_equal(&original,&borrowed);
}
/* The caller has just checked the current task and every frame's self.
 * Only level references and pure allocation reads can occur in this pass.
 * Captured paths/functions are VM-owned reads; mismatches retain the ordered evaluator. */
/* A speculative, side-effect-free read of the same captured path used by
 * the ordered guard below. A mismatch falls back there for its original
 * error/retirement order. No failed speculative read may set r->error. */
static int results_readonly_path(GscRuntime *r,Task *task,int frame,const ResultsPending *p){
    Value root,current,next;const Expr *e;int k,index;
    if(p->path.root<0||p->path.root>=r->program->expr_count)return 0;
    e=&r->program->expr[p->path.root];memset(&root,0,sizeof(root));
    if(e->kind==E_VAR){
        Frame *f=&task->frames[frame];
        if(game_array_selected(r,e->name)){
            if(!game_array_value_valid(r,&r->game_array_root))return 0;
            root=r->game_array_root;
        }else for(k=f->local_count-1;k>=0;k--)if(!strcmp(f->locals[k].name,e->name)){
            root=f->locals[k].value;break;
        }
    }else{
        index=state_field_index(r,&p->path.owner,e->name);
        if(index>=0)root=r->state_fields[index].value;
    }
    if(!results_same_value(&root,&p->path.base))return 0;
    current=p->path.base;
    for(k=0;k<p->key_index&&k<p->path.count-1;k++){
        memset(&next,0,sizeof(next));
        if(current.kind==V_ARRAY){
            if(!array_value_valid(r,current.array))return 0;
            index=array_entry_find(r,current.array,&p->path.keys[k]);
            if(index>=0)next=r->array_entries[index].value;
        }
        if(!results_same_ancestor(&next,&p->ancestors[k]))return 0;
        current=next;
    }
    return 1;
}
static int results_readonly_guards(GscRuntime *r,Task *task,int *read) {
    ResultsTask *state=task->results;
    int n,captured=0,functionCaptured=0;
    for(n=0;n<task->depth;n++) {
        ResultsPending *p=&state->pending[n];
        if(p->function_guard){if(!function_value_valid(r,&p->current))return 0;captured=functionCaptured=1;}
        if(p->path_ready && !results_readonly_path(r,task,n,p))return 0;
        if(p->path_ready)captured=1;
        if(p->owner_guard&&!results_readonly_receiver(r,&p->owner,read))return 0;
        if(p->receiver_guard&&!results_readonly_receiver(r,&p->receiver,read))return 0;
    }
    for(n=0;n<state->node_count;n++) {
        ResultsNode *p=&state->nodes[n];
        if(p->function_guard){if(!function_value_valid(r,&p->b))return 0;captured=functionCaptured=1;}
        if(p->receiver_guard&&!results_readonly_receiver(r,&p->receiver,read))return 0;
    }
    /* Captured paths retain both certificate fences. VM-local expression
     * reads do not assign roots or ancestors; pushing an empty node adds no
     * guards and popping a local result only removes guards. The evaluator
     * already discards its carry at every host-backed operation or return.
     * Function guards still prohibit carry: writing a parent b result can
     * replace their captured function value. */
    if(captured)*read=1;
    return functionCaptured?2:1;
}
static int results_guard_read(GscRuntime *r,Task *task,int line,int *nativeRead) {
    int n,k,read=0;ResultsTask *state=task->results;
    /* All reads in this scope use the bound Session allocation validator.
     * Fence once on each side, including when a read fails. Unbound callers,
     * unsupported captures use the original order. */
    if(r->state_enabled && task->status!=TASK_DEAD && r->readonly_context.frame_active &&
       !r->state_arena_busy && !r->entity_prepare_busy && !r->entity_certificate_busy &&
       readonly_context_selected(r,task)){
        int alive,eligible;
        if(!entity_certificate_current(r,task))return 0;
        alive=readonly_receivers_live(r,task,&read);
        if(r->error[0])return 0;
        eligible=alive ? results_readonly_guards(r,task,&read) : 0;
        /* On the native binding, the successful scan above read only VM
         * values and GAME allocations after the entry certificate. Nothing
         * dispatched an effect or changed an owner. The proof ends here.
         * Failed/unsupported scans and generic hosts retain the exit call. */
        if(read && !(r->native_readonly_scan && alive && eligible) &&
           !entity_certificate_current(r,task))return 0;
        if(!alive){r54_dirty_mark_task(r,task);entity_task_retire(task);return 0;}
        if(eligible){
            if(nativeRead)*nativeRead=eligible==1;
            return 1;
        }
    }else if(!refresh_state_context(r,task)||task->status==TASK_DEAD)return 0;

    for(n=0;n<task->depth;n++) {
        ResultsPending *p=&state->pending[n];
        if(p->function_guard&&!function_value_valid(r,&p->current))return rfail(r,line,"captured function reference is invalid");
        if(p->owner_guard&&!state_receiver(r,&p->owner,line))return 0;
        if(p->receiver_guard&&!state_receiver(r,&p->receiver,line))return 0;
        if(task->status==TASK_DEAD)return 0;
        if(p->path_ready) {
            Value root,current,next;const Expr *e=&r->program->expr[p->path.root];
            memset(&root,0,sizeof(root));
            if(e->kind==E_VAR) {
                Frame *f=&task->frames[n];
                if(game_array_selected(r,e->name)) {
                    if(!game_array_read(r,line,&root))return 0;
                } else for(k=f->local_count-1;k>=0;k--)if(!strcmp(f->locals[k].name,e->name)) {
                    root=f->locals[k].value;break;
                }
            } else {
                int field=state_field_index(r,&p->path.owner,e->name);
                if(field>=0)root=r->state_fields[field].value;
            }
            if(!results_same_value(&root,&p->path.base))
                return rfail(r,line,"captured array root changed during expression");
            current=p->path.base;
            for(k=0;k<p->key_index&&k<p->path.count-1;k++) {
                memset(&next,0,sizeof(next));
                if(current.kind==V_ARRAY&&!array_read_key(r,current.array,&p->path.keys[k],line,&next))return 0;
                if(!results_same_ancestor(&next,&p->ancestors[k]))
                    return rfail(r,line,"captured array ancestor changed during expression");
                current=next;
            }
        }
    }
    for(n=0;n<state->node_count;n++)if(state->nodes[n].function_guard&&!function_value_valid(r,&state->nodes[n].b))return rfail(r,line,"captured computed function reference is invalid");
    for(n=0;n<state->node_count;n++)if(state->nodes[n].receiver_guard&&
        !state_receiver(r,&state->nodes[n].receiver,line))return 0;
    return refresh_state_context(r,task)&&task->status!=TASK_DEAD;
}
static int results_guard(GscRuntime *r,Task *task,int line){
    return results_guard_read(r,task,line,NULL);
}
static int results_node_push(GscRuntime *r,Task *task,int frame,int expression,int destination,int line) {
    ResultsNode *node;ResultsTask *state=task->results;
    if(expression<0||expression>=r->program->expr_count||
       state->node_count>=RESULTS_EXPR_CAPACITY)
        return rfail(r,line,"resumable expression nesting limit");
    node=&state->nodes[state->node_count++];memset(node,0,sizeof(*node));
    node->expression=expression;node->frame=frame;node->destination=destination;return 1;
}
static int results_operator(GscRuntime *r,const Expr *e,Value a,Value b,int line,Value *out) {
    int k;memset(out,0,sizeof(*out));
    if(animation_namespace_kind(a.kind)||animation_namespace_kind(b.kind))return animation_namespace_compare(r,e->op,&a,&b,line,out);
    if(a.kind==V_ANIMATION||b.kind==V_ANIMATION)return animation_compare(r,e->op,&a,&b,line,out);
    if(a.kind==V_HUDELEM||b.kind==V_HUDELEM)return hud_compare(r,e->op,&a,&b,line,out);
    if(a.kind==V_VEHICLE_NODE||b.kind==V_VEHICLE_NODE)return vehicle_node_compare(r,e->op,&a,&b,line,out);
    if(a.kind==V_NODE||b.kind==V_NODE)return node_compare(r,e->op,&a,&b,line,out);
    if(a.kind==V_UNDEFINED)return rfail(r,line,"undefined value in expression");
    if(e->kind==E_UNARY) {
        if(e->op=='~')return integer_bitwise_unary(r,&a,line,out);
        if(e->op=='!') {
            if(a.kind>V_STRING)return rfail(r,line,"vector/entity boolean semantics are unsupported");
            out->kind=V_NUMBER;out->integer=1;out->number=(float)!truth(a);return 1;
        }
        if(r->vector_arithmetic_enabled&&a.kind==V_VECTOR)
            return vector_arithmetic_unary(r,e->op,&a,line,out);
        if(a.kind!=V_NUMBER)return rfail(r,line,"numeric unary operator requires number");
        if(r->scalar_switch_enabled)return scalar_unary(r,&a,line,out);
        *out=a;out->number=-a.number;return 1;
    }
    if(b.kind==V_UNDEFINED)return rfail(r,line,"undefined value in expression");
    if(r->integer_bitwise_enabled&&(e->op=='|'||e->op=='^'||e->op=='&'))
        return integer_bitwise_binary(r,e->op,&a,&b,line,out);
    /* R96 event payload filters compare opaque complete identities. This
     * bounded port policy does not dereference or validate an entity, and is
     * selected only by the explicit payload runtime. Older modes stay strict. */
    if(r->event_payloads_enabled&&a.kind==V_ENTITY&&b.kind==V_ENTITY&&
       (e->op==T_EQ||e->op==T_NE)) {
        if(!a.entity.epoch||!a.entity.generation||!b.entity.epoch||!b.entity.generation)
            return rfail(r,line,"malformed entity comparison identity");
        k=entity_ref_equal(&a.entity,&b.entity);
        out->kind=V_NUMBER;out->integer=1;out->number=(float)(e->op==T_EQ?k:!k);
        return 1;
    }
    if(e->op=='+'&&string_vector_concat_pair(r,&a,&b))return string_vector_concat(r,&a,&b,line,out);
    if(r->vector_arithmetic_enabled&&(a.kind==V_VECTOR||b.kind==V_VECTOR)&&
       e->op!=T_EQ&&e->op!=T_NE)
        return vector_arithmetic_binary(r,e->op,&a,&b,line,out);
    if(a.kind==V_VECTOR&&b.kind==V_VECTOR&&(e->op=='+'||e->op==T_EQ||e->op==T_NE)) {
        if(e->op=='+') {
            out->kind=V_VECTOR;
            for(k=0;k<3;k++) {
                out->vector[k]=a.vector[k]+b.vector[k];
                if(!isfinite(out->vector[k]))return rfail(r,line,"nonfinite vector sum");
            }
        } else {
            k=a.vector[0]==b.vector[0]&&a.vector[1]==b.vector[1]&&a.vector[2]==b.vector[2];
            out->kind=V_NUMBER;out->integer=1;out->number=(float)(e->op==T_EQ?k:!k);
        }
        return 1;
    }
    if((e->op==T_EQ||e->op==T_NE||e->op==T_AND||e->op==T_OR)&&
       (a.kind>V_STRING||b.kind>V_STRING))
        return rfail(r,line,"vector/entity comparison semantics are unsupported");
    if(r239_mixed_scalar_pair(r,e->op,&a,&b))return r239_mixed_scalar_compare(r,e->op,&a,&b,line,out);
    if(r->scalar_switch_enabled)return scalar_binary_selected(r,e->float_arithmetic,e->op,&a,&b,line,out);
    out->kind=V_NUMBER;out->integer=1;
    switch(e->op) {
      case T_EQ:case T_NE:
        if(a.kind!=b.kind)return rfail(r,line,"equality type mismatch");
        k=a.kind==V_STRING?strcmp(a.str,b.str)==0:a.number==b.number;
        out->number=(float)(e->op==T_EQ?k:!k);return 1;
      case T_AND:out->number=(float)(truth(a)&&truth(b));return 1;
      case T_OR:out->number=(float)(truth(a)||truth(b));return 1;
      default:break;
    }
    if(a.kind!=V_NUMBER||b.kind!=V_NUMBER)return rfail(r,line,"numeric operator requires numbers");
    switch(e->op) {
      case '<':out->number=(float)(a.number<b.number);break;
      case '>':out->number=(float)(a.number>b.number);break;
      case T_LE:out->number=(float)(a.number<=b.number);break;
      case T_GE:out->number=(float)(a.number>=b.number);break;
      case '+':out->number=a.number+b.number;out->integer=a.integer&&b.integer;break;
      case '-':out->number=a.number-b.number;out->integer=a.integer&&b.integer;break;
      case '*':out->number=a.number*b.number;out->integer=a.integer&&b.integer;break;
      case '/':
        if(b.number==0)return rfail(r,line,"division by zero");
        out->number=a.number/b.number;out->integer=0;break;
      default:return rfail(r,line,"unsupported operator");
    }
    return isfinite(out->number)?1:rfail(r,line,"nonfinite expression");
}
/* 1: result delivered, 2: callee pushed; 0: refusal/cancellation. No expression
 * is restarted on resume. Parent destination and already computed values live
 * in the task-owned nodes. All node transitions share the scheduler budget. */
static int thread_expression_bound(const GscProgram *g,const Expr *e){
    if(e->argc<0 || e->argc>MAX_PARAMS)return 0;
    if(e->op==GSC_THREAD_EXPR_COMPUTED)return g->function_ref_mode && e->a>=0 && e->a<g->expr_count;
    if(e->op!=GSC_THREAD_EXPR_NAMED)return 0;
    if(e->b<0)return e->target>=0 && e->target<g->func_count;
    return (e->receiver_binding_kind==GSC_RECEIVER_TARGET_LOCAL_SCRIPT ||
            e->receiver_binding_kind==GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT) &&
           e->receiver_script_target>=0 && e->receiver_script_target<g->func_count;
}
static int results_eval_checked(GscRuntime *r,Task *task,Frame *f,int expression,Value *out,int line,int *budget,int entryChecked) {
    ResultsTask *state=task->results;int frame=task->depth-1,k,target,argc,script;
    ResultsPending *pending=&state->pending[frame];
    /* Only the bound read-only context can carry a guard across a local
     * expression-stack change. Host results always take a fresh guard before
     * delivery. The proof is local to this evaluation and this task. */
    int continuationChecked=entryChecked;
    if(!pending->expression_active) {
        if(!results_node_push(r,task,frame,expression,0,line))return 0;
        pending->expression_active=1;
    }
    while(state->node_count) {
        ResultsNode *node=&state->nodes[state->node_count-1];
        const Expr *e=&r->program->expr[node->expression];Value value;
        int done=0,nativeRead=0,localOnly=0;
        if(node->frame!=frame)return rfail(r,line,"invalid expression continuation frame");
        if(!flow_budget(r,budget,line))return 0;
        if(continuationChecked){
            nativeRead=1;continuationChecked=0;
        }else if(!results_guard_read(r,task,line,&nativeRead))return 0;
        memset(&value,0,sizeof(value));
        switch(e->kind) {
          case E_NUMBER:case E_STRING:case E_LOCALIZED_STRING:case E_VAR:case E_EMPTY_ARRAY:case E_FUNC_REF:case E_ANIM_REF:case E_ANIMTREE_VALUE:
            if(!eval_inner(r,f,node->expression,0,&value,line,&localOnly))return 0;
            done=1;break;
          case E_UNARY:case E_CAST:case E_FIELD:case E_ISDEFINED:
            if(e->kind==E_ISDEFINED&&e->call_binding==GSC_EXPR_TARGET_LOCAL_SCRIPT)goto script_call;
            if(node->phase==0) {
                node->phase=1;if(!results_node_push(r,task,frame,e->a,-1,line))return 0;continuationChecked=nativeRead;continue;
            }
            if(e->kind==E_CAST) {
                if(e->float_arithmetic==2){if(!r232_source_float_cast(r,e->op,&node->a,line,&value))return 0;}
                else if(e->float_arithmetic==1){if(!r->timed_arithmetic_enabled)return rfail(r,line,"selected module numeric casts are unsupported");if(!float_arithmetic_cast(r,e->op,&node->a,line,&value))return 0;}
                else
                if(!r->scalar_switch_enabled||!scalar_cast(r,e->op,&node->a,line,&value))return 0;
            } else if(e->kind==E_UNARY) {
                if(!results_operator(r,e,node->a,node->b,line,&value))return 0;
                localOnly=node->a.kind==V_NUMBER || e->op=='!';
            } else if(e->kind==E_FIELD) {
                localOnly=node->a.kind==V_ARRAY || string_size_field(&node->a,e->name);
                if(!(localOnly?array_field_read(r,&node->a,e->name,line,&value):
                    state_field_read(r,&node->a,e->name,line,&value)))return 0;
            } else if(!invoke_builtin_expression(r,e->target,&node->a,1,line,&value))return 0;
            done=1;break;
          case E_BINARY:case E_INDEX:
            if(node->phase==0) {
                node->phase=1;if(!results_node_push(r,task,frame,e->a,-1,line))return 0;continuationChecked=nativeRead;continue;
            }
            if(node->phase==1) {
                if(e->kind==E_BINARY) {
                    if(node->a.kind==V_UNDEFINED)return rfail(r,line,"undefined value in expression");
                    if((e->op==T_AND||e->op==T_OR)&&node->a.kind>V_STRING)
                        return rfail(r,line,"vector/entity boolean semantics are unsupported");
                    if(e->float_arithmetic&&(e->op==T_AND||e->op==T_OR)&&node->a.kind==V_NUMBER&&!node->a.integer)return rfail(r,line,"selected module float boolean operation is unsupported");
                    if((e->op==T_AND&&!truth(node->a))||(e->op==T_OR&&truth(node->a))) {
                        value.kind=V_NUMBER;value.integer=1;value.number=(float)(e->op==T_OR);localOnly=1;done=1;break;
                    }
                }
                node->phase=2;if(!results_node_push(r,task,frame,e->b,-2,line))return 0;continuationChecked=nativeRead;continue;
            }
            if(!(e->kind==E_INDEX?array_index_read(r,&node->a,&node->b,line,&value):
                results_operator(r,e,node->a,node->b,line,&value)))return 0;
            /* Index lookup reads the VM's own values. Both int32 operands
             * select the in-process integer lane of the scalar evaluator. */
            localOnly=e->kind==E_INDEX || (node->a.kind==V_NUMBER && node->a.integer &&
                node->b.kind==V_NUMBER && node->b.integer);
            done=1;break;
          case E_THREAD_CALL:
            if(!r->r243_thread_start_enabled || !thread_expression_bound(r->program,e))
                return rfail(r,line,"thread expression requires a bound script and native thread entry");
            /* Arguments, receiver and optional function pointer follow the
             * same evaluation order as the other script calls below. */
          case E_DYNAMIC_CALL:
            if((e->kind==E_DYNAMIC_CALL || e->op==GSC_THREAD_EXPR_COMPUTED) && !r->function_refs_enabled)return rfail(r,line,"computed call mode is unavailable");
            if(node->phase==RESULTS_CALL_READY){value=node->a;done=1;break;}
            if(node->phase==RESULTS_CALL_WAIT)return rfail(r,line,"computed callee result is unavailable");
            if(node->phase==0) {
                /* Original script compiler emits the reversed argument list
                 * before the receiver and then the computed function value. */
                if(node->argument<e->argc) {
                    k=e->argc-1-node->argument++;
                    if(!results_node_push(r,task,frame,e->args[k],k,line))return 0;
                    continuationChecked=nativeRead;continue;
                }
                node->phase=1;
                if(e->b>=0){if(!results_node_push(r,task,frame,e->b,-3,line))return 0;continue;}
            }
            if(node->phase==1) {
                if(e->b>=0) {
                    if(!state_receiver(r,&node->receiver,line))return 0;
                    node->receiver_guard=1;
                }
                if(e->kind==E_THREAD_CALL && e->op==GSC_THREAD_EXPR_NAMED){
                    target=e->b>=0?e->receiver_script_target:e->target;
                    goto enter_thread_expression;
                }
                node->phase=2;if(!results_node_push(r,task,frame,e->a,-1,line))return 0;continue;
            }
            if(node->phase==2) {
                if(!function_value_valid(r,&node->a))return rfail(r,line,"computed target requires a current script function reference");
                node->b=node->a;node->function_guard=1;node->phase=3;
            }
            if(!function_target(r,&node->b,e->argc,line,&target))return 0;
            if(e->kind==E_THREAD_CALL){
enter_thread_expression:
                if(!results_guard(r,task,line))return 0;
                if(!r243_spawn_initial(r,target,node->args,e->argc,e->b>=0?&node->receiver:&f->self,line,&value))return 0;
                if(task->status==TASK_DEAD || !results_guard(r,task,line))return 0;
                done=1;break;
            }
            node->phase=RESULTS_CALL_WAIT;
            {Value self=e->b>=0?node->receiver:f->self;
             if(!push_frame(r,task,target,node->args,e->argc,&self,line))return 0;}
            memset(&state->pending[task->depth-1],0,sizeof(state->pending[0]));return 2;
          case E_VECTOR:case E_NATIVE_CALL:case E_SCOPED_CALL:case E_METHOD_CALL:
script_call:
            script=e->kind==E_SCOPED_CALL||e->call_binding==GSC_EXPR_TARGET_LOCAL_SCRIPT||
                (e->kind==E_METHOD_CALL&&(e->receiver_binding_kind==GSC_RECEIVER_TARGET_LOCAL_SCRIPT||
                 e->receiver_binding_kind==GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT));
            if(node->phase==RESULTS_CALL_READY) {value=node->a;done=1;break;}
            if(node->phase==RESULTS_CALL_WAIT)return rfail(r,line,"callee result is unavailable");
            /* Native method/vector argument order retains its older bounded
             * policy. R122 changes only selected script call evaluation. */
            if(!script&&e->kind==E_METHOD_CALL&&node->phase==0) {
                node->phase=1;if(!results_node_push(r,task,frame,e->a,-3,line))return 0;continue;
            }
            if(!script&&e->kind==E_METHOD_CALL&&!node->receiver_guard) {
                /* New native arguments still evaluate before a nonentity
                 * receiver error. Captured ENTITY guards remain the existing
                 * bounded port lifetime policy, not native callee-order proof. */
                int defer=r->actor_spawners_enabled&&
                    e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
                    actor_spawn_native_descriptor(e->receiver_native_target)&&node->receiver.kind!=V_ENTITY;
                if(!defer) {
                    if(!state_receiver(r,&node->receiver,line))return 0;
                    node->receiver_guard=1;
                }
            }
            argc=e->kind==E_ISDEFINED?1:e->argc;
            if(argc<0||argc>MAX_PARAMS)return rfail(r,line,"invalid expression arity");
            if(node->argument<argc) {
                k=script?argc-1-node->argument++:node->argument++;
                if(!results_node_push(r,task,frame,e->kind==E_ISDEFINED?e->a:e->args[k],k,line))return 0;
                continuationChecked=nativeRead && e->kind!=E_METHOD_CALL;continue;
            }
            if(script&&e->kind==E_METHOD_CALL) {
                if(node->phase==0) {
                    node->phase=1;if(!results_node_push(r,task,frame,e->a,-3,line))return 0;continue;
                }
                if(!node->receiver_guard) {
                    if(!state_receiver(r,&node->receiver,line))return 0;
                    node->receiver_guard=1;
                }
            }
            if(e->kind==E_VECTOR) {
                value.kind=V_VECTOR;
                for(k=0;k<3;k++) {
                    if(node->args[k].kind!=V_NUMBER||!isfinite(node->args[k].number))
                        return rfail(r,line,"vector component must be a finite number");
                    value.vector[k]=node->args[k].number;
                }
            } else if(e->kind==E_METHOD_CALL&&
                      e->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE) {
                if(R243ActorFloatMask(e->receiver_native_target)){
                    if(e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||entity_method_descriptor(r->program,e->name)!=e->receiver_native_target||!r243_actor_float_call(r,&node->receiver,e->receiver_native_target,argc,line,&value))return 0;
                }else if(e->receiver_native_target==R243_ANIMTIME_METHOD){
                    if(e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||entity_method_descriptor(r->program,e->name)!=R243_ANIMTIME_METHOD||!r243_animtime_call(r,&node->receiver,node->args,argc,line,&value))return 0;
                }else if(e->receiver_native_target==R243_STANCE_METHOD){
                    if(e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||entity_method_descriptor(r->program,e->name)!=R243_STANCE_METHOD||!r243_stance_call(r,&node->receiver,node->args,argc,line,&value))return 0;                }else if(actor_look_kind(e->receiver_native_target)){
                    if(e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||actor_look_descriptor(r->program,e->name)!=e->receiver_native_target||!actor_look_invoke(r,&node->receiver,e->receiver_native_target,node->args,argc,line,&value))return 0;
                } else if(turret_method_kind(e->receiver_native_target)) {
                    if(e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||
                       turret_method_descriptor(r->program,e->name)!=e->receiver_native_target||
                       !turret_invoke(r,&node->receiver,e->receiver_native_target,node->args,argc,line,&value))return 0;
                } else if(attachment_query_kind(e->receiver_native_target)) {
                    if(e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||
                       attachment_query_descriptor(r->program,e->name)!=e->receiver_native_target||
                       !attachment_query_invoke(r,&node->receiver,e->receiver_native_target,node->args,argc,line,&value))return 0;
                } else if(spatial_method_kind(e->receiver_native_target)) {
                    if(e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||
                       spatial_method_descriptor(r->program,e->name)!=e->receiver_native_target||
                       !spatial_native_invoke(r,&node->receiver,e->receiver_native_target,node->args,argc,line,&value))return 0;
                } else if(entity_coordinate(e->receiver_native_target)) {
                    if(e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||
                       entity_coordinate_descriptor(r->program,e->name)!=e->receiver_native_target||
                       !entity_coordinate_invoke(r,&node->receiver,e->receiver_native_target,node->args,argc,line,&value))return 0;
                } else if(entity_operation(e->receiver_native_target)) {
                    if(e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||
                       entity_operation_descriptor(r->program,e->name)!=e->receiver_native_target||
                       !entity_operation_invoke(r,&node->receiver,e->receiver_native_target,node->args,argc,line,&value))return 0;
                } else if(player_method_kind(e->receiver_native_target)) {
                    if(e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||
                       player_method_descriptor(r->program,e->name)!=e->receiver_native_target||
                       !player_native_invoke(r,&node->receiver,e->receiver_native_target,node->args,argc,line,&value))return 0;
                } else if(e->receiver_native_target==ENTITY_METHOD_ISCRITICAL) {
                    if(e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||
                       !callable_name_equal(r->program,e->name,"isincriticalsection")||
                       !actor_critical_invoke(r,&node->receiver,ENTITY_METHOD_ISCRITICAL,node->args,argc,line,&value))return 0;
                } else if(e->receiver_native_target==ENTITY_METHOD_GETNUMBER) {
                    if(!r->native_queries_enabled||e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||
                       (!callable_name_equal(r->program,e->name,"getentitynumber")&&!callable_name_equal(r->program,e->name,"getentnum"))||
                       !native_entity_number(r,&node->receiver,argc,line,&value))return 0;
                } else if(!r->actor_spawners_enabled||!actor_spawn_native_descriptor(e->receiver_native_target)||
                   e->receiver_script_target!=-1||e->receiver_event_target!=-1||e->module[0]||
                   !callable_name_equal(r->program,e->name,actor_spawn_native_name(e->receiver_native_target))||
                   !actor_spawn_expression(r,e->receiver_native_target,&node->receiver,node->args,argc,line,&value))return 0;
            } else if(e->kind==E_SCOPED_CALL||e->kind==E_METHOD_CALL||
                      e->call_binding==GSC_EXPR_TARGET_LOCAL_SCRIPT) {
                Value self=e->kind==E_METHOD_CALL?node->receiver:f->self;
                target=e->kind==E_METHOD_CALL?e->receiver_script_target:e->target;
                if(target<0||target>=r->program->func_count)return rfail(r,line,"invalid script result target");
                node->phase=RESULTS_CALL_WAIT;
                if(!push_frame(r,task,target,node->args,argc,&self,line))return 0;
                memset(&state->pending[task->depth-1],0,sizeof(state->pending[0]));
                return 2;
            } else if(e->call_binding==GSC_EXPR_TARGET_BULLET_TRACE_NATIVE){if(!r->bullet_trace_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!bullet_trace_descriptor(r->program,e->name)||bullet_trace_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid bulletTrace binding");
 if(!bullet_trace_call(r,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_VEHICLE_SPAWN_NATIVE){if(!r->vehicle_spawn_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!vehicle_spawn_descriptor(r->program,e->name)||vehicle_spawn_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid spawnVehicle binding");
 if(!vehicle_spawn_call(r,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_TURRET_PRECACHE_NATIVE){if(!r->turret_precache_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!turret_precache_descriptor(r->program,e->name)||turret_precache_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid precacheTurret binding");
 if(!turret_precache_call(r,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_TURRET_SPAWN_NATIVE){if(!r->turret_spawn_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!turret_spawn_descriptor(r->program,e->name)||turret_spawn_issues(r->program,e->args,e->argc))return rfail(r,line,"invalid spawnTurret binding");
 if(!turret_spawn_call(r,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_HUDELEM_NATIVE) {
                if(!r->hud_elements_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->argc||e->target!=GSC_HUDELEM_CREATE||find_func(r->program,e->owner_module,e->name)>=0||!hud_elem_global(r->program,e->name))return rfail(r,line,"invalid HUD expression binding");
                if(!hud_create(r,e->argc,line,&value))return 0;
            } else if(e->call_binding==R243_EXPR_TARGET_MOVE_DELTA){if(!r->move_delta_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!move_delta_descriptor(r->program,e->name)||move_delta_static_issues(r->program,e->args,e->argc)||!move_delta_call(r,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_START_ORIGIN_NATIVE) {
                if((!r->start_origin_enabled&&!r->namespace_start_origin_enabled)||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=start_origin_descriptor(r->program,e->name)||
                   find_func(r->program,e->owner_module,e->name)>=0||!start_origin_descriptor(r->program,e->name)||
                   start_origin_static_issues(r->program,e->args,e->argc)||!start_origin_selected_call(r,node->args,e->argc,line,e->target,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_AI_ARRAY_NATIVE) {
                if(!r->ai_arrays_enabled||e->module[0]||e->kind!=E_NATIVE_CALL||e->target!=1||
                   find_func(r->program,e->owner_module,e->name)>=0||!ai_array_descriptor(r->program,e->name)||
                   ai_array_static_issues(r->program,e->args,e->argc)||!ai_array_call(r,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_NODE_QUERY_NATIVE) {
                if(!node_query_capability(r,e->target)||e->module[0]||e->kind!=E_NATIVE_CALL||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   node_query_descriptor(r->program,e->name)!=e->target||
                   node_query_static_issues(r->program,e->target,e->args,e->argc)||
                   !node_query_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_VECTOR_MATH_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->vector_math_enabled||e->module[0]||
                   vector_math_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !vector_math_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_ENTITY_QUERY_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->entity_queries_enabled||e->module[0]||
                   entity_query_descriptor(r->program,e->name)!=e->target||
                   entity_query_static_issues(r->program,e->target,e->args,e->argc)||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !entity_query_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_STRUCT_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->structs_enabled||e->module[0]||
                   struct_descriptor(r->program,e->name)!=e->target||
                   struct_static_issues(e->target,e->argc)||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !struct_call(r,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_KEYBINDING_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->keybinding_enabled||e->module[0]||
                   keybinding_descriptor(r->program,e->name,e->args,e->argc)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !keybinding_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_R232_RANDOM_RANGE){if(e->kind!=E_NATIVE_CALL||e->module[0]||e->target!=1||find_func(r->program,e->owner_module,e->name)>=0||!random_range_descriptor(r->program,e->name)||random_range_issues(r->program,e->args,e->argc)||!random_range_call(r,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_RANDOMFLOAT_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->randomfloat_enabled||e->module[0]||
                   randomfloat_descriptor(r->program,e->name,e->args,e->argc)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !randomfloat_call_selected(r,e->target,node->args,e->argc,e->float_arithmetic,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_RANDOMINT_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->randomint_enabled||e->module[0]||
                   randomint_descriptor(r->program,e->name,e->args,e->argc)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !randomint_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_SPAWNER_ARRAY_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->spawner_arrays_enabled||e->module[0]||
                   spawner_array_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !spawner_array_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_ANIMATION_LENGTH_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||e->module[0]||!animation_length_descriptor(r->program,e->name)||
                   find_func(r->program,e->owner_module,e->name)>=0||!animation_length_call(r,node->args,e->argc,e->float_arithmetic,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_MODEL_PART_NATIVE){
                if(e->kind!=E_NATIVE_CALL||e->module[0]||!model_part_descriptor(r->program,e->name)||find_func(r->program,e->owner_module,e->name)>=0||!model_part_call(r,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_WEAPON_CLASSNAME_NATIVE){
                if(e->kind!=E_NATIVE_CALL||e->module[0]||!weapon_classname_descriptor(r->program,e->name)||find_func(r->program,e->owner_module,e->name)>=0||!weapon_classname_call(r,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_WEAPON_MODEL_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||e->module[0]||!weapon_model_descriptor(r->program,e->name)||
                   find_func(r->program,e->owner_module,e->name)>=0||!weapon_model_call(r,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_NATIVE_QUERY) {
                if(e->kind!=E_NATIVE_CALL||!r->native_queries_enabled||e->module[0]||
                   native_query_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !native_query_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_TAG_FX_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->tag_fx_enabled||e->module[0]||
                   tag_fx_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !tag_fx_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_SAVE_GAME_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->save_game_enabled||e->module[0]||
                   save_game_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !save_game_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_PRECACHE_ITEM_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->precache_item_enabled||e->module[0]||
                   precache_item_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !precache_item_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_AMBIENT_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->ambient_enabled||e->module[0]||
                   ambient_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !ambient_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_DEBUG_LINE_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->debug_line_enabled||e->module[0]||
                   debug_line_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !debug_line_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_DEBUG_TEXT_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->debug_text_enabled||e->module[0]||
                   debug_text_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !debug_text_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_SHELLSHOCK_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->shellshock_enabled||e->module[0]||
                   shellshock_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !shellshock_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_CVAR_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->cvars_enabled||e->module[0]||
                   cvar_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !cvar_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_FX_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->fx_enabled||e->module[0]||
                   fx_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !fx_call(r,e->target,node->args,e->argc,line,&value))return 0;
            } else if(e->call_binding==GSC_EXPR_TARGET_QUAKE_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->quake_enabled||e->module[0]||
                   quake_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !quake_statement(r,e->target,node->args,e->argc,line))return 0;
                memset(&value,0,sizeof(value));
            } else if(e->call_binding==GSC_EXPR_TARGET_BLAST_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->blast_enabled||e->module[0]||
                   blast_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !blast_statement(r,e->target,node->args,e->argc,line))return 0;
                memset(&value,0,sizeof(value));
            } else if(e->call_binding==GSC_EXPR_TARGET_MISSION_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->mission_enabled||e->module[0]||
                   mission_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !mission_statement(r,e->target,node->args,e->argc,line))return 0;
                memset(&value,0,sizeof(value));
            } else if(e->call_binding==GSC_EXPR_TARGET_HUD_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->hud_enabled||e->module[0]||
                   hud_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !hud_statement(r,e->target,NULL,node->args,e->argc,line))return 0;
                memset(&value,0,sizeof(value));
            } else if(e->call_binding==GSC_EXPR_TARGET_CONSOLE_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->console_enabled||e->module[0]||
                   console_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !console_statement(r,e->target,node->args,e->argc,line))return 0;
                memset(&value,0,sizeof(value));
            } else if(e->call_binding==GSC_EXPR_TARGET_OBJECTIVE_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->objectives_enabled||e->module[0]||
                   objective_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !objective_statement(r,e->target,node->args,e->argc,line))return 0;
                memset(&value,0,sizeof(value));
            } else if(e->call_binding==GSC_EXPR_TARGET_RESOURCE_NATIVE) {
                if(e->kind!=E_NATIVE_CALL||!r->resources_enabled||e->module[0]||
                   resource_descriptor(r->program,e->name)!=e->target||
                   find_func(r->program,e->owner_module,e->name)>=0||
                   !resource_statement(r,e->target,node->args,e->argc,line))return 0;
                memset(&value,0,sizeof(value));
            } else if(e->call_binding==GSC_EXPR_TARGET_ENTITY_NATIVE) {
                if(!entity_global_expression_binding(r,e))
                    return rfail(r,line,"invalid entity expression binding");
                if(!(e->target==ENTITY_GLOBAL_SPAWN?bridge_spawn(r,node->args,e->argc,
                         e->argc==3?e->args[2]:-1,line,&value):
                     e->target==ENTITY_GLOBAL_GETENTARRAY?array_getentarray(r,node->args,e->argc,line,&value):
                     e->target==ENTITY_GLOBAL_ISAI?actor_presence_expression(r,node->args,e->argc,line,&value):
                     e->target==ENTITY_GLOBAL_ISALIVE||e->target==ENTITY_GLOBAL_ISSENTIENT?
                         predicate_expression(r,e->target,node->args,e->argc,line,&value):
                     bridge_getent(r,node->args,e->argc,line,&value)))return 0;
            } else if(e->call_binding!=GSC_EXPR_TARGET_BUILTIN||
                      !invoke_builtin_expression(r,e->target,node->args,e->argc,line,&value))return 0;
            done=1;break;
          default:return rfail(r,line,"unsupported resumable expression kind");
        }
        if(done) {
            int destination=node->destination;
            int parentChecked=nativeRead && localOnly;
            state->node_count--;
            /* Dropping a node after a VM-local operation only removes guards. Recheck after
             * all other operations, including host-backed variable reads. */
            if(!parentChecked && !results_guard_read(r,task,line,&parentChecked))return 0;
            if(!state->node_count||state->nodes[state->node_count-1].frame!=frame) {
                *out=value;pending->expression_active=0;return 1;
            }
            node=&state->nodes[state->node_count-1];
            if(destination>=0)node->args[destination]=value;
            else if(destination==-1)node->a=value;
            else if(destination==-2)node->b=value;
            else node->receiver=value;
            /* args, a and b do not replace a guarded receiver. A successful
             * read-only carry excludes captured function guards; captured array
             * roots/ancestors are unchanged by these node result stores. */
            continuationChecked=parentChecked && destination>=-2;
        }
    }
    return rfail(r,line,"lost expression continuation");
}
/* The default entry always establishes its own guard. The checked entry
 * is used only immediately after a successful bound read-only guard, before
 * any host call or guard-changing VM mutation. It consumes that proof on
 * its first iteration; all existing post-host and result fences remain. */
static int results_eval(GscRuntime *r,Task *task,Frame *f,int expression,Value *out,int line,int *budget){
 return results_eval_checked(r,task,f,expression,out,line,budget,0);
}
static int results_return(GscRuntime *r,Task *task,const Value *value,int line) {
    ResultsTask *state=task->results;int frame=task->depth-1,consumed;Value returned=*value;
    consumed=frame>0&&state->node_count&&
        state->nodes[state->node_count-1].frame==frame-1&&
        state->nodes[state->node_count-1].phase==RESULTS_CALL_WAIT;
    if(frame==0 && task->initial_thread_result)consumed=1;
    if(!results_guard(r,task,line))return 0;
    if(animation_namespace_kind(returned.kind)&&!animation_namespace_value_valid(r,&returned))return rfail(r,line,"invalid returned animation metadata");
    if(returned.kind==V_ANIMATION&&!animation_value_valid(r,&returned))return rfail(r,line,"invalid returned animation reference");
    if(consumed&&returned.kind==V_FUNCTION&&!function_value_valid(r,&returned))return rfail(r,line,"invalid returned function reference");
    if(consumed&&returned.kind==V_ARRAY&&!array_value_valid(r,returned.array))return rfail(r,line,"invalid returned array");
    if(consumed&&returned.kind==V_STRUCT&&!struct_value_valid(r,&returned))return rfail(r,line,"invalid returned structure");
    if(consumed&&returned.kind==V_HUDELEM&&!hud_value_valid(r,&returned))return rfail(r,line,"invalid returned HUD element");
    if(consumed&&returned.kind==V_VEHICLE_NODE&&!vehicle_node_value_valid(r,&returned))return rfail(r,line,"invalid returned vehicle-node identity");
    if(consumed&&returned.kind==V_NODE&&!node_value_valid(r,&returned))return rfail(r,line,"invalid returned node identity");
    if(returned.kind==V_ANIM)return rfail(r,line,"whole anim owner return is unsupported");
    if(consumed&&returned.kind==V_LEVEL)return rfail(r,line,"whole level result is unsupported");
    if(!results_guard(r,task,line))return 0;
    memset(&state->pending[frame],0,sizeof(state->pending[0]));
    if(!r243_pop_normal_frame(r,task,line))return 0;
    if(frame==0 && task->initial_thread_result && !task->context_retired)*task->initial_thread_result=returned;
    if(task->depth&&state->node_count) {
        ResultsNode *node=&state->nodes[state->node_count-1];
        if(node->frame==task->depth-1&&node->phase==RESULTS_CALL_WAIT) {
            node->a=returned;node->phase=RESULTS_CALL_READY;
        }
    }
    return 1;
}
static int results_mutation(GscRuntime *r,int lhs,int op,const Value *current,const Value *arg,int line,Value *out) {
    int k;memset(out,0,sizeof(*out));
    if(r->integer_bitwise_enabled&&integer_bitwise_mutation_op(op))
        return integer_bitwise_mutation(r,op,current,arg,line,out);
    if(op==GSC_MUT_ADD&&string_vector_concat_pair(r,current,arg))return string_vector_concat(r,current,arg,line,out);
    if(r->vector_arithmetic_enabled&&(current->kind==V_VECTOR||arg->kind==V_VECTOR))
        return vector_arithmetic_mutation(r,op,current,arg,line,out);
    if(current->kind==V_VECTOR&&arg->kind==V_VECTOR&&op==GSC_MUT_ADD&&
       r->program->expr[lhs].kind==E_FIELD&&control_field_equal(r->program->expr[lhs].name,"origin")) {
        out->kind=V_VECTOR;
        for(k=0;k<3;k++) {
            out->vector[k]=current->vector[k]+arg->vector[k];
            if(!isfinite(out->vector[k]))return rfail(r,line,"nonfinite origin sum");
        }
        return 1;
    }
    if(!(r->scalar_switch_enabled&&op==GSC_MUT_ADD&&
         current->kind==V_STRING&&arg->kind==V_STRING)&&
       !(op==GSC_MUT_ADD&&string_int_concat_pair(r,current,arg))&&
       (current->kind!=V_NUMBER||arg->kind!=V_NUMBER||!isfinite(current->number)||!isfinite(arg->number)))
        return rfail(r,line,"numeric mutation requires finite numbers");
    if(r->scalar_switch_enabled)return scalar_mutation(r,op,current,arg,line,out);
    out->kind=V_NUMBER;out->integer=current->integer&&arg->integer;
    switch(op) {
      case GSC_MUT_ADD:case GSC_MUT_INC:out->number=current->number+arg->number;break;
      case GSC_MUT_SUB:case GSC_MUT_DEC:out->number=current->number-arg->number;break;
      case GSC_MUT_MUL:out->number=current->number*arg->number;break;
      case GSC_MUT_DIV:
        if(arg->number==0)return rfail(r,line,"division by zero");
        out->number=current->number/arg->number;out->integer=0;break;
      default:return rfail(r,line,"invalid numeric mutation");
    }
    return isfinite(out->number)?1:rfail(r,line,"nonfinite mutation");
}
/* Lvalue preparation, keys, current value and RHS are independently resumable.
 * The write occurs only at the final phase. Ancestor replacement while yielded
 * refuses rather than retargeting a captured nested array path. */
static int results_assign(GscRuntime *r,Task *task,Frame *f,int lhs,int rhs,int op,int line,int *budget) {
    ResultsPending *p=&task->results->pending[task->depth-1];
    const Expr *e=&r->program->expr[lhs];int status,n,index;Value key,next,result;
    if(!p->assignment_active) {
        p->assignment_active=1;p->lhs=lhs;p->rhs=rhs;p->op=op;p->assign_phase=0;
        if(e->kind==E_INDEX) {
            if(!array_lvalue_valid(r->program,lhs))return rfail(r,line,"unsupported array lvalue");
            index=lhs;while(r->program->expr[index].kind==E_INDEX) {
                p->chain[p->path.count++]=index;index=r->program->expr[index].a;
            }
            p->path.root=index;
        } else if(!flow_lvalue_valid(r->program,lhs))return rfail(r,line,"invalid assignment lvalue");
    }
    while(p->assign_phase<6) {
        int entryRead=0;
        const Expr *root=e->kind==E_INDEX?&r->program->expr[p->path.root]:e;
        if(!flow_budget(r,budget,line)||!results_guard_read(r,task,line,&entryRead))return 0;
        if(p->assign_phase==0) {
            if(root->kind==E_FIELD) {
                status=results_eval_checked(r,task,f,root->a,&p->owner,line,budget,entryRead);
                if(status!=1)return status;
                if(p->owner.kind==V_ARRAY)return rfail(r,line,"array size field is read-only");
                if(!state_receiver(r,&p->owner,line))return 0;
                p->owner_guard=1;
                if(e->kind==E_INDEX) {
                    p->path.owner=p->owner;
                    if(!state_field_read(r,&p->owner,root->name,line,&p->path.base))return 0;
                    p->current=p->path.base;
                } else if(op&&!state_field_read(r,&p->owner,root->name,line,&p->current))return 0;
            } else if(e->kind==E_INDEX||op) {
                status=results_eval_checked(r,task,f,e->kind==E_INDEX?p->path.root:lhs,&p->current,line,budget,entryRead);
                if(status!=1)return status;
                if(e->kind==E_INDEX)p->path.base=p->current;
            }
            if(e->kind==E_INDEX)p->path_ready=1;
            p->assign_phase=e->kind==E_INDEX?1:3;
        } else if(p->assign_phase==1) {
            if(p->key_index>=p->path.count) {p->path_ready=1;p->assign_phase=3;continue;}
            if(p->current.kind!=V_UNDEFINED&&p->current.kind!=V_ARRAY)
                return rfail(r,line,"indexed write requires array containers");
            n=p->key_index;
            status=results_eval_checked(r,task,f,r->program->expr[p->chain[p->path.count-1-n]].b,&key,line,budget,entryRead);
            if(status!=1)return status;
            if(!results_guard(r,task,line)||!array_key(r,&key,line,&p->path.keys[n]))return 0;
            memset(&next,0,sizeof(next));
            if(p->current.kind==V_UNDEFINED) {
                if(!r->sparse_numeric_arrays_enabled&&p->path.keys[n].kind==V_NUMBER&&p->path.keys[n].number!=0)
                    return rfail(r,line,"sparse numeric array writes are unsupported");
            } else {
                if(!array_value_valid(r,p->current.array))return rfail(r,line,"invalid array handle");
                if(!r->sparse_numeric_arrays_enabled&&p->path.keys[n].kind==V_NUMBER&&
                   p->path.keys[n].number>r->arrays[p->current.array-1].numeric_count)
                    return rfail(r,line,"sparse numeric array writes are unsupported");
                if(!array_read_key(r,p->current.array,&p->path.keys[n],line,&next))return 0;
            }
            p->ancestors[n]=results_capture_ancestor(&next);p->current=next;p->key_index++;
        } else if(p->assign_phase==3) {
            memset(&p->value,0,sizeof(p->value));p->value.kind=V_NUMBER;p->value.integer=1;p->value.number=1;
            if(rhs>=0) {
                status=results_eval_checked(r,task,f,rhs,&p->value,line,budget,entryRead);
                if(status!=1)return status;
            }
            p->assign_phase=4;
        } else if(p->assign_phase==4) {
            result=p->value;
            if(op&&!results_mutation(r,lhs,op,&p->current,&p->value,line,&result))return 0;
            if(!results_guard(r,task,line))return 0;
            if(!(e->kind==E_INDEX?array_lvalue_commit(r,task,f,&p->path,&result,array_assignment_clear(r->program,p->rhs,p->op),line):
                e->kind==E_VAR?assignment(r,f,e->name,result,line):
                state_field_write(r,&p->owner,e->name,&result,line)))return 0;
            p->assign_phase=6;
        } else return rfail(r,line,"invalid assignment continuation");
    }
    p->assignment_active=0;p->assign_phase=0;p->owner_guard=0;p->path_ready=0;
    memset(&p->path,0,sizeof(p->path));return 1;
}
static int results_condition(GscRuntime *r,const Value *value,int line,int *passes) {
    if(value->kind==V_UNDEFINED)return rfail(r,line,"undefined condition");
    if(value->kind>V_STRING)return rfail(r,line,"vector/entity condition semantics are unsupported");
    *passes=truth(*value);return 1;
}
static int results_loop(GscRuntime *r,Task *task,Frame *f,ResultsPending *p,int line,int *budget) {
    const GscProgram *g=r->program;int status,passes=1;
    if(p->stage==0) {
        if(p->boundary)p->loop=f->loops[f->loop_count-1];
        else if(!flow_loop_record(g,p->instruction,&p->loop)||f->loop_count>=MAX_CONTROL_DEPTH)
            return rfail(r,line,"invalid loop context");
        p->stage=1;
    }
    if(p->loop.kind==I_SWITCH) {
        if(!p->boundary)return rfail(r,line,"invalid switch boundary");
        f->loop_count--;f->pc=p->loop.end;p->stage=3;return 1;
    }
    if(p->stage==1) {
        if(p->loop.kind==I_FOR) {
            const ForNode *node=&g->loops[p->loop.index];int lhs,rhs,op;
            lhs=p->boundary?node->step_target:node->init_target;
            rhs=p->boundary?node->step_expr:node->init_expr;op=0;
            if(p->boundary) {
                if(node->step_kind==FOR_NO_STEP)lhs=-1;
                else if(node->step_kind!=FOR_ASSIGN)op=node->step_kind==FOR_INCREMENT?GSC_MUT_INC:
                    node->step_kind==FOR_DECREMENT?GSC_MUT_DEC:node->step_op;
            }
            if(lhs>=0) {
                status=results_assign(r,task,f,lhs,rhs,op,line,budget);
                if(status!=1)return status;
            }
        }
        p->stage=2;
    }
    if(p->stage==2) {
        if((p->boundary||p->loop.kind!=I_DO_WHILE)&&p->loop.cond>=0) {
            status=results_eval(r,task,f,p->loop.cond,&p->value,line,budget);
            if(status!=1)return status;
            if(!results_condition(r,&p->value,line,&passes))return 0;
        }
        if(passes) {
            if(!p->boundary)f->loops[f->loop_count++]=p->loop;
            f->pc=p->loop.start;
        } else {
            if(p->boundary)f->loop_count--;
            f->pc=p->loop.end;
        }
        p->stage=3;
    }
    return 1;
}
static int array_collect_at_boundary(GscRuntime *,Task *);
static int run_task_results(GscRuntime *r,Task *task,int *budget) {
    const GscProgram *g=r->program;const Instr *i;const Func *fun;
    ResultsTask *state;ResultsPending *p;Frame *f;int status,n,target,delay,passes;
    double ms;
    if(!task->results) {
        task->results=GSC_RUNTIME_CALLOC(1,sizeof(*task->results));
        if(!task->results)return rfail(r,0,"expression continuation allocation failed");
    }
    state=task->results;
    while(task->status==TASK_READY&&task->depth) {
        int entryRead=0;
        if(!flow_budget(r,budget,0)||!results_guard_read(r,task,0,&entryRead))return 0;
        f=&task->frames[task->depth-1];fun=&g->funcs[f->fn];p=&state->pending[task->depth-1];
        if(!p->active) {
            /* A top-level scheduler invocation owns this fresh statement.
             * No native evaluator or child thread holds a borrowed Value. */
            if(task==r->array_dispatch_root &&
               ((r->array_count>=GSC_ARRAY_CONTAINER_CAPACITY*3/4 && r->array_count>=r->array_next_scan) ||
                (r->array_entry_count>=GSC_ARRAY_ENTRY_CAPACITY*3/4 && r->array_entry_count>=r->array_next_entry_scan))){
                entryRead=0; /* Collection rewrites VM-owned handles. */
                if(!array_collect_at_boundary(r,task))return 0;
                /* Leave room for the largest getentarray result. Rescan less often
                 * when most objects survive, but always revisit a nearly full pool. */
                r->array_next_scan=r->array_count+GSC_ARRAY_CONTAINER_CAPACITY/32;
                if(r->array_next_scan>=GSC_ARRAY_CONTAINER_CAPACITY)
                    r->array_next_scan=GSC_ARRAY_CONTAINER_CAPACITY-1;
                r->array_next_entry_scan=r->array_entry_count+GSC_ARRAY_CONTAINER_CAPACITY;
                if(r->array_next_entry_scan>=GSC_ARRAY_ENTRY_CAPACITY-GSC_ARRAY_CONTAINER_CAPACITY)
                    r->array_next_entry_scan=GSC_ARRAY_ENTRY_CAPACITY-GSC_ARRAY_CONTAINER_CAPACITY;
            }
            if(f->loop_count&&f->pc>=f->loops[f->loop_count-1].end) {
                if(f->pc>f->loops[f->loop_count-1].end)return rfail(r,0,"escaped active loop body");
                p->active=1;p->boundary=1;p->instruction=f->loops[f->loop_count-1].owner;
            } else if(f->pc>=fun->end) {
                Value undefined;memset(&undefined,0,sizeof(undefined));
                if(!results_return(r,task,&undefined,0))return 0;
                continue;
            } else {int next_pc=developer_skip_region(g,f->pc);if(next_pc!=f->pc){f->pc=next_pc;continue;}p->active=1;p->instruction=f->pc++;}
        }
        i=&g->code[p->instruction];status=1;
        if(p->boundary)status=results_loop(r,task,f,p,i->line,budget);
        else switch(i->kind) {
          case I_EMPTY:break;
          case I_SET:
            status=results_eval_checked(r,task,f,i->expr,&p->value,i->line,budget,entryRead);
            if(status==1&&!assignment(r,f,i->name,p->value,i->line))return 0;
            break;
          case I_GAME_SET:case I_MUTATE:
            status=results_assign(r,task,f,i->dest,i->expr,i->kind==I_MUTATE?i->op:0,i->line,budget);break;
          case I_SWITCH:
            if(!r->scalar_switch_enabled)return rfail(r,i->line,"unsupported switch runtime semantics");
            status=results_eval_checked(r,task,f,i->expr,&p->value,i->line,budget,entryRead);
            if(status==1&&!flow_switch_select(r,f,p->instruction,&p->value,i->line))return 0;
            break;
          case I_FOR:case I_WHILE:case I_DO_WHILE:
            status=results_loop(r,task,f,p,i->line,budget);break;
          case I_RETURN:
            if(i->expr>=0)status=results_eval_checked(r,task,f,i->expr,&p->value,i->line,budget,entryRead);
            if(status==1) {
                if(!results_return(r,task,&p->value,i->line))return 0;
                continue;
            }
            break;
          case I_BREAK:case I_CONTINUE:
            if(!flow_transfer(r,f,i))return 0;
            break;
          case I_JUMP:f->pc=i->dest;break;
          case I_JZ:case I_WAIT:case I_WAITER:case I_NOTIFY:
            if(r->event_payloads_enabled&&(i->kind==I_WAITER||i->kind==I_NOTIFY)) {
                int evaluated=i->kind==I_WAITER?1:i->argc;
                while(p->argument<evaluated) {
                    n=p->argument;
                    status=results_eval(r,task,f,i->args[n],&p->args[n],i->line,budget);
                    if(status!=1)break;
                    p->argument++;
                }
                if(status==1&&!run_payload_event(r,task,f,
                    i->kind==I_WAITER?ENTITY_EVENT_WAITTILL:ENTITY_EVENT_NOTIFY,
                    NULL,p->args,i->args,i->argc,i->line))return 0;
                break;
            }
            status=results_eval_checked(r,task,f,i->expr,&p->value,i->line,budget,entryRead);
            if(status!=1)break;
            if(i->kind==I_JZ) {
                if(!results_condition(r,&p->value,i->line,&passes))return 0;
                if(!passes)f->pc=i->dest;
            } else if(i->kind==I_WAIT) {
                if(p->value.kind!=V_NUMBER||!(p->value.number>=0))
                    return rfail(r,i->line,"wait requires nonnegative seconds");
                if(r->r237_frame_buckets_enabled){if(!r237_timed_wait(r,task,&p->value,i->line))return 0;}
                else if(r->frame_cadence_enabled&&float_arithmetic_active(r)) {
                    if(task->cadence_event_resume)return rfail(r,i->line,"selected cadence event-to-timed-wait queue base is held");
                    if(!frame_cadence_wait_delay(r,&p->value,i->line,&delay))return 0;
                    task->wake_ms=r->cadence_drain_ms+delay;
                } else {
                    ms=(double)p->value.number*1000.0;
                    if(ms>INT_MAX-r->now_ms)return rfail(r,i->line,"wait time overflow");
                    delay=(int)floor(ms+0.5);if(delay<1)delay=1;
                    task->wake_ms=r->now_ms+delay;
                }
                task->status=TASK_SLEEP;if(r->precache_string_enabled)precache_string_wait_boundary(r,task);
            } else {
                if(!event_string(r,p->value,task->event,sizeof(task->event),i->line))return 0;
                if(i->kind==I_WAITER) {
                    task->event_scope=EVENT_LEVEL;memset(&task->event_owner,0,sizeof(task->event_owner));
                    task->status=TASK_EVENT;if(r->precache_string_enabled)precache_string_wait_boundary(r,task);
                } else {if(!notify_internal(r,task->event)&&r->r237_frame_buckets_enabled)return 0;task->event[0]=0;}
            }
            break;
          case I_DYNAMIC_CALL:case I_DYNAMIC_THREAD:
            if(!r->function_refs_enabled)return rfail(r,i->line,"computed call mode is unavailable");
            if(p->stage==0) {
                while(p->argument<i->argc) {
                    n=i->argc-1-p->argument;
                    status=results_eval(r,task,f,i->args[n],&p->args[n],i->line,budget);
                    if(status!=1)break;
                    p->argument++;
                }
                if(status!=1)break;
                if(i->receiver>=0) {
                    status=results_eval(r,task,f,i->receiver,&p->receiver,i->line,budget);
                    if(status!=1)break;
                    if(!state_receiver(r,&p->receiver,i->line))return 0;
                    p->receiver_guard=1;
                }
                p->stage=1;
            }
            if(p->stage==1) {
                status=results_eval(r,task,f,i->expr,&p->current,i->line,budget);
                if(status!=1)break;
                if(!function_value_valid(r,&p->current))return rfail(r,i->line,"computed target requires a current script function reference");
                p->function_guard=1;p->stage=2;
            }
            if(!results_guard(r,task,i->line)||!function_target(r,&p->current,i->argc,i->line,&target))return 0;
            {Value self=i->receiver>=0?p->receiver:f->self;
             if(i->kind==I_DYNAMIC_THREAD) {
                if(!spawn(r,target,p->args,i->argc,&self,i->line))return 0;
             } else {
                if(!push_frame(r,task,target,p->args,i->argc,&self,i->line))return 0;
                memset(&state->pending[task->depth-1],0,sizeof(state->pending[0]));
             }}
            break;
          case I_METHOD:case I_METHOD_THREAD:case I_CALL:case I_THREAD:
            target=i->kind==I_CALL?i->target:
                i->kind==I_THREAD?0:
                (i->receiver_binding_kind==GSC_RECEIVER_TARGET_LOCAL_SCRIPT||
                 i->receiver_binding_kind==GSC_RECEIVER_TARGET_QUALIFIED_SCRIPT)?0:-1;
            /* Keep native argument policy separate from original script
             * args RTL -> receiver -> callee sequencing. */
            if(target>=0) {
                while(p->argument<i->argc) {
                    n=i->argc-1-p->argument;
                    status=results_eval(r,task,f,i->args[n],&p->args[n],i->line,budget);
                    if(status!=1)break;
                    p->argument++;
                }
                if(status!=1)break;
            }
            if(i->kind==I_METHOD||i->kind==I_METHOD_THREAD) {
                if(p->stage==0) {
                    int defer;
                    status=results_eval(r,task,f,i->receiver,&p->receiver,i->line,budget);
                    if(status!=1)break;
                    defer=r->actor_spawners_enabled&&i->kind==I_METHOD&&
                        i->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE&&
                        actor_spawn_native_descriptor(i->receiver_native_target)&&p->receiver.kind!=V_ENTITY;
                    if(!defer) {
                        if(!state_receiver(r,&p->receiver,i->line))return 0;
                        p->receiver_guard=1;
                    }
                    p->stage=1;
                }
            }
            while(p->argument<(r->event_payloads_enabled&&i->kind==I_METHOD&&
                  i->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_EVENT&&
                  i->receiver_event_target==ENTITY_EVENT_WAITTILL?1:i->argc)) {
                n=p->argument;
                status=results_eval(r,task,f,i->args[n],&p->args[n],i->line,budget);
                if(status!=1)break;
                p->argument++;
            }
            if(status!=1)break;
            if(!results_guard(r,task,i->line))return 0;
            if(i->kind==I_CALL&&i->target<0) {
                if(!invoke_builtin_statement(r,-1-i->target,p->args,i->argc,i->line))return 0;
            } else if(i->kind==I_METHOD&&i->receiver_binding_kind==GSC_RECEIVER_TARGET_HUDELEM_NATIVE) {
                if(!hud_method_invoke(r,&p->receiver,i->receiver_native_target,p->args,i->argc,i->line))return 0;
            } else if(i->kind==I_METHOD&&i->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_EVENT) {
                if(r->event_payloads_enabled) {
                    if(!run_payload_event(r,task,f,i->receiver_event_target,&p->receiver,
                        p->args,i->args,i->argc,i->line))return 0;
                } else if(!run_entity_event(r,task,i->receiver_event_target,&p->receiver,&p->args[0],i->line))return 0;
            } else if(i->kind==I_METHOD&&i->receiver_binding_kind==GSC_RECEIVER_TARGET_ENTITY_NATIVE) {
                int descriptor=i->receiver_native_target;
                Value ignored;
                if(!(descriptor==ENTITY_METHOD_SCRIPTED_START?r243_scripted_start_invoke(r,&p->receiver,p->args,i->argc,i->line):descriptor==ENTITY_METHOD_SCRIPT_STATE?r243_script_state_invoke(r,&p->receiver,p->args,i->argc,i->line):descriptor==ENTITY_METHOD_SCRIPTED_PREFIX?r243_scripted_prefix_invoke(r,&p->receiver,p->args,i->argc,i->line):R243ActorFloatMask(descriptor)?r243_actor_float_call(r,&p->receiver,descriptor,i->argc,i->line,&ignored):descriptor==COD1_ALLOWED_STANCES_METHOD?cod1_stance_invoke(r,&p->receiver,p->args,i->argc,i->line,NULL,1):descriptor==R243_STANCE_METHOD?r243_stance_call(r,&p->receiver,p->args,i->argc,i->line,&ignored):descriptor==R243_ANIMTIME_METHOD?r243_animtime_call(r,&p->receiver,p->args,i->argc,i->line,&ignored):descriptor==ENTITY_METHOD_ACTOR_DROP?actor_drop_invoke(r,&p->receiver,p->args,i->argc,i->line):actor_look_kind(descriptor)?actor_look_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line,&ignored):actor_animation_kind(descriptor)?actor_animation_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line):turret_animation_kind(descriptor)?turret_animation_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line):turret_method_kind(descriptor)?turret_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line,&ignored):
                     attachment_query_kind(descriptor)?attachment_query_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line,&ignored):
                     descriptor==504?entity_method_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line):
                 (descriptor==503||descriptor==COD1_SET_GOAL_POS_METHOD||descriptor==COD1_SET_GOAL_ENTITY_METHOD)?actor_goal_invoke(r,descriptor,&p->receiver,p->args,i->argc,i->line):
                     (descriptor==502 || descriptor==COD1_ROTATE_VELOCITY || descriptor==COD1_MOVE_GRAVITY || descriptor==COD1_LAUNCH || descriptor==COD1_ROTATE_ROLL || descriptor==COD1_ROTATE_TO || descriptor==COD1_MOVE_TO)?yaw_motion_invoke(r,descriptor,&p->receiver,p->args,i->argc,i->line):
                     (descriptor==501||descriptor==GSC_CONNECTPATHS_METHOD||descriptor==GSC_DISCONNECTPATHS_METHOD||descriptor==GSC_SOLID_METHOD||descriptor==GSC_GRENADE_TOUCH_METHOD||descriptor==GSC_GRENADE_BOUNCE_METHOD)?entity_collision_invoke(r,descriptor,&p->receiver,i->argc,i->line):
                     spatial_method_kind(descriptor)?spatial_native_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line,&ignored):
                     entity_coordinate(descriptor)?entity_coordinate_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line,&ignored):
                     entity_link_method(descriptor)?entity_link_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line):
                     vehicle_method_kind(descriptor)?vehicle_method_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line):
                     entity_operation(descriptor)?entity_operation_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line,&ignored):
                     player_method_kind(descriptor)?player_native_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line,&ignored):
                     descriptor>=ENTITY_METHOD_ATTACH&&descriptor<=ENTITY_METHOD_STOPLOOPSOUND?
                     entity_method_invoke(r,&p->receiver,descriptor,p->args,i->argc,i->line):
                     actor_spawn_native_descriptor(descriptor)?
                     actor_spawn_expression(r,descriptor,&p->receiver,p->args,i->argc,i->line,&ignored):
                     descriptor==ENTITY_METHOD_GETNUMBER?native_entity_number(r,&p->receiver,i->argc,i->line,&ignored):
                     descriptor==ENTITY_METHOD_SETMODEL?bridge_setmodel(r,&p->receiver,p->args,i->argc,i->line):
                     descriptor==ENTITY_METHOD_HIDE||descriptor==ENTITY_METHOD_SHOW?
                     bridge_setvisible(r,&p->receiver,descriptor,i->argc,i->line):
                     bridge_setorigin(r,&p->receiver,p->args,i->argc,i->line)))return 0;
            } else {
                Value self=(i->kind==I_METHOD||i->kind==I_METHOD_THREAD)?p->receiver:f->self;
                target=(i->kind==I_METHOD||i->kind==I_METHOD_THREAD)?i->receiver_script_target:i->target;
                if(target<0||target>=g->func_count)return rfail(r,i->line,"invalid script call target");
                if(i->kind==I_THREAD||i->kind==I_METHOD_THREAD) {
                    if(!spawn(r,target,p->args,i->argc,&self,i->line))return 0;
                } else {
                    if(!push_frame(r,task,target,p->args,i->argc,&self,i->line))return 0;
                    memset(&state->pending[task->depth-1],0,sizeof(state->pending[0]));
                }
            }
            break;
          default:return rfail(r,i->line,"unsupported resumable instruction");
        }
        if(!status)return 0;
        if(status==2)continue;
        /* p still identifies the original caller even after a statement call
         * pushed a frame. Its completed statement is cleared exactly once. */
        /* A completed assignment has cleared path. Only ancestors below
         * key_index have been written. Keep them as roots through a yielding
         * loop condition, then clear them with the rest of this statement. */
        memset(p,0,offsetof(ResultsPending,ancestors)+(size_t)p->key_index*sizeof(p->ancestors[0]));
    }
    if(!task->depth)task->status=TASK_DEAD;
    return 1;
}
