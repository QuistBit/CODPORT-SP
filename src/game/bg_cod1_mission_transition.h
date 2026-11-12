/* R229 bounded Training completion consumer. Native GAME2001E6B0/2001E790:
 * fade starts +250ms for750ms, deadline +1000ms, victory then +50ms before map.
 * Service emission and map/session ownership checks belong to the GAME adapter.
 * This header alone does not implement a UI, request an engine map or save VM. */
#ifndef BG_COD1_MISSION_TRANSITION_H
#define BG_COD1_MISSION_TRANSITION_H
#include <stdint.h>
#include <limits.h>
#include <string.h>
typedef struct {
 uint64_t session,epoch;
 int phase,at,last,deadline;
} Cod1MissionTransition;
enum { COD1_MISSION_NO_EFFECT=0,COD1_MISSION_FADE=1,
 COD1_MISSION_VICTORY=2,COD1_MISSION_NEXT_MAP=3 };
enum { COD1_FAILURE_PENDING=5, COD1_FAILURE_FADING, COD1_FAILURE_SCREEN, COD1_FAILURE_ENDED };
static void Cod1MissionTransitionReset(Cod1MissionTransition *s){if(s)memset(s,0,sizeof(*s));}
static int Cod1MissionTransitionBegin(Cod1MissionTransition *s,uint64_t session,
 uint64_t epoch,int now,const char *map,int persistent){
 if(!s||s->phase||!session||!epoch||now<0||now>INT_MAX-1100||persistent||
    !map||strcmp(map,"us_intro"))return 0;
 s->session=session;s->epoch=epoch;s->at=now;s->last=now;s->deadline=now+1000;s->phase=1;return 1;
}
/* Ownership loss cancels; a stale consumer cannot complete a new map request.
 * The effect is consumed once before the caller performs engine callbacks. */
static int Cod1MissionTransitionTick(Cod1MissionTransition *s,uint64_t session,
 uint64_t epoch,int now,int *effect){
 if(!s||!effect)return 0;*effect=COD1_MISSION_NO_EFFECT;
 if(!s->phase)return 1;
 if(session!=s->session||epoch!=s->epoch||now<s->last){Cod1MissionTransitionReset(s);return 0;}
 s->last=now;
 if(s->phase==1){s->phase=2;*effect=COD1_MISSION_FADE;return 1;}
 if(s->phase==2&&now>s->deadline){
  if(now>INT_MAX-50){Cod1MissionTransitionReset(s);return 0;}
  s->phase=3;s->deadline=now+50;*effect=COD1_MISSION_VICTORY;return 1;
 }
 if(s->phase==3&&now>s->deadline){s->phase=4;*effect=COD1_MISSION_NEXT_MAP;}
 return 1;
}
#endif
