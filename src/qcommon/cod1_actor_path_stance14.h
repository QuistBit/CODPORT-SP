#ifndef COD1_ACTOR_PATH_STANCE14_H
#define COD1_ACTOR_PATH_STANCE14_H
#include <stdint.h>
#include <string.h>
/* Finite binary32 equality, including signed zero, without ambient x87/SSE
 * exception/DAZ state. No arithmetic is needed by the native path selector. */
static int C1APS14_Finite(const float*p){uint32_t b;memcpy(&b,p,4);return (b&0x7f800000u)!=0x7f800000u;}
static int C1APS14_Equal(const float*a,const float*b){uint32_t x,y;memcpy(&x,a,4);memcpy(&y,b,4);return x==y||(!(x&0x7fffffffu)&&!(y&0x7fffffffu));}
/* GAME63080 selects the last matching path node, then GAME72230 derives
 * its stance bits. The caller owns the real path, cursor and node records.
 * This component neither creates a path nor substitutes a goal for one. */
static int C1APS14_SelectNode(unsigned count,unsigned cursor,const float base[3],
 const float points[][3],const int32_t *indices,unsigned nodeCount,int32_t *out){
 unsigned i,k;int n,last;
 if(!count||count>32||cursor>=count||!base||!points||!indices||!nodeCount||!out)return 0;
 for(k=0;k<3;k++)if(!C1APS14_Finite(base+k))return 0;
 for(i=0;i<count;i++){
  if(indices[i]>=0&&(unsigned)indices[i]>=nodeCount)return 0;
  for(k=0;k<3;k++)if(!C1APS14_Finite(points[i]+k))return 0;
 }
 last=(int)count-1;n=last;
 if(!C1APS14_Equal(base,points[last])||!C1APS14_Equal(base+1,points[last]+1)||!C1APS14_Equal(base+2,points[last]+2))--n;
 while(n>=(int)cursor&&indices[n]<0)--n;
 if(n<(int)cursor)n=last;
 *out=indices[n]<0?-1:indices[n];return 1;
}
static unsigned C1APS14_NodeMask(uint16_t flags){return 7u&~((unsigned)(flags>>2)&7u);}
static int C1APS14_Allowed(unsigned actorMask,unsigned pathMask,unsigned bit,int*out){
 unsigned mask;
 if(!out||(actorMask&~7u)||(pathMask&~7u)||(bit!=1&&bit!=2&&bit!=4))return 0;
 mask=actorMask&pathMask;if(!mask)mask=actorMask;*out=(mask&bit)!=0;return 1;
}
#endif
