/* Original 460ae0 copies child payload/type and AddRefs it, never deep clones.
 * GetVariable keeps destination flags and ORs source metadata; this port admits
 * only a fresh distinct destination, with no fake merge/refcount/GC parity. */
static int inheritance_value_valid(const GscRuntime *r,const Value *v) {
    int n;
    switch(v->kind){
    case V_UNDEFINED:return 1;
    case V_NUMBER:
        return (v->integer==0||v->integer==1)&&(v->integer_exact==0||v->integer_exact==1)&&
            (!v->integer_exact||v->integer)&&isfinite(v->number);
    case V_STRING:case V_LOCALIZED_STRING:return memchr(v->str,0,sizeof(v->str))!=NULL;
    case V_VECTOR:for(n=0;n<3;n++)if(!isfinite(v->vector[n]))return 0;return 1;
    case V_ENTITY:return v->entity.epoch&&v->entity.generation; /* opaque, no dereference */
    case V_ARRAY:return array_value_valid(r,v->array);
    case V_STRUCT:return struct_value_valid(r,v);
    case V_FUNCTION:return function_value_valid(r,v);
    default:return 0;
    }
}
typedef struct {StateField field;int source_slot,destination_slot;} StateInheritanceCell;
static int inherit_entity_fields(GscRuntime*r,const GscEntityFieldInheritance*input,
 size_t*copied,int(*guard)(void*,const GscEntityFieldInheritance*),void*user){
 GscEntityFieldInheritance request,guard_request;StateInheritanceCell*stage=NULL;
 Task*caller;int n,k,count=0,found=0,free_count=0,seen,outer,ok=0;
 if(!r||!input||!copied||!r->started||!r->program->linked||r->error[0]||
  !r->state_enabled||!r->arrays_enabled||!r->structs_enabled||!r->scalar_switch_enabled||!r->names_enabled||
  Cod1ModelStringOverlap(input,sizeof(*input),copied,sizeof(*copied))||
  runtime_private_overlap(r,input,sizeof(*input))||runtime_private_overlap(r,copied,sizeof(*copied)))return 0;
 request=*input;
 if(!request.source.epoch||!request.source.generation||!request.destination.epoch||!request.destination.generation||state_ref_equal(&request.source,&request.destination))return 0;
 if(r->entity_inheritance_busy)return rfail(r,0,"entity field inheritance reentered");
 if(r->entity_field_busy)return rfail(r,0,"entity inheritance during native field callback");
 if(r->entity_method_busy)return rfail(r,0,"entity inheritance during native method callback");
 if(r->entity_prepare_busy||r->entity_certificate_busy||r->state_arena_busy)return rfail(r,0,"entity inheritance during guarded host callback");
 outer=!r->advancing;caller=r->executing_task;if(!outer&&!caller)return rfail(r,0,"entity field inheritance has no active execution context");
 if(outer)r->advancing=1;r->entity_inheritance_busy=1;
 /* Count actual source records, never allocate whole-capacity stack arrays.
  * No owner callback runs before complete reservation and snapshot. */
 for(n=0;n<r->state_field_capacity;n++){const StateField*f=r->state_fields+n;
  if(!f->used||f->owner_kind!=V_ENTITY)continue;
  if(state_ref_equal(&f->owner,&request.destination))goto done;
  if(state_ref_equal(&f->owner,&request.source))count++;
 }
 if(!state_fields_can_fit(r,(size_t)count))goto done; /* Legacy bounded refusal. */
 if(count){stage=state_storage_allocate(r,(size_t)count,sizeof(*stage),0);if(!stage)goto done;
  r->state_scratch=stage;r->state_scratch_bytes=(size_t)count*sizeof(*stage);}
 if(!state_fields_reserve_free(r,(size_t)count,0))goto done;
 for(n=0;n<r->state_field_capacity;n++){const StateField*f=r->state_fields+n;
  if(!f->used){if(free_count<count)stage[free_count++].destination_slot=n;continue;}
  if(f->owner_kind!=V_ENTITY)continue;
  if(state_ref_equal(&f->owner,&request.destination))goto done;
  if(!state_ref_equal(&f->owner,&request.source))continue;
  if(found>=count||!memchr(f->name,0,sizeof(f->name))||!f->name[0]||f->struct_owner||control_metadata_name(f->name)||
   control_property_descriptor(r->program,f->name)>=0||!inheritance_value_valid(r,&f->value))goto done;
  for(k=0;k<found;k++)if(r->controls_enabled?control_field_equal(stage[k].field.name,f->name):!strcmp(stage[k].field.name,f->name))goto done;
  stage[found].source_slot=n;stage[found++].field=*f;
 }
 if(found!=count||free_count!=count)goto done;
 if(caller&&(!refresh_state_context(r,caller)||caller->status==TASK_DEAD))goto done;
 if(state_entity_live(r,&request.source)!=1||state_entity_live(r,&request.destination)!=1)goto done;
 if(caller&&(!refresh_state_context(r,caller)||caller->status==TASK_DEAD))goto done;
 for(n=0;n<count;n++)if(entity_field_classify(r,&request.source,stage[n].field.name,0)!=0||entity_field_classify(r,&request.destination,stage[n].field.name,0)!=0)goto done;
 if(state_entity_live(r,&request.source)!=1||state_entity_live(r,&request.destination)!=1)goto done;
 if(caller&&(!refresh_state_context(r,caller)||caller->status==TASK_DEAD))goto done;
 if(r->error[0])goto done;
 seen=0;
 for(n=0;n<r->state_field_capacity;n++){const StateField*f=r->state_fields+n;
  if(!f->used||f->owner_kind!=V_ENTITY)continue;
  if(state_ref_equal(&f->owner,&request.destination))goto done;
  if(state_ref_equal(&f->owner,&request.source))seen++;
 }
 if(seen!=count)goto done;
 for(n=0;n<count;n++)if(r->state_fields[stage[n].destination_slot].used||
  memcmp(r->state_fields+stage[n].source_slot,&stage[n].field,sizeof(StateField))||!inheritance_value_valid(r,&stage[n].field.value))goto done;
 guard_request=request;if(guard&&guard(user,&guard_request)!=1)goto done;
 if(r->error[0]||memcmp(&guard_request,&request,sizeof(request)))goto done;
 /* Pure final rechecks, then one infallible shallow-value publication. */
 for(n=0;n<count;n++)if(r->state_fields[stage[n].destination_slot].used||memcmp(r->state_fields+stage[n].source_slot,&stage[n].field,sizeof(StateField)))goto done;
 for(n=0;n<count;n++){stage[n].field.owner=request.destination;r->state_fields[stage[n].destination_slot]=stage[n].field;}
 *copied=(size_t)count;ok=1;
done:
 if(stage){if(!state_storage_free(r,stage))ok=0;r->state_scratch=NULL;r->state_scratch_bytes=0;}
 r->entity_inheritance_busy=0;if(outer)r->advancing=0;return ok&&!r->error[0];
}
int gsc_inherit_entity_fields(GscRuntime *r,const GscEntityFieldInheritance *input,size_t *copied) {
    if(r&&r->entity_sequence_busy)return rfail(r,0,"entity inheritance during coordinated entry");
    return inherit_entity_fields(r,input,copied,NULL,NULL);
}
