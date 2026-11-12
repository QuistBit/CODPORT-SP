#ifndef BG_COD1_ACTOR_TELEPORT14_H
#define BG_COD1_ACTOR_TELEPORT14_H
/* GAME67D20 one-vector branch, GAME6AC50 and GAME5F2E0.
 * This is an unselected computation core, NOT an Actor/VM host or authority.
 * Operates only on a caller's private, nonaliased state/result buffers.
 * A future caller must capture genuine entity/Actor/sentient/phase/player/trace
 * owners and preserve their lifetime through publication. In particular, do
 * not feed guessed mode25C or use path sight's different query contract.
 * Original optional angles are not in this interface. Native getters happen
 * before this core; their errors have no committed teleport prefix.
 * Domain: finite bounded vectors/path, normal round-to-nearest float32 and
 * double arithmetic (53-bit). Native exceptional float parity remains open. */
#include <stdint.h>
#include <string.h>
#include <math.h>
typedef struct {
 float origin[3],trajectory[3],delta[3];
 uint32_t type,time,duration,flags;
 float velocity278[3],velocity298[3];
 uint32_t path[246],cell774,mode25c;
 unsigned char sentient58;
} C1TP14State;
/* Return1 only for a completed query; *blocked is exactly0 or1. A failed
 * query is an integration failure, distinct from native teleport returning0.
 * The future adapter supplies actual player and Actor exclusion identities. */
typedef int (*C1TP14Trace)(void*,const float*,const float*,int*);
static float C1TP14Read(const uint32_t*p){float v;memcpy(&v,p,4);return v;}
static void C1TP14Write(uint32_t*p,float v){memcpy(p,&v,4);}
static int C1TP14Finite(float x){return isfinite(x)&&fabsf(x)<=1000000.0f;}
static int C1TP14Vec(const float*v){unsigned i;if(!v)return 0;for(i=0;i<3;i++)if(!C1TP14Finite(v[i]))return 0;return 1;}
/* Raw DWORDs retain the existing canonical Actor path layout at+374.
 * Count/end/index are signed native WORDs at+6F4/+6F6/+6FA. */
static int C1TP14Path(uint32_t*words,const float*position){
 int count=(int16_t)words[224],end=(int16_t)(words[224]>>16),index=(int16_t)(words[225]>>16),i,j;
 float dx,dy,minDistance,acc=0,best=0,cross,addition,old,remaining;const uint32_t*row;
 if(count-1<=index)return 1; /* Actual native early return, no route reads. */
 if(count<2||count>32||end<count||end>32||index<0||!C1TP14Vec(position))return 0;
 for(i=count-2;i<end;i++)for(j=0;j<6;j++)if(!C1TP14Finite(C1TP14Read(words+i*7+j)))return 0;
 old=C1TP14Read(words+236);remaining=C1TP14Read(words+241);
 if(!C1TP14Finite(old)||!C1TP14Finite(remaining))return 0;
 row=words+(count-2)*7;dx=(float)((double)C1TP14Read(row)-position[0]);dy=(float)((double)C1TP14Read(row+1)-position[1]);
 minDistance=(float)sqrt((double)dy*dy+(double)dx*dx);
 i=count-1;
 for(;;){
  float distance;row=words+i*7;
  dx=(float)((double)C1TP14Read(row)-position[0]);dy=(float)((double)C1TP14Read(row+1)-position[1]);
  distance=(float)sqrt((double)dy*dy+(double)dx*dx);
  if(distance<minDistance){minDistance=distance;best=acc;}
  if(++i>=end)break;
  cross=fabsf((float)((double)dy*C1TP14Read(row+3)-(double)dx*C1TP14Read(row+4)));
  acc=(float)((double)cross*C1TP14Read(row+5)+acc);
  if(!(acc<32768.0f))break;
 }
 row=words+(count-2)*7;dx=(float)((double)C1TP14Read(row)-position[0]);dy=(float)((double)C1TP14Read(row+1)-position[1]);
 cross=fabsf((float)((double)dy*C1TP14Read(row+3)-(double)dx*C1TP14Read(row+4)));
 addition=(float)(((double)C1TP14Read(row+5)-remaining)*cross+best);
 C1TP14Write(words+236,(float)((double)addition+old));return 1;
}
static int C1TP14Visible(const float*eye,const float*position,float limit,C1TP14Trace trace,void*ctx,int*visible){
 static const float sample[11][3]={{.5f,.5f,.8f},{.5f,.5f,.5f},{.5f,.5f,.2f},{0,0,1},{0,1,1},{1,1,1},{1,0,1},{1,0,0},{1,1,0},{0,1,0},{0,0,0}};
 double x,y,z;float factor;uint32_t bits=0x3f2f826fu;unsigned i,j;
 /* DATA2009A300 is pinned by the original binary, not a rounded decimal. */
 memcpy(&factor,&bits,4);x=(double)eye[0]-position[0];y=(double)eye[1]-position[1];z=(double)eye[2]-position[2];
 if((z*z+y*y+x*x)*(double)factor>limit){*visible=0;return 1;}
 for(i=0;i<11;i++){
  float end[3],savedEnd[3],savedEye[3];int blocked=-1,status;
  for(j=0;j<3;j++)end[j]=(float)((j==2?72.0:30.0)*sample[i][j]+position[j]+(j==2?0.0:-15.0));
  memcpy(savedEnd,end,12);memcpy(savedEye,eye,12);status=trace(ctx,eye,end,&blocked);
  if(status!=1||memcmp(savedEye,eye,12)||memcmp(savedEnd,end,12)||(blocked!=0&&blocked!=1))return 0;
  if(!blocked){*visible=1;return 1;}
 }
 *visible=0;return 1;
}
/* Return1 means the selected computation completed; result preserves native
 * 0(refused) versus1(committed). No callback or output publication on malformed
 * inputs. All live GAME/engine commits still belong to the future owner. */
static int C1TP14Apply(C1TP14State*state,const float*goal,uint32_t phase,const float*eye,float limit,C1TP14Trace trace,void*ctx,int*result){
 C1TP14State next,before;float d[3],squared,capturedGoal[3],capturedEye[3];unsigned i;int visible;
 if(!state||!result||!C1TP14Vec(goal)||!C1TP14Vec(state->origin))return 0;
 before=next=*state;memcpy(capturedGoal,goal,12);
 if(!phase){
  for(i=0;i<3;i++)d[i]=(float)((double)state->origin[i]-goal[i]);
  squared=(float)((double)d[2]*d[2]+(double)d[1]*d[1]+(double)d[0]*d[0]);
  if(squared<=100.0f){
   if((int16_t)state->path[224]>0&&state->path[245]){*result=0;return 1;}
  }else{
   if(!trace||!C1TP14Vec(eye)||!isfinite(limit)||limit<0)return 0;
   memcpy(capturedEye,eye,12);
   if(!C1TP14Visible(eye,goal,limit,trace,ctx,&visible))return 0;
   if(memcmp(state,&before,sizeof(before))||memcmp(goal,capturedGoal,12)||memcmp(eye,capturedEye,12))return 0;
   if(visible){*result=0;return 1;}
   if(!C1TP14Visible(eye,state->origin,limit,trace,ctx,&visible))return 0;
   if(memcmp(state,&before,sizeof(before))||memcmp(goal,capturedGoal,12)||memcmp(eye,capturedEye,12))return 0;
   if(visible){*result=0;return 1;}
   if(squared>16384.0f)next.flags^=8;
  }
 }
 if(!C1TP14Path(next.path,goal))return 0;
 memcpy(next.origin,goal,12);memcpy(next.trajectory,goal,12);
 next.type=next.time=next.duration=0;memset(next.delta,0,12);
 memset(next.velocity278,0,12);memset(next.velocity298,0,12);
 next.path[224]=0;next.cell774=0;if(next.mode25c==1)next.mode25c=7;
 next.sentient58=0;*state=next;*result=1;return 1;
}
#endif
