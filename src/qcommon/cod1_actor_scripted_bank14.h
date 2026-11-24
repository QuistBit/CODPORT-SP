#ifndef COD1_ACTOR_SCRIPTED_BANK14_H
#define COD1_ACTOR_SCRIPTED_BANK14_H
#include "cod1_actor_face_unmounted14.h"
#include "cod1_actor_notetrack_metadata14.h"
/* Optional original Foley clips. The earlier resource domains remain usable by
 * their existing component callers. Runtime and client select the same closure. */
#ifdef COD1_TOWER_GREETING
#define COD1_BRIEFING_EXIT
#endif
#ifdef COD1_BRIEFING_EXIT
#define COD1_BRIEFING_NOWJUMP
#endif
#ifdef COD1_BRIEFING_NOWJUMP
#define COD1_BRIEFING_LETSMOVE
#endif
#ifdef COD1_BRIEFING_LETSMOVE
#define COD1_BRIEFING_GATE
#endif
#ifdef COD1_BRIEFING_GATE
#define COD1_BRIEFING_FORWARD_BACK
#endif
#ifdef COD1_BRIEFING_FORWARD_BACK
#define COD1_BRIEFING_FORWARD
#endif
#ifdef COD1_BRIEFING_FORWARD
#define COD1_BRIEFING_STRAFE
#endif
#ifdef COD1_BRIEFING_STRAFE
#define COD1_BRIEFING_OBJECTIVES
#endif
#ifdef COD1_BRIEFING_OBJECTIVES
#define COD1_BRIEFING_READSIGNS
#define COD1_BRIEFING_COMPASS_SEQUENCE
#if defined(COD1_TOWER_GREETING)
#define C1ASB14_COUNT 60
#define C1ASB14_STANDING 5
#define C1ASB14_FACIALS 25
#elif defined(COD1_BRIEFING_EXIT)
#define C1ASB14_COUNT 24
#elif defined(COD1_BRIEFING_NOWJUMP)
#define C1ASB14_COUNT 22
#elif defined(COD1_BRIEFING_LETSMOVE)
#define C1ASB14_COUNT 21
#elif defined(COD1_BRIEFING_GATE)
#define C1ASB14_COUNT 20
#elif defined(COD1_BRIEFING_FORWARD_BACK)
#define C1ASB14_COUNT 16
#elif defined(COD1_BRIEFING_FORWARD)
#define C1ASB14_COUNT 15
#elif defined(COD1_BRIEFING_STRAFE)
#define C1ASB14_COUNT 13
#else
#define C1ASB14_COUNT 9
#endif
#elif defined(COD1_BRIEFING_READSIGNS)
#define COD1_BRIEFING_COMPASS_SEQUENCE
#define C1ASB14_COUNT 7
#elif defined(COD1_BRIEFING_COMPASS_SEQUENCE)
#define C1ASB14_COUNT 6
#elif defined(COD1_BRIEFING_COMPASS_FACE)
#define C1ASB14_COUNT 3
#else
#define C1ASB14_COUNT 2
#endif
#ifndef C1ASB14_FACIALS
#define C1ASB14_FACIALS 0
#endif
#define C1ASB14_NODES (58+C1ASB14_COUNT)
#ifndef C1ASB14_STANDING
#define C1ASB14_STANDING 0
#endif
#define C1ASB14_BASE_NODES (C1ASB14_NODES-C1ASB14_FACIALS-C1ASB14_STANDING-(C1ASB14_STANDING?1:0))
#define C1ASB14_STAND_FIRST (C1ASB14_BASE_NODES+C1ASB14_FACIALS)
static int C1ASB14_Standing(unsigned n){return C1ASB14_STANDING && ((n>=107 && n<=109)||n==117||n==118||n==131);}
#define C1ASB14_LEAVES (47+C1ASB14_COUNT)
#ifdef COD1_TOWER_GREETING
/* Slot 31 carries grounded gait separately from the scripted tree. */
#define C1ASB14_PHASES 38
#else
#define C1ASB14_PHASES (7+C1ASB14_COUNT)
#endif
#if C1ASB14_PHASES > 42
#error Scripted phases exceed entityState capacity
#endif
typedef struct {c1ar14Root_t*root[C1ASB14_COUNT];c1ab14Clip_t*body[C1ASB14_COUNT];c1anm14Metadata_t metadata[C1ASB14_COUNT];} c1asb14Bank_t;
static const c1afu14Definition_t c1asb14Additional[]={
 {801,2,-1,-1}, /* body5 -> scripted801 */
 {879,56,-1,-1}, /* level_training */
 {880,57,47,7}, /* training_foley_todayobstacle */
 {1647,50,48,8} /* eyes_straight1614 -> Foley face */
#if defined(COD1_BRIEFING_COMPASS_FACE) || defined(COD1_BRIEFING_COMPASS_SEQUENCE)
 ,{1654,50,49,9} /* Foley move06, after the five training signs */
#ifdef COD1_BRIEFING_COMPASS_SEQUENCE
 ,{1655,50,50,10},{1656,50,51,11},{1657,50,52,12}
#ifdef COD1_BRIEFING_READSIGNS
 ,{881,57,53,13}
#ifdef COD1_BRIEFING_OBJECTIVES
 ,{886,57,54,14},{1653,50,55,15}
#ifdef COD1_BRIEFING_STRAFE
 , {882,57,56,16}, {1649,50,57,17}, {883,57,58,18}, {1651,50,59,19}
#ifdef COD1_BRIEFING_FORWARD
 , {885,57,60,20}, {1652,50,61,21}
#endif
#ifdef COD1_BRIEFING_FORWARD_BACK
 , {884,57,62,22}
#ifdef COD1_BRIEFING_GATE
 , {890,57,63,23}, {1658,50,64,24}, {891,57,65,25}, {1659,50,66,26}
#ifdef COD1_BRIEFING_LETSMOVE
 , {893,57,67,27}
#ifdef COD1_BRIEFING_NOWJUMP
 , {895,57,68,28}
#ifdef COD1_BRIEFING_EXIT
 , {896,57,69,29}, {897,57,70,30}
#ifdef COD1_TOWER_GREETING
 , {899,57,71,32}, {900,57,72,33}, {887,57,73,34}, {888,57,74,35}, {889,57,75,36},
 {1660,50,76,-1}, {1661,50,77,-1}, {1664,50,78,-1}, {1665,50,79,-1},
 {1666,50,80,-1}, {1667,50,81,-1}, {1668,50,82,-1}, {1669,50,83,-1},
 {1670,50,84,-1}, {1671,50,85,-1}, {1673,50,86,-1}, {1674,50,87,-1},
 {1675,50,88,-1}, {1676,50,89,-1}, {1677,50,90,-1}, {1678,50,91,-1},
 {1679,50,92,-1}, {1681,50,93,-1}, {1683,50,94,-1}, {1684,50,95,-1},
 {1685,50,96,-1}, {1687,50,97,-1}, {1688,50,98,-1}, {1689,50,99,-1},
 {1690,50,100,-1},
 {107,3,101,-1},{108,3,102,-1},{109,3,103,-1},{117,3,104,-1},{118,3,105,-1},{131,3,106,-1}
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
};
static const char*const c1asb14Names[]={"training_foley_todayobstacle","Training_facial_Foley_01_listenup"
#if defined(COD1_BRIEFING_COMPASS_FACE) || defined(COD1_BRIEFING_COMPASS_SEQUENCE)
 ,"Training_facial_Foley_08_objectivehighlighted"
#ifdef COD1_BRIEFING_COMPASS_SEQUENCE
 ,"Training_facial_Foley_09_locationobjective","Training_facial_Foley_10_approach",
 "Training_facial_Foley_11_approachobjective"
#ifdef COD1_BRIEFING_READSIGNS
 ,"training_foley_readsigns"
#ifdef COD1_BRIEFING_OBJECTIVES
 ,"training_foley_nowcheck","Training_facial_Foley_07_checkobjectives"
#ifdef COD1_BRIEFING_STRAFE
 , "training_foley_stepleft",
   "Training_facial_Foley_03_fivepacesleft",
   "training_foley_stepright",
   "Training_facial_Foley_05_fivebackwards"
#ifdef COD1_BRIEFING_FORWARD
 , "training_foley_forwards", "Training_facial_Foley_06_fiveforwards"
#endif
#ifdef COD1_BRIEFING_FORWARD_BACK
 , "training_foley_backwards"
#ifdef COD1_BRIEFING_GATE
 , "training_foley_checkedoff", "Training_facial_Foley_12_thatsit", "training_foley_opengate", "Training_facial_Foley_13_opengate"
#ifdef COD1_BRIEFING_LETSMOVE
 , "training_foley_letsmove"
#ifdef COD1_BRIEFING_NOWJUMP
 , "training_foley_nowjump"
#ifdef COD1_BRIEFING_EXIT
 , "training_foley_gothrudoor", "training_foley_ladiesstay"
#ifdef COD1_TOWER_GREETING
 , "training_moody_waving_Imuphere", "training_moody_dialogue_idle1", "training_foley_talking_loopA", "training_foley_talking_loopB", "training_foley_breathing_loop",
 "Training_facial_Moody_011",
 "Training_facial_Moody_012",
 "Training_facial_Moody_015",
 "Training_facial_Moody_016",
 "Training_facial_Moody_017",
 "Training_facial_Moody_018",
 "Training_facial_Moody_019",
 "Training_facial_Moody_020",
 "Training_facial_Moody_021_movethedefense",
 "Training_facial_Moody_022_youwillnotice",
 "Training_facial_Moody_024_inclosequarters",
 "Training_facial_Moody_025_switchweapons",
 "Training_facial_Moody_026_fire3more",
 "Training_facial_Moody_027_nowstanding",
 "Training_facial_Moody_028_pickupgrenades",
 "Training_facial_Moody_029_movebehind",
 "Training_facial_Moody_030_rollgrenade",
 "Training_facial_Moody_032_ifugethurt",
 "Training_facial_Moody_034_ourlaststation",
 "Training_facial_Moody_035a_pickemup",
 "Training_facial_Moody_035b_lotoffire",
 "Training_facial_Moody_037_lazerexplosives",
 "Training_facial_Moody_038_notestopwatch",
 "Training_facial_Moody_039_fireinthehole",
 "Training_facial_Moody_040_goodjob",
 "stand_alert_1","stand_alert_2","stand_alert_3","stand_alertb_idle1","stand_alertb_twitch1","stand2crouch_attack"
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
};
static const c1afu14Definition_t*C1ASB14_Definition(unsigned i){return i<56?c1afu14Definitions+i:i<C1ASB14_NODES?c1asb14Additional+i-56:NULL;}
static const char*C1ASB14_Name(unsigned i){return i<47?C1AFU14_Name(i):i<C1ASB14_LEAVES?c1asb14Names[i-47]:NULL;}
static int C1ASB14_Body(int index){
 return index>=0 && index<C1ASB14_COUNT && c1asb14Additional[2+index].parent==c1asb14Additional[2].parent;
}
static int C1ASB14_TowerLoop(unsigned ordinal){
 return ordinal==900 || ordinal==887 || ordinal==888 || ordinal==889;
}
static int C1ASB14_Valid(unsigned i,const c1ar14Root_t*r,const c1ab14Clip_t*c){
 static const unsigned frames[]={157,167,78,145,135,64,151,69,70,54,60,53,51,50,51,47,146,166,145,149,66,108,91,69,115,157,64,64,27,152,150,185,136,186,97,144,202,144,175,383,390,76,105,114,167,167,276,94,41,220,92,269,51,230};
 int body=C1ASB14_Body((int)i);
 unsigned ordinal,loop,bones,flags,note;
 if(i>=C1ASB14_COUNT)return 0;
 ordinal=c1asb14Additional[2+i].ordinal;
 if(ordinal==131)return r&&c&&r->magic==C1AR14_MAGIC&&c->magic==C1AB14_MAGIC&&
   r->frames==31&&c->frames==31&&r->serializedFrames==31&&c->serializedFrames==31&&
   r->intervals==30&&c->intervals==30&&r->fps==30&&c->fps==30&&r->bones==54&&c->bones==54&&
   r->flags==2&&c->flags==2&&!r->rotationKeys&&r->translationKeys==5&&r->prefixBytes==78&&!r->noteCount;
 if(C1ASB14_Standing(ordinal)){
  static const unsigned lengths[]={80,72,89,100,158},flags[]={3,3,2,3,2},notes[]={0,0,2,0,4};
  static const char *const named[]={"footscrape","footscrape","reload done","finish","reload done","finish"};
  static const unsigned positions[]={25,62,54,73,135,149};
  unsigned clip=i-54,j,start=ordinal==118?2:0;
  if(clip>=5||!r||!c||r->magic!=C1AR14_MAGIC||c->magic!=C1AB14_MAGIC||r->frames!=lengths[clip]||
     c->frames!=r->frames||r->bones!=55||c->bones!=55||r->flags!=flags[clip]||c->flags!=r->flags||
     r->fps!=30||c->fps!=30||r->serializedFrames!=r->frames+(r->flags&1)||
     c->serializedFrames!=r->serializedFrames||r->intervals+1!=r->serializedFrames||c->intervals!=r->intervals||
     r->rotationKeys||r->translationKeys||r->prefixBytes!=13||r->noteCount!=notes[clip])return 0;
  for(j=0;j<notes[clip];++j)if(strcmp(r->notes[j].name,named[start+j])||r->notes[j].frame!=positions[start+j])return 0;
  return 1;
 }
 loop=C1ASB14_TowerLoop(ordinal);
 bones=body?54:19;flags=(body?2:0)|loop;note=body?1:(ordinal>=1660 && ordinal<=1669?5:3);
 return i<C1ASB14_COUNT&&r&&c&&r->magic==C1AR14_MAGIC&&c->magic==C1AB14_MAGIC&&r->frames==frames[i]&&c->frames==frames[i]&&
 r->serializedFrames==frames[i]+loop&&c->serializedFrames==r->serializedFrames&&r->intervals+1==r->serializedFrames&&c->intervals==r->intervals&&
 r->fps==30&&c->fps==30&&r->bones==bones&&c->bones==bones&&r->flags==flags&&c->flags==flags&&
 !r->rotationKeys&&!r->translationKeys&&r->prefixBytes==(body?13u:9u)&&((loop || ordinal==899 || c1asb14Additional[2+i].ordinal==893 || (c1asb14Additional[2+i].ordinal>=895 && c1asb14Additional[2+i].ordinal<=897)) ? r->noteCount==0 :
  (r->noteCount==1&&!strcmp(r->notes[0].name,"dialogue")&&r->notes[0].frame==note));
}
static void C1ASB14_Free(c1asb14Bank_t*b){unsigned i;if(!b)return;for(i=0;i<C1ASB14_COUNT;i++){C1AR14_Free(b->root[i]);C1AB14_Free(b->body[i]);}memset(b,0,sizeof(*b));}
static int C1ASB14_Ready(const c1asb14Bank_t*b){unsigned i;if(!b)return 0;for(i=0;i<C1ASB14_COUNT;i++)if(!C1ASB14_Valid(i,b->root[i],b->body[i])||!C1ANM14_Valid(b->metadata+i))return 0;return 1;}
static int C1ASB14_Add(c1asb14Bank_t*b,unsigned i,const void*raw,size_t n){c1ar14Root_t*r=NULL;c1ab14Clip_t*c=NULL;c1anm14Metadata_t m;
 if(!b||i>=C1ASB14_COUNT||!raw||!n||C1AB14_Overlap(b,sizeof(*b),raw,n)||b->root[i]||b->body[i])return 0;
 if(!C1AR14_Decode(raw,n,&r)||!C1AB14_Decode(raw,n,&c)||!C1ASB14_Valid(i,r,c)||!C1ANM14_Build(r,&m)){C1AR14_Free(r);C1AB14_Free(c);return 0;}
 b->root[i]=r;b->body[i]=c;b->metadata[i]=m;return 1;
}
static int C1ASB14_ClipPrivate(const void*p,size_t n,const c1ar14Root_t*r,const c1ab14Clip_t*c){unsigned i;
 if(C1AB14_Overlap(p,n,r,r?sizeof(*r):0)||C1AB14_Overlap(p,n,c,c?sizeof(*c):0))return 1;
 if(c)for(i=0;i<c->bones;i++){const c1ab14Bone_t*v=c->bone+i;if(C1AB14_Overlap(p,n,v->qi,v->nq*2)||C1AB14_Overlap(p,n,v->q,v->nq*8)||C1AB14_Overlap(p,n,v->ti,v->nt*2)||C1AB14_Overlap(p,n,v->t,v->nt*12))return 1;}return 0;
}
static int C1ASB14_Bank(const c1ad14Resource_t*r,const c1afu14Bank_t*base,const c1asb14Bank_t*b,c1ast14Bank_t*out){
 c1ast14Bank_t stage;unsigned i;const c1ar14Root_t*root;const c1ab14Clip_t*clip;
 if(!r||!r->assembly||!base||!b||!out||!C1ASB14_Ready(b)||
 C1AB14_Overlap(out,sizeof(*out),r,sizeof(*r))||C1AB14_Overlap(out,sizeof(*out),r->assembly,sizeof(*r->assembly))||
 C1AB14_Overlap(out,sizeof(*out),base,sizeof(*base))||C1AB14_Overlap(out,sizeof(*out),b,sizeof(*b))||!C1AFU14_Bank(r,base,&stage))return 0;
 for(i=0;i<r->assembly->count;i++)if(C1AB14_Overlap(out,sizeof(*out),r->raw[i],r->raw[i]?sizeof(*r->raw[i]):0))return 0;
 for(i=0;i<C1ASB14_LEAVES;i++){
  root=i<2?base->base.root[i]:i<42?base->base.look.root[i-2]:i<47?base->faceRoot[i-42]:b->root[i-47];
  clip=i<47?stage.clip[i]:b->body[i-47];if(C1ASB14_ClipPrivate(out,sizeof(*out),root,clip))return 0;
 }
 stage.count=C1ASB14_LEAVES;for(i=0;i<C1ASB14_COUNT;i++){stage.clip[47+i]=b->body[i];if(!C1AD14_Map(r,stage.clip[47+i],stage.map[47+i]))return 0;}
 *out=stage;return 1;
}
/* The scripted unmounted wire has two separate 25-float transport
 * lanes for the original Training dialogue faces. Every leaf, including
 * fading siblings, retains its own weight and phase. */
static int C1ASB14_NodesFull(const float current[98],const float phases[2][21],
 const float facial[2][25],c1ast14Node_t out[C1ASB14_NODES]){
 c1ast14Node_t stage[C1ASB14_NODES];unsigned i,g;
 if(!current||!phases||phases[1][10]!=0||!out||C1AB14_Overlap(current,98*4,out,sizeof(stage))||
    C1AB14_Overlap(phases,42*4,out,sizeof(stage))||(facial && C1AB14_Overlap(facial,50*4,out,sizeof(stage)))||
    C1AB14_Overlap(c1afu14Definitions,sizeof(c1afu14Definitions),out,sizeof(stage))||
    C1AB14_Overlap(c1asb14Additional,sizeof(c1asb14Additional),out,sizeof(stage)))return 0;
 for(i=0;i<98;i++)if(!(current[i]>=0 && current[i]<=1) || (i>=C1ASB14_BASE_NODES+2*C1ASB14_STANDING+(C1ASB14_STANDING?1:0) && current[i]!=0))return 0;
 if(current[0]!=1)return 0;
 for(g=0;g<2;g++)for(i=0;i<21;i++)if(!(phases[g][i]>=0 && phases[g][i]<=1) ||
    (g*21+i>=C1ASB14_PHASES && phases[g][i]!=0))return 0;
 for(i=0;i<C1ASB14_NODES;i++){
  const c1afu14Definition_t*d=C1ASB14_Definition(i);
  stage[i].ordinal=d->ordinal;stage[i].parent=d->parent;stage[i].leaf=d->leaf;
  if(i<C1ASB14_BASE_NODES){
   stage[i].currentWeight=current[i];stage[i].phase=d->phase<0?0:phases[d->phase/21][d->phase%21];
  }else if(d->ordinal==131 && C1ASB14_STANDING){
   stage[i].currentWeight=current[97];stage[i].phase=phases[1][16];
   if(!current[97] && stage[i].phase)return 0;
  }else if(i>=C1ASB14_STAND_FIRST){
   unsigned wire=C1ASB14_BASE_NODES+i-C1ASB14_STAND_FIRST;
   stage[i].currentWeight=current[wire];stage[i].phase=current[wire+C1ASB14_STANDING];
   if(!stage[i].currentWeight && stage[i].phase)return 0;
  }else{
   unsigned slot=i-C1ASB14_BASE_NODES;
   float weight=facial?facial[0][slot]:0,phase=facial?facial[1][slot]:0;
   if(!(weight>=0 && weight<=1 && phase>=0 && phase<=1) || (!weight && phase))return 0;
   stage[i].currentWeight=weight;stage[i].phase=phase;
  }
 }
 memcpy(out,stage,sizeof(stage));return 1;
}
static int C1ASB14_Nodes(const float current[98],const float phases[2][21],c1ast14Node_t out[C1ASB14_NODES]){
 return C1ASB14_NodesFull(current,phases,NULL,out);
}
#endif
