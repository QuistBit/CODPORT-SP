#ifndef COD1_ACTOR_LOOK_BANK14_H
#define COD1_ACTOR_LOOK_BANK14_H
#include "cod1_actor_root14.h"
#include "cod1_actor_bone14.h"
/* Actual generic_human Look source leaf names, source declaration order.
 * Pure owned decoding only. FS, map lease, currentness and tree admission are
 * caller responsibilities. No generic unknown animation or missing-note stub. */
#define C1AL14_COUNT 40
static const char*const c1al14Names[C1AL14_COUNT]={"eyes_lookdown_30","eyes_lookup_30","eyes_lookleft_30","eyes_lookright_30","stand_aim_look_left_90","stand_aim_aim_left_60","stand_alert_look_left_90","stand_alertB_look_left_90","crouch_aim_look_left_90","crouch_aim_aim_left_60","crouch_alerta_look_left_90","crouch_alertb_look_left_90","cornerA_stand_left_lookleft","casualcornerA_left_lookleft","casualcornerB_left_lookleft","casualcornerA_right_lookleft","casualcornerB_right_lookleft","casualcornercrouchA_left_lookleft","casualcornercrouchB_left_lookleft","casualcornercrouchA_right_lookleft","casualcornercrouchB_right_lookleft","sniper60_lookleft_fullbody","stand_aim_look_right_90","stand_aim_aim_right_60","stand_alert_look_right_90","stand_alertB_look_right_90","crouch_aim_look_right_90","crouch_aim_aim_right_60","crouch_alerta_look_right_90","crouch_alertb_look_right_90","cornerA_stand_left_lookright","casualcornerA_left_lookright","casualcornerB_left_lookright","casualcornerA_right_lookright","casualcornerB_right_lookright","casualcornercrouchA_left_lookright","casualcornercrouchB_left_lookright","casualcornercrouchA_right_lookright","casualcornercrouchB_right_lookright","sniper60_lookright_fullbody"};
typedef struct {c1ar14Root_t*root[C1AL14_COUNT];c1ab14Clip_t*body[C1AL14_COUNT];} c1al14Bank_t;
static int C1AL14_Index(const char*name){unsigned i;if(!name)return -1;for(i=0;i<C1AL14_COUNT;i++)if(!strcmp(name,c1al14Names[i]))return (int)i;return -1;}
static void C1AL14_Free(c1al14Bank_t*b){unsigned i;if(b)for(i=0;i<C1AL14_COUNT;i++){C1AR14_Free(b->root[i]);C1AB14_Free(b->body[i]);b->root[i]=NULL;b->body[i]=NULL;}}
static int C1AL14_Valid(unsigned i,const c1ar14Root_t*r,const c1ab14Clip_t*c){unsigned flags=(i==21||i==39)?2:0;return i<C1AL14_COUNT&&r&&c&&r->magic==C1AR14_MAGIC&&c->magic==C1AB14_MAGIC&&r->noteCount==0&&r->frames==1&&c->frames==1&&r->bones==c->bones&&r->flags==flags&&c->flags==flags&&r->fps==30&&c->fps==30&&r->intervals==0&&c->intervals==0&&c->bones>0&&c->bones<128;}
static int C1AL14_Ready(const c1al14Bank_t*b){unsigned i;if(!b)return 0;for(i=0;i<C1AL14_COUNT;i++)if(!C1AL14_Valid(i,b->root[i],b->body[i]))return 0;return 1;}
static int C1AL14_Add(c1al14Bank_t*b,unsigned i,const void*raw,size_t size){c1ar14Root_t*r=NULL;c1ab14Clip_t*c=NULL;if(!b||i>=C1AL14_COUNT||b->root[i]||b->body[i]||!raw||!size||C1AB14_Overlap(b,sizeof(*b),raw,size))return 0;if(!C1AR14_Decode(raw,size,&r)||!C1AB14_Decode(raw,size,&c)||!C1AL14_Valid(i,r,c)){C1AR14_Free(r);C1AB14_Free(c);return 0;}b->root[i]=r;b->body[i]=c;return 1;}
#endif

