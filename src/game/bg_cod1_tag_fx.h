/* R127 paired publication envelope. The private 255-entry token dictionary
 * retains retail slot/token rules. It consumes no configstring/gamestate text.
 * Every reliable request carries its entry, including repeats and late joins.
 * Entity generation and model cookie are identities, never client pose data. */
#ifndef BG_COD1_TAG_FX_H
#define BG_COD1_TAG_FX_H
#include "bg_cod1_script_fx.h"

#define COD1_TAG_FX_COMMAND "c1t"
#define COD1_TAG_FX_ARGC 10
#define COD1_TAG_FX_SLOTS 256
#define COD1_TAG_FX_TOKEN_CAPACITY 130
#define COD1_TAG_FX_HEX_CAPACITY 259
#define COD1_TAG_FX_COMMAND_MAX_LENGTH 357
#define COD1_TAG_FX_COMMAND_CAPACITY 358

typedef struct {
    uint64_t epoch, sequence, map_epoch, generation, model_identity;
    unsigned int entity_slot, dictionary_slot;
    char token[COD1_TAG_FX_TOKEN_CAPACITY];
} cod1TagFx_t;

#if defined(_MSC_VER)
#define CT_INLINE __inline
#elif defined(__GNUC__)
#define CT_INLINE __inline__
#else
#define CT_INLINE inline
#endif

static CT_INLINE size_t CT_TokenLength(const char *text) {
    size_t n;
    int index;
    if (!text) return 0;
    for (n=0; n<COD1_TAG_FX_TOKEN_CAPACITY; ++n) if (!text[n]) break;
    if (n<2 || n>=COD1_TAG_FX_TOKEN_CAPACITY ||
        text[0]<'0' || text[0]>'6' || text[1]<'0' || text[1]>'9') return 0;
    index=(text[0]-'0')*10+text[1]-'0';
    if (index<1 || index>63) return 0;
    for (n=2; text[n]; ++n)
        if ((unsigned char)text[n]<32 || (unsigned char)text[n]>126 || text[n]=='"') return 0;
    return n;
}

/* Exact, case-sensitive lookup. An empty entry terminates the search even
 * when a later entry would match. Slot zero is never inspected or returned. */
static CT_INLINE int CT_FindSlot(const char table[COD1_TAG_FX_SLOTS][COD1_TAG_FX_TOKEN_CAPACITY],
    const char *token, int *insert) {
    int i;
    if (!table || !insert || !CT_TokenLength(token)) return 0;
    for (i=1; i<COD1_TAG_FX_SLOTS; ++i) {
        if (!table[i][0]) { *insert=1; return i; }
        if (!CT_TokenLength(table[i])) return 0;
        if (!strcmp(table[i],token)) { *insert=0; return i; }
    }
    return 0;
}
static CT_INLINE int CT_Validate(const cod1TagFx_t *event) {
    return event && event->epoch && event->epoch!=UINT64_MAX && event->sequence &&
        event->map_epoch && event->generation && event->model_identity &&
        event->model_identity!=UINT64_MAX && event->entity_slot<1024 &&
        event->dictionary_slot>0 && event->dictionary_slot<COD1_TAG_FX_SLOTS &&
        CT_TokenLength(event->token);
}
static CT_INLINE int CT_EncodeCommand(char *out, size_t capacity,
    const cod1TagFx_t *event) {
    char encoded[COD1_TAG_FX_COMMAND_CAPACITY];
    const size_t widths[7]={16,16,16,4,16,16,2};
    uint64_t fields[7];
    size_t offset=6,n,i;
    if (!out || !CT_Validate(event)) return 0;
    n=CT_TokenLength(event->token);
    if (capacity<100+2*n) return 0;
    fields[0]=event->epoch; fields[1]=event->sequence; fields[2]=event->map_epoch;
    fields[3]=event->entity_slot; fields[4]=event->generation;
    fields[5]=event->model_identity; fields[6]=event->dictionary_slot;
    memcpy(encoded,"c1t 1 ",6);
    for (i=0;i<7;++i) {
        CQ_WriteHex(encoded+offset,widths[i],fields[i]); offset+=widths[i];
        encoded[offset++]=' ';
    }
    for (i=0;i<n;++i) { CQ_WriteHex(encoded+offset,2,(unsigned char)event->token[i]); offset+=2; }
    encoded[offset]=0;
    memcpy(out,encoded,offset+1); return 1;
}
static CT_INLINE int CT_DecodeCommand(int argc, const char *const *argv,
    cod1TagFx_t *out) {
    cod1TagFx_t event;
    const size_t widths[7]={16,16,16,4,16,16,2};
    uint64_t fields[7],byte;
    size_t n,i;
    char pair[3];
    if (!out || !argv || argc!=COD1_TAG_FX_ARGC || !argv[0] || !argv[1] ||
        strcmp(argv[0],COD1_TAG_FX_COMMAND) || strcmp(argv[1],"1") || !argv[9]) return 0;
    for (i=0;i<7;++i) if (!CQ_ParseHex(argv[2+i],widths[i],&fields[i])) return 0;
    for (n=0;n<COD1_TAG_FX_HEX_CAPACITY;++n) if (!argv[9][n]) break;
    if (n<4 || n>=COD1_TAG_FX_HEX_CAPACITY || (n&1)) return 0;
    memset(&event,0,sizeof(event));
    event.epoch=fields[0]; event.sequence=fields[1]; event.map_epoch=fields[2];
    event.entity_slot=(unsigned int)fields[3]; event.generation=fields[4];
    event.model_identity=fields[5]; event.dictionary_slot=(unsigned int)fields[6];
    pair[2]=0;
    for (i=0;i<n/2;++i) {
        pair[0]=argv[9][2*i]; pair[1]=argv[9][2*i+1];
        if (!CQ_ParseHex(pair,2,&byte) || byte<32 || byte>126 || byte=='"') return 0;
        event.token[i]=(char)byte;
    }
    if (!CT_Validate(&event)) return 0;
    *out=event; return 1;
}
#undef CT_INLINE
#endif
