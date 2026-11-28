/* Exact-byte structural index. Published only after a successful complete
 * table parse; semantic checks of matching rows always run in the parser. */
#define COD1_SS56_BUCKETS 256u
#define COD1_SS56_ROW_LIMIT 32768u
#define COD1_SS56_INDEX_BYTES (2u*1024u*1024u)
typedef struct {uint32_t offset,line,next,hash;} Cod1SSRow56;
typedef struct {
 Cod1SSRow56 *rows;uint32_t count,head[COD1_SS56_BUCKETS];int ready;
} Cod1SSRows56;
static uint32_t Cod1SS_RowHash56(const char *s){
 uint32_t h=UINT32_C(2166136261);unsigned c;
 while((c=(unsigned char)*s++)!=0){if(c>='A'&&c<='Z')c+=32;h=(h^c)*UINT32_C(16777619);}return h;
}
/* Count/fill the same physical rows, preserving original line numbers.
 * Name-first headers only; other layouts retain the complete parser. */
static int Cod1SS_RowScan56(const char *data,size_t size,Cod1SSRows56 *index,uint32_t *count){
 const char *p=data,*end=data+size,*le,*next;uint32_t line=0,n=0,tail[COD1_SS56_BUCKETS]={0};int header=0;
 char cell[256];
 if(size>=3&&(unsigned char)p[0]==239&&(unsigned char)p[1]==187&&(unsigned char)p[2]==191)p+=3;
 while(p<end){
  int bad=0;uint32_t hash,bucket,slot;const char *start=p;
  ++line;le=Cod1SS_Delimiter(p,end,'\r','\n');next=le;if(next<end&&*next=='\r')++next;if(next<end&&*next=='\n')++next;p=next;
  if(start==le)continue;
  (void)Cod1SS_Cell(start,le,cell,sizeof(cell),&bad);if(bad)return 0;
  if(!cell[0]||cell[0]=='#')continue;
  if(!header){if(!Cod1SS_Equal(cell,"name"))return 0;header=1;continue;}
  if(n>=COD1_SS56_ROW_LIMIT)return 0;
  if(index){
   if(n>=index->count)return 0;
   hash=Cod1SS_RowHash56(cell);bucket=hash%COD1_SS56_BUCKETS;slot=n+1;
   index->rows[n].offset=(uint32_t)(start-data);index->rows[n].line=line;
   index->rows[n].next=0;index->rows[n].hash=hash;
   if(tail[bucket])index->rows[tail[bucket]-1].next=slot;else index->head[bucket]=slot;
   tail[bucket]=slot;
  }
  ++n;
 }
 *count=n;return header;
}
static void Cod1SS_RowBuild56(Cod1SSRows56 *index,const char *data,size_t size,size_t *used){
 uint32_t count=0,filled=0;size_t bytes;int saved=errno;
 /* No repeated build after unsupported layout or allocation/capacity refusal. */
 if(index->ready)return;
 index->ready=-1;
 if(!Cod1SS_RowScan56(data,size,NULL,&count))return;
 bytes=(size_t)count*sizeof(Cod1SSRow56);
 if(bytes>COD1_SS56_INDEX_BYTES-*used)return;
 if(bytes){index->rows=malloc(bytes);errno=saved;if(!index->rows)return;}
 index->count=count;
 if(!Cod1SS_RowScan56(data,size,index,&filled)||filled!=count){
  free(index->rows);errno=saved;memset(index,0,sizeof(*index));index->ready=-1;return;
 }
 *used+=bytes;index->ready=1;
}
