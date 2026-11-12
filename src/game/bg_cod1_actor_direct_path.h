#ifndef BG_COD1_ACTOR_DIRECT_PATH_H
#define BG_COD1_ACTOR_DIRECT_PATH_H
#include <string.h>
#include <math.h>
/* A bounded path record, not a claim of native path search. Length summation
 * follows original GAME63010; the concrete owner must certify its geometry. */
typedef struct {
 int count,lower,upper;
 float partial,lookahead,tail,length[32];
 int node[32];float point[32][3],direction[2],slope;
} c1dpRecord_t;
static int C1DP_Distance(const c1dpRecord_t*p,float*out){
 int i,last;float value;unsigned char saved[108];unsigned short cw=0x027f;
 if(!p||!out||p->count<0||p->count>32)return 0;
 if(!p->count){*out=0;return 1;}
 if(p->lower<0||p->upper<p->lower||p->upper>=p->count||
    !isfinite(p->partial)||p->partial<0||!isfinite(p->lookahead)||p->lookahead<0||!isfinite(p->tail)||p->tail<0)return 0;
 i=p->upper-1;last=p->lower;
 if(p->upper!=p->lower)for(;i>=last;i--)if(!isfinite(p->length[i])||p->length[i]<0)return 0;
 __asm fnsave saved
 __asm fldcw cw
 value=p->partial;
 __asm fld value
 value=p->lookahead;
 __asm fld value
 __asm faddp st(1),st
 if(p->upper!=p->lower){
  i=p->upper-1;
  if(i==p->count-2){value=p->tail;__asm fadd value
   i--;
  }
  for(;i>=last;i--){value=p->length[i];__asm fadd value}
 }
 __asm fstp value
 __asm frstor saved
 if(!isfinite(value)||value<0)return 0;*out=value;return 1;
}
/* Original E0C0/61F3B: delta is rounded to float, squared XY sum is rounded,
 * square root is rounded, inverse is rounded, then direction products round.
 * Vertical delta is a separate slope; it does not increase path distance. */
static int C1DP_Direct(const float from[3],const float to[3],c1dpRecord_t*out){
 c1dpRecord_t p;float dx,dy,dz,sum,len,inv,x,y,slope;int i;
 unsigned char saved[108];unsigned short cw=0x027f;
 if(!from||!to||!out)return 0;for(i=0;i<3;i++)if(!isfinite(from[i])||!isfinite(to[i])||fabsf(from[i])>1000000||fabsf(to[i])>1000000)return 0;
 __asm fnsave saved
 __asm fldcw cw
 x=to[0];y=from[0];__asm fld x
 __asm fsub y
 __asm fstp dx
 x=to[1];y=from[1];__asm fld x
 __asm fsub y
 __asm fstp dy
 x=to[2];y=from[2];__asm fld x
 __asm fsub y
 __asm fstp dz
 __asm fld dx
 __asm fmul dx
 __asm fld dy
 __asm fmul dy
 __asm faddp st(1),st
 __asm fstp sum
 __asm fld sum
 __asm fsqrt
 __asm fstp len
 x=dx;y=dy;slope=0;
 if(len!=0){inv=1;__asm fld inv
  __asm fdiv len
  __asm fstp inv
  __asm fld inv
  __asm fmul dx
  __asm fstp x
  __asm fld inv
  __asm fmul dy
  __asm fstp y
  __asm fld dz
  __asm fdiv len
  __asm fstp slope
 }
 __asm frstor saved
 if(!isfinite(len)||!isfinite(x)||!isfinite(y)||!isfinite(slope))return 0;
 memset(&p,0,sizeof(p));p.count=1;p.node[0]=-1;memcpy(p.point[0],to,12);
 p.lookahead=len;p.direction[0]=x;p.direction[1]=y;p.slope=slope;*out=p;return 1;
}
#endif
