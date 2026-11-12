/* Freeze every private field/array record. Native graph ownership may change;
 * no VM state, task, field slot, handle, alias or clock may change here. */
typedef struct {
 StateField *allocation,*fields;int capacity,array_count,entry_count,struct_count,game_enabled;
 uint32_t limit;uint64_t token;const GscProgram*program;Value game_root;
 ArrayContainer arrays[GSC_ARRAY_CONTAINER_CAPACITY];ArrayEntry entries[GSC_ARRAY_ENTRY_CAPACITY];
} PlacedCohortStorage;
static void cohort_storage_free(GscRuntime*r,PlacedCohortStorage*p){if(!p)return;
 (void)state_storage_free(r,p->fields);p->fields=NULL;(void)state_storage_free(r,p);
}
static int cohort_storage_same(const GscRuntime*r,const PlacedCohortStorage*p){
 return r&&p&&!r->state_arena_busy&&!r->state_scratch&&r->state_fields==p->allocation&&
  r->state_field_capacity==p->capacity&&r->state_field_limit==p->limit&&r->array_token==p->token&&r->program==p->program&&
  r->array_count==p->array_count&&r->array_entry_count==p->entry_count&&r->struct_count==p->struct_count&&
  r->game_array_enabled==p->game_enabled&&!memcmp(&r->game_array_root,&p->game_root,sizeof(p->game_root))&&
  !memcmp(r->arrays,p->arrays,sizeof(p->arrays))&&!memcmp(r->array_entries,p->entries,sizeof(p->entries))&&
  !memcmp(r->state_fields,p->fields,(size_t)p->capacity*sizeof(*p->fields));
}
static PlacedCohortStorage*cohort_storage_capture(GscRuntime*r){PlacedCohortStorage*p;
 if(!r||r->state_field_capacity<=0||r->state_field_capacity>GSC_STATE_FIELD_OWNED_LIMIT||!r->state_fields||r->state_scratch)return NULL;
 p=state_storage_allocate(r,1,sizeof(*p),0);if(!p)return NULL;
 p->fields=state_storage_allocate(r,(size_t)r->state_field_capacity,sizeof(*p->fields),0);
 if(!p->fields){cohort_storage_free(r,p);return NULL;}
 p->allocation=r->state_fields;p->capacity=r->state_field_capacity;p->limit=r->state_field_limit;p->token=r->array_token;p->program=r->program;
 p->array_count=r->array_count;p->entry_count=r->array_entry_count;p->struct_count=r->struct_count;
 p->game_enabled=r->game_array_enabled;p->game_root=r->game_array_root;
 memcpy(p->fields,r->state_fields,(size_t)p->capacity*sizeof(*p->fields));memcpy(p->arrays,r->arrays,sizeof(p->arrays));memcpy(p->entries,r->array_entries,sizeof(p->entries));return p;
}
static int cohort_storage_private(const PlacedCohortStorage*p,const void*b,size_t n){return p&&
 (Cod1ModelStringOverlap(b,n,p,sizeof(*p))||Cod1ModelStringOverlap(b,n,p->fields,(size_t)p->capacity*sizeof(*p->fields)));}
