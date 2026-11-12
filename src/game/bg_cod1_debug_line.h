/* R119 paired port line transport. Retail syscall0x52 engine sink is unknown.
 * This structural codec retains raw int32 optional_integer; the real game and
 * client additionally refuse nonzero until its semantics are recovered. */
#ifndef BG_COD1_DEBUG_LINE_H
#define BG_COD1_DEBUG_LINE_H
#include "bg_cod1_debug_text.h"
#define COD1_DEBUG_LINE_COMMAND "c1l"
#define COD1_DEBUG_LINE_ARGC 16
#define COD1_DEBUG_LINE_COMMAND_LENGTH 147
#define COD1_DEBUG_LINE_COMMAND_CAPACITY 160
#define COD1_DEBUG_LINE_LIFETIME_MS 100
#define COD1_DEBUG_LINE_POOL 128
#define COD1_DEBUG_LINE_WIDTH 1.0f
#define CS_COD1_DEBUG_LINE_METADATA (CS_COD1_DEBUG_TEXT_METADATA + 1)
#if defined(MAX_STRING_CHARS) && COD1_DEBUG_LINE_COMMAND_CAPACITY > MAX_STRING_CHARS
#error debug line command exceeds engine command storage
#endif
#if defined(MAX_CONFIGSTRINGS) && CS_COD1_DEBUG_LINE_METADATA >= MAX_CONFIGSTRINGS
#error debug line metadata exceeds configstring storage
#endif
typedef struct {
    uint64_t epoch,operation_id;
    int server_time;
    float start[3],end[3],color[4];
    int32_t optional_integer;
} cod1DebugLine_t;
#if defined(_MSC_VER)
#define CDL_INLINE __inline
#elif defined(__GNUC__)
#define CDL_INLINE __inline__
#else
#define CDL_INLINE inline
#endif
static CDL_INLINE int CDL_Validate(const cod1DebugLine_t *event) {
    int i;
    if(!event || !event->epoch || event->epoch==UINT64_MAX ||
       !event->operation_id || event->server_time<0) return 0;
    for(i=0;i<3;++i) if(!CQ_Finite(event->start[i]) || !CQ_Finite(event->end[i])) return 0;
    for(i=0;i<4;++i) if(!CQ_Finite(event->color[i])) return 0;
    return 1;
}
static CDL_INLINE int CDL_Follows(const cod1DebugLine_t *event,uint64_t epoch,uint64_t operation_id) {
    return CDL_Validate(event) && event->epoch==epoch && event->operation_id>operation_id;
}
static CDL_INLINE int CDL_EncodeCommand(char *out,size_t capacity,const cod1DebugLine_t *event) {
    char encoded[COD1_DEBUG_LINE_COMMAND_LENGTH+1];uint32_t fields[12];
    size_t offset=39;int i;
    if(!out || capacity<sizeof(encoded) || !CDL_Validate(event)) return 0;
    fields[0]=(uint32_t)event->server_time;
    for(i=0;i<3;++i) {fields[1+i]=CQ_FloatBits(event->start[i]);fields[4+i]=CQ_FloatBits(event->end[i]);}
    for(i=0;i<4;++i) fields[7+i]=CQ_FloatBits(event->color[i]);
    memcpy(&fields[11],&event->optional_integer,sizeof(fields[11]));
    memcpy(encoded,"c1l 1 ",6);CQ_WriteHex(encoded+6,16,event->epoch);
    encoded[22]=' ';CQ_WriteHex(encoded+23,16,event->operation_id);
    for(i=0;i<12;++i) {encoded[offset++]=' ';CQ_WriteHex(encoded+offset,8,fields[i]);offset+=8;}
    encoded[offset]=0;memcpy(out,encoded,sizeof(encoded));return 1;
}
static CDL_INLINE int CDL_DecodeCommand(int argc,const char *const *argv,cod1DebugLine_t *out) {
    cod1DebugLine_t event;uint64_t fields[12];uint32_t raw;int i;
    if(!out || !argv || argc!=COD1_DEBUG_LINE_ARGC || !argv[0] || !argv[1] ||
       strcmp(argv[0],COD1_DEBUG_LINE_COMMAND) || strcmp(argv[1],"1")) return 0;
    memset(&event,0,sizeof(event));
    if(!CQ_ParseHex(argv[2],16,&event.epoch) || !CQ_ParseHex(argv[3],16,&event.operation_id)) return 0;
    for(i=0;i<12;++i) if(!CQ_ParseHex(argv[4+i],8,&fields[i])) return 0;
    if(fields[0]>UINT32_C(0x7fffffff)) return 0;
    event.server_time=(int)fields[0];
    for(i=0;i<3;++i) {event.start[i]=CQ_FloatFromBits((uint32_t)fields[1+i]);event.end[i]=CQ_FloatFromBits((uint32_t)fields[4+i]);}
    for(i=0;i<4;++i) event.color[i]=CQ_FloatFromBits((uint32_t)fields[7+i]);
    raw=(uint32_t)fields[11];memcpy(&event.optional_integer,&raw,sizeof(raw));
    if(!CDL_Validate(&event)) return 0;
    *out=event;return 1;
}
static CDL_INLINE int CDL_EncodeMetadata(char *out,size_t capacity,uint64_t epoch) {
    char encoded[23];
    if(!out || capacity<sizeof(encoded) || !epoch || epoch==UINT64_MAX) return 0;
    memcpy(encoded,"c1b 1 ",6);CQ_WriteHex(encoded+6,16,epoch);encoded[22]=0;
    memcpy(out,encoded,sizeof(encoded));return 1;
}
static CDL_INLINE int CDL_DecodeMetadata(const char *text,uint64_t *out) {
    char hex[17];uint64_t epoch;size_t n;
    if(!text || !out) return 0;
    for(n=0;n<=22;++n) if(!text[n]) break;
    if(n!=22 || memcmp(text,"c1b 1 ",6)) return 0;
    memcpy(hex,text+6,16);hex[16]=0;
    if(!CQ_ParseHex(hex,16,&epoch) || !epoch || epoch==UINT64_MAX) return 0;
    *out=epoch;return 1;
}
#undef CDL_INLINE
#endif
