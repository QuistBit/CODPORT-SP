/* GAME 6D350/6D480/6D580 and 67140/516F0, nondebug finite profile.
 * Numeric state, never an overlay of the original actor allocation.
 * The caller owns the original ground query and tree transaction. */
#ifndef BG_COD1_ACTIVE_PRONE14_H
#define BG_COD1_ACTIVE_PRONE14_H
#include <stdint.h>
#include <string.h>
typedef struct {
 uint32_t nodes[3];float inverse[2],angle;int ground,disabled,start,duration;
 float height,bodyPitch,feetPitch;
} c1apr14State_t;
static int C1APR14_Active(c1apr14State_t*s,int now){
 if(!s->start||s->disabled)return 0;
 if(s->duration<0){if((int64_t)s->start-s->duration<now){s->start=0;return 0;}}
 else if(s->duration>0&&(int64_t)s->start+s->duration<now)s->duration=0;
 return 1;
}
static double C1APR14_Fraction(c1apr14State_t*s,int now){
 if(!s->start)return 0;
 if(!s->duration)return 1;
 if(s->duration<0){if((int64_t)s->start-s->duration<now){s->start=0;return 0;}return 1.0-(double)(now-s->start)/-s->duration;}
 if((int64_t)s->start+s->duration<now){s->duration=0;return 1;}
 return (double)(now-s->start)/s->duration;
}
/* Return 1 only when original EnterProne performs its initial ground query. */
static int C1APR14_Enter(c1apr14State_t*s,int now,int duration){
 double fraction;
 if(!C1APR14_Active(s,now)){s->disabled=0;s->start=now;s->duration=duration;return 1;}
 if(!s->duration||s->duration==duration)return 0;
 fraction=C1APR14_Fraction(s,now);
 if(fraction<1)s->start=now-(int)((double)duration*fraction);
 s->duration=duration;return 0;
}
static void C1APR14_Exit(c1apr14State_t*s,int now,int duration){
 if(!C1APR14_Active(s,now))return;
 if(s->duration&&s->duration!=duration)s->start=now-(int)((double)duration*C1APR14_Fraction(s,now));
 else s->start=now;
 s->duration=-duration;
}
static int C1APR14_Nodes(c1apr14State_t*s,const unsigned nodes[3],float down,float up){
 /* Valid finite nonzero input domain; partial getter/error prefixes separate. */
 if(!(down<0&&down>=-360&&up>0&&up<=360)||!nodes[0]||!nodes[1]||!nodes[2])return 0;
 s->inverse[0]=(float)(1.0/(double)down);s->inverse[1]=(float)(1.0/(double)up);
 memcpy(s->nodes,nodes,sizeof(s->nodes));return 1;
}
static int C1APR14_Weights(c1apr14State_t*s,int force,float weights[3]){
 float angle=(float)((double)s->feetPitch-s->bodyPitch),weighted;
 while(angle>180)angle=(float)((double)angle-360);
 while(angle<-180)angle=(float)((double)angle+360);
 if(force)s->angle=360;
 if(s->angle==angle)return 0;
 weights[0]=weights[2]=0;
 weighted=(float)((double)angle*s->inverse[angle<0?0:1]);
 if(weighted>.99f)weighted=.99f;
 weights[angle<0?0:2]=weighted;weights[1]=(float)(1.0-(double)weighted);
 s->angle=angle;return 1;
}
#endif
