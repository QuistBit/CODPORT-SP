#ifndef G_COD1_SCRIPT_SOUND_CORE_H
#define G_COD1_SCRIPT_SOUND_CORE_H
/* R192 semantic core of GAME PlaySound(0x20032a10), SoundIndex(0x200447d0)
 * and AddEvent(0x20046610). This is NOT a wire layout, RTCW flag translation,
 * GAME owner, VM binding, client playback or completion-notify implementation.
 * Callers own buffers/providers and must add allocation-lifetime guards before
 * connecting engine callbacks. Selected string domain: ASCII, <=127 bytes.
 * Diagnostic formatting and native string refcounts are outside this core. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define COD1_SCRIPT_SOUND_TEXT 128
#define COD1_SCRIPT_SOUND_INDEX_LAST 255

typedef struct {
    uint32_t flags, event_time;
    uint32_t entity_sequence, client_sequence;
    uint8_t entity_events[4], entity_params[4];
    uint32_t client_events[4], client_params[4];
    int has_client;
} Cod1ScriptSoundState;

typedef struct {
    void *user;
    int (*read_alias)(void *, char *, size_t);
    int (*alias_exists)(void *, const char *); /* 1=yes, 0=missing, other=HELD */
    int (*get_index)(void *, unsigned, char *, size_t);
    int (*set_index)(void *, unsigned, const char *);
    int (*read_count)(void *, uint32_t *);
    int (*read_mode)(void *, int32_t *);       /* argument 2, BEFORE argument 1 */
    int (*read_notify)(void *, uint32_t *);    /* const-string handle, argument 1 */
    int (*notify)(void *, unsigned, uint32_t, int32_t);
} Cod1ScriptSoundOps;

typedef enum {
    COD1_SS_HELD=0, COD1_SS_OK=1, COD1_SS_RECEIVER=2,
    COD1_SS_ALIAS_GETTER=3, COD1_SS_RESOURCE_MISSING=4,
    COD1_SS_INDEX_FULL=5, COD1_SS_ARITY=6, COD1_SS_MODE_GETTER=7,
    COD1_SS_NOTIFY_GETTER=8, COD1_SS_NOTIFY_DELIVERY=9
} Cod1ScriptSoundResult;

static int Cod1ScriptSoundText(const char *text) {
    size_t i;
    if(!text)return 0;
    for(i=0;i<COD1_SCRIPT_SOUND_TEXT;++i) {
        unsigned char c=(unsigned char)text[i];
        if(!c)return 1;
        if(c>=128)return 0;
    }
    return 0;
}
static int Cod1ScriptSoundEqual(const char *a,const char *b) {
    unsigned x,y;
    do {
        x=(unsigned char)*a++; y=(unsigned char)*b++;
        if(x>='A'&&x<='Z')x+='a'-'A';
        if(y>='A'&&y<='Z')y+='a'-'A';
        if(x!=y)return 0;
    } while(x);
    return 1;
}
/* Configstring offsets 0x11f+i are native evidence, not passed through to the
 * donor engine. This interface exposes only the alias-local 1..255 index. */
static Cod1ScriptSoundResult Cod1ScriptSoundIndex(const Cod1ScriptSoundOps *ops,
    const char *alias,unsigned *out) {
    unsigned i; char text[COD1_SCRIPT_SOUND_TEXT];
    if(!ops||!out||!Cod1ScriptSoundText(alias))return COD1_SS_HELD;
    if(!alias[0]){*out=0;return COD1_SS_OK;}
    if(!ops->get_index)return COD1_SS_HELD;
    for(i=1;i<=COD1_SCRIPT_SOUND_INDEX_LAST;++i) {
        memset(text,0xa5,sizeof(text));
        if(ops->get_index(ops->user,i,text,sizeof(text))!=1||!Cod1ScriptSoundText(text))return COD1_SS_HELD;
        if(!text[0]) {
            if(!ops->set_index||ops->set_index(ops->user,i,alias)!=1)return COD1_SS_HELD;
            *out=i;return COD1_SS_OK;
        }
        if(Cod1ScriptSoundEqual(text,alias)){*out=i;return COD1_SS_OK;}
    }
    return COD1_SS_INDEX_FULL;
}
static void Cod1ScriptSoundAddEvent(Cod1ScriptSoundState *s,unsigned event,
    unsigned parameter,uint32_t time_bits) {
    unsigned i;
    if(s->has_client) {
        i=s->client_sequence&3u;
        s->client_events[i]=(uint8_t)event;
        s->client_params[i]=(uint8_t)parameter;
        ++s->client_sequence; /* intentional native modulo-2^32 increment */
    } else {
        i=s->entity_sequence&3u;
        s->entity_events[i]=(uint8_t)event;
        s->entity_params[i]=(uint8_t)parameter;
        ++s->entity_sequence;
    }
    s->event_time=time_bits;
}
static Cod1ScriptSoundResult Cod1ScriptSoundInvoke(Cod1ScriptSoundState *state,
    int valid_receiver,uint32_t time_bits,const Cod1ScriptSoundOps *ops) {
    char alias[COD1_SCRIPT_SOUND_TEXT]; unsigned index;
    uint32_t argc,notify=0;int32_t mode=0;int exists;Cod1ScriptSoundResult result;
    if(!state||!ops)return COD1_SS_HELD;
    if(!valid_receiver)return COD1_SS_RECEIVER;
    memset(alias,0xa5,sizeof(alias));
    if(!ops->read_alias||ops->read_alias(ops->user,alias,sizeof(alias))!=1)return COD1_SS_ALIAS_GETTER;
    if(!Cod1ScriptSoundText(alias)||!ops->alias_exists)return COD1_SS_HELD;
    exists=ops->alias_exists(ops->user,alias);
    if(!exists)return COD1_SS_RESOURCE_MISSING;
    if(exists!=1)return COD1_SS_HELD;
    result=Cod1ScriptSoundIndex(ops,alias,&index);
    if(result!=COD1_SS_OK)return result;
    /* Original numeric masks only: native entity+0xa0 = (old & ~1) | 0x20.
     * Do NOT transplant these bits to RTCW r.svFlags without translation. */
    state->flags=(state->flags&~UINT32_C(1))|UINT32_C(0x20);
    if(!ops->read_count||ops->read_count(ops->user,&argc)!=1)return COD1_SS_HELD;
    if(argc<1||argc>3)return COD1_SS_ARITY;
    if(argc==3&&(!ops->read_mode||ops->read_mode(ops->user,&mode)!=1))return COD1_SS_MODE_GETTER;
    if(argc>=2&&(!ops->read_notify||ops->read_notify(ops->user,&notify)!=1))return COD1_SS_NOTIFY_GETTER;
    if(!index)return COD1_SS_OK;
    Cod1ScriptSoundAddEvent(state,(uint16_t)notify?0xadu:0xacu,index,time_bits);
    if((uint16_t)notify&&(!ops->notify||ops->notify(ops->user,index,notify,mode)!=1))return COD1_SS_NOTIFY_DELIVERY;
    return COD1_SS_OK;
}
#endif
