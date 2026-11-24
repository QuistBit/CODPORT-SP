#ifndef BG_COD1_ACTOR_PHYSICS_H
#define BG_COD1_ACTOR_PHYSICS_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#define C1AP_BYTES 264
#define C1AP_LOCAL_BYTES 92
/* Exact native 48-byte trace layout; opaque fields are retained, not inferred. */
typedef struct {float fraction,end[3],normal[3];uint32_t surface,contents,opaque;uint16_t entity,part;uint8_t extra[2],allsolid,startsolid;} C1APTrace;
typedef int(*C1APQuery)(void*,const float*,const float*,const float*,const float*,int,uint32_t,C1APTrace*);
static unsigned C1APU(const unsigned char*p,unsigned at){unsigned x;memcpy(&x,p+at,4);return x;}
static float C1APF(const unsigned char*p,unsigned at){float x;memcpy(&x,p+at,4);return x;}
static unsigned C1APS(const unsigned char*p,unsigned at){uint16_t x;memcpy(&x,p+at,2);return x;}
static void C1APSetU(unsigned char*p,unsigned at,unsigned x){memcpy(p+at,&x,4);}
static void C1APSetS(unsigned char*p,unsigned at,unsigned x){uint16_t y=(uint16_t)x;memcpy(p+at,&y,2);}
static void C1APSetF(unsigned char*p,unsigned at,float x){memcpy(p+at,&x,4);}
static int C1APVec(const float*p){unsigned i;for(i=0;i<3;i++)if(!isfinite(p[i])||fabsf(p[i])>1000000)return 0;return 1;}
static int C1APTouch(unsigned char*p,unsigned entity){unsigned i,n=C1APU(p,0x6c);if(n>32||entity>2047)return 0;if(entity==2046||n==32)return 1;for(i=0;i<n;i++)if(C1APU(p,0x70+i*4)==entity)return 1;C1APSetU(p,0x70+n*4,entity);C1APSetU(p,0x6c,n+1);return 1;}
static int C1APQueryCall(C1APQuery fn,void*ctx,const float*s,const float*e,const unsigned char*p,uint32_t mask,C1APTrace*out){
 float a[3],b[3],lo[3],hi[3];C1APTrace tr;int ok;
 memcpy(a,s,12);memcpy(b,e,12);memcpy(lo,p+0x4c,12);memcpy(hi,p+0x58,12);memset(&tr,0,sizeof(tr));
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108],discard[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=fn(ctx,a,b,lo,hi,(int)C1APU(p,0x3c),mask,&tr);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=fn(ctx,a,b,lo,hi,(int)C1APU(p,0x3c),mask,&tr);
#endif
 if(ok!=1||memcmp(a,s,12)||memcmp(b,e,12)||memcmp(lo,p+0x4c,12)||memcmp(hi,p+0x58,12)||!isfinite(tr.fraction)||tr.fraction<0||tr.fraction>1||!C1APVec(tr.end)||!C1APVec(tr.normal)||tr.entity>2047||tr.startsolid>1||tr.allsolid>1)return 0;
 *out=tr;return 1;
}
/* GAME664B0 +65890, exact ground probe and touch list. Caller pins physics
 * owner and world; local scratch is the native EA568..EA5C4 projection. */
/* GAME658D0. Walkable-plane branch deliberately rescales XY before the
 * caller restores speed; ordinary plane clipping retains the PC53 dot. */
static void C1APClip(float*out,const float*velocity,const float*normal,float overbounce){
 float x=velocity[0],y=velocity[1],z=velocity[2];double dot;
 if(normal[2]>=(float).7&&(double)z*z<=(double)x*x+(double)y*y){
  out[2]=(float)(-(double)x*normal[0]-(double)normal[1]*y);out[0]=(float)((double)x*normal[2]);out[1]=(float)((double)y*normal[2]);return;
 }
 dot=((double)normal[1]*y+(double)x*normal[0])+(double)z*normal[2];
 dot=dot<0?dot*overbounce:dot/overbounce;
 out[0]=(float)((double)x-dot*normal[0]);out[1]=(float)((double)y-dot*normal[1]);out[2]=(float)((double)z-dot*normal[2]);
}
static int C1APGroundInner(unsigned char*p,unsigned char*l,C1APQuery query,void*ctx){
 float start[3],end[3],origin[3],down;C1APTrace tr;unsigned mask=C1APU(l,0x58);
 memcpy(origin,p,12);memcpy(start,origin,12);memcpy(end,origin,12);start[2]=(float)((double)origin[2]+.25);
 down=C1APF(p,0x14)<=0&&C1APS(p,0x18)!=2047?18:.25f;end[2]=(float)((double)origin[2]-down);
 if(!C1APQueryCall(query,ctx,start,end,p,mask,&tr))return 0;memcpy(l+12,&tr,48);
 if(tr.startsolid){
  if(tr.contents&0x2000000){
   if(!C1APTouch(p,tr.entity))return 0;mask&=~0x2000000u;C1APSetU(l,0x58,mask);
   if(!C1APQueryCall(query,ctx,start,end,p,mask,&tr))return 0;memcpy(l+12,&tr,48);
  }
  if(tr.startsolid){
   if(!C1APQueryCall(query,ctx,origin,end,p,mask,&tr))return 0;memcpy(l+12,&tr,48);
   if(tr.startsolid){C1APSetS(p,0x18,tr.entity);C1APSetU(l,8,0);C1APSetU(l,4,1);return 1;}
  }
 }
 if(tr.fraction==1||(C1APF(p,0x14)>0&&(((double)tr.normal[1]*C1APF(p,0x10)+(double)tr.normal[0]*C1APF(p,0x0c))+(double)tr.normal[2]*C1APF(p,0x14)>10))){C1APSetS(p,0x18,2047);C1APSetU(l,8,0);C1APSetU(l,4,0);return 1;}
 if(tr.normal[2]<(float).7){
  C1APSetS(p,0x18,2047);C1APSetU(l,8,1);C1APSetU(l,4,0);
  if(C1APU(p,0xf0)==2047&&(tr.normal[0]!=0||tr.normal[1]!=0)){
   C1APSetU(p,0xf0,tr.entity);memcpy(p+0xf4,p,8);C1APSetF(p,0xfc,tr.normal[0]);C1APSetF(p,0x100,tr.normal[1]);p[0x104]=0;
  }
  return 1;
 }
 if(tr.end[2]<C1APF(p,8))C1APSetF(p,8,tr.end[2]);C1APSetS(p,0x18,tr.entity);C1APSetU(l,8,1);C1APSetU(l,4,1);return C1APTouch(p,tr.entity);
}
static int C1APOverlap(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(!a||!b||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;return x<=y?y-x<an:x-y<bn;}
static int C1APGround(unsigned char*out,unsigned char*localOut,const unsigned char*input,const unsigned char*local,C1APQuery query,void*ctx){
 unsigned char p[C1AP_BYTES],l[C1AP_LOCAL_BYTES],savedInput[C1AP_BYTES],savedLocal[C1AP_LOCAL_BYTES];unsigned i;int ok;float v[3];
 if(!out||!localOut||!input||!local||!query||sizeof(C1APTrace)!=48||C1APU(input,0x6c)>32||C1APU(input,0x3c)>2047)return 0;
 if(C1APOverlap(out,C1AP_BYTES,localOut,C1AP_LOCAL_BYTES)||C1APOverlap(input,C1AP_BYTES,local,C1AP_LOCAL_BYTES)||
  C1APOverlap(out,C1AP_BYTES,local,C1AP_LOCAL_BYTES)||C1APOverlap(localOut,C1AP_LOCAL_BYTES,input,C1AP_BYTES)||
  (out!=input&&C1APOverlap(out,C1AP_BYTES,input,C1AP_BYTES))||(localOut!=local&&C1APOverlap(localOut,C1AP_LOCAL_BYTES,local,C1AP_LOCAL_BYTES))||C1APS(input,0x18)>2047)return 0;
 for(i=0;i<3;i++)if(C1APF(input,0x4c+i*4)>C1APF(input,0x58+i*4))return 0;
 for(i=0;i<24;i+=12){memcpy(v,input+i,12);if(!C1APVec(v))return 0;}
 for(i=0x4c;i<=0x58;i+=12){memcpy(v,input+i,12);if(!C1APVec(v))return 0;}
 memcpy(p,input,sizeof(p));memcpy(l,local,sizeof(l));memcpy(savedInput,input,sizeof(p));memcpy(savedLocal,local,sizeof(l));
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108],discard[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=C1APGroundInner(p,l,query,ctx);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 ok=C1APGroundInner(p,l,query,ctx);
#endif
 if(memcmp(input,savedInput,sizeof(p))||memcmp(local,savedLocal,sizeof(l)))return 0;
 if(ok){memcpy(out,p,sizeof(p));memcpy(localOut,l,sizeof(l));}return ok;
}
#endif


