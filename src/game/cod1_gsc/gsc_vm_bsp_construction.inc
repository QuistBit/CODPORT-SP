/* This profile implements construction effects only. It deliberately has no
 * lookup/spawn/gameplay/resource methods and never represents their admission. */
GscRuntime *gsc_runtime_new_bsp_constructor(const GscProgram*p,GscFogCallback fog,void*u,
 const GscEntityHost*entities,const GscEntityFieldHost*fields,char*error,size_t cap){GscRuntime*r;
 if(!entities||!entities->validate||entities->lookup||entities->spawn||entities->spawn_with_flags||entities->setorigin||
    !fields||!fields->classify||fields->read||fields->write)return NULL;
 /* Ordinary runtime constructors preflight every executable Program feature.
  * This object has no executable profile: allocate its actual private state
  * directly, and keep every script entry/resource-bootstrap route closed. */
 if(!p||!p->linked||!fog)return NULL;
 r=GSC_RUNTIME_CALLOC(1,sizeof(*r));
 if(!r){if(error&&cap)snprintf(error,cap,"BSP constructor state allocation failed");return NULL;}
 r->program=p;r->fog=fog;r->user=u;r->resources_closed=1;
 if(!state_fields_reserve_free(r,GSC_STATE_FIELD_CAPACITY,0)){gsc_runtime_free(r);if(error&&cap)snprintf(error,cap,"BSP state arena initial allocation failed");return NULL;}
 r->native_isdefined_import=r->native_exp_fog_import=r->native_cull_fog_import=-1;
 r->native_getent_import=r->native_setorigin_import=r->native_spawn_import=-1;
 r->native_setmodel_import=r->native_hide_import=r->native_show_import=-1;
 if(!array_token_new(&r->array_token)){gsc_runtime_free(r);return NULL;}
 r->bsp_construction_only=1;r->entities=*entities;r->entities_enabled=1;
 r->entity_fields=*fields;r->entity_fields_enabled=1;
 r->state_enabled=r->arrays_enabled=r->results_enabled=r->structs_enabled=1;
 r->lifetime_enabled=r->scalar_switch_enabled=r->names_enabled=1;return r;
}
int gsc_construct_bsp_entity(GscRuntime*r,const GscPreparedEntityEntry*q,const GscPreparedEntityEntryHost*h,
 GscPreparedEntityEntryRun*out,char*e,size_t n){
 if(!r||!r->bsp_construction_only)return 0;
 return prepare_entity_entry_phase(r,q,h,out,e,n,2);
}
int gsc_bsp_fields_current(const GscRuntime*r,const GscPreparedEntityEntry*q){uint32_t i;int n,found,count=0;
 if(!r||(!r->bsp_construction_only&&!r->bsp_promoted)||!q||q->field_count>GSC_ENTITY_INITIAL_FIELDS_MAX)return 0;
 for(i=0;i<q->field_count;i++)if(!memchr(q->fields[i].name,0,sizeof(q->fields[i].name))||!memchr(q->fields[i].string,0,sizeof(q->fields[i].string)))return 0;
 for(n=0;n<r->state_field_capacity;n++)if(r->state_fields[n].used&&r->state_fields[n].owner_kind==V_ENTITY&&
   state_ref_equal(&r->state_fields[n].owner,&q->entry.entity))count++;
 if(count!=(int)q->field_count)return 0;
 for(i=0;i<q->field_count;i++){
  found=0;for(n=0;n<r->state_field_capacity;n++){const StateField*f=&r->state_fields[n];
   if(f->used&&f->owner_kind==V_ENTITY&&state_ref_equal(&f->owner,&q->entry.entity)&&
      !strcmp(f->name,q->fields[i].name)&&f->value.kind==V_STRING&&!strcmp(f->value.str,q->fields[i].string))found++;}
  if(found!=1)return 0;
 }return 1;
}

int gsc_runtime_commit_bsp_promotion(GscRuntime*r){
 if(!r||!r->bsp_construction_only||r->bsp_promoted||r->started||r->error[0]||
    r->advancing||r->executing_task||r->entity_sequence_busy||r->entity_prepare_busy||
    r->entity_certificate_busy||!r->native_registry||!r->entity_fields_enabled||gsc_active_threads(r))return 0;
 r->bsp_promoted=1;r->bsp_construction_only=0;return 1;
}
