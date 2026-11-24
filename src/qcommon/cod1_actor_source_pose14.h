#ifndef COD1_ACTOR_SOURCE_POSE14_H
#define COD1_ACTOR_SOURCE_POSE14_H
#include "cod1_actor_source_pose_profile14.h"
#include "cod1_actor_generic_tree14.h"
#include "cod1_actor_sparse_tree14.h"
/* A projection of EVERY reachable positive current child, never a gait
 * selector. All source parents are retained; unknown active leaves and
 * over-capacity trees fail atomically. The caller owns epoch/lifetime fences. */
#define C1ASP14_MAX 60u
typedef struct {unsigned count,source[C1ASP14_MAX];float weight[C1ASP14_MAX],phase[C1ASP14_MAX],controller[6];} c1asp14Packet_t;
typedef struct {unsigned count,leaves,resource[C1ASP14_MAX];c1ast14Node_t node[C1ASP14_MAX];} c1asp14Decoded_t;
static unsigned C1ASP14_Bits(float f){unsigned b;memcpy(&b,&f,4);return b;}
static int C1ASP14_Unit(float f){unsigned b=C1ASP14_Bits(f);return !(b&0x7fffffffu)||(!(b&0x80000000u)&&b<=0x3f800000u);}
static int C1ASP14_Positive(float f){return (C1ASP14_Bits(f)&0x7fffffffu)!=0;}
static int C1ASP14_ControllerValidInner(const float v[6]){
 unsigned i,bits=C1ASP14_Bits(v[0]);double norm;
 if(!bits){for(i=1;i<6;i++)if(C1ASP14_Bits(v[i]))return 0;return 1;}
 if(bits!=0x3f800000u||C1ASP14_Bits(v[1])||C1ASP14_Bits(v[3])||
    (C1ASP14_Bits(v[2])&0x7fffffffu)>0x3f800000u||(C1ASP14_Bits(v[4])&0x7fffffffu)>0x3f800000u||
    (C1ASP14_Bits(v[5])&0x7fffffffu)>0x49742400u)return 0;
 norm=(double)v[2]*v[2]+(double)v[4]*v[4];return norm>.999999&&norm<1.000001;
}
static int C1ASP14_DecodeInner(const c1asp14Packet_t*p,c1asp14Decoded_t*out){
 c1asp14Decoded_t v;unsigned i,j,previous=0;int parent;
 if(!p||!out||C1AB14_Overlap(p,sizeof(*p),out,sizeof(*out))||
    C1AB14_Overlap(out,sizeof(*out),c1asp14Definition,sizeof(c1asp14Definition))||
    !C1ASP14_ControllerValidInner(p->controller)||!p->count||p->count>C1ASP14_MAX||p->source[0]||C1ASP14_Bits(p->weight[0])!=0x3f800000u)return 0;
 memset(&v,0,sizeof(v));v.count=p->count;
 for(i=0;i<C1ASP14_MAX;i++){
  unsigned source=p->source[i];const c1asp14Definition_t*d; c1ast14Node_t*n=v.node+i;
  if(i>=p->count){if(source||C1ASP14_Bits(p->weight[i])||C1ASP14_Bits(p->phase[i]))return 0;continue;}
  if(source>=C1ASP14_CATALOG||(i&&source<=previous)||!C1ASP14_Unit(p->weight[i])||!C1ASP14_Positive(p->weight[i])||!C1ASP14_Unit(p->phase[i]))return 0;
  previous=source;d=c1asp14Definition+source;parent=-1;
  if(i){for(j=0;j<i;j++)if(p->source[j]==d->parent){parent=(int)j;break;}if(parent<0)return 0;}
  n->ordinal=source;n->parent=parent;n->leaf=-1;n->currentWeight=p->weight[i];n->phase=p->phase[i];
  if(d->children){if(C1ASP14_Bits(p->phase[i]))return 0;}
  else{if(d->leaf<0||d->leaf>=C1ASP14_RESOURCES)return 0;v.resource[v.leaves]=(unsigned)d->leaf;n->leaf=(int)v.leaves++;}
 }
 *out=v;return 1;
}
static int C1ASP14_CollectOutputsInner(const c1agt14Definition_t*d,const c1atl14Node_t*n,unsigned count,c1asp14Packet_t*out,c1asp14Decoded_t*decoded){
 c1asp14Packet_t v;c1asp14Decoded_t check;unsigned char active[C1ASP14_CATALOG];unsigned i,j;
 if(!d||!n||!out||count!=C1ASP14_CATALOG||C1AB14_Overlap(d,count*sizeof(*d),out,sizeof(*out))||
    C1AB14_Overlap(n,count*sizeof(*n),out,sizeof(*out))||C1AB14_Overlap(out,sizeof(*out),c1asp14Definition,sizeof(c1asp14Definition)))return 0;
 if(decoded&&(C1AB14_Overlap(decoded,sizeof(*decoded),out,sizeof(*out))||
    C1AB14_Overlap(decoded,sizeof(*decoded),d,count*sizeof(*d))||
    C1AB14_Overlap(decoded,sizeof(*decoded),n,count*sizeof(*n))||
    C1AB14_Overlap(decoded,sizeof(*decoded),c1asp14Definition,sizeof(c1asp14Definition))))return 0;
 memset(&v,0,sizeof(v));memset(active,0,sizeof(active));
 for(i=0;i<count;i++){
  const c1asp14Definition_t*def=c1asp14Definition+i;
  if(d[i].parent!=def->parent||d[i].children!=def->children||!C1ASP14_Unit(n[i].currentWeight)||!C1ASP14_Unit(n[i].phase))return 0;
  if((!i||active[def->parent])&&C1ASP14_Positive(n[i].currentWeight)){
   if(v.count>=C1ASP14_MAX||(!def->children&&def->leaf<0))return 0;
   active[i]=1;j=v.count++;v.source[j]=i;v.weight[j]=n[i].currentWeight;v.phase[j]=def->children?0:n[i].phase;
  }
 }
 if(!C1ASP14_DecodeInner(&v,decoded?decoded:&check))return 0;*out=v;return 1;
}
static int C1ASP14_CollectInner(const c1agt14Definition_t*d,const c1atl14Node_t*n,unsigned count,c1asp14Packet_t*out){
 return C1ASP14_CollectOutputsInner(d,n,count,out,NULL);
}
#if defined(_MSC_VER)&&defined(_M_IX86)
#define C1ASP14_BEGIN unsigned char saved[108];unsigned mx,nearMx=0x1f80;const unsigned short cw=0x027f; __asm {fnsave saved} __asm {stmxcsr mx} __asm {fldcw cw} __asm {ldmxcsr nearMx}
#define C1ASP14_END __asm {ldmxcsr mx} __asm {frstor saved}
#else
#define C1ASP14_BEGIN
#define C1ASP14_END
#endif
static int C1ASP14_ControllerValid(const float v[6]){int ok;C1ASP14_BEGIN;ok=C1ASP14_ControllerValidInner(v);C1ASP14_END;return ok;}
static int C1ASP14_Decode(const c1asp14Packet_t*p,c1asp14Decoded_t*out){int ok;C1ASP14_BEGIN;ok=C1ASP14_DecodeInner(p,out);C1ASP14_END;return ok;}
static int C1ASP14_Collect(const c1agt14Definition_t*d,const c1atl14Node_t*n,unsigned count,c1asp14Packet_t*out){int ok;C1ASP14_BEGIN;ok=C1ASP14_CollectInner(d,n,count,out);C1ASP14_END;return ok;}
static int C1ASP14_CollectDecoded(const c1agt14Definition_t*d,const c1atl14Node_t*n,unsigned count,
 c1asp14Packet_t*out,c1asp14Decoded_t*decoded){
 int ok;C1ASP14_BEGIN;ok=C1ASP14_CollectOutputsInner(d,n,count,out,decoded);C1ASP14_END;return ok;
}
#endif
