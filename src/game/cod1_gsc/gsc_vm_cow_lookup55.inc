/* Private COW stage lookup. The live arena is never indexed here. Within a
 * staged commit existing member owners/keys are immutable; new members append.
 * Reset at each acquisition, including reuse of R329 scratch. Preserve the
 * first matching physical index even if duplicate keys exist. */
static int r238_stage_find_reference55(const R238ArrayStage*s,uint32_t owner,const ArrayKey*key){
 int n;for(n=0;n<s->entries;n++)if(s->members[n].owner==(int)owner&&array_key_equal(&s->members[n].key,key))return n;
 return -1;
}
static int r238_stage_find(R238ArrayStage*s,uint32_t owner,const ArrayKey*key){
 int n;uint16_t slot;
 if(!owner||owner>GSC_ARRAY_CONTAINER_CAPACITY||s->entries<0||s->entries>GSC_ARRAY_ENTRY_CAPACITY)
  return r238_stage_find_reference55(s,owner,key);
 /* A single lookup is common. Do not initialize the directory for it. */
 if(s->lookup_entries==-1){s->lookup_entries=-2;return r238_stage_find_reference55(s,owner,key);}
 if(s->lookup_entries<0||s->lookup_entries>s->entries){
  memset(s->lookup_head,0,sizeof(s->lookup_head));memset(s->lookup_tail,0,sizeof(s->lookup_tail));
  s->lookup_entries=0;
 }
 for(n=s->lookup_entries;n<s->entries;n++){
  int id=s->members[n].owner;uint16_t tail;
  if(id<1||id>GSC_ARRAY_CONTAINER_CAPACITY)continue;
  slot=(uint16_t)(n+1);tail=s->lookup_tail[id-1];s->lookup_next[n]=0;
  if(tail)s->lookup_next[tail-1]=slot;else s->lookup_head[id-1]=slot;
  s->lookup_tail[id-1]=slot;
 }
 s->lookup_entries=s->entries;
 for(slot=s->lookup_head[owner-1];slot;slot=s->lookup_next[slot-1])
  if(array_key_equal(&s->members[slot-1].key,key))return (int)slot-1;
 return -1;
}
