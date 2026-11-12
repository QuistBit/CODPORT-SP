#ifndef COD1_ACTOR_NOTETRACK_METADATA14_H
#define COD1_ACTOR_NOTETRACK_METADATA14_H
#include "cod1_actor_root14.h"
#include "cod1_actor_named_clock14.h"
/* Owned resource->clock metadata only. Original4781A0 appends implicit end at1;
 * this adapter reproduces that metadata and NEVER sends a VM notification.
 * Tokens are local opaque IDs within this immutable metadata object. Actor
 * lease/runtime/catalog/full registration generation belong to the caller;
 * magic is a format marker and does not imply scene or script readiness. */
#define C1ANM14_MAGIC 0xc1a04d14u
typedef struct {
 uint32_t magic;unsigned count,authoredCount,loop;float frequency;
 c1anc14Note_t notes[C1ANC14_MAX_NOTES];char payload[C1ANC14_MAX_NOTES][128];
} c1anm14Metadata_t;
static int C1ANM14_Valid(const c1anm14Metadata_t*m){
 unsigned i;const char*end;
 if(m->magic!=C1ANM14_MAGIC||!m->count||m->count>C1ANC14_MAX_NOTES||m->authoredCount+1!=m->count||m->loop>1||(m->loop&&m->authoredCount)||
    !C1ATL14_Finite(m->frequency)||m->frequency<=0||m->frequency>120)return 0;
 for(i=0;i<m->count;i++){
  end=memchr(m->payload[i],0,128);
  if(!end||end==m->payload[i]||m->notes[i].token!=i+1||!C1ATL14_Finite(m->notes[i].phase)||m->notes[i].phase<0||m->notes[i].phase>1||(i&&m->notes[i].phase<m->notes[i-1].phase))return 0;
 }
 return m->notes[m->count-1].phase==1&&!strcmp(m->payload[m->count-1],"end");
}
static int C1ANM14_BuildInner(const c1ar14Root_t*r,c1anm14Metadata_t*m){
 unsigned i;const char*end;
 /* Explicit bounded refusal. The original parser can represent unsorted,
  * zero-interval or out-of-interval notes; this helper does not claim that
  * general domain, nor arbitrary named looping clips. */
 if(r->magic!=C1AR14_MAGIC||!r->frames||r->frames>4095||!r->bones||r->bones>128||(r->flags&~3u)||!r->fps||r->fps>120||
    r->serializedFrames!=r->frames+(r->flags&1u)||!r->intervals||r->intervals+1!=r->serializedFrames||r->noteCount>C1AR14_NOTES||((r->flags&1u)&&r->noteCount))return 0;
 for(i=0;i<r->noteCount;i++){
  end=memchr(r->notes[i].name,0,128);
  if(!end||end==r->notes[i].name||r->notes[i].frame>r->intervals||(i&&r->notes[i].frame<r->notes[i-1].frame))return 0;
 }
 memset(m,0,sizeof(*m));m->count=r->noteCount+1;m->authoredCount=r->noteCount;m->loop=r->flags&1u;
 /* Native478202..478229: unsigned integer FILD/FIDIV, then FSTP float under
  * PC53/nearest. root.intervals is the processed WORD0 denominator. */
 m->frequency=(float)((double)r->fps/r->intervals);
 for(i=0;i<r->noteCount;i++){
  end=memchr(r->notes[i].name,0,128);m->notes[i].token=(uint16_t)(i+1);
  m->notes[i].phase=(float)((double)r->notes[i].frame/r->intervals);
  memcpy(m->payload[i],r->notes[i].name,(size_t)(end-r->notes[i].name));
 }
 m->notes[i].token=(uint16_t)(i+1);m->notes[i].phase=1;memcpy(m->payload[i],"end",4);m->magic=C1ANM14_MAGIC;
 return C1ANM14_Valid(m);
}
static int C1ANM14_Build(const c1ar14Root_t*r,c1anm14Metadata_t*out){
 c1anm14Metadata_t value;int ok;
 if(!r||!out||C1ANC14_Overlap(r,sizeof(*r),out,sizeof(*out)))return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1ANM14_BuildInner(r,&value);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1ANM14_BuildInner(r,&value);
#endif
 if(!ok)return 0;memcpy(out,&value,sizeof(value));return 1;
}
static int C1ANM14_ResolveInner(const c1anm14Metadata_t*m,unsigned token,char value[128]){
 if(!C1ANM14_Valid(m)||!token||token>m->count)return 0;
 memcpy(value,m->payload[token-1],128);return 1;
}
static int C1ANM14_Resolve(const c1anm14Metadata_t*m,unsigned token,char out[128]){
 char value[128];int ok;
 if(!m||!out||C1ANC14_Overlap(m,sizeof(*m),out,128))return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1ANM14_ResolveInner(m,token,value);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1ANM14_ResolveInner(m,token,value);
#endif
 if(!ok)return 0;memcpy(out,value,128);return 1;
}
#endif
