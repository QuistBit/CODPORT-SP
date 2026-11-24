/* R92 private flow execution. Syntax IR remains read-only. Each call frame owns
 * bounded loop contexts, so sleeping/event tasks resume at the same phase. */
static int flow_reserved(const char *name) {
    return literal_name(name)||!strcmp(name,"self")||!strcmp(name,"level");
}
static int flow_budget(GscRuntime *r,int *budget,int line) {
    return --*budget>=0?1:rfail(r,line,"instruction budget exceeded");
}
static int flow_lvalue_valid(const GscProgram *g,int index) {
    const Expr *e;
    if(index<0||index>=g->expr_count)return 0;
    e=&g->expr[index];
    if(g->arrays_mode&&e->kind==E_INDEX)return array_lvalue_valid(g,index);
    if(g->arrays_mode&&e->kind==E_FIELD&&control_field_equal(e->name,"size")&&
       g->expr[e->a].kind==E_EMPTY_ARRAY)return 0;
    if(e->kind==E_VAR)return e->name[0]&&!flow_reserved(e->name)&&strlen(e->name)<64;
    if(e->kind!=E_FIELD||e->a<0||e->a>=index||!e->name[0])return 0;
    /* Native metadata is immutable; a statically named level field is ordinary
     * VM state. Unknown aliases conservatively cannot write metadata. */
        if(!g->struct_mode&&control_metadata_name(e->name)&&
       !(g->expr[e->a].kind==E_VAR&&!strcmp(g->expr[e->a].name,"level")))return 0;
    return 1;
}
static int flow_lvalue_owner(GscRuntime *r,Frame *f,int index,int line,Value *owner) {
    const Expr *e=&r->program->expr[index];
    memset(owner,0,sizeof(*owner));
    return e->kind==E_VAR?1:eval(r,f,e->a,0,owner,line);
}
static int flow_lvalue_read(GscRuntime *r,Frame *f,int index,const Value *owner,
                             int line,Value *value) {
    const Expr *e=&r->program->expr[index];
    return e->kind==E_VAR?eval(r,f,index,0,value,line):
        state_field_read(r,owner,e->name,line,value);
}
static int flow_lvalue_write(GscRuntime *r,Task *task,Frame *f,int index,
                              const Value *owner,const Value *value,int line) {
    const Expr *e=&r->program->expr[index];
    if(!refresh_state_context(r,task))return 0;
    if(task->status==TASK_DEAD)return 1;
    return e->kind==E_VAR?assignment(r,f,e->name,*value,line):
        state_field_write(r,owner,e->name,value,line);
}
static int flow_assign(GscRuntime *r,Task *task,Frame *f,int lhs,int rhs,int line) {
    Value owner,value;
    if(r->arrays_enabled&&r->program->expr[lhs].kind==E_INDEX)
        return array_assign(r,task,f,lhs,rhs,0,line);
    if(!flow_lvalue_valid(r->program,lhs))return rfail(r,line,"invalid flow assignment lvalue");
    if(!flow_lvalue_owner(r,f,lhs,line,&owner))return 0;
    if(r->arrays_enabled&&owner.kind==V_ARRAY)return rfail(r,line,"array size field is read-only");
    if(r->arrays_enabled&&r->program->expr[lhs].kind==E_FIELD&&!state_receiver(r,&owner,line))return 0;
    if(!eval(r,f,rhs,0,&value,line))return 0;
    return flow_lvalue_write(r,task,f,lhs,&owner,&value,line);
}
static int flow_mutation(GscRuntime *r,Task *task,Frame *f,int lhs,int rhs,int op,int line) {
    Value owner,current,arg,result;int k;
    if(r->arrays_enabled&&r->program->expr[lhs].kind==E_INDEX)
        return array_assign(r,task,f,lhs,rhs,op,line);
    if(!flow_lvalue_valid(r->program,lhs)||op<GSC_MUT_ADD||
       (op>GSC_MUT_DEC&&!(r->integer_bitwise_enabled&&integer_bitwise_mutation_op(op)))||
       ((op<=GSC_MUT_DIV||integer_bitwise_mutation_op(op))!=(rhs>=0)))return rfail(r,line,"invalid flow mutation");
    if(!flow_lvalue_owner(r,f,lhs,line,&owner)||
       !flow_lvalue_read(r,f,lhs,&owner,line,&current))return 0;
    if(r->arrays_enabled&&r->program->expr[lhs].kind==E_FIELD&&!state_receiver(r,&owner,line))return 0;
    memset(&arg,0,sizeof(arg));arg.kind=V_NUMBER;arg.integer=1;arg.number=1;
    if(rhs>=0&&!eval(r,f,rhs,0,&arg,line))return 0;
    memset(&result,0,sizeof(result));
    if(r->integer_bitwise_enabled&&integer_bitwise_mutation_op(op)) {
        if(!integer_bitwise_mutation(r,op,&current,&arg,line,&result))return 0;
    } else if(op==GSC_MUT_ADD&&string_vector_concat_pair(r,&current,&arg)) {
        if(!string_vector_concat(r,&current,&arg,line,&result))return 0;
    } else if(r->vector_arithmetic_enabled&&(current.kind==V_VECTOR||arg.kind==V_VECTOR)) {
        if(!vector_arithmetic_mutation(r,op,&current,&arg,line,&result))return 0;
    } else if(current.kind==V_VECTOR&&arg.kind==V_VECTOR&&op==GSC_MUT_ADD&&
       r->program->expr[lhs].kind==E_FIELD&&
       control_field_equal(r->program->expr[lhs].name,"origin")) {
        result.kind=V_VECTOR;
        for(k=0;k<3;k++) {
            result.vector[k]=current.vector[k]+arg.vector[k];
            if(!isfinite(result.vector[k]))return rfail(r,line,"nonfinite origin sum");
        }
    } else {
        if(!(r->scalar_switch_enabled&&op==GSC_MUT_ADD&&
             current.kind==V_STRING&&arg.kind==V_STRING)&&
           !(op==GSC_MUT_ADD&&string_int_concat_pair(r,&current,&arg))&&
           (current.kind!=V_NUMBER||arg.kind!=V_NUMBER||
            !isfinite(current.number)||!isfinite(arg.number)))
            return rfail(r,line,"numeric mutation requires finite numbers");
        if(r->scalar_switch_enabled) {
            if(!scalar_mutation(r,op,&current,&arg,line,&result))return 0;
        } else {
        result.kind=V_NUMBER;result.integer=current.integer&&arg.integer;
        switch(op) {
          case GSC_MUT_ADD:case GSC_MUT_INC:result.number=current.number+arg.number;break;
          case GSC_MUT_SUB:case GSC_MUT_DEC:result.number=current.number-arg.number;break;
          case GSC_MUT_MUL:result.number=current.number*arg.number;break;
          case GSC_MUT_DIV:
            if(arg.number==0.0f)return rfail(r,line,"division by zero");
            result.number=current.number/arg.number;result.integer=0;break;
          default:return rfail(r,line,"invalid numeric mutation");
        }
        if(!isfinite(result.number))return rfail(r,line,"nonfinite mutation");
        }
    }
    return flow_lvalue_write(r,task,f,lhs,&owner,&result,line);
}
static int flow_condition(GscRuntime *r,Task *task,Frame *f,int expression,
                            int line,int *budget,int *passes) {
    Value value;
    if(!flow_budget(r,budget,line))return 0;
    if(expression<0){*passes=1;return 1;}
    if(!eval(r,f,expression,0,&value,line)||!refresh_state_context(r,task))return 0;
    if(task->status==TASK_DEAD)return 1;
    if(value.kind==V_UNDEFINED)return rfail(r,line,"undefined loop condition");
    if(value.kind>V_STRING)return rfail(r,line,"vector/entity loop condition semantics are unsupported");
    *passes=truth(value);return 1;
}
static int flow_loop_record(const GscProgram *g,int owner,FlowLoop *out) {
    const Instr *i;
    if(owner<0||owner>=g->code_count)return 0;
    i=&g->code[owner];memset(out,0,sizeof(*out));
    out->owner=owner;out->kind=i->kind;out->index=i->dest;
    if(i->kind==I_FOR) {
        GscForInfo info;
        if(!gsc_program_for_get(g,i->dest,&info)||info.instruction!=owner)return 0;
        out->start=info.body_start;out->end=info.body_end;out->cond=info.cond_expr;
    } else if(i->kind==I_WHILE) {
        const WhileNode *loop;
        if(i->dest<0||i->dest>=g->while_count)return 0;
        loop=&g->whiles[i->dest];
        if(loop->body_start!=owner+1||loop->cond_expr!=i->expr||
           loop->cond_expr<0||loop->cond_expr>=g->expr_count||loop->line!=i->line)return 0;
        out->start=loop->body_start;out->end=loop->body_end;out->cond=loop->cond_expr;
    } else if(i->kind==I_DO_WHILE) {
        GscDoWhileInfo info;
        if(!gsc_program_do_while_get(g,i->dest,&info)||info.instruction!=owner)return 0;
        out->start=info.body_start;out->end=info.body_end;out->cond=info.cond_expr;
    } else if(i->kind==I_SWITCH&&g->scalar_switch_mode) {
        const SwitchNode *node;
        if(i->dest<0||i->dest>=g->switch_count)return 0;
        node=&g->switches[i->dest];
        if(node->instruction!=owner||node->selector_expr!=i->expr)return 0;
        out->start=node->body_start;out->end=node->body_end;out->cond=node->selector_expr;
    } else return 0;
    return out->start==owner+1&&out->end>=out->start&&out->end<=g->code_count;
}
/* Native opcode0x54 accepts only type5 ints within its 24-bit key range
 * and raw interned strings. Case traversal is lexical for fallthrough; matching
 * searches every sibling before default. The selector is evaluated once. */
static int flow_switch_select(GscRuntime *r,Frame *f,int owner,const Value *selector,int line) {
    FlowLoop control;const SwitchNode *node;int n,matched=-1;int32_t integer=0;
    if(!r->scalar_switch_enabled||!flow_loop_record(r->program,owner,&control)||
       control.kind!=I_SWITCH||f->loop_count>=MAX_CONTROL_DEPTH)
        return rfail(r,line,"invalid switch context");
    node=&r->program->switches[control.index];
    if(selector->kind==V_NUMBER&&selector->integer) {
        if(!scalar_get_integer(r,selector,line,&integer))return 0;
        if(integer< -8257536||integer>8388607)
            return rfail(r,line,"switch integer outside native key range");
    } else if(selector->kind!=V_STRING)return rfail(r,line,"switch selector requires integer or ordinary string");
    for(n=node->first_case;n>=0;n=r->program->cases[n].next_case) {
        const CaseNode *label=&r->program->cases[n];
        if((selector->kind==V_NUMBER&&label->kind==GSC_CASE_INTEGER&&integer==label->integer)||
           (selector->kind==V_STRING&&label->kind==GSC_CASE_STRING&&!strcmp(selector->str,label->string))) {matched=n;break;}
    }
    if(matched<0)matched=node->default_case;
    if(matched<0||r->program->cases[matched].body_start==control.end){f->pc=control.end;return 1;}
    f->loops[f->loop_count++]=control;f->pc=r->program->cases[matched].body_start;return 1;
}
static int flow_for_step(GscRuntime *r,Task *task,Frame *f,const FlowLoop *loop,int *budget) {
    const ForNode *node=&r->program->loops[loop->index];int line=node->line;
    if(!flow_budget(r,budget,line))return 0;
    if(node->step_kind==FOR_NO_STEP)return 1;
    if(node->step_kind==FOR_ASSIGN)
        return flow_assign(r,task,f,node->step_target,node->step_expr,line);
    return flow_mutation(r,task,f,node->step_target,node->step_expr,
        node->step_kind==FOR_INCREMENT?GSC_MUT_INC:
        node->step_kind==FOR_DECREMENT?GSC_MUT_DEC:node->step_op,line);
}
static int flow_loop_begin(GscRuntime *r,Task *task,Frame *f,int owner,
                            const Instr *instruction,int *budget) {
    FlowLoop loop;int passes=1;
    if(!flow_loop_record(r->program,owner,&loop)||f->loop_count>=MAX_CONTROL_DEPTH)
        return rfail(r,instruction->line,"invalid loop context");
    if(instruction->kind==I_FOR) {
        const ForNode *node=&r->program->loops[loop.index];
        if(node->init_target>=0) {
            if(!flow_budget(r,budget,instruction->line)||
               !flow_assign(r,task,f,node->init_target,node->init_expr,instruction->line))return 0;
            if(task->status==TASK_DEAD)return 1;
        }
    }
    if(instruction->kind!=I_DO_WHILE&&
       !flow_condition(r,task,f,loop.cond,instruction->line,budget,&passes))return 0;
    if(task->status==TASK_DEAD)return 1;
    if(!passes){f->pc=loop.end;return 1;}
    f->loops[f->loop_count++]=loop;f->pc=loop.start;return 1;
}
static int flow_loop_boundary(GscRuntime *r,Task *task,Frame *f,int *budget) {
    FlowLoop loop=f->loops[f->loop_count-1];int passes=1;
    int line=r->program->code[loop.owner].line;
    if(loop.kind==I_SWITCH){f->loop_count--;f->pc=loop.end;return 1;}
    if(loop.kind==I_FOR&&!flow_for_step(r,task,f,&loop,budget))return 0;
    if(task->status==TASK_DEAD)return 1;
    if(!flow_condition(r,task,f,loop.cond,line,budget,&passes))return 0;
    if(task->status==TASK_DEAD)return 1;
    if(passes)f->pc=loop.start;
    else {f->loop_count--;f->pc=loop.end;}
    return 1;
}
static int flow_transfer(GscRuntime *r,Frame *f,const Instr *instruction) {
    int n;
    for(n=f->loop_count-1;n>=0;n--)if(f->loops[n].owner==instruction->target) {
        if(instruction->kind==I_CONTINUE&&f->loops[n].kind==I_SWITCH)
            return rfail(r,instruction->line,"continue cannot target a switch");
        f->pc=f->loops[n].end;
        f->loop_count=instruction->kind==I_BREAK?n:n+1;
        return 1;
    }
    return rfail(r,instruction->line,"break/continue target is not an active loop");
}
static int flow_preflight_error(char *error,size_t size,int line,const char *why) {
    if(error&&size)snprintf(error,size,"line %d: %s",line,why);
    return 0;
}
static int flow_mutation_preflight(const GscProgram *g,int lhs,int rhs,int op,
                                    int line,char *error,size_t size) {
    int type=rhs<0?-1:known_value_kind(g,rhs);
    if(!flow_lvalue_valid(g,lhs)||op<GSC_MUT_ADD||
       (op>GSC_MUT_DEC&&!(g->integer_bitwise_selected&&integer_bitwise_mutation_op(op)))||
       rhs< -1||rhs>=g->expr_count||((op<=GSC_MUT_DIV||integer_bitwise_mutation_op(op))!=(rhs>=0)))
        return flow_preflight_error(error,size,line,"unsupported mutation lvalue or operator");
    if(type>=0&&type!=V_NUMBER&&
       !(type==V_STRING&&g->scalar_switch_mode&&op==GSC_MUT_ADD)&&
       !(g->vector_arithmetic_selected&&type==V_VECTOR&&(op==GSC_MUT_ADD||op==GSC_MUT_SUB))&&
       !(type==V_VECTOR&&op==GSC_MUT_ADD&&
       g->expr[lhs].kind==E_FIELD&&control_field_equal(g->expr[lhs].name,"origin")))
        return flow_preflight_error(error,size,line,"unsupported mutation literal type");
    if(type==V_NUMBER&&!(g->vector_arithmetic_selected&&(op==GSC_MUT_MUL||op==GSC_MUT_DIV))&&g->expr[lhs].kind==E_FIELD&&
       control_field_equal(g->expr[lhs].name,"origin")&&
       !(g->expr[g->expr[lhs].a].kind==E_VAR&&!strcmp(g->expr[g->expr[lhs].a].name,"level")))
        return flow_preflight_error(error,size,line,"entity origin mutation requires a vector");
    if(op==GSC_MUT_DIV&&g->expr[rhs].kind==E_NUMBER&&g->expr[rhs].number==0)
        return flow_preflight_error(error,size,line,"literal division by zero");
    return 1;
}
/* Validate all retained headers, lvalues and lexical transfers, including dead
 * functions. No execution, callbacks or syntax rewriting occurs here. */
static int flow_runtime_preflight(const GscProgram *g,const GscEntityAccessHost *access,
                                  const GscEntityControlHost *controls,char *error,size_t size) {
    int fn,pc,n,loop_count;FlowLoop stack[MAX_CONTROL_DEPTH];
    for(fn=0;fn<g->func_count;fn++) {
        const Func *function=&g->funcs[fn];loop_count=0;
        for(n=0;n<function->argc;n++)if(flow_reserved(function->params[n]))
            return flow_preflight_error(error,size,0,"reserved parameter binding");
        for(pc=function->start;pc<function->end;pc++) {
            const Instr *i=&g->code[pc];
            while(loop_count&&pc==stack[loop_count-1].end)loop_count--;
            if(loop_count&&pc>stack[loop_count-1].end)
                return flow_preflight_error(error,size,i->line,"invalid nested loop span");
            if(i->kind==I_SET&&flow_reserved(i->name))
                return flow_preflight_error(error,size,i->line,"reserved assignment binding");
            if(i->kind==I_MUTATE) {
                if(!flow_mutation_preflight(g,i->dest,i->expr,i->op,i->line,error,size))return 0;
            }
            if(i->kind==I_FOR||i->kind==I_WHILE||i->kind==I_DO_WHILE||
               (i->kind==I_SWITCH&&g->scalar_switch_mode)) {
                FlowLoop loop;
                if(!flow_loop_record(g,pc,&loop)||loop.end>function->end||
                   (loop_count&&loop.end>stack[loop_count-1].end)||loop_count>=MAX_CONTROL_DEPTH)
                    return flow_preflight_error(error,size,i->line,"invalid loop header or body span");
                if(i->kind==I_SWITCH) {
                    const SwitchNode *node=&g->switches[loop.index];int label;
                    for(label=node->first_case;label>=0;label=g->cases[label].next_case) {
                        const CaseNode *c=&g->cases[label];
                        if(c->owner_switch!=loop.index||c->body_start<loop.start||c->body_end>loop.end||
                           c->body_end<c->body_start||
                           (c->kind==GSC_CASE_INTEGER&&(c->integer< -8257536||c->integer>8388607))||
                           (c->kind!=GSC_CASE_INTEGER&&c->kind!=GSC_CASE_STRING&&c->kind!=GSC_CASE_DEFAULT))
                            return flow_preflight_error(error,size,c->line,"unsupported switch label or span");
                    }
                }
                if(i->kind==I_FOR) {
                    const ForNode *node=&g->loops[loop.index];
                    if((node->init_target>=0&&!flow_lvalue_valid(g,node->init_target))||
                       (node->step_target>=0&&!flow_lvalue_valid(g,node->step_target)))
                        return flow_preflight_error(error,size,i->line,"unsupported for-header lvalue");
                    if(node->step_kind==FOR_COMPOUND_ASSIGN||node->step_kind==FOR_INCREMENT||
                       node->step_kind==FOR_DECREMENT) {
                        if(!flow_mutation_preflight(g,node->step_target,node->step_expr,
                           node->step_kind==FOR_INCREMENT?GSC_MUT_INC:
                           node->step_kind==FOR_DECREMENT?GSC_MUT_DEC:node->step_op,
                           i->line,error,size))return 0;
                    }
                }
                stack[loop_count++]=loop;
                if(loop.start==loop.end)loop_count--;
            } else if(i->kind==I_BREAK||i->kind==I_CONTINUE) {
                for(n=loop_count-1;n>=0&&stack[n].owner!=i->target;n--){}
                if(n<0)return flow_preflight_error(error,size,i->line,"invalid lexical loop transfer");
            } else if(i->kind==I_RETURN&&(i->expr< -1||i->expr>=g->expr_count))
                return flow_preflight_error(error,size,i->line,"invalid return expression");
            else if((i->kind==I_JZ||i->kind==I_JUMP)&&
                    (i->dest<=pc||i->dest>function->end))
                return flow_preflight_error(error,size,i->line,"invalid branch span");
        }
    }
    /* Mutator/header fields do not carry parser field_write_target. Check their
     * required host writes separately while preserving the inspection IR. */
    for(n=0;n<g->expr_count;n++) {
        const Expr *e=&g->expr[n];int property;
        if(e->kind!=E_FIELD)continue;
        property=control_property_descriptor(g,e->name);
        if((property==ENTITY_PROPERTY_ORIGIN&&(!controls||!controls->setorigin))||
           (property==ENTITY_PROPERTY_ANGLES&&(!access||!access->setangles))) {
            int writes=0,k;
            for(k=0;k<g->code_count&&!writes;k++)if(g->code[k].kind==I_MUTATE&&g->code[k].dest==n)writes=1;
            for(k=0;k<g->loop_count&&!writes;k++)
                if(g->loops[k].init_target==n||g->loops[k].step_target==n)writes=1;
            if(writes)return flow_preflight_error(error,size,e->audit_line,"field mutation requires enabled pose capability");
        }
    }
    return 1;
}
