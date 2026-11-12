/* R120 private paired ambience transport. Native control state is separate
 * from music; the mixer policy is documented separately. No network/save layout. */
#ifndef BG_COD1_AMBIENT_H
#define BG_COD1_AMBIENT_H
#include "bg_cod1_debug_line.h"
#include "bg_cod1_music.h"
#include "../qcommon/cod1_ambient_module.h"
#define COD1_AMBIENT_ALIAS_BYTES 128
#define COD1_AMBIENT_FILES COD1_MUSIC_FILES
#define COD1_AMBIENT_LIST_BYTES COD1_MUSIC_LIST_BYTES
#define COD1_AMBIENT_TABLE_BYTES COD1_MUSIC_TABLE_BYTES
#define COD1_AMBIENT_TOTAL_BYTES COD1_MUSIC_TOTAL_BYTES
#define COD1_AMBIENT_COMMAND "c1a"
#define COD1_AMBIENT_ARGC 8
#define COD1_AMBIENT_COMMAND_CAPACITY 324
#define CS_COD1_AMBIENT_METADATA (CS_COD1_DEBUG_LINE_METADATA + 1)
#if defined(MAX_STRING_CHARS) && COD1_AMBIENT_COMMAND_CAPACITY > MAX_STRING_CHARS
#error ambience command exceeds engine command storage
#endif
#if defined(MAX_CONFIGSTRINGS) && CS_COD1_AMBIENT_METADATA >= MAX_CONFIGSTRINGS
#error ambience metadata exceeds configstring storage
#endif
enum { COD1_AMBIENT_PLAY=1, COD1_AMBIENT_STOP=2, COD1_AMBIENT_RESET=3 };
typedef struct { char path[CW_VIEW_PATH]; float gain; int looping; } cod1AmbientPlan;
typedef cod1MusicFiles cod1AmbientFiles;
/* Executable import payload; copied by the receiver before callbacks. */
typedef struct cod1AmbientPlayback_s {
    int kind,transitionMs;
    uint64_t epoch,operation_id,client_incarnation;
    int client_time;
    char alias[COD1_AMBIENT_ALIAS_BYTES];
    cod1AmbientPlan plan;
} cod1AmbientPlayback;
typedef struct {
    uint64_t epoch,operation_id;
    int server_time,deadline,kind;
    char alias[COD1_AMBIENT_ALIAS_BYTES];
} cod1Ambient_t;
#if defined(_MSC_VER)
#define CA_INLINE __inline
#elif defined(__GNUC__)
#define CA_INLINE __inline__
#else
#define CA_INLINE inline
#endif
/* The engine's protected ambience module counter is trusted realm authority;
 * the low word is a bounded private scene generation. */
static CA_INLINE uint32_t CA_ModuleId(uint64_t epoch) { return (uint32_t)(epoch>>32); }
static CA_INLINE uint32_t CA_Generation(uint64_t epoch) { return (uint32_t)epoch; }
static CA_INLINE int CA_Epoch(uint64_t epoch) {
    uint32_t generation=CA_Generation(epoch),module=CA_ModuleId(epoch);
    return module && module<=COD1_AMBIENT_MODULE_MAX && generation && generation!=UINT32_MAX;
}
/* The protected SYSTEMINFO counter emits canonical positive decimal. Zero is
 * unavailable/exhausted; malformed/overflowing values do not publish output. */
static CA_INLINE int CA_ModuleIdToken(const char *text,uint32_t *out) {
    uint64_t magnitude=0,limit=COD1_AMBIENT_MODULE_MAX;size_t i=0;
    if(!text || !out || !text[0] || text[0]=='0') return 0;
    for(;text[i];++i) {
        unsigned int digit;
        if(i>10 || text[i]<'0' || text[i]>'9') return 0;
        digit=(unsigned int)(text[i]-'0');
        if(magnitude>(limit-digit)/10) return 0;
        magnitude=magnitude*10+digit;
    }
    *out=(uint32_t)magnitude;
    return 1;
}
static CA_INLINE int CA_Validate(const cod1Ambient_t *event) {
    if(!event || !CA_Epoch(event->epoch) || !event->operation_id ||
       event->server_time<0 || event->deadline<event->server_time ||
       !memchr(event->alias,0,sizeof(event->alias))) return 0;
    if(event->kind==COD1_AMBIENT_PLAY) return Cod1MusicAliasToken(event->alias);
    return event->kind==COD1_AMBIENT_STOP && !event->alias[0];
}
static CA_INLINE int CA_Follows(const cod1Ambient_t *event,uint64_t epoch,uint64_t operation_id) {
    return CA_Validate(event) && event->epoch==epoch && event->operation_id>operation_id;
}
static CA_INLINE int CA_EncodeCommand(char *out,size_t capacity,const cod1Ambient_t *event) {
    char encoded[COD1_AMBIENT_COMMAND_CAPACITY];size_t n=67,i,length;
    if(!out || !CA_Validate(event)) return 0;
    memcpy(encoded,"c1a 1 ",6);CQ_WriteHex(encoded+6,16,event->epoch);encoded[22]=' ';
    CQ_WriteHex(encoded+23,16,event->operation_id);encoded[39]=' ';
    CQ_WriteHex(encoded+40,8,(uint32_t)event->server_time);encoded[48]=' ';
    CQ_WriteHex(encoded+49,8,(uint32_t)event->deadline);encoded[57]=' ';
    CQ_WriteHex(encoded+58,8,(uint32_t)event->kind);encoded[66]=' ';
    length=strlen(event->alias);
    if(!length) encoded[n++]='-';
    else for(i=0;i<length;++i) {CQ_WriteHex(encoded+n,2,(unsigned char)event->alias[i]);n+=2;}
    encoded[n++]=0;
    if(capacity<n) return 0;
    memcpy(out,encoded,n);return 1;
}
static CA_INLINE int CA_DecodeCommand(int argc,const char *const *argv,cod1Ambient_t *out) {
    cod1Ambient_t event;uint64_t fields[3],byte;char hex[3];size_t n,i;
    if(!out || !argv || argc!=COD1_AMBIENT_ARGC || !argv[0] || !argv[1] ||
       strcmp(argv[0],COD1_AMBIENT_COMMAND) || strcmp(argv[1],"1")) return 0;
    memset(&event,0,sizeof(event));
    if(!CQ_ParseHex(argv[2],16,&event.epoch) || !CQ_ParseHex(argv[3],16,&event.operation_id)) return 0;
    for(i=0;i<3;++i) if(!CQ_ParseHex(argv[4+i],8,&fields[i]) || fields[i]>UINT32_C(0x7fffffff)) return 0;
    event.server_time=(int)fields[0];event.deadline=(int)fields[1];event.kind=(int)fields[2];
    if(!argv[7]) return 0;
    for(n=0;n<=2*(COD1_AMBIENT_ALIAS_BYTES-1);++n) if(!argv[7][n]) break;
    if(n==1 && argv[7][0]=='-') event.alias[0]=0;
    else {
        if(!n || n>2*(COD1_AMBIENT_ALIAS_BYTES-1) || n%2) return 0;
        for(i=0;i<n/2;++i) {
            memcpy(hex,argv[7]+2*i,2);hex[2]=0;
            if(!CQ_ParseHex(hex,2,&byte) || !byte) return 0;
            event.alias[i]=(char)byte;
        }
    }
    if(!CA_Validate(&event)) return 0;
    *out=event;return 1;
}
static CA_INLINE int CA_EncodeMetadata(char *out,size_t capacity,uint64_t epoch) {
    char encoded[23];
    if(!out || capacity<sizeof(encoded) || !CA_Epoch(epoch)) return 0;
    memcpy(encoded,"c1e 1 ",6);CQ_WriteHex(encoded+6,16,epoch);encoded[22]=0;
    memcpy(out,encoded,sizeof(encoded));return 1;
}
static CA_INLINE int CA_DecodeMetadata(const char *text,uint64_t *out) {
    char hex[17];uint64_t epoch;size_t n;
    if(!text || !out) return 0;
    for(n=0;n<=22;++n) if(!text[n]) break;
    if(n!=22 || memcmp(text,"c1e 1 ",6)) return 0;
    memcpy(hex,text+6,16);hex[16]=0;
    if(!CQ_ParseHex(hex,16,&epoch) || !CA_Epoch(epoch)) return 0;
    *out=epoch;return 1;
}
#undef CA_INLINE
/* Selected bounded alias admission, not recovered native CSV parity. Only a
 * single fixed-gain local streamed row is represented; random/duplicate,
 * pitch/master/sequence/probability variants are refused. */
static int Cod1AmbientTable(const char *data,int size,const char *alias,
                         const char *map,cod1AmbientPlan *out) {
    enum {N,F,V0,V1,P0,P1,CH,TYPE,PROB,LOOP,MASTER,LOAD,SEQ,COUNT};
    static const char *keys[COUNT]={"name","file","vol_min","vol_max","pitch_min","pitch_max","channel","type","probability","loop","masterslave","loadspec","sequence"};
    int columns[COUNT],header=0,found=0,i;const char *p=data,*end=data+size;
    char cell[256],values[COUNT][256];
    for(i=0;i<COUNT;++i)columns[i]=-1;
    if(size>=3&&(unsigned char)p[0]==239&&(unsigned char)p[1]==187&&(unsigned char)p[2]==191)p+=3;
    while(p<end){const char *finish=p,*next;int column=0,bad=0;
        while(finish<end&&*finish!='\r'&&*finish!='\n')++finish;
        next=finish;while(next<end&&(*next=='\r'||*next=='\n'))++next;
        for(i=0;i<COUNT;++i){values[i][0]=0;if(!header)columns[i]=-1;}
        while(p<finish){p=CW_AliasCell(p,finish,cell,sizeof(cell),&bad);
            if(!column&&(!cell[0]||cell[0]=='#'))break;
            if(!header){for(i=0;i<COUNT;++i)if(CW_AliasEqual(cell,keys[i]))columns[i]=column;}
            else {for(i=0;i<COUNT;++i)if(columns[i]==column)CW_AliasCopy(values[i],sizeof(values[i]),cell);}
            ++column;
        }
        if(!header){if(!bad&&columns[N]>=0&&columns[F]>=0)header=1;}
        else if(CW_AliasEqual(values[N],alias)){
            float gain,gainMax,pitch,pitchMax,probability,sequence;
            if(bad||++found>1||!CW_ViewPath(out->path,values[F],"sound/",0)||
               !Cod1MusicNumber(values[V0],1.0f,&gain)||!Cod1MusicNumber(values[V1],gain,&gainMax)||
               !Cod1MusicNumber(values[P0],1.0f,&pitch)||!Cod1MusicNumber(values[P1],pitch,&pitchMax)||
               !Cod1MusicNumber(values[PROB],1.0f,&probability)||!Cod1MusicNumber(values[SEQ],0.0f,&sequence)||
               gain<0||gain>4||gain!=gainMax||pitch!=1||pitchMax!=1||probability!=1||sequence!=0||
               !CW_AliasEqual(values[CH],"local")||!CW_AliasEqual(values[TYPE],"streamed")||values[MASTER][0]||
               !CW_SoundAliasLoadSpecMatches(values[LOAD],map))return -1;
            if(!values[LOOP][0]||CW_AliasEqual(values[LOOP],"nonlooping"))out->looping=0;
            else if(CW_AliasEqual(values[LOOP],"looping"))out->looping=1;
            else return -1;
            {size_t length=strlen(out->path);
                if(length<4||(!CW_AliasEqual(out->path+length-4,".wav")&&!CW_AliasEqual(out->path+length-4,".mp3")))return -1;}
            out->gain=gain;
        }
        p=next;
    }
    return found;
}
static int Cod1AmbientFileCompare(const void *a,const void *b){return strcmp((const char *)a,(const char *)b);}
/* VFS precedence is supplied by the mounted filesystem for each named file.
 * Cross-table duplicate names are refused, not silently assigned precedence. */
static int Cod1AmbientResolve(const cod1AmbientFiles *files,const char *alias,
                           const char *map,cod1AmbientPlan *out) {
    char *list,*data;char (*names)[64];char path[128];cod1AmbientPlan pending;
    int count,i,j,pos=0,total=0,found=0,result=0,size,matches;
    if(!files||!files->list||!files->read||!files->release||!files->exists||
       !Cod1MusicAliasToken(alias)||!out)return 0;
    list=(char *)calloc(1,COD1_AMBIENT_LIST_BYTES);
    names=(char (*)[64])calloc(COD1_AMBIENT_FILES,64);
    if(!list||!names){free(list);free(names);return 0;}
    count=files->list(files->user,list,COD1_AMBIENT_LIST_BYTES);
    if(count<=0||count>=COD1_AMBIENT_FILES)goto done;
    for(i=0;i<count;++i){int length=0;
        while(pos+length<COD1_AMBIENT_LIST_BYTES&&list[pos+length])++length;
        if(length<5||length>=64||pos+length>=COD1_AMBIENT_LIST_BYTES-2||strstr(list+pos,"..")||
           !CW_AliasEqual(list+pos+length-4,".csv"))goto done;
        for(j=0;j<length;++j){char c=list[pos+j];
            if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))goto done;}
        memcpy(names[i],list+pos,(size_t)length+1);pos+=length+1;
    }
    qsort(names,(size_t)count,64,Cod1AmbientFileCompare);
    for(i=0;i<count;++i){
        if(i&&!strcmp(names[i],names[i-1]))goto done;
        snprintf(path,sizeof(path),"soundaliases/%s",names[i]);data=NULL;size=0;
        if(!files->read(files->user,path,&data,&size)||!data)goto done;
        if(size<=0||size>COD1_AMBIENT_TABLE_BYTES||size>COD1_AMBIENT_TOTAL_BYTES-total||memchr(data,0,(size_t)size)){
            files->release(files->user,data);goto done;}
        total+=size;memset(&pending,0,sizeof(pending));
        matches=Cod1AmbientTable(data,size,alias,map,&pending);files->release(files->user,data);
        if(matches<0||found+matches>1)goto done;
        if(matches){*out=pending;found=1;}
    }
    if(found&&files->exists(files->user,out->path))result=1;
done:
    free(names);free(list);if(!result)memset(out,0,sizeof(*out));return result;
}
#endif
