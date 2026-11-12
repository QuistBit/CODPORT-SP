#ifndef BG_COD1_TURRET_HEIGHT_LIMIT_H
#define BG_COD1_TURRET_HEIGHT_LIMIT_H
/* Original GAME 2006CBBD..2006CD19 endpoint and 2006C53F..2006C5C7
 * interior pitch-limit arithmetic. Tags are MODEL-local as returned by45BF0;
 * ROOT relative Z and sampled endpoint belong to the same parent-local frame.
 * No animation selection, attachment, successful movement or firing implied. */
#include <stdint.h>
#include <math.h>
#include <string.h>
typedef struct {uint32_t flags;float pitchLimit;} c1thlState_t;
static int C1THL_Overlap(const void*a,size_t n,const void*b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n&&m&&(x<=y?y-x<n:x-y<m);}
static int C1THL_Finite(float x){return x>=-4000000.0f&&x<=4000000.0f;}
static void C1THL_Normalize(float v[2]){float n=(float)((double)v[0]*v[0]+(double)v[1]*v[1]);float l=(float)sqrt((double)n);if(l!=0){float r=(float)(1.0/(double)l);v[0]=(float)((double)r*v[0]);v[1]=(float)((double)r*v[1]);}}
static int C1THL_Inner(c1thlState_t*state,int endpoint,float gunPitch,const float offset[3],const float weapon[3],const float aim[3],float relativeZ,float rowZ){
 c1thlState_t s;float dx,dy,h,a[2],b[2],dot;double exactDot,angle;int i;
 if(!state||!offset||!weapon||!aim||(endpoint!=-1&&endpoint!=1)||!C1THL_Finite(gunPitch)||!C1THL_Finite(relativeZ)||!C1THL_Finite(rowZ)||!C1THL_Finite(state->pitchLimit))return 0;
 if(C1THL_Overlap(state,sizeof(*state),offset,12)||C1THL_Overlap(state,sizeof(*state),weapon,12)||C1THL_Overlap(state,sizeof(*state),aim,12))return 0;
 for(i=0;i<3;i++)if(!C1THL_Finite(offset[i])||!C1THL_Finite(weapon[i])||!C1THL_Finite(aim[i]))return 0;
 s=*state;
 if(!(s.flags&0x20u)||offset[0]!=0||offset[1]!=0||offset[2]!=0){s.flags&=~0x200u;*state=s;return 1;}
 dx=aim[0]-weapon[0];dy=aim[1]-weapon[1];h=(float)sqrt((double)dx*dx+(double)dy*dy);
 a[0]=h;a[1]=weapon[2]-aim[2];b[0]=h;b[1]=(float)((double)relativeZ-rowZ-aim[2]);C1THL_Normalize(a);C1THL_Normalize(b);
 exactDot=(double)b[1]*a[1]+(double)b[0]*a[0];dot=(float)exactDot;
 if(exactDot<=0)angle=90;else if(dot>=1)angle=0;else angle=atan2(sqrt((1.0+(double)dot)*(1.0-(double)dot)),(double)dot)*(double)180.0f*(double)0.31830987334251404f;
 if(endpoint<0){s.flags=(s.flags&~0x400u)|0x200u;s.pitchLimit=(float)((double)gunPitch-angle);}else{s.flags|=0x600u;s.pitchLimit=(float)(angle+gunPitch);}
 if(!C1THL_Finite(s.pitchLimit))return 0;*state=s;return 1;
}
/* Native FST stores a rounded float, while subsequent FCOMP still uses
 * the unrounded PC53 result. Compare adjusted, not the stored limit. */
static int C1THL_InteriorInner(c1thlState_t*state,float minPitch,float maxPitch,const float offset[3]){
 c1thlState_t s;double adjusted;float step;uint32_t bits=0x3dcccccdu;int i;
 if(!state||!offset||!C1THL_Finite(minPitch)||!C1THL_Finite(maxPitch)||minPitch>maxPitch||
    !C1THL_Finite(state->pitchLimit)||C1THL_Overlap(state,sizeof(*state),offset,12))return 0;
 for(i=0;i<3;i++)if(!C1THL_Finite(offset[i]))return 0;
 s=*state;memcpy(&step,&bits,4);
 if(!(s.flags&0x20u)||offset[0]!=0||offset[1]!=0||offset[2]!=0)s.flags&=~0x200u;
 else if(s.flags&0x200u){
  if(s.flags&0x400u){adjusted=(double)s.pitchLimit-(double)step;s.pitchLimit=(float)adjusted;if(adjusted<=(double)minPitch)s.flags&=~0x200u;}
  else{adjusted=(double)s.pitchLimit+(double)step;s.pitchLimit=(float)adjusted;if(adjusted>=(double)maxPitch)s.flags&=~0x200u;}
 }
 if(!C1THL_Finite(s.pitchLimit))return 0;*state=s;return 1;
}
static int C1THL_Interior(c1thlState_t*state,float minPitch,float maxPitch,const float offset[3]){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=C1THL_InteriorInner(state,minPitch,maxPitch,offset);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=0;
#endif
 return ok;
}
static int C1THL_Update(c1thlState_t*state,int endpoint,float gunPitch,const float offset[3],const float weapon[3],const float aim[3],float relativeZ,float rowZ){int ok;
#if defined(_MSC_VER)&&defined(_M_IX86)
 unsigned char outer[108];uint32_t savedMx,nearestMx=0x1f80u;const unsigned short nearest=0x027f;
 __asm fnsave outer
 __asm stmxcsr savedMx
 __asm fldcw nearest
 __asm ldmxcsr nearestMx
 ok=C1THL_Inner(state,endpoint,gunPitch,offset,weapon,aim,relativeZ,rowZ);
 __asm ldmxcsr savedMx
 __asm frstor outer
#else
 ok=0;
#endif
 return ok;
}
#endif
