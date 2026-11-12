#ifndef COD1_ACTOR_SCRIPT_STATE14_H
#define COD1_ACTOR_SCRIPT_STATE14_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Semantic copies of native owned WORDs +11c8/+11ca/+11cc. No native pool
 * identifiers or reference counts are fabricated. All strings in this
 * selected portable domain are NUL-terminated and shorter than128bytes.
 * Getter conversion, Actor ownership and callback lifetime belong to caller.
 * Begin/assign/check are deliberately separate: native failures retain the
 * prefix of assignments, rather than rolling the whole transition back. */
typedef struct { uint32_t present;char text[128]; } c1ast14Text_t;
typedef struct {
    c1ast14Text_t current,previous,reason;
} c1ast14State_t;
static int C1AST14_Text(const c1ast14Text_t*t){
    return t&&t->present<=1&&(!t->present||memchr(t->text,0,sizeof(t->text)));
}
static int C1AST14_State(const c1ast14State_t*s){
    return s&&C1AST14_Text(&s->current)&&C1AST14_Text(&s->previous)&&C1AST14_Text(&s->reason);
}
static int C1AST14_Begin(c1ast14State_t*s,int argc){
    if(!C1AST14_State(s))return -1;
    if(argc!=2)return 0;
    s->previous=s->current;return 1;
}
static int C1AST14_Assign(c1ast14Text_t*destination,const char*text,size_t capacity){
    c1ast14Text_t value;const char*end;
    if(!destination||!text||!capacity)return 0;
    if(capacity>sizeof(value.text))capacity=sizeof(value.text);
    end=memchr(text,0,capacity);if(!end)return 0;
    memset(&value,0,sizeof(value));value.present=1;
    memcpy(value.text,text,(size_t)(end-text));*destination=value;return 1;
}
/* -1 held/malformed,0 native repeated-state error,1 a different state.
 * This check itself does not erase the reason committed immediately before. */
static int C1AST14_Changed(const c1ast14State_t*s){
    if(!C1AST14_State(s))return -1;
    return s->current.present!=s->previous.present||
        (s->current.present&&strcmp(s->current.text,s->previous.text)!=0);
}
/* Only actual570d0 main-selection owner may call this at its native phase.
 * It retains previous history. This is not a way to suppress repeat errors. */
static int C1AST14_MainSelected(c1ast14State_t*s){
    if(!C1AST14_State(s))return 0;
    memset(&s->current,0,sizeof(s->current));
    memset(&s->reason,0,sizeof(s->reason));return 1;
}
#endif
