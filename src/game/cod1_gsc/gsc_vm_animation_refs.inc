uint64_t gsc_runtime_instance_identity(const GscRuntime *r){return r&&r->array_token?r->array_token:0;}
int gsc_program_select_animation_references(GscProgram *g){if(!g||g->invalid||g->linked||!g->anim_data_selected||g->animation_refs_selected||g->animation_namespace_selected)return 0;g->animation_refs_selected=1;return 1;}
static int animation_value_valid(const GscRuntime *r,const Value *v){int n;if(!r||!r->animation_refs_enabled||!v||v->kind!=V_ANIMATION||v->object_token!=r->animation_catalog_token)return 0;for(n=0;n<r->animation_ref_count;++n)if(r->animation_bindings[n].entry==v->object)return 1;return 0;}
static int animation_references_current(GscRuntime *r,Task *task){GscMapExecutionCertificate copy;int current;uint64_t time_root=0;
 if(!r->animation_refs_enabled){return 1;}
 if(task){time_root=r237_owner_stamp(r);if(!r237_owner_current(r,time_root))return rfail(r,0,"animation references Task has no current time-root");}
 if(task&&task->status==TASK_DEAD){return 0;}
 if(r->error[0])return 0;
 if(r->entity_certificate_busy||r->entity_prepare_busy)return rfail(r,0,"animation catalog callback reentered");
 copy=r->animation_refs;r->entity_certificate_busy=1;current=copy.current(copy.receipt);r->entity_certificate_busy=0;
 if(task&&!r237_owner_current(r,time_root))return rfail(r,0,"time-root changed during animation references certificate");
 if(r->error[0]){return 0;}
 if(current!=0&&current!=1){return rfail(r,0,"invalid animation catalog certificate result");}
 if(!current&&task)entity_task_retire(task);
 return current;
}
int gsc_runtime_select_animation_references(GscRuntime *r,const GscAnimationCatalog *catalog,const GscMapExecutionCertificate *input){
 static uint64_t binding_serial;static int binding_busy;AnimationBinding staged[GSC_ANIMATION_REFERENCE_CAPACITY];GscMapExecutionCertificate certificate;int count=0,n,k,current;
 if(binding_busy||binding_serial==UINT64_MAX||!r||!catalog||!input||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||!r->anim_data_enabled||r->animation_refs_enabled||!r->program->animation_refs_selected||
    r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy||
    runtime_private_overlap(r,catalog,sizeof(*catalog))||runtime_private_overlap(r,input,sizeof(*input)))return 0;
 if(!catalog->token||!catalog->count||catalog->count>GSC_ANIMATION_CATALOG_CAPACITY)return 0;
 certificate=*input;if(!certificate.receipt||!certificate.current)return 0;
 memset(staged,0,sizeof(staged));
 for(n=0;n<r->program->expr_count;++n){const Expr *e=&r->program->expr[n];const StringLiteral *context=NULL;unsigned found=0;
  if(e->kind!=E_ANIM_REF){continue;}
  if(count>=GSC_ANIMATION_REFERENCE_CAPACITY||e->owner_module<0||e->owner_module>=r->program->module_count)return 0;
  for(k=0;k<r->program->string_count;++k){const StringLiteral *literal=&r->program->strings[k];if(literal->module==e->owner_module&&literal->owner_kind==GSC_STRING_OWNER_ANIMTREE&&literal->source_offset<e->audit_offset&&(!context||literal->source_offset>context->source_offset))context=literal;}
  if(!string_literal_decoded(r->program,context)||strcmp(context->decoded,catalog->tree))return 0;
  for(k=0;k<(int)catalog->count;++k)if(animation_catalog_name_equal(catalog->entries[k].name,e->name)){found=catalog->entries[k].ordinal;break;}
  if(!found){return 0;}
  staged[count].expr=n;staged[count].entry=found;staged[count].flags=catalog->entries[k].flags;strcpy(staged[count].tree,catalog->tree);strcpy(staged[count].name,catalog->entries[k].name);++count;
 }
 binding_busy=1;r->entity_certificate_busy=1;current=certificate.current(certificate.receipt);r->entity_certificate_busy=0;
 if(current!=1||r->error[0]||!anim_data_current(r,NULL)){binding_busy=0;return 0;}
 binding_busy=0;
 memcpy(r->animation_bindings,staged,(size_t)count*sizeof(*staged));r->animation_ref_count=count;r->animation_catalog_token=++binding_serial;r->animation_refs=certificate;r->animation_refs_enabled=1;return 1;
}
static int animation_reference_value(GscRuntime *r,const Expr *e,int expression,int line,Value *out){int n;Value value;
 if(!r->animation_refs_enabled||!e||e->kind!=E_ANIM_REF||!out)return rfail(r,line,"animation reference requires selected resource authority");
 if(!anim_data_current(r,r->executing_task))return 0;
 for(n=0;n<r->animation_ref_count;++n)if(r->animation_bindings[n].expr==expression){memset(&value,0,sizeof(value));value.kind=V_ANIMATION;value.object=r->animation_bindings[n].entry;value.object_token=r->animation_catalog_token;*out=value;return 1;}
 return rfail(r,line,"animation reference has no copied resource binding");
}
/* A container exported through old hosts/events may not smuggle new values.
 * Entity references remain opaque identities; private entity bags are not an
 * exported snapshot. Arrays and runtime-owned structs are traversed together. */
static int animation_value_exportable(const GscRuntime *r,const Value *root){
    unsigned char arrays[GSC_ARRAY_CONTAINER_CAPACITY]={0},*fields;
    const Value **pending,*value;
    size_t limit,top=0;
    int i,ok=0;
    if(root->kind==V_ANIMATION||root->kind==V_ANIM||animation_namespace_kind(root->kind))return 0;
    if(root->kind!=V_ARRAY&&root->kind!=V_STRUCT)return 1;
    if(r->state_field_capacity<0||r->array_entry_count<0)return 0;
    /* Queue each stored child at most once. Identity-only structures cost no
     * graph storage, irrespective of their serial or how many have retired.
     * Each visited property is keyed by its live slot, so cycles through
     * structures and arrays terminate without renumbering external handles. */
    limit=1u+(size_t)r->array_entry_count+(size_t)r->state_field_capacity;
    if(limit>SIZE_MAX/sizeof(*pending))return 0;
    pending=malloc(limit*sizeof(*pending));
    fields=calloc(r->state_field_capacity?r->state_field_capacity:1,1);
    if(!pending||!fields)goto done;
    pending[top++]=root;
    while(top){
        value=pending[--top];
        if(value->kind==V_ANIMATION||value->kind==V_ANIM||animation_namespace_kind(value->kind))goto done;
        if(value->kind==V_ARRAY){
            if(!array_value_valid(r,value->array))goto done;
            if(arrays[value->array-1])continue;
            arrays[value->array-1]=1;
            for(i=0;i<r->array_entry_count;i++)if(r->array_entries[i].owner==(int)value->array)
                pending[top++]=&r->array_entries[i].value;
        }else if(value->kind==V_STRUCT){
            if(!struct_value_valid(r,value))goto done;
            for(i=0;i<r->state_field_capacity;i++){
                const StateField *field=&r->state_fields[i];
                if(!fields[i]&&field->used&&field->owner_kind==V_STRUCT&&field->struct_owner==value->object){
                    fields[i]=1;
                    pending[top++]=&field->value;
                }
            }
        }
    }
    ok=1;
done:
    free(fields);free(pending);return ok;
}
static int animation_compare(GscRuntime *r,int op,const Value *a,const Value *b,int line,Value *out){
 if((op!=T_EQ&&op!=T_NE)||a->kind!=V_ANIMATION||b->kind!=V_ANIMATION||!animation_value_valid(r,a)||!animation_value_valid(r,b))return rfail(r,line,"animation operands require two current references and equality only");
 if(!anim_data_current(r,r->executing_task)){return 0;}
 scalar_integer(out,op==T_EQ?a->object==b->object:a->object!=b->object);return 1;
}
