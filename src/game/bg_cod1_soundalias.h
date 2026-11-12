#ifndef BG_COD1_SOUNDALIAS_H
#define BG_COD1_SOUNDALIAS_H
#include "bg_cod1_viewprofile.h"
/* Bounded CSV reader shared by legacy path lookup and native one-shot alias
 * metadata (volume/pitch/range/probability/loadspec).  Consumers still decide
 * how much of the lower sound-engine attenuation semantics they can reproduce. */
static int CW_AliasEqual(const char *a,const char *b) {
    for(;*a && *b;a++,b++) {
        int x=*a,y=*b;if(x>='A' && x<='Z')x+=32;if(y>='A' && y<='Z')y+=32;
        if(x!=y)return 0;
    }
    return *a==*b;
}
static const char *CW_AliasCell(const char *p,const char *end,char *out,int capacity,int *bad) {
    int quoted=0,closed=0,n=0;
    if(p<end && *p=='"'){quoted=1;p++;}
    while(p<end) {
        char c=*p++;
        if(quoted && c=='"') {
            if(p<end && *p=='"')p++;
            else {quoted=0;closed=1;continue;}
        } else if(!quoted && c==',')break;
        else if(closed){if(c!=' ' && c!='\t')*bad=1;continue;}
        if(n<capacity-1)out[n++]=c;else *bad=1;
    }
    if(quoted)*bad=1;out[n]=0;return p;
}


static void CW_AliasCopy(char *dst,int capacity,const char *src) {
    int n=0;if(!dst||capacity<=0)return;if(!src)src="";
    while(n+1<capacity&&src[n]){dst[n]=src[n];n++;}dst[n]=0;
}

typedef struct {
    char path[CW_VIEW_PATH];
    int sequence;
    float volumeMin, volumeMax;
    float pitchMin, pitchMax;
    float distMin, distMax;
    float probability;
    char channel[16];
    char loadspec[96];
    char masterSlave[32];
} cwSoundAliasVariant;

static float CW_AliasFloat(const char *s,float fallback) {
    char *endp;double v;
    if(!s || !*s)return fallback;
    v=strtod(s,&endp);if(endp==s || !isfinite(v))return fallback;
    return (float)v;
}
static int CW_AliasInt(const char *s,int fallback) {
    char *endp;long v;
    if(!s || !*s)return fallback;
    v=strtol(s,&endp,10);if(endp==s)return fallback;
    return (int)v;
}
/* Full bounded rows for native one-shot playback. Empty cells use the defaults
 * documented by CoD's own CSV header. This does not merge aliases across files;
 * VFS/table precedence remains the caller's policy, as with Paths(). */
static int CW_SoundAliasVariants(const char *data,int size,const char *alias,
    cwSoundAliasVariant *out,int capacity) {
    enum { F_NAME,F_SEQUENCE,F_FILE,F_VMIN,F_VMAX,F_PMIN,F_PMAX,F_DMIN,F_DMAX,F_CHANNEL,F_PROB,F_LOADSPEC,F_MASTER,F_COUNT };
    const char *p=data,*end,*lineEnd,*next;int idx[F_COUNT],col,n=0,bad,i,header=0;
    char cell[256],values[F_COUNT][256];
    const char *keys[F_COUNT]={"name","sequence","file","vol_min","vol_max","pitch_min","pitch_max","dist_min","dist_max","channel","probability","loadspec","masterslave"};
    if(!data||size<=0||!alias||!*alias||!out||capacity<=0)return 0;
    for(i=0;i<F_COUNT;i++)idx[i]=-1;
    end=data+size;if(size>=3&&(unsigned char)p[0]==239&&(unsigned char)p[1]==187&&(unsigned char)p[2]==191)p+=3;
    while(p<end&&n<capacity){
        lineEnd=p;while(lineEnd<end&&*lineEnd!='\n'&&*lineEnd!='\r')lineEnd++;
        next=lineEnd;while(next<end&&(*next=='\n'||*next=='\r'))next++;
        bad=col=0;for(i=0;i<F_COUNT;i++)values[i][0]=0;
        while(p<lineEnd){
            p=CW_AliasCell(p,lineEnd,cell,sizeof(cell),&bad);
            if(col==0&&(!cell[0]||cell[0]=='#'))break;
            if(!header){
                for(i=0;i<F_COUNT;i++)if(CW_AliasEqual(cell,keys[i]))idx[i]=col;
            } else {
                for(i=0;i<F_COUNT;i++)if(idx[i]==col)CW_AliasCopy(values[i],sizeof(values[i]),cell);
            }
            col++;
        }
        if(!bad){
            if(!header && idx[F_NAME]>=0 && idx[F_FILE]>=0)header=1;
            else if(header && CW_AliasEqual(values[F_NAME],alias)){
                cwSoundAliasVariant *v=&out[n];float x;
                memset(v,0,sizeof(*v));
                if(CW_ViewPath(v->path,values[F_FILE],"sound/",0)){
                    v->sequence=CW_AliasInt(values[F_SEQUENCE],0);
                    v->volumeMin=CW_AliasFloat(values[F_VMIN],1.0f);
                    v->volumeMax=CW_AliasFloat(values[F_VMAX],v->volumeMin);
                    v->pitchMin=CW_AliasFloat(values[F_PMIN],1.0f);
                    v->pitchMax=CW_AliasFloat(values[F_PMAX],v->pitchMin);
                    v->distMin=CW_AliasFloat(values[F_DMIN],120.0f);
                    v->distMax=CW_AliasFloat(values[F_DMAX],0.0f);
                    v->probability=CW_AliasFloat(values[F_PROB],1.0f);
                    if(v->volumeMax<v->volumeMin){x=v->volumeMin;v->volumeMin=v->volumeMax;v->volumeMax=x;}
                    if(v->pitchMax<v->pitchMin){x=v->pitchMin;v->pitchMin=v->pitchMax;v->pitchMax=x;}
                    if(v->probability<0)v->probability=0;
                    CW_AliasCopy(v->channel,sizeof(v->channel),values[F_CHANNEL][0]?values[F_CHANNEL]:"auto");
                    CW_AliasCopy(v->loadspec,sizeof(v->loadspec),values[F_LOADSPEC]);
                    CW_AliasCopy(v->masterSlave,sizeof(v->masterSlave),values[F_MASTER]);
                    n++;
                }
            }
        }
        p=next;
    }
    return n;
}
/* CoD loadspec is a whitespace-separated map filter.  Blank means all maps;
 * positive tokens select only named maps, while !name excludes that map.
 * The retail sound CSV uses both forms (notably Truckride impact aliases). */

/* Case-insensitive substring helper for locating retail aliases when a behavior
 * is hard-coded by the original cgame rather than named in WeaponDef. */
static int CW_AliasContains(const char *text,const char *token) {
    const char *p,*a,*b;int x,y;
    if(!text||!token||!*token)return 0;
    for(p=text;*p;p++){
        a=p;b=token;
        while(*a&&*b){x=*a;y=*b;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)break;a++;b++;}
        if(!*b)return 1;
    }
    return 0;
}
/* Same bounded CSV semantics as CW_SoundAliasVariants, but selects rows by a
 * token in the file column.  This is intentionally a discovery fallback: the
 * normal alias-name lookup stays authoritative whenever the retail alias name
 * is known. */
static int CW_SoundAliasVariantsByFileToken(const char *data,int size,const char *token,
    cwSoundAliasVariant *out,int capacity) {
    enum { F_NAME,F_SEQUENCE,F_FILE,F_VMIN,F_VMAX,F_PMIN,F_PMAX,F_DMIN,F_DMAX,F_CHANNEL,F_PROB,F_LOADSPEC,F_MASTER,F_COUNT };
    const char *p=data,*end,*lineEnd,*next;int idx[F_COUNT],col,n=0,bad,i,header=0;
    char cell[256],values[F_COUNT][256];
    const char *keys[F_COUNT]={"name","sequence","file","vol_min","vol_max","pitch_min","pitch_max","dist_min","dist_max","channel","probability","loadspec","masterslave"};
    if(!data||size<=0||!token||!*token||!out||capacity<=0)return 0;
    for(i=0;i<F_COUNT;i++)idx[i]=-1;
    end=data+size;if(size>=3&&(unsigned char)p[0]==239&&(unsigned char)p[1]==187&&(unsigned char)p[2]==191)p+=3;
    while(p<end&&n<capacity){
        lineEnd=p;while(lineEnd<end&&*lineEnd!='\n'&&*lineEnd!='\r')lineEnd++;
        next=lineEnd;while(next<end&&(*next=='\n'||*next=='\r'))next++;
        bad=col=0;for(i=0;i<F_COUNT;i++)values[i][0]=0;
        while(p<lineEnd){
            p=CW_AliasCell(p,lineEnd,cell,sizeof(cell),&bad);
            if(col==0&&(!cell[0]||cell[0]=='#'))break;
            if(!header){for(i=0;i<F_COUNT;i++)if(CW_AliasEqual(cell,keys[i]))idx[i]=col;}
            else {for(i=0;i<F_COUNT;i++)if(idx[i]==col)CW_AliasCopy(values[i],sizeof(values[i]),cell);}
            col++;
        }
        if(!bad){
            if(!header&&idx[F_NAME]>=0&&idx[F_FILE]>=0)header=1;
            else if(header&&CW_AliasContains(values[F_FILE],token)){
                cwSoundAliasVariant *v=&out[n];float x;
                memset(v,0,sizeof(*v));
                if(CW_ViewPath(v->path,values[F_FILE],"sound/",0)){
                    v->sequence=CW_AliasInt(values[F_SEQUENCE],0);
                    v->volumeMin=CW_AliasFloat(values[F_VMIN],1.0f);v->volumeMax=CW_AliasFloat(values[F_VMAX],v->volumeMin);
                    v->pitchMin=CW_AliasFloat(values[F_PMIN],1.0f);v->pitchMax=CW_AliasFloat(values[F_PMAX],v->pitchMin);
                    v->distMin=CW_AliasFloat(values[F_DMIN],120.0f);v->distMax=CW_AliasFloat(values[F_DMAX],0.0f);
                    v->probability=CW_AliasFloat(values[F_PROB],1.0f);
                    if(v->volumeMax<v->volumeMin){x=v->volumeMin;v->volumeMin=v->volumeMax;v->volumeMax=x;}
                    if(v->pitchMax<v->pitchMin){x=v->pitchMin;v->pitchMin=v->pitchMax;v->pitchMax=x;}
                    if(v->probability<0)v->probability=0;
                    CW_AliasCopy(v->channel,sizeof(v->channel),values[F_CHANNEL][0]?values[F_CHANNEL]:"auto");
                    CW_AliasCopy(v->loadspec,sizeof(v->loadspec),values[F_LOADSPEC]);
                    CW_AliasCopy(v->masterSlave,sizeof(v->masterSlave),values[F_MASTER]);
                    n++;
                }
            }
        }
        p=next;
    }
    return n;
}

static int CW_SoundAliasLoadSpecMatches(const char *spec,const char *mapName) {
    const char *p;int havePositive=0,matchedPositive=0;char token[96],map[96];int n;
    if(!spec||!*spec)return 1;
    if(!mapName)mapName="";
    p=mapName;
    if((p[0]=='m'||p[0]=='M')&&(p[1]=='a'||p[1]=='A')&&(p[2]=='p'||p[2]=='P')&&(p[3]=='s'||p[3]=='S')&&(p[4]=='/'||p[4]=='\\'))p+=5;
    CW_AliasCopy(map,sizeof(map),p);
    n=(int)strlen(map);if(n>4&&map[n-4]=='.'&&(map[n-3]=='b'||map[n-3]=='B')&&(map[n-2]=='s'||map[n-2]=='S')&&(map[n-1]=='p'||map[n-1]=='P'))map[n-4]=0;
    p=spec;
    while(*p){
        int neg=0,i=0;while(*p==' '||*p=='\t')p++;if(!*p)break;
        /* Original CSV also spells negation as "! Pavlov". Whitespace
         * belongs to that operator; the next map remains excluded. */
        if(*p=='!'){neg=1;p++;while(*p==' '||*p=='\t')p++;if(!*p||*p=='!')return 0;}else havePositive=1;
        while(*p&&*p!=' '&&*p!='\t'){if(i+1<(int)sizeof(token))token[i++]=*p;p++;}token[i]=0;
        if(!token[0])continue;
        if(CW_AliasEqual(token,map)){if(neg)return 0;matchedPositive=1;}
    }
    return havePositive?matchedPositive:1;
}

static int CW_SoundAliasPaths(const char *data,int size,const char *alias,char paths[][CW_VIEW_PATH],int capacity) {
    const char *p=data,*end,*lineEnd,*next;
    int nc=-1,fc=-1,col,n=0,bad,headerNc,headerFc;
    char cell[256],name[256],file[256];
    if(!data || size<=0 || !alias || !*alias || capacity<=0)return 0;
    end=data+size;
    if(size>=3 && (unsigned char)p[0]==239 && (unsigned char)p[1]==187 && (unsigned char)p[2]==191)p+=3;
    while(p<end && n<capacity) {
        lineEnd=p;while(lineEnd<end && *lineEnd!='\n' && *lineEnd!='\r')lineEnd++;
        next=lineEnd;while(next<end && (*next=='\n' || *next=='\r'))next++;
        bad=col=0;name[0]=file[0]=0;headerNc=headerFc=-1;
        while(p<lineEnd) {
            p=CW_AliasCell(p,lineEnd,cell,sizeof(cell),&bad);
            if(col==0 && (!cell[0] || cell[0]=='#'))break;
            if(nc<0) {
                if(CW_AliasEqual(cell,"name"))headerNc=col;
                if(CW_AliasEqual(cell,"file"))headerFc=col;
            } else {
                if(col==nc)strcpy(name,cell);
                if(col==fc)strcpy(file,cell);
            }
            col++;
        }
        if(!bad) {
            if(nc<0 && headerNc>=0 && headerFc>=0){nc=headerNc;fc=headerFc;}
            else if(nc>=0 && CW_AliasEqual(name,alias) && CW_ViewPath(paths[n],file,"sound/",0))n++;
        }
        p=next;
    }
    return n;
}
#endif
