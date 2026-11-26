#include <assert.h>
#include <stdio.h>
#include "cod1_anim_profile31.h"
int main(void){
 c1ap31Window w={0},out={0};unsigned i;
 assert(C1AP31_Ladder(317)==0&&C1AP31_Ladder(318)==1&&C1AP31_Ladder(319)==2&&C1AP31_Ladder(320)==4&&C1AP31_Ladder(321)==8&&C1AP31_Ladder(322)==0);
 assert(!C1AP31_Record(&w,&out,1,100,7,10,1,2,12,4,1));
 assert(!C1AP31_Record(&w,&out,1,100,7,15,1,1,8,2,2));
 assert(!C1AP31_Record(&w,&out,1,100,8,20,0,4,22,6,0));
 assert(!C1AP31_Record(&w,&out,1,150,9,30,1,1,10,3,8));
 assert(C1AP31_Record(&w,&out,1,350,10,5,1,1,2,1,4));
 assert(out.calls==4&&out.failures==1&&out.actors==3&&out.ladderActors==2);
 assert(out.sum==75&&out.maxTick==45&&out.maxCall==30&&out.iterations==8&&out.maxIterations==4);
 assert(out.maxNodes==22&&out.maxLeaves==6&&out.ladderMask==11&&out.first==100&&out.last==150);
 assert(w.calls==1&&w.actors==1&&w.ladderMask==4&&w.sum==5);
 assert(!C1AP31_Record(&w,&out,2,400,7,3,1,0,0,0,0)&&w.calls==1&&w.epoch==2);
 assert(!C1AP31_Record(&w,&out,2,399,7,3,1,0,0,0,0)&&w.calls==1&&w.first==399);
 assert(!C1AP31_Record(&w,&out,0,400,7,5,1,0,0,0,0)&&w.calls==1);
 assert(!C1AP31_Record(&w,&out,2,-1,7,5,1,0,0,0,0)&&w.calls==1);
 C1AP31_Reset(&w);for(i=0;i<C1AP31_OWNERS;i++)assert(!C1AP31_Record(&w,&out,2,0,i,1,1,1,1,1,1));
 assert(!C1AP31_Record(&w,&out,2,0,C1AP31_OWNERS,1,1,1,1,1,1));
 assert(w.actors==C1AP31_OWNERS&&w.ladderActors==C1AP31_OWNERS&&w.calls==C1AP31_OWNERS+1);
 puts("R31 aggregation: PASS");return 0;
}
