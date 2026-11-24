#ifndef BG_COD1_PATH_BUILD_H
#define BG_COD1_PATH_BUILD_H
#include "bg_cod1_path_follow.h"
/* GAME5FA70 on a real search-parent chain. A native refusal (unsupported
 * negotiation when allowTraverse is false) is distinct from provider failure.
 * No graph links, missing nodes, or successful arrival are synthesized. */
static int C1PBBuild(unsigned char*out,int*nativeResult,const unsigned char*input,const unsigned char*records,unsigned nodeCount,const int*parents,int endNode,int startNode,int team,const float*origin,const float*goal,int includeGoal,int allowTraverse,int now,c1mmp14Query_t query,C1PRContact contact,void*ctx){
 unsigned char p[C1PF_BYTES];struct C1PBPoint {float xyz[3];int index;} *points;
 unsigned count=0,i,j,shift=0;int next,lower,oldCount,result=0;unsigned oldFlag;float dx,dy;double length,inverse;
 if(!out||!nativeResult||!input||!records||!parents||!nodeCount||nodeCount>2048||endNode<0||startNode<0||(unsigned)endNode>=nodeCount||(unsigned)startNode>=nodeCount||team<0||team>3||
    !C1MMP14_Vec(origin)||!C1MMP14_Vec(goal)||(includeGoal!=0&&includeGoal!=1)||(allowTraverse!=0&&allowTraverse!=1)||
    (out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,origin,12)||C1PSOverlap(out,C1PF_BYTES,goal,12)||
    C1PSOverlap(out,C1PF_BYTES,records,nodeCount*116)||C1PSOverlap(out,C1PF_BYTES,parents,nodeCount*sizeof(int))||C1PSOverlap(out,C1PF_BYTES,nativeResult,sizeof(int)))return 0;
 if(!C1PFRetire(p,input,origin))return 0;
 points=calloc(nodeCount+1,sizeof(*points));if(!points)return 0;
 memcpy(points[0].xyz,includeGoal?goal:(const float*)(records+endNode*116+0x38),12);points[0].index=-1;count=1;
 if(!includeGoal||(goal[0]==C1PFFloat(records+endNode*116,0x38)&&goal[1]==C1PFFloat(records+endNode*116,0x3c))){points[0].index=endNode;next=endNode==startNode?-1:parents[endNode];}
 else next=endNode;
 while(next!=-1){
  if(next<0||(unsigned)next>=nodeCount||count>=nodeCount+1)goto done;
  memcpy(points[count].xyz,records+next*116+0x38,12);points[count].index=next;if(!C1MMP14_Vec(points[count].xyz))goto done;
  ++count;next=next==startNode?-1:parents[next];
 }
 if(points[count-1].index!=startNode)goto done;
 for(i=0;i<count&&i<32;i++){memcpy(p+i*28,points[i].xyz,12);C1PFSetWord(p,i*28+24,(unsigned)points[i].index);}
 oldFlag=C1PFWord(p,0x3b8)&0x80;oldCount=C1PFShort(p,0x380);C1PFSetWord(p,0x3b8,0);
 if(oldCount==0||points[0].xyz[0]!=C1PFFloat(p,0x38c)||points[0].xyz[1]!=C1PFFloat(p,0x390)||points[0].xyz[2]!=C1PFFloat(p,0x394))C1PFSetWord(p,0x3d4,0);
 C1PFSetShort(p,0x380,0);C1PFSetShort(p,0x382,0);C1PFSetShort(p,0x384,0);memcpy(p+0x38c,points[0].xyz,12);
 lower=(int)count-1;
 for(;lower>0;lower--){
  int a=points[lower].index,b=points[lower-1].index;
  if(a>=0&&b>=0&&C1PFWord(records+a*116,0x24)==16&&C1PFWord(records+b*116,0x24)==17&&C1PFShort(records+a*116,0x30)==C1PFShort(records+b*116,0x2c))break;
 }
 if(lower>0&&!allowTraverse){memcpy(out,p,sizeof(p));*nativeResult=0;result=1;goto done;}
 if(count>32){shift=count-32;count=32;lower-=shift;if(lower<0)lower=0;C1PFSetWord(p,0x3b8,4);
  for(i=0;i<count;i++){memcpy(p+i*28,points[i+shift].xyz,12);C1PFSetWord(p,i*28+24,(unsigned)points[i+shift].index);}
 }
 C1PFSetShort(p,0x386,lower);memcpy(p+0x3c8,p+(count-1)*28,12);
 for(j=12;j<=20;j+=4)C1PFSetFloat(p,(count-1)*28+j,0);
 for(i=0;i+1<count;i++){
  dx=(float)((double)C1PFFloat(p,i*28)-C1PFFloat(p,(i+1)*28));dy=(float)((double)C1PFFloat(p,i*28+4)-C1PFFloat(p,(i+1)*28+4));
  length=sqrt((double)dy*dy+(double)dx*dx);if(!isfinite(length)||length==0)goto done;inverse=1.0/length;
  C1PFSetFloat(p,i*28+12,(float)((double)dx*inverse));C1PFSetFloat(p,i*28+16,(float)((double)dy*inverse));C1PFSetFloat(p,i*28+20,(float)length);
 }
 C1PFSetFloat(p,0x3c4,count>1?C1PFFloat(p,(count-2)*28+20):0);C1PFSetShort(p,0x380,count);C1PFSetShort(p,0x382,count);
 if(allowTraverse)C1PFSetWord(p,0x3b8,C1PFWord(p,0x3b8)|0x10);C1PFSetWord(p,0x3c0,team);C1PFSetWord(p,0x3bc,now);
 if(C1PFCarry(p,p,origin,-1,query,contact,ctx)!=1)goto done;
 if(oldFlag)C1PFSetFloat(p,0x3b0,32768);memcpy(out,p,sizeof(p));*nativeResult=1;result=1;
done:free(points);return result;
}
#endif
