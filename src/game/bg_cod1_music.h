/* SPDX-License-Identifier: GPL-3.0-or-later
 * R107 bounded mounted-CSV music plan. This admits one fixed-gain streamed
 * variant only. It does not reproduce random selection, pitch variation,
 * alias merging or completed-sound notifications. */
#ifndef BG_COD1_MUSIC_H
#define BG_COD1_MUSIC_H
#include "bg_cod1_soundalias.h"
#define COD1_MUSIC_ALIAS_BYTES 128
#define COD1_MUSIC_FILES 512
#define COD1_MUSIC_LIST_BYTES 65536
#define COD1_MUSIC_TABLE_BYTES 1048576
#define COD1_MUSIC_TOTAL_BYTES 16777216
typedef struct { char path[CW_VIEW_PATH]; float gain; int looping; } cod1MusicPlan;
enum { COD1_MUSIC_PLAY=1, COD1_MUSIC_STOP=2 };
/* Private cgame/executable import payload. No network or savegame layout. */
typedef struct cod1MusicPlayback_s {
    int kind, transitionMs;
    cod1MusicPlan plan;
} cod1MusicPlayback;
typedef struct {
    void *user;
    int (*list)(void *,char *,int);
    int (*read)(void *,const char *,char **,int *);
    void (*release)(void *,char *);
    int (*exists)(void *,const char *);
} cod1MusicFiles;
static int Cod1MusicAliasToken(const char *value) {
    int i;
    if(!value||!*value)return 0;
    for(i=0;i<COD1_MUSIC_ALIAS_BYTES&&value[i];++i)
        if(!((value[i]>='a'&&value[i]<='z')||(value[i]>='A'&&value[i]<='Z')||
             (value[i]>='0'&&value[i]<='9')||value[i]=='_'||value[i]=='-'))return 0;
    return i<COD1_MUSIC_ALIAS_BYTES;
}
static int Cod1MusicMilliseconds(const char *value,int *out) {
    unsigned int result=0;const char *p=value;
    if(!p||!*p||!out)return 0;
    for(;*p;++p){unsigned int digit;if(*p<'0'||*p>'9')return 0;
        digit=(unsigned int)(*p-'0');if(result>(2147483647u-digit)/10u)return 0;
        result=result*10u+digit;}
    *out=(int)result;return 1;
}
static int Cod1MusicNumber(const char *value,float fallback,float *out) {
    char *end;double number;
    if(!value||!*value){*out=fallback;return 1;}
    number=strtod(value,&end);
    if(end==value||*end||!isfinite(number)||number>3.4028234663852886e38||number< -3.4028234663852886e38)return 0;
    *out=(float)number;return isfinite(*out);
}
/* Return matching-row count, or -1 for a matching malformed/unsupported row.
 * A second matching row is refused even if its loadspec excludes this map. */
static int Cod1MusicTable(const char *data,int size,const char *alias,
                         const char *map,cod1MusicPlan *out) {
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
               !CW_AliasEqual(values[CH],"music")||!CW_AliasEqual(values[TYPE],"streamed")||values[MASTER][0]||
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
static int Cod1MusicFileCompare(const void *a,const void *b){return strcmp((const char *)a,(const char *)b);}
/* VFS precedence is supplied by the mounted filesystem for each named file.
 * Cross-table duplicate names are refused, not silently assigned precedence. */
static int Cod1MusicResolve(const cod1MusicFiles *files,const char *alias,
                           const char *map,cod1MusicPlan *out) {
    char *list,*data;char (*names)[64];char path[128];cod1MusicPlan pending;
    int count,i,j,pos=0,total=0,found=0,result=0,size,matches;
    if(!files||!files->list||!files->read||!files->release||!files->exists||
       !Cod1MusicAliasToken(alias)||!out)return 0;
    list=(char *)calloc(1,COD1_MUSIC_LIST_BYTES);
    names=(char (*)[64])calloc(COD1_MUSIC_FILES,64);
    if(!list||!names){free(list);free(names);return 0;}
    count=files->list(files->user,list,COD1_MUSIC_LIST_BYTES);
    if(count<=0||count>=COD1_MUSIC_FILES)goto done;
    for(i=0;i<count;++i){int length=0;
        while(pos+length<COD1_MUSIC_LIST_BYTES&&list[pos+length])++length;
        if(length<5||length>=64||pos+length>=COD1_MUSIC_LIST_BYTES-2||strstr(list+pos,"..")||
           !CW_AliasEqual(list+pos+length-4,".csv"))goto done;
        for(j=0;j<length;++j){char c=list[pos+j];
            if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))goto done;}
        memcpy(names[i],list+pos,(size_t)length+1);pos+=length+1;
    }
    qsort(names,(size_t)count,64,Cod1MusicFileCompare);
    for(i=0;i<count;++i){
        if(i&&!strcmp(names[i],names[i-1]))goto done;
        snprintf(path,sizeof(path),"soundaliases/%s",names[i]);data=NULL;size=0;
        if(!files->read(files->user,path,&data,&size)||!data)goto done;
        if(size<=0||size>COD1_MUSIC_TABLE_BYTES||size>COD1_MUSIC_TOTAL_BYTES-total||memchr(data,0,(size_t)size)){
            files->release(files->user,data);goto done;}
        total+=size;memset(&pending,0,sizeof(pending));
        matches=Cod1MusicTable(data,size,alias,map,&pending);files->release(files->user,data);
        if(matches<0||found+matches>1)goto done;
        if(matches){*out=pending;found=1;}
    }
    if(found&&files->exists(files->user,out->path))result=1;
done:
    free(names);free(list);if(!result)memset(out,0,sizeof(*out));return result;
}
#endif
