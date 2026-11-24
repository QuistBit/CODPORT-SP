#ifndef COD1_ACTOR_GENERIC_METADATA14_H
#define COD1_ACTOR_GENERIC_METADATA14_H
#include "cod1_actor_notetrack_metadata14.h"
/* Same retained 4781A0 metadata representation, now including authored
 * marks in looping clips. This adds resources, not playback or VM events.
 * The old named-clock validators remain unchanged and still reject them. */
static int C1AGM14_Valid(const c1anm14Metadata_t*m){
 unsigned i;const char*end;
 if(!m||m->magic!=C1ANM14_MAGIC||!m->count||m->count>C1ANC14_MAX_NOTES||
    m->authoredCount+1!=m->count||m->loop>1||!C1ATL14_Finite(m->frequency)||m->frequency<0||m->frequency>120||
    (!m->frequency&&(m->loop||m->authoredCount)))return 0;
 for(i=0;i<m->count;i++){
  end=memchr(m->payload[i],0,128);
  if(!end||end==m->payload[i]||m->notes[i].token!=i+1||!C1ATL14_Finite(m->notes[i].phase)||
     m->notes[i].phase<0||m->notes[i].phase>1||(i&&m->notes[i].phase<m->notes[i-1].phase))return 0;
 }
 return m->notes[m->count-1].phase==1&&!strcmp(m->payload[m->count-1],"end");
}
static int C1AGM14_BuildInner(const c1ar14Root_t*r,c1anm14Metadata_t*m){
 unsigned i;const char*end;
 if(r->magic!=C1AR14_MAGIC||!r->frames||r->frames>4095||!r->bones||r->bones>128||
    (r->flags&~3u)||!r->fps||r->fps>120||r->serializedFrames!=r->frames+(r->flags&1u)||
    r->intervals+1!=r->serializedFrames||(!r->intervals&&r->noteCount)||r->noteCount>C1AR14_NOTES||r->noteCount>=C1ANC14_MAX_NOTES)return 0;
 for(i=0;i<r->noteCount;i++){
  end=memchr(r->notes[i].name,0,128);
  if(!end||end==r->notes[i].name||r->notes[i].frame>r->intervals||(i&&r->notes[i].frame<r->notes[i-1].frame))return 0;
 }
 memset(m,0,sizeof(*m));m->count=r->noteCount+1;m->authoredCount=r->noteCount;m->loop=r->flags&1u;
 m->frequency=r->intervals?(float)((double)r->fps/r->intervals):0;
 for(i=0;i<r->noteCount;i++){
  m->notes[i].token=(uint16_t)(i+1);m->notes[i].phase=(float)((double)r->notes[i].frame/r->intervals);
  strcpy(m->payload[i],r->notes[i].name);
 }
 m->notes[i].token=(uint16_t)(i+1);m->notes[i].phase=1;strcpy(m->payload[i],"end");m->magic=C1ANM14_MAGIC;
 return C1AGM14_Valid(m);
}
static int C1AGM14_Build(const c1ar14Root_t*r,c1anm14Metadata_t*out){
 c1anm14Metadata_t value;int ok;
 if(!r||!out||C1ANC14_Overlap(r,sizeof(*r),out,sizeof(*out)))return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108];uint32_t mx,nearest=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearest
 ok=C1AGM14_BuildInner(r,&value);
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1AGM14_BuildInner(r,&value);
#endif
 if(!ok)return 0;*out=value;return 1;
}
#endif
