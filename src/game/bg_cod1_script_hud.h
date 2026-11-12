/* R200 bounded paired HUD state. A script mutation sends one snapshot; time
 * advances entirely on the client. This is a port wire schema, not retail ABI. */
#ifndef BG_COD1_SCRIPT_HUD_H
#define BG_COD1_SCRIPT_HUD_H
#include "bg_cod1_script_quake.h"
#include "../qcommon/cod1_item_protocol.h"
#define CS_COD1_SCRIPT_HUD 1748
#define COD1_SCRIPT_HUD_CAPACITY 8
#define COD1_SCRIPT_HUD_COMMAND "c1h"
#define COD1_SCRIPT_HUD_ARGC 18
#define COD1_SCRIPT_HUD_INTRO_ARGC 26
#define COD1_SCRIPT_HUD_MAX_ARGC 26
#define COD1_SCRIPT_HUD_PACKET_BYTES 512
#define COD1_SCRIPT_HUD_MARKER_BYTES 23
#if CS_COD1_SCRIPT_HUD <= CS_COD1_ITEM_PRECACHE
#error HUD marker overlaps prior metadata
#endif
#if defined(MAX_CONFIGSTRINGS) && CS_COD1_SCRIPT_HUD >= MAX_CONFIGSTRINGS
#error HUD marker exceeds configstring table
#endif
typedef struct {
    uint64_t epoch,owner,sequence;
    uint32_t index,generation,alive,clock;
    int server_time,remaining_ms,total_ms,width,height;
    float x,y;
    uint32_t align_x,align_y;
    uint32_t content,alpha,from_alpha; /* 0=legacy/invisible, 1=black, 2=localized text */
    int fade_start,fade_ms;float font_scale,sort;char text[128];
} cod1ScriptHud_t;
#if defined(_MSC_VER)
#define CH_INLINE __inline
#else
#define CH_INLINE inline
#endif
static CH_INLINE int CH_IntroValid(const cod1ScriptHud_t *s){
    if(!s->content)return 1;
    return s->content<=2&&!s->clock&&s->alpha<=255&&s->from_alpha<=255&&s->fade_start>=0&&s->fade_ms>=0&&s->fade_ms<=60000&&
        CQ_Finite(s->font_scale)&&s->font_scale>0&&s->font_scale<=16&&CQ_Finite(s->sort)&&s->sort>=-8192&&s->sort<=8192&&
        memchr(s->text,0,sizeof(s->text))&&s->width<=1024&&s->height<=1024;
}
static CH_INLINE float CH_Alpha(const cod1ScriptHud_t *s,int now){
    double t=s->fade_ms?(double)((int64_t)now-s->fade_start)/s->fade_ms:1.0;
    if(t<0)t=0;if(t>1)t=1;
    return (float)((s->from_alpha+((double)s->alpha-s->from_alpha)*t)/255.0);
}
static CH_INLINE int CH_Validate(const cod1ScriptHud_t *s){
    return s&&CH_IntroValid(s)&&s->epoch&&s->epoch!=UINT64_MAX&&s->owner&&s->sequence&&s->index<COD1_SCRIPT_HUD_CAPACITY&&s->generation&&
        s->alive<=1&&s->clock<=1&&(!s->clock||s->alive)&&s->server_time>=0&&
        s->remaining_ms>=0&&s->total_ms>=0&&s->width>=0&&s->height>=0&&CQ_Finite(s->x)&&CQ_Finite(s->y)&&
        s->x>=-8192&&s->x<=8192&&s->y>=-8192&&s->y<=8192&&s->align_x<=2&&s->align_y<=2&&
        (!s->clock||(s->remaining_ms>=0&&s->remaining_ms<=s->total_ms&&s->total_ms>0&&s->total_ms<=86400000&&s->width>=1&&s->width<=1024&&s->height>=1&&s->height<=1024));
}
static CH_INLINE int CH_Encode(char *out,size_t capacity,const cod1ScriptHud_t *s){
    uint32_t fields[20];char text[COD1_SCRIPT_HUD_PACKET_BYTES];size_t n=5;int i,count=s&&s->content?20:13;
    if(!out||capacity<sizeof(text)||!CH_Validate(s))return 0;
    memcpy(text,count==20?"c1h 2":"c1h 1",5);
    text[n++]=' ';CQ_WriteHex(text+n,16,s->epoch);n+=16;
    text[n++]=' ';CQ_WriteHex(text+n,16,s->owner);n+=16;
    text[n++]=' ';CQ_WriteHex(text+n,16,s->sequence);n+=16;
    fields[0]=s->index;fields[1]=s->generation;fields[2]=s->alive;fields[3]=s->clock;fields[4]=(uint32_t)s->server_time;
    fields[5]=(uint32_t)s->remaining_ms;fields[6]=(uint32_t)s->total_ms;fields[7]=(uint32_t)s->width;fields[8]=(uint32_t)s->height;
    fields[9]=CQ_FloatBits(s->x);fields[10]=CQ_FloatBits(s->y);fields[11]=s->align_x;fields[12]=s->align_y;
    if(count==20){fields[13]=s->content;fields[14]=s->alpha;fields[15]=s->from_alpha;fields[16]=s->fade_start;fields[17]=s->fade_ms;fields[18]=CQ_FloatBits(s->font_scale);fields[19]=CQ_FloatBits(s->sort);}
    for(i=0;i<count;i++){text[n++]=' ';CQ_WriteHex(text+n,8,fields[i]);n+=8;}text[n]=0;
    if(count==20){text[n++]=' ';if(!s->text[0])text[n++]='-';else for(i=0;s->text[i];i++){CQ_WriteHex(text+n,2,(unsigned char)s->text[i]);n+=2;}text[n]=0;}
    memcpy(out,text,n+1);return 1;
}
static CH_INLINE int CH_Decode(int argc,const char *const *argv,cod1ScriptHud_t *out){
    cod1ScriptHud_t s;uint64_t f[20],byte;int i,count;size_t len;
    if(!out||!argv||!argv[0]||!argv[1]||strcmp(argv[0],"c1h"))return 0;
    count=!strcmp(argv[1],"1")?13:!strcmp(argv[1],"2")?20:0;
    if(!count||argc!=(count==13?COD1_SCRIPT_HUD_ARGC:COD1_SCRIPT_HUD_INTRO_ARGC))return 0;
    memset(&s,0,sizeof(s));if(!CQ_ParseHex(argv[2],16,&s.epoch)||!CQ_ParseHex(argv[3],16,&s.owner)||!CQ_ParseHex(argv[4],16,&s.sequence))return 0;
    for(i=0;i<count;i++)if(!CQ_ParseHex(argv[5+i],8,&f[i]))return 0;
    for(i=4;i<=8;i++)if(f[i]>INT32_MAX)return 0;
    s.index=(uint32_t)f[0];s.generation=(uint32_t)f[1];s.alive=(uint32_t)f[2];s.clock=(uint32_t)f[3];
    s.server_time=(int)f[4];s.remaining_ms=(int)f[5];s.total_ms=(int)f[6];s.width=(int)f[7];s.height=(int)f[8];
    s.x=CQ_FloatFromBits((uint32_t)f[9]);s.y=CQ_FloatFromBits((uint32_t)f[10]);s.align_x=(uint32_t)f[11];s.align_y=(uint32_t)f[12];
    if(count==20){
        s.content=(uint32_t)f[13];s.alpha=(uint32_t)f[14];s.from_alpha=(uint32_t)f[15];
        if(!s.content||f[16]>INT32_MAX||f[17]>60000||!argv[25])return 0;
        s.fade_start=(int)f[16];s.fade_ms=(int)f[17];s.font_scale=CQ_FloatFromBits((uint32_t)f[18]);s.sort=CQ_FloatFromBits((uint32_t)f[19]);
        len=strlen(argv[25]);if(strcmp(argv[25],"-")){if(len>254||(len&1))return 0;
            for(i=0;i<(int)len/2;i++){char hex[3]={argv[25][i*2],argv[25][i*2+1],0};if(!CQ_ParseHex(hex,2,&byte)||!byte)return 0;s.text[i]=(char)byte;}}
    }
    if(!CH_Validate(&s))return 0;*out=s;return 1;
}
static CH_INLINE int CH_Marker(char *out,size_t capacity,uint64_t epoch){
    char text[COD1_SCRIPT_HUD_MARKER_BYTES];if(!out||capacity<sizeof(text)||!epoch||epoch==UINT64_MAX)return 0;
    memcpy(text,"h1m 1 ",6);CQ_WriteHex(text+6,16,epoch);text[22]=0;memcpy(out,text,sizeof(text));return 1;
}
static CH_INLINE int CH_ReadMarker(const char *text,uint64_t *epoch){
    size_t n;uint64_t value;char hex[17];if(!text||!epoch)return 0;
    for(n=0;n<23;n++)if(!text[n])break;if(n!=22||memcmp(text,"h1m 1 ",6))return 0;
    memcpy(hex,text+6,16);hex[16]=0;if(!CQ_ParseHex(hex,16,&value)||!value||value==UINT64_MAX)return 0;*epoch=value;return 1;
}
/* The original setup asks for ten seconds on a sixty-second dial. Zero is
 * twelve o'clock, positive remaining time is clockwise from twelve. */
static CH_INLINE float CH_Remaining(const cod1ScriptHud_t *s,int now){
    int64_t elapsed=(int64_t)now-s->server_time,left;if(elapsed<0)elapsed=0;
    left=(int64_t)s->remaining_ms-elapsed;if(left<0)left=0;return (float)left;
}
#undef CH_INLINE
#endif
