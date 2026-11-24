#ifndef BG_COD1_PATH_REACHABLE_H
#define BG_COD1_PATH_REACHABLE_H
#include "../qcommon/cod1_actor_may_move14.h"
typedef int (*C1PRContact)(void*,const float*,const float*,const float*,const float*,int,int*);
typedef struct {int reachable;float end[3];} C1PRResult;
static int C1PRContactCall(C1PRContact fn,void*ctx,const float*a,const float*b,const float*lo,const float*hi,int probe,int*hit){
 int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char saved[108],discard[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=fn(ctx,a,b,lo,hi,probe,hit);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
#else
 ok=fn(ctx,a,b,lo,hi,probe,hit);
#endif
 return ok;
}
/* GAME61B30. The providers represent actual spatial queries and actual hit
 * owners; exhaustion/provider failure cannot be published as native false. */
static int C1PRInner(const float*origin,const float*target,int pass,int probe,uint32_t mask,c1mmp14Query_t query,C1PRContact contact,void*ctx,C1PRResult*out){
 static const float mins[3]={-15,-15,18},maxs[3]={15,15,72};
 float start[3],end[3],downStart[3],downEnd[3];c1mmp14Trace_t tr;unsigned iteration;int clear;
 memcpy(start,origin,12);memcpy(end,target,12);end[2]=origin[2];
 if(probe!=2047){
  float a[3],b[3],lo[3],hi[3];int hit=-1,status;
  if(!contact)return 0;memcpy(a,start,12);memcpy(b,end,12);memcpy(lo,mins,12);memcpy(hi,maxs,12);
  status=C1PRContactCall(contact,ctx,a,b,lo,hi,probe,&hit);
  if(status!=1||(hit!=0&&hit!=1)||memcmp(a,start,12)||memcmp(b,end,12)||memcmp(lo,mins,12)||memcmp(hi,maxs,12))return 0;
  if(hit){out->reachable=0;memcpy(out->end,origin,12);return 1;}
 }
 for(iteration=0;iteration<128;iteration++){
  if(!C1MMP14_Trace(query,ctx,1,start,end,mins,maxs,pass,mask,&tr))return 0;
  clear=!tr.allsolid&&tr.fraction==1;
  if(!clear){
   float dx,dy;
   if(tr.sentient){if(!tr.sentientPass)goto blocked;mask&=~0x4000u;}
   else {dx=(float)((double)start[0]-tr.end[0]);dy=(float)((double)start[1]-tr.end[1]);if((double)dx*dx+(double)dy*dy<225)goto blocked;}
  }
  downStart[0]=tr.end[0];downStart[1]=tr.end[1];downStart[2]=start[2];memcpy(downEnd,downStart,12);downEnd[2]=(float)((double)start[2]-72);
  if(!C1MMP14_Trace(query,ctx,1,downStart,downEnd,mins,maxs,pass,mask,&tr))return 0;
  if(tr.end[2]>=start[2]&&tr.fraction!=1&&tr.normal[2]<(float).7)goto blocked;
  start[0]=tr.end[0];start[1]=tr.end[1];start[2]=(float)((double)tr.end[2]+18);
  if(clear){memcpy(out->end,start,12);out->reachable=fabs((double)start[2]-target[2])<72;return 1;}
  end[2]=start[2];
 }
 return 0;
blocked:out->reachable=0;memcpy(out->end,tr.end,12);return 1;
}
static int C1PRApply(const float*origin,const float*target,int pass,int probe,uint32_t mask,c1mmp14Query_t query,C1PRContact contact,void*ctx,C1PRResult*out){
 float savedOrigin[3],savedTarget[3];C1PRResult result;int ok;
 if(!out||!query||!C1MMP14_Vec(origin)||!C1MMP14_Vec(target)||pass<0||pass>2047||probe<0||probe>2047)return 0;
 memcpy(savedOrigin,origin,12);memcpy(savedTarget,target,12);memset(&result,0,sizeof(result));
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char outer[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave outer
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1PRInner(savedOrigin,savedTarget,pass,probe,mask,query,contact,ctx,&result);
 __asm ldmxcsr mx
 __asm frstor outer
 }
#else
 ok=C1PRInner(savedOrigin,savedTarget,pass,probe,mask,query,contact,ctx,&result);
#endif
 if(!ok||memcmp(savedOrigin,origin,12)||memcmp(savedTarget,target,12))return 0;
 *out=result;return 1;
}
#endif
