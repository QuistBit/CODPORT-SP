/* One owner-thread CSV read buffer shared by one-shot and loop sound.
 * The parser owns a lease until release; nested or oversized reads use
 * independent storage. Reset must not invalidate an outstanding lease. */
#define COD1_SS56_READ_KEEP (1024u*1024u)
typedef struct {
 char *bytes;size_t capacity;int busy,retire;
} Cod1SSReadBuffer56;
static void Cod1SS_ReadReset56(Cod1SSReadBuffer56 *s){
 if(s->busy){s->retire=1;return;}
 free(s->bytes);memset(s,0,sizeof(*s));
}
static char *Cod1SS_ReadAcquire56(Cod1SSReadBuffer56 *s,size_t size){
 char *p;
 if(!size)return NULL;
 if(s->busy||size>COD1_SS56_READ_KEEP)return malloc(size);
 if(size>s->capacity){
  p=realloc(s->bytes,size);if(!p)return NULL;
  s->bytes=p;s->capacity=size;
 }
 s->busy=1;return s->bytes;
}
static void Cod1SS_ReadRelease56(Cod1SSReadBuffer56 *s,char *data){
 if(data&&data==s->bytes){
  s->busy=0;if(s->retire)Cod1SS_ReadReset56(s);
 }else free(data);
}
