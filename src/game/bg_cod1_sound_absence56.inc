/* Optional exact-byte cache of successful table misses only. A NONE result
 * has not parsed any eligible variant or performed numeric conversions.
 * Do not cache failures or READY plans. Live VFS callbacks remain mandatory.
 * Successful complete parses also admit an exact-byte structural row index. */
#define COD1_SS56_TABLES 16
#define COD1_SS56_KEYS 32
#define COD1_SS56_BYTES (8u*1024u*1024u)
typedef struct {
 char alias[COD1_SS_TEXT_BYTES],map[128];int profile,loop,valid;
} Cod1SSAbsentKey56;
typedef struct {
 char path[COD1_SS_PATH_BYTES];char *bytes;size_t size;unsigned next;
 Cod1SSAbsentKey56 keys[COD1_SS56_KEYS];Cod1SSRows56 index;
} Cod1SSAbsentTable56;
typedef struct {
 size_t used,index_used;Cod1SSAbsentTable56 tables[COD1_SS56_TABLES];
} Cod1SSAbsentCache56;
static void Cod1SS_AbsentDrop56(Cod1SSAbsentCache56 *cache,Cod1SSAbsentTable56 *t){
 if(t->bytes){int saved=errno;cache->used-=t->size;free(t->bytes);errno=saved;}
 if(t->index.rows){int saved=errno;cache->index_used-=(size_t)t->index.count*sizeof(Cod1SSRow56);free(t->index.rows);errno=saved;}
 memset(t,0,sizeof(*t));
}
static void Cod1SS_AbsentReset56(Cod1SSAbsentCache56 *cache){
 unsigned i;for(i=0;i<COD1_SS56_TABLES;++i)Cod1SS_AbsentDrop56(cache,&cache->tables[i]);
}
static Cod1SSResourceStatus Cod1SS_ParseCached56(Cod1SSAbsentCache56 *cache,const char *path,
 const char *data,size_t size,const char *alias,const char *map,Cod1SSPlan *out,
 Cod1SSDiagnostic *diag,int profile,int loop){
 Cod1SSAbsentTable56 *t=NULL,*empty=NULL;Cod1SSResourceStatus status;unsigned i;
 /* The enclosing resolver validated the request; retain the parser for
  * unsupported inputs and avoid admitting an unbounded cache key. */
 if(!cache||!data||!size||size>COD1_SS_TABLE_BYTES||!out||
    !Cod1SS_Length(path,COD1_SS_PATH_BYTES,NULL)||
    !Cod1SS_Token(alias,COD1_SS_TEXT_BYTES)||Cod1SS_Loadspec("",map)<0)
  return Cod1ScriptSoundParseTableMode(data,size,alias,map,out,diag,profile,loop);
 for(i=0;i<COD1_SS56_TABLES;++i){
  Cod1SSAbsentTable56 *x=&cache->tables[i];
  if(!x->bytes){if(!empty)empty=x;continue;}
  if(!strcmp(x->path,path)){t=x;break;}
 }
 if(t&&(t->size!=size||memcmp(t->bytes,data,size))){
  Cod1SS_AbsentDrop56(cache,t);empty=t;t=NULL;
 }
 if(t)for(i=0;i<COD1_SS56_KEYS;++i){
  const Cod1SSAbsentKey56 *k=&t->keys[i];
  if(k->valid&&k->profile==profile&&k->loop==loop&&!strcmp(k->alias,alias)&&!strcmp(k->map,map)){
   if(diag){diag->reason=COD1_SS_REASON_NONE;diag->line=0;}
   return COD1_SS_RESOURCE_NONE;
  }
 }
 status=Cod1ScriptSoundParseTableIndexed56(data,size,alias,map,out,diag,profile,loop,t?&t->index:NULL);
 if(status==COD1_SS_RESOURCE_HELD)return status;
 if(!t&&empty&&size<=COD1_SS56_BYTES-cache->used){
  int saved=errno;char *copy=malloc(size);errno=saved;
  if(copy){memcpy(copy,data,size);t=empty;t->bytes=copy;t->size=size;strcpy(t->path,path);cache->used+=size;}
 }
 if(t&&!t->index.ready)Cod1SS_RowBuild56(&t->index,t->bytes,t->size,&cache->index_used);
 if(t&&status==COD1_SS_RESOURCE_NONE){
  Cod1SSAbsentKey56 *k=&t->keys[t->next];
  strcpy(k->alias,alias);strcpy(k->map,map);k->profile=profile;k->loop=loop;k->valid=1;
  t->next=(t->next+1)%COD1_SS56_KEYS;
 }
 return status;
}
