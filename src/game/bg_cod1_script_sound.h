#ifndef BG_COD1_SCRIPT_SOUND_H
#define BG_COD1_SCRIPT_SOUND_H
/* R192 bounded resource plan for GSC entity one-shots. Retail CSV comments are
 * the schema authority; this is NOT native loader/PCM/mixer numerical parity.
 * No RNG choice, handle registration, playback or missing-sound fallback here.
 * Kept separate from the older permissive weapon/brass alias readers. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <errno.h>

#define COD1_SS_LOADED_CAPTION_PROFILE 3
#define COD1_SS_CAMPAIGN_VOICE_PROFILE 4
#define COD1_SS_TEXT_BYTES 128
#define COD1_SS_PATH_BYTES 256
#define COD1_SS_VARIANTS 64
#define COD1_SS_TABLES 128
#define COD1_SS_LIST_BYTES 32768
#define COD1_SS_TABLE_BYTES (4u*1024u*1024u)

typedef enum {
 COD1_SS_RESOURCE_NONE=0,COD1_SS_RESOURCE_READY=1,COD1_SS_RESOURCE_HELD=-1
} Cod1SSResourceStatus;
typedef enum {
 COD1_SS_REASON_NONE=0,COD1_SS_REASON_INPUT,COD1_SS_REASON_HEADER,
 COD1_SS_REASON_CSV,COD1_SS_REASON_PATH,COD1_SS_REASON_NUMBER,
 COD1_SS_REASON_UNSUPPORTED,COD1_SS_REASON_DUPLICATE,COD1_SS_REASON_CAPACITY,
 COD1_SS_REASON_LOADSPEC,COD1_SS_REASON_VFS,COD1_SS_REASON_STALE,
 COD1_SS_REASON_AMBIGUOUS,COD1_SS_REASON_MISSING_FILE
} Cod1SSReason;
typedef struct {Cod1SSReason reason;unsigned line;} Cod1SSDiagnostic;
typedef struct {
 char path[COD1_SS_PATH_BYTES],channel[16];uint32_t sequence;
 unsigned streamed,mix_mode;float slave_cap;char subtitle[COD1_SS_TEXT_BYTES];
 float volume_min,volume_max,pitch_min,pitch_max,dist_min,dist_max,probability;
} Cod1SSVariant;
typedef struct {
 char alias[COD1_SS_TEXT_BYTES],table[COD1_SS_PATH_BYTES];unsigned count;
 Cod1SSVariant variants[COD1_SS_VARIANTS];
} Cod1SSPlan;
typedef struct {
 void *user;
 int (*current)(void *); /* host lease; checked around every external callback */
 int (*list)(void *,char *,size_t); /* packed NUL-terminated CSV basenames */
 int (*read)(void *,const char *,char **,size_t *);
 void (*release)(void *,char *);
 int (*exists)(void *,const char *); /* VFS existence, NOT PCM validation */
} Cod1SSFiles;
static int Cod1SS_Equal(const char *a,const char *b) {
 unsigned x,y;do{x=(unsigned char)*a++;y=(unsigned char)*b++;
 if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;}while(x);return 1;
}
static int Cod1SS_Length(const char *p,size_t cap,size_t *n) {
 size_t i;if(!p)return 0;for(i=0;i<cap;++i)if(!p[i]){if(n)*n=i;return 1;}return 0;
}
static Cod1SSResourceStatus Cod1SS_Fail(Cod1SSDiagnostic *d,Cod1SSReason reason,unsigned line) {
 if(d){d->reason=reason;d->line=line;}return COD1_SS_RESOURCE_HELD;
}
static int Cod1SS_Token(const char *p,size_t cap) {
 size_t i,n;if(!Cod1SS_Length(p,cap,&n)||!n)return 0;
 for(i=0;i<n;++i){unsigned c=(unsigned char)p[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return 0;}return 1;
}
/* Strict line-local CSV; quoted commas/doubled quotes supported. Embedded
 * linebreaks in quoted fields are HELD, not silently concatenated/truncated. */
/* Find the first of two byte delimiters without reading beyond the
 * supplied span. This changes scanning only; quoted/invalid CSV still uses
 * the same parser and every unrelated row is validated. */
#if defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
#include <emmintrin.h>
#include <intrin.h>
#endif
static const char *Cod1SS_Delimiter(const char *p,const char *end,char a,char b){
#if defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
 const __m128i first=_mm_set1_epi8(a),second=_mm_set1_epi8(b);
 while(end-p>=16){
  const __m128i bytes=_mm_loadu_si128((const __m128i*)p);
  unsigned mask=(unsigned)_mm_movemask_epi8(_mm_or_si128(_mm_cmpeq_epi8(bytes,first),_mm_cmpeq_epi8(bytes,second)));
  if(mask){unsigned long bit;_BitScanForward(&bit,mask);return p+bit;}
  p+=16;
 }
#endif
 while(p<end&&*p!=a&&*p!=b)++p;return p;
}
static const char *Cod1SS_Cell(const char *p,const char *end,char *out,size_t cap,int *bad) {
 size_t n=0;int quoted=0,closed=0;char c;
 if(cap&&p<end&&*p!='"'){
  const char *stop=Cod1SS_Delimiter(p,end,',','"');
  if(stop==end||*stop==','){
   size_t bytes=(size_t)(stop-p),take=bytes<cap?bytes:cap-1;
   memcpy(out,p,take);out[take]=0;if(bytes>=cap)*bad=1;
   return stop<end?stop+1:stop;
  }
 }
 if(p<end&&*p=='"'){quoted=1;++p;}
 while(p<end){c=*p++;
  if(quoted&&c=='"'){if(p<end&&*p=='"')++p;else{quoted=0;closed=1;continue;}}
  else if(!quoted&&c==',')break;
  else if(closed){if(c!=' '&&c!='\t')*bad=1;continue;}
  else if(!quoted&&c=='"')*bad=1;
  if(n+1<cap)out[n++]=c;else *bad=1;
 }
 if(quoted)*bad=1;out[n]=0;return p;
}
static int Cod1SS_Number(const char *p,float fallback,float *out) {
 const char *s=p,*end;char *e;double v;int digits=0;
 while(*s==' '||*s=='\t')++s;if(!*s){*out=fallback;return 1;}
 if(*s=='+'||*s=='-')++s;
 while(*s>='0'&&*s<='9'){++s;++digits;}
 if(*s=='.'){++s;while(*s>='0'&&*s<='9'){++s;++digits;}}
 if(!digits)return 0;
 if(*s=='e'||*s=='E'){++s;if(*s=='+'||*s=='-')++s;digits=0;while(*s>='0'&&*s<='9'){++s;++digits;}if(!digits)return 0;}
 end=s;while(*s==' '||*s=='\t')++s;if(*s)return 0;
 errno=0;v=strtod(p,&e);if(e!=end||errno==ERANGE||!(v>=0&&v<=FLT_MAX))return 0;
 *out=(float)v;return v==0||*out>0; /* do not silently accept FLOAT32 underflow */
}
static int Cod1SS_Sequence(const char *p,uint32_t *out) {
 uint32_t x=0;int n=0;while(*p==' '||*p=='\t')++p;if(!*p){*out=0;return 1;}
 while(*p>='0'&&*p<='9'){x=x*10u+(unsigned)(*p++-'0');++n;if(x>65535u)return 0;}
 while(*p==' '||*p=='\t')++p;if(*p||!n)return 0;*out=x;return 1;
}
static int Cod1SS_Path(const char *p,char out[COD1_SS_PATH_BYTES]) {
 size_t n,i,start=0;if(!Cod1SS_Length(p,COD1_SS_PATH_BYTES,&n)||n<5||n+6>=COD1_SS_PATH_BYTES||p[0]=='/')return 0;
 if(!Cod1SS_Equal(p+n-4,".wav"))return 0;
 for(i=0;i<=n;++i){unsigned c=(unsigned char)p[i];
  if(!c||c=='/'){size_t len=i-start;if(!len||(len==1&&p[start]=='.')||(len==2&&p[start]=='.'&&p[start+1]=='.'))return 0;start=i+1;}
  else if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'||c=='@'))return 0;
 }
 memcpy(out,"sound/",6);memcpy(out+6,p,n+1);return 1;
}
/* Retained port loadspec contract: positive map tokens and ! exclusions.
 * Reject malformed/overlong selectors rather than truncating them. */
static int Cod1SS_Loadspec(const char *spec,const char *map) {
 char normalized[96],token[96];const char *p=map;size_t n,j;int positive=0,match=0,excluded=0,neg;
 if(!Cod1SS_Length(map,128,&n)||!n)return -1;
 if(n>=5&&((p[0]=='m'||p[0]=='M')&&(p[1]=='a'||p[1]=='A')&&(p[2]=='p'||p[2]=='P')&&(p[3]=='s'||p[3]=='S')&&(p[4]=='/'||p[4]=='\\'))){p+=5;n-=5;}
 if(n>=4&&Cod1SS_Equal(p+n-4,".bsp"))n-=4;
 if(!n||n>=sizeof(normalized))return -1;memcpy(normalized,p,n);normalized[n]=0;if(!Cod1SS_Token(normalized,sizeof(normalized)))return -1;
 p=spec;while(*p){while(*p==' '||*p=='\t')++p;if(!*p)break;neg=*p=='!';
  /* Original iw_sound.csv authors both !Truckride and ! burnville. Treat
   * optional horizontal whitespace as part of the negation operator; the
   * required following map token still passes the strict token validator. */
  if(neg){++p;while(*p==' '||*p=='\t')++p;}else positive=1;j=0;
  while(*p&&*p!=' '&&*p!='\t'){if(j+1>=sizeof(token))return -1;token[j++]=*p++;}token[j]=0;
  if(!Cod1SS_Token(token,sizeof(token)))return -1;if(Cod1SS_Equal(token,normalized)){if(neg)excluded=1;else match=1;}
 }
 return !excluded&&(!positive||match);
}
static Cod1SSResourceStatus Cod1ScriptSoundParseTableMode(const char *data,size_t size,
 const char *alias,const char *map,Cod1SSPlan *out,Cod1SSDiagnostic *diag,int allowVoice,int loopMode) {
 enum {NAME,SEQUENCE,FILEPATH,VMIN,VMAX,PMIN,PMAX,DMIN,DMAX,CHANNEL,TYPE,PROB,LOOP,MASTER,LOADSPEC,SUBTITLE,NFIELDS};
 const char *keys[NFIELDS]={"name","sequence","file","vol_min","vol_max","pitch_min","pitch_max","dist_min","dist_max","channel","type","probability","loop","masterslave","loadspec","subtitle"};
 const char *p,*end,*le,*next;int idx[NFIELDS],field[64],header=0,col,bad,k,selected;unsigned line=0,j;
 char cell[256],values[NFIELDS][256];Cod1SSPlan plan;Cod1SSVariant v;size_t aliaslen;
 if(diag){diag->reason=COD1_SS_REASON_NONE;diag->line=0;}
 if(!data||!size||size>COD1_SS_TABLE_BYTES||!out||!Cod1SS_Length(alias,COD1_SS_TEXT_BYTES,&aliaslen)||!aliaslen||!Cod1SS_Token(alias,COD1_SS_TEXT_BYTES)||Cod1SS_Loadspec("",map)<0||memchr(data,0,size))return Cod1SS_Fail(diag,COD1_SS_REASON_INPUT,0);
 memset(&plan,0,sizeof(plan));memcpy(plan.alias,alias,aliaslen+1);for(k=0;k<NFIELDS;++k)idx[k]=-1;
 p=data;end=p+size;if(size>=3&&(unsigned char)p[0]==239&&(unsigned char)p[1]==187&&(unsigned char)p[2]==191)p+=3;
 while(p<end){++line;le=Cod1SS_Delimiter(p,end,'\r','\n');next=le;if(next<end&&*next=='\r')++next;if(next<end&&*next=='\n')++next;
  bad=0;for(k=0;k<NFIELDS;++k)values[k][0]=0;col=0;
  if(p==le){p=next;continue;}
  /* Empty/# first columns are ignored by the original CSV's own header. */
  {const char *first=Cod1SS_Cell(p,le,cell,sizeof(cell),&bad);(void)first;if(!bad&&(!cell[0]||cell[0]=='#')){p=next;continue;}}
  while(p<le){p=Cod1SS_Cell(p,le,cell,sizeof(cell),&bad);if(col>=64)bad=1;
   if(!header){for(k=0;k<NFIELDS;++k)if(Cod1SS_Equal(cell,keys[k])){if(idx[k]>=0)bad=1;idx[k]=col;}}
   else if(col<64 && field[col]>=0)memcpy(values[field[col]],cell,strlen(cell)+1);
   ++col;
  }
  if(bad)return Cod1SS_Fail(diag,COD1_SS_REASON_CSV,line);
  if(!header){if(idx[NAME]<0||idx[FILEPATH]<0)return Cod1SS_Fail(diag,COD1_SS_REASON_HEADER,line);/* Header validation above bounds every mapped column. Each recognized
   * column has one field; absent cells remain empty strings. Keep parsing
   * all cells, including unrelated aliases, for the same CSV diagnostics. */
   for(k=0;k<64;++k)field[k]=-1;
   for(k=0;k<NFIELDS;++k)if(idx[k]>=0)field[idx[k]]=k;
   header=1;p=next;continue;}
  p=next;if(!Cod1SS_Equal(values[NAME],alias))continue;
  selected=Cod1SS_Loadspec(values[LOADSPEC],map);if(selected<0)return Cod1SS_Fail(diag,COD1_SS_REASON_LOADSPEC,line);if(!selected)continue;
  if(plan.count==COD1_SS_VARIANTS)return Cod1SS_Fail(diag,COD1_SS_REASON_CAPACITY,line);
  if((loopMode?!Cod1SS_Equal(values[LOOP],"looping"):(values[LOOP][0]&&!Cod1SS_Equal(values[LOOP],"nonlooping")))||(values[MASTER][0]&&(loopMode||allowVoice<2)))return Cod1SS_Fail(diag,COD1_SS_REASON_UNSUPPORTED,line);
  memset(&v,0,sizeof(v));
  v.streamed=Cod1SS_Equal(values[TYPE],"streamed");
  if(!v.streamed && values[TYPE][0] && !Cod1SS_Equal(values[TYPE],"loaded"))
   return Cod1SS_Fail(diag,COD1_SS_REASON_UNSUPPORTED,line);
  if(values[SUBTITLE][0]){
   if(loopMode || !allowVoice || (!v.streamed && allowVoice!=COD1_SS_LOADED_CAPTION_PROFILE) ||
      !Cod1SS_Equal(values[CHANNEL],"voice") || strncmp(values[SUBTITLE],"SUBTITLE_",9) ||
      !values[SUBTITLE][9] || !Cod1SS_Token(values[SUBTITLE],sizeof(v.subtitle)))
    return Cod1SS_Fail(diag,COD1_SS_REASON_UNSUPPORTED,line);
   strcpy(v.subtitle,values[SUBTITLE]);
  }else if(v.streamed && !(allowVoice==COD1_SS_CAMPAIGN_VOICE_PROFILE&&!loopMode&&Cod1SS_Equal(values[CHANNEL],"voice")) && (!loopMode || !Cod1SS_Equal(values[CHANNEL],"auto"))){
   return Cod1SS_Fail(diag,COD1_SS_REASON_UNSUPPORTED,line);
  }
  /* Native42B9D0: master and numeric slave are distinct flags. Only the
   * explicitly selected R234 owned voice engine admits this extension. */
  if(values[MASTER][0]){if(!v.streamed)return Cod1SS_Fail(diag,COD1_SS_REASON_UNSUPPORTED,line);
   if(Cod1SS_Equal(values[MASTER],"master"))v.mix_mode=1;
   else {v.mix_mode=2;if(!Cod1SS_Number(values[MASTER],0,&v.slave_cap))return Cod1SS_Fail(diag,COD1_SS_REASON_NUMBER,line);}
  }
  if(!Cod1SS_Path(values[FILEPATH],v.path))return Cod1SS_Fail(diag,COD1_SS_REASON_PATH,line);
  if(!Cod1SS_Sequence(values[SEQUENCE],&v.sequence)||!Cod1SS_Number(values[VMIN],1,&v.volume_min)||!Cod1SS_Number(values[VMAX],v.volume_min,&v.volume_max)||!Cod1SS_Number(values[PMIN],1,&v.pitch_min)||!Cod1SS_Number(values[PMAX],v.pitch_min,&v.pitch_max)||!Cod1SS_Number(values[DMIN],120,&v.dist_min)||!Cod1SS_Number(values[DMAX],0,&v.dist_max)||!Cod1SS_Number(values[PROB],1,&v.probability)||v.volume_max<v.volume_min||v.pitch_max<v.pitch_min||v.pitch_min<=0)return Cod1SS_Fail(diag,COD1_SS_REASON_NUMBER,line);
  if(!values[CHANNEL][0])strcpy(v.channel,"auto");else {
   static const char *allowed[]={"auto","body","weapon","voice","item","local"};int found=0;for(k=0;k<6;++k)if(Cod1SS_Equal(values[CHANNEL],allowed[k])){strcpy(v.channel,allowed[k]);found=1;break;}
   if(!found)return Cod1SS_Fail(diag,COD1_SS_REASON_UNSUPPORTED,line);
  }
  for(j=0;j<plan.count;++j)if(plan.variants[j].sequence==v.sequence)return Cod1SS_Fail(diag,COD1_SS_REASON_DUPLICATE,line);
  plan.variants[plan.count++]=v;
 }
 if(!header)return Cod1SS_Fail(diag,COD1_SS_REASON_HEADER,line);
 if(!plan.count)return COD1_SS_RESOURCE_NONE;
 *out=plan;return COD1_SS_RESOURCE_READY;
}
static Cod1SSResourceStatus Cod1ScriptSoundParseTableSelected(const char *data,size_t size,
 const char *alias,const char *map,Cod1SSPlan *out,Cod1SSDiagnostic *diag,int allowVoice) {
 return Cod1ScriptSoundParseTableMode(data,size,alias,map,out,diag,allowVoice,0);
}
/* Explicit resource planning only. Loaded or streamed loop rows; streamed
 * auto-channel without subtitle/master/slave is the selected subset. Native
 * GAME existence is a registry query, not this narrower mixer-admission gate.
 * No audio is started and no arbitrary alias is certified by this parser. */
static Cod1SSResourceStatus Cod1LoopSoundParseTable(const char *data,size_t size,
 const char *alias,const char *map,Cod1SSPlan *out,Cod1SSDiagnostic *diag) {
 return Cod1ScriptSoundParseTableMode(data,size,alias,map,out,diag,0,1);
}
/* Legacy profiles retain the loaded-only contract; streamed speech requires
 * explicit selection and carries its original subtitle identity. Parsing is
 * resource planning only, never a playback/completion acknowledgement. */
static Cod1SSResourceStatus Cod1ScriptSoundParseTable(const char *data,size_t size,
 const char *alias,const char *map,Cod1SSPlan *out,Cod1SSDiagnostic *diag) {
 return Cod1ScriptSoundParseTableSelected(data,size,alias,map,out,diag,0);
}
static int Cod1SS_Basename(const char *s) {
 size_t n,i;if(!Cod1SS_Length(s,COD1_SS_PATH_BYTES,&n)||n<5||n+13>=COD1_SS_PATH_BYTES||!Cod1SS_Equal(s+n-4,".csv"))return 0;
 for(i=0;i<n;++i){unsigned c=(unsigned char)s[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return 0;}
 return s[0]!='.'&&strstr(s,"..")==NULL;
}
/* Full table scan, fail-closed on cross-table ambiguity. Original table merge/
 * override order has NOT been recovered. All eligible variants must exist;
 * there is no first-four truncation, direct-file shortcut, or RTCW fallback. */
static Cod1SSResourceStatus Cod1ScriptSoundResolveMode(const Cod1SSFiles *files,const char *alias,
 const char *map,Cod1SSPlan *out,Cod1SSDiagnostic *diag,int allowVoice,int loopMode) {
 char names[COD1_SS_LIST_BYTES],path[COD1_SS_PATH_BYTES],*data;size_t offsets[COD1_SS_TABLES],at=0,n,size;int count,i,j,read_ok,status,found=0,current;
 Cod1SSPlan result,candidate;Cod1SSDiagnostic detail;
 if(diag){diag->reason=COD1_SS_REASON_NONE;diag->line=0;}
 if(!files||!out||!files->current||!files->list||!files->read||!files->release||!files->exists||!Cod1SS_Token(alias,COD1_SS_TEXT_BYTES)||Cod1SS_Loadspec("",map)<0)return Cod1SS_Fail(diag,COD1_SS_REASON_INPUT,0);
 if(files->current(files->user)!=1)return Cod1SS_Fail(diag,COD1_SS_REASON_STALE,0);
 memset(names,0,sizeof(names));count=files->list(files->user,names,sizeof(names));
 if(files->current(files->user)!=1)return Cod1SS_Fail(diag,COD1_SS_REASON_STALE,0);
 if(count<0||count>COD1_SS_TABLES)return Cod1SS_Fail(diag,COD1_SS_REASON_CAPACITY,0);
 for(i=0;i<count;++i){if(at>=sizeof(names)||!Cod1SS_Length(names+at,sizeof(names)-at,&n)||!Cod1SS_Basename(names+at))return Cod1SS_Fail(diag,COD1_SS_REASON_PATH,0);offsets[i]=at;at+=n+1;
  for(j=0;j<i;++j)if(Cod1SS_Equal(names+offsets[j],names+offsets[i]))return Cod1SS_Fail(diag,COD1_SS_REASON_DUPLICATE,0);
 }
 if(at>=sizeof(names)||names[at])return Cod1SS_Fail(diag,COD1_SS_REASON_CAPACITY,0);
 memset(&result,0,sizeof(result));
 for(i=0;i<count;++i){strcpy(path,"soundaliases/");strcat(path,names+offsets[i]);data=NULL;size=0;
  if(files->current(files->user)!=1)return Cod1SS_Fail(diag,COD1_SS_REASON_STALE,0);
  read_ok=files->read(files->user,path,&data,&size);current=files->current(files->user);
  status=COD1_SS_RESOURCE_HELD;detail.reason=COD1_SS_REASON_VFS;detail.line=0;
  if(current==1&&read_ok==1&&data&&size&&size<=COD1_SS_TABLE_BYTES)status=Cod1ScriptSoundParseTableMode(data,size,alias,map,&candidate,&detail,allowVoice,loopMode);
  /* A provider-owned buffer is released even when its read failed or lease
   * expired. No subsequent state/output publication occurs after stale release. */
  if(data)files->release(files->user,data);
  if(current!=1||files->current(files->user)!=1)return Cod1SS_Fail(diag,COD1_SS_REASON_STALE,0);
  if(status==COD1_SS_RESOURCE_HELD){if(diag)*diag=detail;return COD1_SS_RESOURCE_HELD;}
  if(status==COD1_SS_RESOURCE_READY){if(found)return Cod1SS_Fail(diag,COD1_SS_REASON_AMBIGUOUS,0);result=candidate;strcpy(result.table,path);found=1;}
 }
 if(!found)return COD1_SS_RESOURCE_NONE;
 for(i=0;i<(int)result.count;++i){if(files->current(files->user)!=1)return Cod1SS_Fail(diag,COD1_SS_REASON_STALE,0);status=files->exists(files->user,result.variants[i].path);
  if(files->current(files->user)!=1)return Cod1SS_Fail(diag,COD1_SS_REASON_STALE,0);
  if(status!=1)return Cod1SS_Fail(diag,COD1_SS_REASON_MISSING_FILE,0);
 }
 if(files->current(files->user)!=1)return Cod1SS_Fail(diag,COD1_SS_REASON_STALE,0);
 *out=result;return COD1_SS_RESOURCE_READY;
}
static Cod1SSResourceStatus Cod1ScriptSoundResolveSelected(const Cod1SSFiles *files,const char *alias,
 const char *map,Cod1SSPlan *out,Cod1SSDiagnostic *diag,int allowVoice) {
 return Cod1ScriptSoundResolveMode(files,alias,map,out,diag,allowVoice,0);
}
static Cod1SSResourceStatus Cod1LoopSoundResolve(const Cod1SSFiles *files,const char *alias,
 const char *map,Cod1SSPlan *out,Cod1SSDiagnostic *diag) {
 return Cod1ScriptSoundResolveMode(files,alias,map,out,diag,0,1);
}
static Cod1SSResourceStatus Cod1ScriptSoundResolve(const Cod1SSFiles *files,const char *alias,
 const char *map,Cod1SSPlan *out,Cod1SSDiagnostic *diag) {
 return Cod1ScriptSoundResolveSelected(files,alias,map,out,diag,0);
}
#endif
