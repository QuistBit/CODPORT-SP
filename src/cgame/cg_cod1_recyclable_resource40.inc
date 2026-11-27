/* A recyclable configstring binds one live owner at a time, but the
 * decoded model composition is immutable for the map revision. Keep a
 * bounded set of exact definitions independently of owner generations. */
#define CGFA4_RECYCLABLE_CACHE 4
static cgfa4Resource_t cgfa4Recyclable[CGFA4_RECYCLABLE_CACHE];
static unsigned cgfa4RecyclableNext;
static void CGFA4_RecyclableReset(void){
 unsigned i;
 for(i=0;i<CGFA4_RECYCLABLE_CACHE;i++){
  C1AD14_Free(cgfa4Recyclable[i].resource);
  memset(cgfa4Recyclable+i,0,sizeof(cgfa4Recyclable[i]));
 }
 cgfa4RecyclableNext=0;
}
/* Pure selection: a failed/rejected load must not evict an existing entry
 * or advance the replacement cursor. Only the successful publisher does. */
static cgfa4Resource_t *CGFA4_RecyclableSlot(const cod1TurretActorResourceState_t *def,int version,int *hit){
 unsigned i;cgfa4Resource_t *empty=NULL;
 *hit=0;
 for(i=0;i<CGFA4_RECYCLABLE_CACHE;i++){
  cgfa4Resource_t *r=cgfa4Recyclable+i;
  if(r->handle && r->version==version && !memcmp(&r->definition,def,sizeof(*def))){*hit=1;return r;}
  if(!r->resource&&!empty)empty=r;
 }
 return empty?empty:cgfa4Recyclable+cgfa4RecyclableNext;
}
static void CGFA4_RecyclablePublished(const cgfa4Resource_t *slot){
 cgfa4RecyclableNext=((unsigned)(slot-cgfa4Recyclable)+1)%CGFA4_RECYCLABLE_CACHE;
}
