#ifndef BG_COD1_PATH_FOLLOW_H
#define BG_COD1_PATH_FOLLOW_H
#include "bg_cod1_path_search.h"
#include "bg_cod1_path_reachable.h"
#define C1PF_BYTES 984
static int C1PFShort(const unsigned char*p,unsigned o){int16_t x;memcpy(&x,p+o,2);return x;}
static unsigned C1PFWord(const unsigned char*p,unsigned o){unsigned x;memcpy(&x,p+o,4);return x;}
static float C1PFFloat(const unsigned char*p,unsigned o){float x;memcpy(&x,p+o,4);return x;}
static void C1PFSetShort(unsigned char*p,unsigned o,int x){uint16_t v=(uint16_t)x;memcpy(p+o,&v,2);}
static void C1PFSetWord(unsigned char*p,unsigned o,unsigned x){memcpy(p+o,&x,4);}
static void C1PFSetFloat(unsigned char*p,unsigned o,float x){memcpy(p+o,&x,4);}
static int C1PFData(const unsigned char*p,const float*origin){
 unsigned i,j;int count,total,lower;
 if(!p||!origin)return 0;count=C1PFShort(p,0x380);total=C1PFShort(p,0x382);lower=C1PFShort(p,0x386);
 if(count<1||count>32||total<count||total>32||lower<0||lower>=count)return 0;
 for(i=0;i<3;i++)if(!isfinite(origin[i])||fabsf(origin[i])>1000000)return 0;
 for(i=0;i<(unsigned)total;i++)for(j=0;j<6;j++){float v=C1PFFloat(p,i*28+j*4);if(!isfinite(v)||fabsf(v)>1000000)return 0;}
 for(i=0x398;i<=0x3b4;i+=4)if(!isfinite(C1PFFloat(p,i)))return 0;
 for(i=0x3c4;i<=0x3d0;i+=4)if(!isfinite(C1PFFloat(p,i)))return 0;
 return 1;
}
/* Native62040 advances the retained starting point against the current
 * direction. This consumes an existing canonical path; it never invents one. */
static int C1PFForward(unsigned char*out,const unsigned char*input,const float*origin){
 unsigned char p[C1PF_BYTES];int count,lower,i,first=1,progress;double a,b,fraction,distance;float length,rounded;
 if(!out||!C1PFData(input,origin)||(out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,origin,12))return 0;
 memcpy(p,input,sizeof(p));count=C1PFShort(p,0x380);lower=C1PFShort(p,0x386);
 for(i=count-2;i>=lower;i--,first=0){
  unsigned off=(unsigned)i*28,base=first?0x3c8:off+28;
  a=((double)C1PFFloat(p,off+4)-origin[1])*C1PFFloat(p,0x39c)+((double)C1PFFloat(p,off)-origin[0])*C1PFFloat(p,0x398);
  if(a<=0)continue;
  length=C1PFFloat(p,first?0x3c4:off+20);
  b=((double)C1PFFloat(p,base+4)-origin[1])*C1PFFloat(p,0x39c)+((double)C1PFFloat(p,base)-origin[0])*C1PFFloat(p,0x398);
  fraction=b/(b-a);distance=(double)length*fraction;rounded=(float)distance;
  if(!isfinite(fraction)||!isfinite(distance))return 0;
  if(distance>=length){
   if(i<=lower)break;
   memcpy(p+0x3c8,p+off,12);--i;memcpy(p+0x3c4,p+i*28+20,4);
  }else{
   C1PFSetFloat(p,0x3c8,(float)((double)rounded*C1PFFloat(p,off+12)+C1PFFloat(p,base)));
   C1PFSetFloat(p,0x3cc,(float)((double)rounded*C1PFFloat(p,off+16)+C1PFFloat(p,base+4)));
   C1PFSetFloat(p,0x3d0,(float)(((double)C1PFFloat(p,off+8)-C1PFFloat(p,base+8))*fraction+C1PFFloat(p,base+8)));
   C1PFSetFloat(p,0x3c4,(float)((double)length-rounded));
  }
  progress=C1PFShort(p,0x384);
  if(progress>=0){int16_t shifted=(int16_t)(uint16_t)(progress-count+i+2);C1PFSetShort(p,0x384,shifted<0?0:shifted);}
  C1PFSetShort(p,0x380,i+2);break;
 }
 memcpy(out,p,sizeof(p));return 1;
}
static void C1PFNormalize2(float v[2]){
 float squared=(float)((double)v[1]*v[1]+(double)v[0]*v[0]);float length=(float)sqrt((double)squared);
 if(length!=0){float inverse=(float)(1.0/(double)length);v[0]=(float)((double)v[0]*inverse);v[1]=(float)((double)v[1]*inverse);}
}
/* Native62770/62700: retain direction smoothing over the same actual path.
 * Float stores and unrounded x87 intermediates are deliberately distinct. */
static int C1PFDirection(unsigned char*out,const unsigned char*input,const float*origin){
 unsigned char p[C1PF_BYTES];float direction[2],point[2],saved[2],length,budget,cross,v[2];
 int count,lower,i,first=1;double dx,dy,a,b,area,back;
 if(!out||!C1PFData(input,origin)||(out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,origin,12)||C1PFFloat(input,0x3b0)<=0)return 0;
 memcpy(p,input,sizeof(p));count=C1PFShort(p,0x380);lower=C1PFShort(p,0x386);memcpy(direction,p+0x398,8);memcpy(v,direction,8);
 if(count<=1||(C1PFWord(p,0x3b8)&1))goto done;
 length=C1PFFloat(p,0x3ac);if(length>60)length=60;
 point[0]=(float)((double)direction[0]*length+origin[0]);point[1]=(float)((double)direction[1]*length+origin[1]);
 dx=(double)C1PFFloat(p,0x3c8)-point[0];dy=(double)C1PFFloat(p,0x3cc)-point[1];saved[0]=(float)dx;saved[1]=(float)dy;
 if(dy*direction[1]+dx*direction[0]>0)goto done;
 for(i=count-2;i>=lower;i--,first=0){
  a=((double)C1PFFloat(p,i*28+4)-point[1])*direction[1]+((double)C1PFFloat(p,i*28)-point[0])*direction[0];
  if(a>0)break;
 }
 if(i<lower)goto done;
 length=C1PFFloat(p,first?0x3c4:i*28+20);budget=C1PFFloat(p,0x3b0);
 if(first)b=(double)saved[1]*direction[1]+(double)saved[0]*direction[0];
 else b=((double)C1PFFloat(p,(i+1)*28+4)-point[1])*direction[1]+((double)C1PFFloat(p,(i+1)*28)-point[0])*direction[0];
 cross=fabsf((float)(((double)C1PFFloat(p,i*28+4)-point[1])*C1PFFloat(p,i*28+12)-((double)C1PFFloat(p,i*28)-point[0])*C1PFFloat(p,i*28+16)));
 area=((double)length-(b/(b-a))*length)*cross;
 if(!isfinite(area))return 0;
 if(area>=budget)goto interpolate;
 while(i>lower){
  --i;cross=fabsf((float)(((double)C1PFFloat(p,i*28+4)-point[1])*C1PFFloat(p,i*28+12)-((double)C1PFFloat(p,i*28)-point[0])*C1PFFloat(p,i*28+16)));
  area+=(double)cross*C1PFFloat(p,i*28+20);
  if(area>=budget)goto interpolate;
 }
 v[0]=(float)((double)C1PFFloat(p,lower*28)-point[0]);v[1]=(float)((double)C1PFFloat(p,lower*28+4)-point[1]);goto normalize;
interpolate:
 if(cross==0)return 0;back=(area-budget)/cross;
 v[0]=(float)(((double)C1PFFloat(p,i*28)-back*C1PFFloat(p,i*28+12))-point[0]);
 v[1]=(float)(((double)C1PFFloat(p,i*28+4)-back*C1PFFloat(p,i*28+16))-point[1]);
normalize:C1PFNormalize2(v);if(!isfinite(v[0])||!isfinite(v[1]))return 0;
done:memcpy(p+0x3a4,v,8);memcpy(out,p,sizeof(p));return 1;
}
/* Native62230 reconsiders the retained prefix when the actor is behind the
 * current starting point. It can restore points still owned by totalCount. */
static int C1PFBackward(unsigned char*out,const unsigned char*input,const float*origin){
 unsigned char p[C1PF_BYTES];int count,total,lower,i,prev;float cross,threshold,term,storedDot;
 double area,dx,dy,dot,a,fraction,distance,remaining;unsigned thresholdBits=0x3e34b4b2;float scale;
 if(!out||!C1PFData(input,origin)||(out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,origin,12)||C1PFFloat(input,0x3b0)<=0)return 0;
 memcpy(p,input,sizeof(p));count=C1PFShort(p,0x380);total=C1PFShort(p,0x382);lower=C1PFShort(p,0x386);if(count<2||count-1==lower)return 0;
 memcpy(&scale,&thresholdBits,4);threshold=(float)((double)C1PFFloat(p,0x3b0)*scale);
 i=count-1;prev=i-1;
 cross=fabsf((float)(((double)C1PFFloat(p,prev*28+4)-origin[1])*C1PFFloat(p,prev*28+12)-((double)C1PFFloat(p,prev*28)-origin[0])*C1PFFloat(p,prev*28+16)));
 area=-(double)cross*C1PFFloat(p,0x3c4);
 for(;;){
  dx=(double)C1PFFloat(p,i*28)-origin[0];dy=(double)C1PFFloat(p,i*28+4)-origin[1];
  if(dy*dy+dx*dx>65536){C1PFSetWord(p,0x3b8,C1PFWord(p,0x3b8)|0x20);goto at_previous;}
  dot=dy*C1PFFloat(p,0x39c)+dx*C1PFFloat(p,0x398);storedDot=(float)dot;
  if(dot<=0){
   a=((double)C1PFFloat(p,prev*28+4)-origin[1])*C1PFFloat(p,0x39c)+((double)C1PFFloat(p,prev*28)-origin[0])*C1PFFloat(p,0x398);
   if(a<=0)goto at_previous;
   fraction=(double)storedDot/((double)storedDot-a);break;
  }
  {double value=(double)cross*C1PFFloat(p,prev*28+20);term=(float)value;area+=value;}
  if(area>=threshold){fraction=(area-threshold)/term;area-=term;break;}
  ++i;
  if(i>=total){
   memcpy(p+0x3c4,p+prev*28+20,4);memcpy(p+0x3c8,p+(i-1)*28,12);C1PFSetShort(p,0x380,total);goto budget;
  }
  prev=i-1;
  cross=fabsf((float)(dy*C1PFFloat(p,prev*28+12)-dx*C1PFFloat(p,prev*28+16)));
 }
 distance=fraction*C1PFFloat(p,prev*28+20);if(!isfinite(distance)||!isfinite(fraction))return 0;
 if(distance>=C1PFFloat(p,prev*28+20)){
  --i;if(i<=lower)goto done;
  memcpy(p+0x3c8,p+prev*28,12);memcpy(p+0x3c4,p+(i-1)*28+20,4);++i;
 }else{
  remaining=(double)C1PFFloat(p,prev*28+20)-distance;C1PFSetFloat(p,0x3c4,(float)remaining);area+=remaining*cross;
  C1PFSetFloat(p,0x3c8,(float)(distance*C1PFFloat(p,prev*28+12)+C1PFFloat(p,i*28)));
  C1PFSetFloat(p,0x3cc,(float)(distance*C1PFFloat(p,prev*28+16)+C1PFFloat(p,i*28+4)));
  C1PFSetFloat(p,0x3d0,(float)(((double)C1PFFloat(p,prev*28+8)-C1PFFloat(p,i*28+8))*fraction+C1PFFloat(p,i*28+8)));++i;
 }
 C1PFSetShort(p,0x380,i);goto budget;
at_previous:
 memcpy(p+0x3c8,p+prev*28,12);C1PFSetFloat(p,0x3c4,i>lower+1?C1PFFloat(p,(i-2)*28+20):0);
 area+=(double)cross*C1PFFloat(p,prev*28+20);C1PFSetShort(p,0x380,i);
budget:
 if(area>0){double value;if(!(C1PFWord(p,0x3b8)&2))area*=.75;value=area+C1PFFloat(p,0x3b0);C1PFSetFloat(p,0x3b0,value>65536?65536:(float)value);}
done:memcpy(out,p,sizeof(p));return 1;
}
/* Native5F470 retires the turn-area contribution of consumed path points. */
static int C1PFBudget(unsigned char*out,const unsigned char*input,const float*origin){
 unsigned char p[C1PF_BYTES];int count,total,i;float cross;double area,value;
 if(!out||!C1PFData(input,origin)||(out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,origin,12))return 0;
 memcpy(p,input,sizeof(p));count=C1PFShort(p,0x380);total=C1PFShort(p,0x382);
 if(count!=total){
  if(count<2)return 0;i=count-2;
  cross=fabsf((float)(((double)C1PFFloat(p,i*28+4)-origin[1])*C1PFFloat(p,i*28+12)-((double)C1PFFloat(p,i*28)-origin[0])*C1PFFloat(p,i*28+16)));
  area=((double)C1PFFloat(p,i*28+20)-C1PFFloat(p,0x3c4))*cross;
  for(i=count-1;i<total-1;i++){
   cross=fabsf((float)(((double)C1PFFloat(p,i*28+4)-origin[1])*C1PFFloat(p,i*28+12)-((double)C1PFFloat(p,i*28)-origin[0])*C1PFFloat(p,i*28+16)));
   area+=(double)cross*C1PFFloat(p,i*28+20);
  }
  value=(double)C1PFFloat(p,0x3b0)-area;C1PFSetFloat(p,0x3b0,value<64?64:(float)value);
 }
 memcpy(out,p,sizeof(p));return 1;
}
/* Native61A60/61AE0 adjust the next turn-area budget after reachability. */
static int C1PFFeedback(unsigned char*out,const unsigned char*input,int clear){
 unsigned char p[C1PF_BYTES];unsigned flags;float budget,scale;unsigned bits=0x3f969696;double value;
 if(!out||!input||(clear!=0&&clear!=1)||(out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES)))return 0;
 budget=C1PFFloat(input,0x3b0);if(!isfinite(budget)||budget<0||budget>1000000)return 0;
 memcpy(p,input,sizeof(p));flags=C1PFWord(p,0x3b8);
 if(clear){
  if(flags&2){memcpy(&scale,&bits,4);value=(double)budget*scale+(float)6.4;budget=value>65536?65536:(float)value;}
  flags|=2;
 }else{
  value=(flags&2)?((double)budget-(float)6.4)*.75:((double)budget-(float)12.8)*.5625;
  budget=(float)value;if(budget<(float).001)budget=(float).001;flags&=~3u;
 }
 C1PFSetFloat(p,0x3b0,budget);C1PFSetWord(p,0x3b8,flags);memcpy(out,p,sizeof(p));return 1;
}
/* Native61E30. passedIsActor is the fenced live classification of path38A,
 * required only when progress is nonzero and38A is not ENTITYNUM_NONE. */
static int C1PFTarget(unsigned char*out,const unsigned char*input,const float*origin,const float*target,int skipTrace,float partial,int newUpper,int passedIsActor,c1mmp14Query_t query,C1PRContact contact,void*ctx){
 unsigned char p[C1PF_BYTES];float at[3],goal[3],v[2],length,slope;int count,upper,probe=2047;unsigned mask=0x820011;C1PRResult reachable;
 if(!out||!C1PFData(input,origin)||!C1MMP14_Vec(target)||(skipTrace!=0&&skipTrace!=1)||!isfinite(partial)||partial<0||partial>1000000||
    (out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,origin,12)||C1PSOverlap(out,C1PF_BYTES,target,12))return 0;
 memcpy(p,input,sizeof(p));memcpy(at,origin,12);memcpy(goal,target,12);count=C1PFShort(p,0x380);upper=C1PFShort(p,0x388);
 if(upper<0||newUpper<0||newUpper>=count)return 0;
 if(upper>=count-1){upper=count-1;C1PFSetFloat(p,0x3b4,0);C1PFSetShort(p,0x388,upper);}
 if(skipTrace){if(!C1PFFeedback(p,p,0))return 0;}
 else{
  if(C1PFShort(p,0x384)!=0&&C1PFShort(p,0x38a)!=2047){
   int pass=C1PFShort(p,0x38a);if(pass<0||pass>2047||(passedIsActor!=0&&passedIsActor!=1))return 0;
   if(passedIsActor)probe=pass;else mask=0x2820011;
  }
  if(!C1PRApply(at,goal,2047,probe,mask,query,contact,ctx,&reachable))return 0;
  if(reachable.reachable){if(!C1PFFeedback(p,p,1))return 0;}
  else{
   float segment=C1PFFloat(p,upper==count-2?0x3c4:upper*28+20);int retain=(C1PFWord(p,0x3b8)&2)&&upper<count&&segment>=C1PFFloat(p,0x3b4);
   if(!C1PFFeedback(p,p,0))return 0;if(retain)goto done;
  }
 }
 v[0]=(float)((double)goal[0]-at[0]);v[1]=(float)((double)goal[1]-at[1]);slope=(float)((double)goal[2]-at[2]);
 length=(float)sqrt((double)(float)((double)v[0]*v[0]+(double)v[1]*v[1]));C1PFNormalize2(v);
 memcpy(p+0x398,v,8);C1PFSetFloat(p,0x3a0,length!=0?(float)((double)slope/length):0);C1PFSetFloat(p,0x3ac,length);C1PFSetFloat(p,0x3b4,partial);C1PFSetShort(p,0x388,newUpper);
done:
 if(memcmp(at,origin,12)||memcmp(goal,target,12))return 0;memcpy(out,p,sizeof(p));return 1;
}
/* Native62520 chooses the lookahead point from the turn-area budget. */
static int C1PFLookahead(unsigned char*out,const unsigned char*input,const float*origin,int skipTrace,int passedIsActor,c1mmp14Query_t query,C1PRContact contact,void*ctx){
 unsigned char p[C1PF_BYTES];int count,lower,i,first=1;float area=0,budget,cross,length=0,partial,goal[3];double sum,back;
 if(!out||!C1PFData(input,origin)||(out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,origin,12))return 0;
 memcpy(p,input,sizeof(p));count=C1PFShort(p,0x380);lower=C1PFShort(p,0x386);budget=C1PFFloat(p,0x3b0);if(budget<=0)return 0;
 for(i=count-2;i>=lower;i--,first=0){
  cross=fabsf((float)(((double)C1PFFloat(p,i*28+4)-origin[1])*C1PFFloat(p,i*28+12)-((double)C1PFFloat(p,i*28)-origin[0])*C1PFFloat(p,i*28+16)));
  length=C1PFFloat(p,first?0x3c4:i*28+20);sum=(double)cross*length+area;area=(float)sum;
  if(sum>=budget){
   if(cross==0||length==0)return 0;back=((double)area-budget)/cross;partial=(float)back;if(back>length)partial=length;
   goal[0]=(float)((double)C1PFFloat(p,i*28)-(double)partial*C1PFFloat(p,i*28+12));
   goal[1]=(float)((double)C1PFFloat(p,i*28+4)-(double)partial*C1PFFloat(p,i*28+16));
   goal[2]=(float)((double)C1PFFloat(p,i*28+8)-(((double)C1PFFloat(p,i*28+8)-C1PFFloat(p,first?0x3d0:(i+1)*28+8))/length)*partial);
   C1PFSetWord(p,0x3b8,C1PFWord(p,0x3b8)&~0x41u);
   if(!C1PFTarget(p,p,origin,goal,skipTrace,partial,i,passedIsActor,query,contact,ctx))return 0;
   memcpy(out,p,sizeof(p));return 1;
  }
 }
 C1PFSetWord(p,0x3b8,C1PFWord(p,0x3b8)|0x41);memcpy(goal,p+(count-1==lower?0x3c8:lower*28),12);
 if(!C1PFTarget(p,p,origin,goal,skipTrace,0,lower,passedIsActor,query,contact,ctx))return 0;
 budget=C1PFFloat(p,0x3b0);if(area>32768){if(budget>area)C1PFSetFloat(p,0x3b0,area);}else if(budget>32768)C1PFSetFloat(p,0x3b0,32768);
 memcpy(out,p,sizeof(p));return 1;
}
/* Native629C0 retained-path update. Return2 requests native5FE80 repathing;
 * no output is published in that case. The owner must execute the real graph
 * operation before committing movement;2 is never success or arrival. */
static int C1PFUpdate(unsigned char*out,const unsigned char*input,const float*origin,int skipTrace,int retireBudget,int passedIsActor,c1mmp14Query_t query,C1PRContact contact,void*ctx){
 unsigned char p[C1PF_BYTES];int count,lower;unsigned flags;double dot;float v[2],length,slope;
 if(!out||!C1PFData(input,origin)||(skipTrace!=0&&skipTrace!=1)||(retireBudget!=0&&retireBudget!=1)||
    (out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,origin,12))return 0;
 memcpy(p,input,sizeof(p));count=C1PFShort(p,0x380);lower=C1PFShort(p,0x386);flags=C1PFWord(p,0x3b8)&~0x20u;C1PFSetWord(p,0x3b8,flags);
 if(lower==count-1){
  v[0]=(float)((double)C1PFFloat(p,0x3c8)-origin[0]);v[1]=(float)((double)C1PFFloat(p,0x3cc)-origin[1]);slope=(float)((double)C1PFFloat(p,0x3d0)-origin[2]);
  length=(float)sqrt((double)(float)((double)v[0]*v[0]+(double)v[1]*v[1]));C1PFNormalize2(v);memcpy(p+0x398,v,8);
  C1PFSetFloat(p,0x3a0,length!=0?(float)((double)slope/length):0);C1PFSetFloat(p,0x3ac,length);C1PFSetWord(p,0x3b8,flags|0x41);C1PFSetFloat(p,0x3b4,0);C1PFSetShort(p,0x388,lower);
 }else{
  dot=((double)C1PFFloat(p,0x3cc)-origin[1])*C1PFFloat(p,0x39c)+((double)C1PFFloat(p,0x3c8)-origin[0])*C1PFFloat(p,0x398);
  if(dot<=0){
   if((flags&4)&&count*4<=C1PFShort(p,0x382))return 2;
   if(!C1PFForward(p,p,origin))return 0;
   if(retireBudget&&!C1PFBudget(p,p,origin))return 0;
  }else if(C1PFFloat(p,0x3b0)>=64){if(!C1PFBackward(p,p,origin))return 0;}
  if(!C1PFLookahead(p,p,origin,skipTrace,passedIsActor,query,contact,ctx))return 0;
 }
 if(!C1PFDirection(p,p,origin))return 0;memcpy(out,p,sizeof(p));return 1;
}
/* Native5F550 initializes direction toward the retained start, then updates. */
static int C1PFStart(unsigned char*out,const unsigned char*input,const float*origin,int retireBudget,int passedIsActor,c1mmp14Query_t query,C1PRContact contact,void*ctx){
 unsigned char p[C1PF_BYTES];float v[2],length,slope;int result;
 if(!out||!C1PFData(input,origin)||(out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,origin,12))return 0;
 memcpy(p,input,sizeof(p));v[0]=(float)((double)C1PFFloat(p,0x3c8)-origin[0]);v[1]=(float)((double)C1PFFloat(p,0x3cc)-origin[1]);slope=(float)((double)C1PFFloat(p,0x3d0)-origin[2]);
 length=(float)sqrt((double)(float)((double)v[0]*v[0]+(double)v[1]*v[1]));C1PFNormalize2(v);memcpy(p+0x398,v,8);C1PFSetFloat(p,0x3a0,length!=0?(float)((double)slope/length):0);C1PFSetFloat(p,0x3ac,length);
 C1PFSetFloat(p,0x3b4,0);C1PFSetShort(p,0x388,C1PFShort(p,0x380)-1);
 result=C1PFUpdate(p,p,origin,0,retireBudget,passedIsActor,query,contact,ctx);if(result!=1)return result;memcpy(out,p,sizeof(p));return 1;
}
/* Native5F2E0 carries the nearest retained-prefix area into a rebuilt path. */
static int C1PFRetire(unsigned char*out,const unsigned char*input,const float*origin){
 unsigned char p[C1PF_BYTES];int count,total,lower,i;float dx,dy,best,distance,area=0,nearestArea=0,cross;
 if(!out||!input||!C1MMP14_Vec(origin)||(out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,origin,12))return 0;
 memcpy(p,input,sizeof(p));count=C1PFShort(p,0x380);total=C1PFShort(p,0x382);lower=C1PFShort(p,0x386);
 if(count<0||count>32||total<count||total>32||lower<0||lower>32)return 0;
 if(count-1<=lower){memcpy(out,p,sizeof(p));return 1;}
 if(!C1PFData(p,origin))return 0;
 i=count-2;dx=(float)((double)origin[0]-C1PFFloat(p,i*28));dy=(float)((double)origin[1]-C1PFFloat(p,i*28+4));best=(float)sqrt((double)dy*dy+(double)dx*dx);
 for(i=count-1;;i++){
  dx=(float)((double)C1PFFloat(p,i*28)-origin[0]);dy=(float)((double)C1PFFloat(p,i*28+4)-origin[1]);distance=(float)sqrt((double)dy*dy+(double)dx*dx);
  if(distance<=best){best=distance;nearestArea=area;}
  if(i+1>=total)break;
  cross=fabsf((float)((double)dy*C1PFFloat(p,i*28+12)-(double)dx*C1PFFloat(p,i*28+16)));
  area=(float)((double)cross*C1PFFloat(p,i*28+20)+area);if(area>32768)break;
 }
 i=count-2;dx=(float)((double)C1PFFloat(p,i*28)-origin[0]);dy=(float)((double)C1PFFloat(p,i*28+4)-origin[1]);
 cross=fabsf((float)((double)dy*C1PFFloat(p,i*28+12)-(double)dx*C1PFFloat(p,i*28+16)));
 nearestArea=(float)(((double)C1PFFloat(p,i*28+20)-C1PFFloat(p,0x3c4))*cross+nearestArea);
 C1PFSetFloat(p,0x3b0,(float)((double)nearestArea+C1PFFloat(p,0x3b0)));memcpy(out,p,sizeof(p));return 1;
}
/* Native5F5E0 preserves previous travel direction while admitting a new path. */
static int C1PFCarry(unsigned char*out,const unsigned char*input,const float*origin,int passedIsActor,c1mmp14Query_t query,C1PRContact contact,void*ctx){
 unsigned char p[C1PF_BYTES];float v[2],old[2],length,budget,area=0,bestArea=0,lastCross,cross,absCross,dirCross,bestDot,crossArea=0;
 int i,lower,first=1,turn,result;double value,dot,back;
 if(!out||!C1PFData(input,origin)||(out!=input&&C1PSOverlap(out,C1PF_BYTES,input,C1PF_BYTES))||C1PSOverlap(out,C1PF_BYTES,origin,12))return 0;
 memcpy(p,input,sizeof(p));if(C1PFFloat(p,0x3ac)==0)goto start_zero;
 memcpy(old,p+0x398,8);budget=C1PFFloat(p,0x3b0);if(budget<=0)return 0;
 v[0]=(float)((double)C1PFFloat(p,0x3c8)-origin[0]);v[1]=(float)((double)C1PFFloat(p,0x3cc)-origin[1]);
 length=(float)sqrt((double)(float)((double)v[0]*v[0]+(double)v[1]*v[1]));C1PFNormalize2(v);
 lastCross=(float)(((double)v[1]*old[0]-(double)v[0]*old[1])*length);bestDot=(float)((double)v[1]*old[1]+(double)v[0]*old[0]);lower=C1PFShort(p,0x386);
 if(C1PFShort(p,0x380)-2<lower)goto start_zero;
 for(i=C1PFShort(p,0x380)-2;i>=lower;i--,first=0){
  v[0]=(float)((double)C1PFFloat(p,i*28)-origin[0]);v[1]=(float)((double)C1PFFloat(p,i*28+4)-origin[1]);length=C1PFFloat(p,first?0x3c4:i*28+20);
  value=(double)v[1]*C1PFFloat(p,i*28+12)-(double)v[0]*C1PFFloat(p,i*28+16);
  turn=value*((double)old[1]*C1PFFloat(p,i*28+12)-(double)old[0]*C1PFFloat(p,i*28+16))>0;
  cross=(float)value;absCross=fabsf(cross);dirCross=(float)((double)v[1]*old[0]-(double)v[0]*old[1]);
  if(turn&&(double)dirCross*lastCross<0)goto crossing;
  area=(float)((double)absCross*length+area);
  if(area>=budget)goto endpoint;
  lastCross=dirCross;C1PFNormalize2(v);dot=(double)v[1]*old[1]+(double)v[0]*old[0];
  if(dot>=bestDot){bestDot=(float)dot;bestArea=area;}
 }
 if(bestArea==area)goto start_zero;
 goto start_one;
crossing:
 value=(((double)absCross*lastCross)*length)/((double)lastCross-dirCross)+area;crossArea=(float)value;
 if(value>=budget)goto update_one;
 while(i>lower){
  area=(float)((double)absCross*length+area);if(area>=budget)goto crossing_budget;
  --i;lastCross=dirCross;v[0]=(float)((double)C1PFFloat(p,i*28)-origin[0]);v[1]=(float)((double)C1PFFloat(p,i*28+4)-origin[1]);length=C1PFFloat(p,i*28+20);
  value=(double)v[1]*C1PFFloat(p,i*28+12)-(double)v[0]*C1PFFloat(p,i*28+16);
  turn=value*((double)old[1]*C1PFFloat(p,i*28+12)-(double)old[0]*C1PFFloat(p,i*28+16))>0;
  absCross=fabsf((float)value);dirCross=(float)((double)v[1]*old[0]-(double)v[0]*old[1]);
  if(turn&&(double)dirCross*lastCross<0){
   value=(((double)absCross*lastCross)*length)/((double)lastCross-dirCross)+area;crossArea=(float)value;if(value>=budget)goto update_one;
  }
 }
crossing_budget:C1PFSetFloat(p,0x3b0,crossArea<64?64:crossArea);
update_one:result=C1PFUpdate(p,p,origin,0,1,passedIsActor,query,contact,ctx);goto done;
endpoint:
 if(absCross==0)return 0;back=((double)area-budget)/absCross;
 v[0]=(float)(((double)C1PFFloat(p,i*28)-back*C1PFFloat(p,i*28+12))-origin[0]);
 v[1]=(float)((double)(float)((double)C1PFFloat(p,i*28+4)-back*C1PFFloat(p,i*28+16))-origin[1]);C1PFNormalize2(v);
 dot=(double)v[1]*old[1]+(double)v[0]*old[0];if(dot>=bestDot)goto start_zero;
start_one:
 result=C1PFStart(p,p,origin,1,passedIsActor,query,contact,ctx);
 if(result==1&&!(C1PFWord(p,0x3b8)&2)&&C1PFFloat(p,0x3b0)>bestArea)C1PFSetFloat(p,0x3b0,bestArea<64?64:bestArea);
 goto done;
start_zero:result=C1PFStart(p,p,origin,0,passedIsActor,query,contact,ctx);
done:if(result!=1)return result;memcpy(out,p,sizeof(p));return 1;
}
#endif
