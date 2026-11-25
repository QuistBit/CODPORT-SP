#ifndef BG_COD1_NODE_REACHABLE_H
#define BG_COD1_NODE_REACHABLE_H
#include "bg_cod1_node_candidates.h"
/* Native6E170 deliberately never returns zero, even for equal distances.
 * Preserve original74920 ordering; a platform qsort changes equal candidates.
 * Sorting loop is the retained, qualified node-sort implementation specialized
 * to8-byte candidates and this native comparator. */
static int C1NRCompare(const C1NQCandidate*p,int a,int b,const unsigned char*flags){
 unsigned aa=flags[p[a].node],bb=flags[p[b].node];
 if(!aa&&bb)return 1;if(aa&&!bb)return -1;
 return p[a].distanceSquared<p[b].distanceSquared?-1:1;
}
static void C1NRSwap(C1NQCandidate*p,int a,int b){C1NQCandidate v=p[a];p[a]=p[b];p[b]=v;}
static void C1NRSort(C1NQCandidate*p,unsigned count,const unsigned char*records){
 unsigned char flags[2048];unsigned k;
 int lo=0,hi,mid,l,r,max,i,n=0,los[32],his[32];if(count<2)return;hi=(int)count-1;
 /* Candidate IDs are validated by C1NQQuery before sorting. Only the
  * zero/nonzero class is used by the native comparator. */
 for(k=0;k<count;k++)flags[p[k].node]=(unsigned char)(C1NQU16(records+p[k].node*116+0x20)!=0);
 for(;;){
  if(hi-lo+1<=8){
   while(hi>lo){max=lo;for(i=lo+1;i<=hi;i++)if(C1NRCompare(p,i,max,flags)>0)max=i;C1NRSwap(p,max,hi);hi--;}
  }else{
   mid=lo+(hi-lo+1)/2;
   if(C1NRCompare(p,lo,mid,flags)>0)C1NRSwap(p,lo,mid);
   if(C1NRCompare(p,lo,hi,flags)>0)C1NRSwap(p,lo,hi);
   if(C1NRCompare(p,mid,hi,flags)>0)C1NRSwap(p,mid,hi);
   l=lo;r=hi;
   for(;;){
    if(mid>l){do{l++;}while(l<mid&&C1NRCompare(p,l,mid,flags)<=0);}
    if(mid<=l){do{l++;}while(l<=hi&&C1NRCompare(p,l,mid,flags)<=0);}
    do{r--;}while(r>mid&&C1NRCompare(p,r,mid,flags)>0);
    if(l>r)break;
    C1NRSwap(p,l,r);if(mid==r)mid=l;
   }
   r++;
   if(mid<r){do{r--;}while(r>mid&&C1NRCompare(p,r,mid,flags)==0);}
   if(mid>=r){do{r--;}while(r>lo&&C1NRCompare(p,r,mid,flags)==0);}
   if(r-lo>=hi-l){if(lo<r){los[n]=lo;his[n++]=r;}if(l<hi){lo=l;continue;}}
   else{if(l<hi){los[n]=l;his[n++]=hi;}if(lo<r){hi=r;continue;}}
  }
  if(!n)break;--n;lo=los[n];hi=his[n];
 }
}
typedef int(*C1NRTrace)(void*,const float*,const float*,const float*,const float*,unsigned,unsigned,unsigned,int*);
static int C1NRTraceCall(C1NRTrace fn,void*ctx,const float*start,const float*mins,const float*maxs,const float*end,int*out){
 float a[3],lo[3],hi[3],b[3];int status,blocked=-1;
 memcpy(a,start,12);memcpy(lo,mins,12);memcpy(hi,maxs,12);memcpy(b,end,12);
#if defined(_MSC_VER)&&defined(_M_IX86)
 {unsigned char saved[108],discard[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 status=fn(ctx,a,lo,hi,b,2047,2047,0x820011,&blocked);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
 }
#else
 status=fn(ctx,a,lo,hi,b,2047,2047,0x820011,&blocked);
#endif
 if(status!=1||(blocked!=0&&blocked!=1)||memcmp(a,start,12)||memcmp(lo,mins,12)||memcmp(hi,maxs,12)||memcmp(b,end,12))return 0;*out=blocked;return 1;
}
typedef struct {int node;unsigned candidates,traces;} C1NRResult;
/* Native71240 (including optional2D exclusion planes). Complete candidate
 * query/sort/selection over a supplied DATA graph. Trace callbacks must run
 * inside an outer live-world transaction. Only copied input is used after the
 * first callback; graph/allocation authority remains the caller's obligation. */
static int C1NRNearest(const unsigned char*records,size_t bytes,const C1NTree*tree,
 const float*origin,float radius,unsigned mask,unsigned capacity,
 const float (*normals)[2],const float*limits,unsigned planeCount,
 C1NRTrace trace,void*ctx,C1NRResult*out){
 unsigned char*copy=NULL;C1NQCandidate candidates[2048];unsigned deferred[2048],deferredCount=0,i,j;
 float point[3],plane[64][2],bound[64];C1NRResult result={-1,0,0};int ok=0,blocked,inside;
 static const float mins[3]={-15,-15,17},maxs[3]={15,15,72};
 static const float boxlo[3]={-16,-16,-1},boxhi[3]={16,16,73};
 if(!records||!tree||!tree->count||tree->count>2048||bytes<(size_t)tree->count*116||!origin||!out||!trace||
    planeCount>64||(planeCount&&(!normals||!limits))||capacity>2048)return 0;
 if(C1PSOverlap(out,sizeof(*out),records,bytes)||C1PSOverlap(out,sizeof(*out),tree,sizeof(*tree))||
    C1PSOverlap(out,sizeof(*out),origin,12)||
    (tree->nodes&&C1PSOverlap(out,sizeof(*out),tree->nodes,tree->used*sizeof(*tree->nodes)))||
    (tree->order&&C1PSOverlap(out,sizeof(*out),tree->order,tree->count*sizeof(*tree->order)))||
    (planeCount&&(C1PSOverlap(out,sizeof(*out),normals,planeCount*8)||C1PSOverlap(out,sizeof(*out),limits,planeCount*4))))return 0;
 memcpy(point,origin,12);
 for(i=0;i<planeCount;i++){
  for(j=0;j<2;j++){if(!isfinite(normals[i][j])||fabsf(normals[i][j])>1000000)return 0;plane[i][j]=normals[i][j];}
  if(!isfinite(limits[i])||fabsf(limits[i])>1000000)return 0;bound[i]=limits[i];
 }
 copy=malloc(tree->count*116);if(!copy)return 0;memcpy(copy,records,tree->count*116);
 if(!C1NQQuery(copy,tree->count*116,tree,point,radius,64,mask,candidates,capacity,&result.candidates))goto done;
 C1NRSort(candidates,result.candidates,copy);
 for(i=0;i<result.candidates;i++){
  const unsigned char*r=copy+candidates[i].node*116;float position[3];memcpy(position,r+0x38,12);inside=1;
  for(j=0;j<3;j++)if(!((double)position[j]+boxlo[j]<point[j]&&(double)position[j]+boxhi[j]>point[j]))inside=0;
  if(inside){result.node=(int)candidates[i].node;ok=1;goto done;}
  for(j=0;j<planeCount;j++)if((double)plane[j][1]*position[1]+(double)plane[j][0]*position[0]>bound[j])break;
  if(j<planeCount){deferred[deferredCount++]=candidates[i].node;continue;}
  blocked=-1;result.traces++;
  if(!C1NRTraceCall(trace,ctx,point,mins,maxs,position,&blocked))goto done;
  if(!blocked){result.node=(int)candidates[i].node;ok=1;goto done;}
 }
 for(i=0;i<deferredCount;i++){
  float position[3];memcpy(position,copy+deferred[i]*116+0x38,12);blocked=-1;result.traces++;
  if(!C1NRTraceCall(trace,ctx,point,mins,maxs,position,&blocked))goto done;
  if(!blocked){result.node=(int)deferred[i];ok=1;goto done;}
 }
 ok=1;
done:free(copy);if(ok)*out=result;return ok;
}
#endif
