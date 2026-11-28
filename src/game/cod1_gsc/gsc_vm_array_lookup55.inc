/* Derived directory of physical array member slots. The committed arena is
 * authoritative. All destructive edits rebuild; append-only publications
 * extend the directory. Read-only lookups never mutate metadata. */
_Static_assert(GSC_ARRAY_ENTRY_CAPACITY<=UINT16_MAX,"array owner directory uses one-based uint16 slots");
static void array_index_extend55(GscRuntime*r,int first){
 int n;
 for(n=first;n<r->array_entry_count;n++){
  int id=r->array_entries[n].owner;uint16_t slot,tail;
  if(id<1||id>GSC_ARRAY_CONTAINER_CAPACITY)continue;
  slot=(uint16_t)(n+1);tail=r->array_lookup_tail[id-1];r->array_lookup_next[n]=0;
  if(tail)r->array_lookup_next[tail-1]=slot;else r->array_lookup_head[id-1]=slot;
  r->array_lookup_tail[id-1]=slot;
 }
 r->array_lookup_entries=r->array_entry_count;
}
static void array_index_rebuild55(GscRuntime*r){
 memset(r->array_lookup_head,0,sizeof(r->array_lookup_head));
 memset(r->array_lookup_tail,0,sizeof(r->array_lookup_tail));
 r->array_lookup_entries=-1;
 if(r->array_entry_count>=0&&r->array_entry_count<=GSC_ARRAY_ENTRY_CAPACITY)
  array_index_extend55(r,0);
}
static void array_index_appended55(GscRuntime*r,int old_count){
 if(old_count<0||old_count>r->array_entry_count||r->array_entry_count>GSC_ARRAY_ENTRY_CAPACITY||r->array_lookup_entries!=old_count){array_index_rebuild55(r);return;}
 array_index_extend55(r,old_count);
}
/* Caller retains the original capacity checks and publication ordering. */
static void array_entry_append55(GscRuntime*r,const ArrayEntry*entry){
 int old_count=r->array_entry_count;
 r->array_entries[r->array_entry_count++]=*entry;
 array_index_appended55(r,old_count);
}
static int array_entry_find_reference55(const GscRuntime *r,uint32_t owner,const ArrayKey *key) {
    int n;
    for(n=0;n<r->array_entry_count;n++)if(r->array_entries[n].owner==(int)owner&&
       array_key_equal(&r->array_entries[n].key,key))return n;
    return -1;
}
static int array_entry_find(const GscRuntime *r,uint32_t owner,const ArrayKey *key){
 uint16_t slot,previous=0,tail;
 if(!owner||owner>GSC_ARRAY_CONTAINER_CAPACITY||r->array_entry_count<0||r->array_entry_count>GSC_ARRAY_ENTRY_CAPACITY||r->array_lookup_entries!=r->array_entry_count)
  return array_entry_find_reference55(r,owner,key);
 tail=r->array_lookup_tail[owner-1];
 for(slot=r->array_lookup_head[owner-1];slot;slot=r->array_lookup_next[slot-1]){
  if(slot<=previous||slot>r->array_entry_count||slot>tail||r->array_entries[slot-1].owner!=(int)owner)
   return array_entry_find_reference55(r,owner,key);
  if(array_key_equal(&r->array_entries[slot-1].key,key))return (int)slot-1;
  previous=slot;
 }
 if(previous!=tail)return array_entry_find_reference55(r,owner,key);
 return -1;
}
