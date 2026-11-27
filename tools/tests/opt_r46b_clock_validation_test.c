/* Actual extracted R40 and current clock arithmetic and admission.
 * The fixture uses the source layouts and synthetic authored metadata/trees. */
#include <assert.h>
#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C1AGF14_MAX 4096u
#define C1AGF14_NONE UINT32_MAX
#define C1ANC14_MAX_NOTES 256u
#define C1ANM14_MAGIC 0xc1a04d14u
#define C1AGC14_MAX_EVENTS 1024u
typedef struct {uint16_t token;float phase;} c1anc14Note_t;
typedef struct {uint32_t magic;unsigned count,authoredCount,loop;float frequency;c1anc14Note_t notes[C1ANC14_MAX_NOTES];char payload[C1ANC14_MAX_NOTES][128];} c1anm14Metadata_t;
typedef struct {unsigned parent,children,flags;} c1agt14Definition_t;
typedef struct {float phase,oldPhase,remaining,goalWeight,currentWeight,rate;unsigned cycle,oldCycle;} c1atl14Node_t;
typedef struct {uint16_t descendant;int16_t cursor;uint16_t token,clientType;} c1afl14State_t;
typedef struct {unsigned count;unsigned first[C1AGF14_MAX],next[C1AGF14_MAX],order[C1AGF14_MAX];} c1agf14Topology_t;
typedef struct {unsigned node,leaf;uint16_t notify,payload;} c1agc14Event_t;
typedef struct {unsigned count,resume;float usedDt;c1agc14Event_t event[C1AGC14_MAX_EVENTS];} c1agc14Step_t;
typedef struct {const c1agt14Definition_t*d;const c1agf14Topology_t*t;const c1anm14Metadata_t*const*m;const float*frequency;c1atl14Node_t*n;c1afl14State_t*f;c1agc14Step_t*out;int owner,ok;} c1agc14Context_t;
typedef struct {float current,old,next,delta;unsigned cycle,oldCycle;} c1agc14Search_t;
static int C1ATL14_Finite(float f){uint32_t u;memcpy(&u,&f,4);return (u&0x7f800000u)!=0x7f800000u;}
#include "tree.inc"
#define C1AGT14_Valid C1AGT14_ValidScalar
#include "deps.inc"
#include "metadata.inc"
static unsigned validations;
static int counted(const c1anm14Metadata_t*m){++validations;return C1AGM14_Valid(m);}
#define C1AGM14_Valid counted
#include "kernel.inc"
#define C1AGC14_Inner ReferenceInner
#include "old_inner.inc"
#undef C1AGC14_Inner
#include "new_inner.inc"
#undef C1AGM14_Valid
static c1agt14Definition_t d[C1AGF14_MAX];static c1agf14Topology_t t;
static const c1anm14Metadata_t *m[C1AGF14_MAX];static c1anm14Metadata_t a,b;
static float frequency[C1AGF14_MAX];static c1atl14Node_t n[C1AGF14_MAX],oldn[C1AGF14_MAX],newn[C1AGF14_MAX];
static c1afl14State_t f[C1AGF14_MAX],oldf[C1AGF14_MAX],newf[C1AGF14_MAX];
static unsigned tests,successes,refusals,events,resumes;static unsigned long oldChecks,newChecks;
static uint32_t rnd(void){static uint32_t s=0x312ef19au;s^=s<<13;s^=s>>17;s^=s<<5;return s;}
static float bits(uint32_t u){float x;memcpy(&x,&u,4);return x;}
static void meta(c1anm14Metadata_t*x,unsigned count,int loop){unsigned i;memset(x,0,sizeof(*x));x->magic=C1ANM14_MAGIC;x->count=count;x->authoredCount=count-1;x->loop=(unsigned)loop;x->frequency=2;for(i=0;i<count;i++){x->notes[i].token=(uint16_t)(i+1);x->notes[i].phase=(float)(i+1)/(float)count;strcpy(x->payload[i],i+1==count?"end":"note");}}
static void setup(unsigned count,unsigned notes,int loop){unsigned i;assert(count>=5);memset(d,0,sizeof(d));memset(n,0,sizeof(n));memset(f,0,sizeof(f));memset(m,0,sizeof(m));memset(frequency,0,sizeof(frequency));memset(&t,0xff,sizeof(t));t.count=count;
 /* root -> 1 -> 2 -> 3 plus root -> 4 and dormant root children. */
 d[0].children=count-3;d[1].children=d[2].children=1;d[1].flags=loop?1:2;d[2].parent=1;d[3].parent=2;
 t.first[0]=1;t.next[1]=4;t.first[1]=2;t.first[2]=3;
 for(i=4;i+1<count;i++)t.next[i]=i+1;
 for(i=0;i<count;i++){t.order[i]=i;n[i].rate=1;f[i].cursor=-1;}
 for(i=0;i<5;i++){n[i].currentWeight=n[i].goalWeight=1;}
 meta(&a,notes,loop);meta(&b,notes,loop);m[3]=&a;m[4]=&b;frequency[3]=frequency[4]=2;
 for(i=1;i<4;i++){f[i].token=(uint16_t)(i+1);f[i].cursor=0;if(i!=3)f[i].descendant=3;}
}
static int compare(float dt,int owner,int notify,int expect){c1agc14Step_t ro,no;c1agc14Context_t r={d,&t,m,frequency,oldn,oldf,&ro,owner,0},c={d,&t,m,frequency,newn,newf,&no,owner,0};int x,y,rf,nf;unsigned rv,nv;
 memcpy(oldn,n,sizeof(n));memcpy(newn,n,sizeof(n));memcpy(oldf,f,sizeof(f));memcpy(newf,f,sizeof(f));memset(&ro,0xab,sizeof(ro));memset(&no,0xab,sizeof(no));
 feclearexcept(FE_ALL_EXCEPT);validations=0;x=ReferenceInner(&r,dt,notify);rv=validations;rf=fetestexcept(FE_ALL_EXCEPT);
 feclearexcept(FE_ALL_EXCEPT);validations=0;y=C1AGC14_Inner(&c,dt,notify);nv=validations;nf=fetestexcept(FE_ALL_EXCEPT);
 assert(x==y&&r.ok==c.ok&&rf==nf&&nv<=rv&&!memcmp(oldn,newn,sizeof(n))&&!memcmp(oldf,newf,sizeof(f))&&!memcmp(&ro,&no,sizeof(ro)));
 if(expect>=0)assert(x==expect);
 ++tests;oldChecks+=rv;newChecks+=nv;if(x){++successes;events+=ro.count;resumes+=ro.resume;}else ++refusals;
 return x;
}
int main(void){unsigned i,j;unsigned long base,after;
 setup(2078,16,0);base=oldChecks;after=newChecks;assert(compare(.1f,1,1,1));assert(oldChecks-base==5&&newChecks-after==2);
 /* Reuse is only inside an invocation; metadata changes at same address
  * must be rejected, including a resource with no active current weight. */
 a.notes[0].token=0;compare(.1f,1,1,0);a.notes[0].token=1;compare(.1f,1,1,1);
 n[3].currentWeight=n[3].goalWeight=0;a.payload[0][0]=0;compare(.1f,1,1,0);
 for(i=0;i<28;i++){setup(2078,8,0);switch(i){
 case 0:m[3]=NULL;break;case 1:a.magic=0;break;case 2:a.count=0;break;case 3:a.count=257;break;case 4:a.authoredCount=1;break;case 5:a.loop=2;break;case 6:a.frequency=bits(0x7fc00001);break;case 7:a.frequency=-1;break;case 8:a.frequency=0;break;case 9:memset(a.payload[0],'x',128);break;case 10:a.notes[0].token=2;break;case 11:a.notes[0].phase=bits(0x7fc00001);break;case 12:a.notes[0].phase=-1;break;case 13:a.notes[0].phase=2;break;case 14:a.notes[0].phase=.9f;break;case 15:a.notes[a.count-1].phase=.99f;break;case 16:strcpy(a.payload[a.count-1],"bad");break;case 17:f[2].cursor=8;break;case 18:f[3].cursor=-2;break;case 19:f[3].descendant=3;break;case 20:f[1].descendant=2078;break;case 21:f[1].descendant=4;break;case 22:frequency[2077]=121;break;case 23:n[2077].rate=101;break;case 24:d[2077].parent=2077;break;case 25:b.magic=0;break;case 26:frequency[0]=bits(0x7fc00001);break;case 27:f[2077].cursor=256;break;
 }compare(.05f,1,1,0);}
 for(i=0;i<4;i++){const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};assert(!fesetround(modes[i]));
  for(j=0;j<120;j++){unsigned count=j%3==0?5:j%3==1?2078:4096;unsigned notes=j%7==0?256:1+rnd()%24;int loop=(int)(j%2);unsigned k;float phase;
   setup(count,notes,loop);phase=(float)(rnd()%8)/8.f;
   for(k=1;k<5;k++){n[k].phase=n[k].oldPhase=phase;n[k].cycle=n[k].oldCycle=(j%11==0)?65535:0;}
   for(k=1;k<4;k++){f[k].cursor=(int16_t)C1ANC14_Cursor(a.notes,a.count,phase);if(j%5==0)f[k].token=0;}
   if(j%9==0){n[4].currentWeight=n[4].goalWeight=0;m[4]=NULL;}
   if(j%13==0){m[4]=&a;frequency[4]=frequency[3];}
   if(j%17==0){a.notes[0].phase=bits(1);a.frequency=bits(1);frequency[3]=bits(1);for(k=1;k<4;k++){n[k].phase=n[k].oldPhase=0;f[k].cursor=0;}}
   compare(j%6==0?0.f:.05f*(float)(1+j%4),j%8!=0,j%3!=0,-1);
  }
 }
 assert(!fesetround(FE_TONEAREST));setup(5,4,1);compare(-1.f,1,1,0);compare(bits(0x7fc00000),1,1,0);compare(.05f,2,1,0);compare(.05f,1,2,0);
 assert(successes&&refusals&&events&&resumes&&newChecks<oldChecks);
 printf("R46B: %u cases (%u success, %u refusal), %u events, %u resumes, metadata validations %lu -> %lu: PASS\n",tests,successes,refusals,events,resumes,oldChecks,newChecks);return 0;
}
