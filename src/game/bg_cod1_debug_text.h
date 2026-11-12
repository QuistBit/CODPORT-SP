/* R118 private print3d transport. This is a paired port renderer policy, not
 * an assertion about retail syscall 0x51's unknown engine sink/lifetime. */
#ifndef BG_COD1_DEBUG_TEXT_H
#define BG_COD1_DEBUG_TEXT_H
#include "bg_cod1_script_quake.h"

#define COD1_DEBUG_TEXT_COMMAND "c1t"
#define COD1_DEBUG_TEXT_ARGC 15
#define COD1_DEBUG_TEXT_COMMAND_CAPACITY 640
#define COD1_DEBUG_TEXT_CAPACITY 256
#define COD1_DEBUG_TEXT_LIFETIME_MS 100
#define COD1_DEBUG_TEXT_POOL 32
#if defined(MAX_STRING_CHARS) && COD1_DEBUG_TEXT_COMMAND_CAPACITY > MAX_STRING_CHARS
#error debug text command exceeds engine command storage
#endif
/* R117 privately reserves sixteen slots immediately after the FX table. */
#define CS_COD1_DEBUG_TEXT_METADATA (CS_COD1_SCRIPT_FX_TABLE + MAX_COD1_SCRIPT_FX + 16)
#if defined(MAX_CONFIGSTRINGS) && CS_COD1_DEBUG_TEXT_METADATA >= MAX_CONFIGSTRINGS
#error debug text metadata exceeds configstring storage
#endif
typedef struct {
    uint64_t epoch, operation_id;
    int server_time;
    float origin[3], color[4], scale;
    size_t length;
    char text[COD1_DEBUG_TEXT_CAPACITY];
} cod1DebugText_t;

#if defined(_MSC_VER)
#define CTD_INLINE __inline
#elif defined(__GNUC__)
#define CTD_INLINE __inline__
#else
#define CTD_INLINE inline
#endif
static CTD_INLINE int CTD_Validate(const cod1DebugText_t *event) {
    size_t i;
    if(!event || !event->epoch || event->epoch==UINT64_MAX ||
       !event->operation_id || event->server_time<0 ||
       event->length>=COD1_DEBUG_TEXT_CAPACITY || event->text[event->length]) return 0;
    for(i=0;i<3;++i) if(!CQ_Finite(event->origin[i])) return 0;
    for(i=0;i<4;++i) if(!CQ_Finite(event->color[i])) return 0;
    if(!CQ_Finite(event->scale)) return 0;
    for(i=0;i<event->length;++i)
        if((unsigned char)event->text[i]<32 || (unsigned char)event->text[i]>126) return 0;
    return 1;
}
static CTD_INLINE int CTD_Follows(const cod1DebugText_t *event,
    uint64_t epoch,uint64_t operation_id) {
    return CTD_Validate(event) && event->epoch==epoch && event->operation_id>operation_id;
}
static CTD_INLINE int CTD_EncodeCommand(char *out,size_t capacity,
    const cod1DebugText_t *event) {
    char encoded[COD1_DEBUG_TEXT_COMMAND_CAPACITY];
    uint32_t fields[8];size_t offset=39,i,needed;
    if(!out || !CTD_Validate(event)) return 0;
    needed=124+(event->length?event->length*2:1)+1;
    if(capacity<needed || needed>sizeof(encoded)) return 0;
    memcpy(encoded,"c1t 1 ",6);CQ_WriteHex(encoded+6,16,event->epoch);
    encoded[22]=' ';CQ_WriteHex(encoded+23,16,event->operation_id);
    encoded[offset++]=' ';CQ_WriteHex(encoded+offset,8,(uint32_t)event->server_time);offset+=8;
    for(i=0;i<3;++i) fields[i]=CQ_FloatBits(event->origin[i]);
    for(i=0;i<4;++i) fields[3+i]=CQ_FloatBits(event->color[i]);
    fields[7]=CQ_FloatBits(event->scale);
    for(i=0;i<8;++i) {encoded[offset++]=' ';CQ_WriteHex(encoded+offset,8,fields[i]);offset+=8;}
    encoded[offset++]=' ';CQ_WriteHex(encoded+offset,2,event->length);offset+=2;
    encoded[offset++]=' ';
    if(!event->length) encoded[offset++]='-';
    else for(i=0;i<event->length;++i) {CQ_WriteHex(encoded+offset,2,(unsigned char)event->text[i]);offset+=2;}
    encoded[offset]=0;memcpy(out,encoded,offset+1);return 1;
}
static CTD_INLINE int CTD_DecodeCommand(int argc,const char *const *argv,
    cod1DebugText_t *out) {
    cod1DebugText_t event;uint64_t fields[8],time,length;
    size_t i,n;int a,b;
    if(!out || !argv || argc!=COD1_DEBUG_TEXT_ARGC || !argv[0] || !argv[1] ||
       strcmp(argv[0],COD1_DEBUG_TEXT_COMMAND) || strcmp(argv[1],"1")) return 0;
    memset(&event,0,sizeof(event));
    if(!CQ_ParseHex(argv[2],16,&event.epoch) ||
       !CQ_ParseHex(argv[3],16,&event.operation_id) ||
       !CQ_ParseHex(argv[4],8,&time) || time>UINT32_C(0x7fffffff)) return 0;
    event.server_time=(int)time;
    for(i=0;i<8;++i) if(!CQ_ParseHex(argv[5+i],8,&fields[i])) return 0;
    for(i=0;i<3;++i) event.origin[i]=CQ_FloatFromBits((uint32_t)fields[i]);
    for(i=0;i<4;++i) event.color[i]=CQ_FloatFromBits((uint32_t)fields[3+i]);
    event.scale=CQ_FloatFromBits((uint32_t)fields[7]);
    if(!CQ_ParseHex(argv[13],2,&length) || !argv[14]) return 0;
    event.length=(size_t)length;
    if(!length) {if(strcmp(argv[14],"-")) return 0;}
    else {
        for(n=0;n<=length*2;++n) if(!argv[14][n]) break;
        if(n!=length*2) return 0;
        for(i=0;i<length;++i) {
            a=CQ_HexDigit(argv[14][i*2]);b=CQ_HexDigit(argv[14][i*2+1]);
            if(a<0 || b<0) return 0;
            event.text[i]=(char)((a<<4)|b);
        }
    }
    if(!CTD_Validate(&event)) return 0;
    *out=event;return 1;
}
static CTD_INLINE int CTD_EncodeMetadata(char *out,size_t capacity,uint64_t epoch) {
    char encoded[23];
    if(!out || capacity<sizeof(encoded) || !epoch || epoch==UINT64_MAX) return 0;
    memcpy(encoded,"c1d 1 ",6);CQ_WriteHex(encoded+6,16,epoch);encoded[22]=0;
    memcpy(out,encoded,sizeof(encoded));return 1;
}
static CTD_INLINE int CTD_DecodeMetadata(const char *text,uint64_t *out) {
    char hex[17];uint64_t epoch;size_t n;
    if(!text || !out) return 0;
    for(n=0;n<=22;++n) if(!text[n]) break;
    if(n!=22 || memcmp(text,"c1d 1 ",6)) return 0;
    memcpy(hex,text+6,16);hex[16]=0;
    if(!CQ_ParseHex(hex,16,&epoch) || !epoch || epoch==UINT64_MAX) return 0;
    *out=epoch;return 1;
}
#undef CTD_INLINE
#endif
