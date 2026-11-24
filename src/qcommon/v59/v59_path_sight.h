/* Native 41E960 / 41E660 / 41DE10 capsule sight, PC53.
 * A separate query from contact-biased movement traces. Surface primitives
 * are an explicit provider boundary, never a fabricated clear result. */
#ifndef V59_PATH_SIGHT_H
#define V59_PATH_SIGHT_H
#include "v59_internal.h"
#include "../cod1_path_sight.h"
typedef int (*C1VSSurface)(void*,const C1PathSight*,const surface*,int*);
typedef struct {
 const v59_world*w;C1PathSight shape;uint32_t mask;
 unsigned char *brushseen,*surfaceseen;
 C1VSSurface surfaceQuery;void*context;int failure;
 unsigned leaves,brushes,surfaces;
} C1VSQuery;
static int C1VSBrush(const C1PathSight*t,const brush*b){
 float enter,leave;int j;
 if(!C1PS_Axes(t,b->mins,b->maxs,&enter,&leave))return 0;
 for(j=6;j<b->n;j++)if(!C1PS_Plane(t,b->planes[j].n,b->planes[j].d,&enter,&leave))return 0;
 return 1;
}
static int C1VSBounds(const C1PathSight*t,const float*b){
 int k;for(k=0;k<3;k++){
  float lo=C1PS_Store((double)(t->start[k]<t->end[k]?t->start[k]:t->end[k])-t->extents[k]);
  float hi=C1PS_Store((double)(t->start[k]>t->end[k]?t->start[k]:t->end[k])+t->extents[k]);
  if(lo>b[k+3]||hi<b[k])return 0;
 }return 1;
}
static int C1VSVisit(C1VSQuery*q,const uint32_t*br,int nb,const uint32_t*sr,int ns){
 int k,i,blocked;for(k=0;k<nb;k++){
  i=br[k];if(i<0||i>=q->w->nb){q->failure=1;return 0;}
  if(q->brushseen[i])continue;q->brushseen[i]=1;
  if(!(q->w->brushes[i].contents&q->mask))continue;q->brushes++;
  if(C1VSBrush(&q->shape,q->w->brushes+i))return i+1;
 }
 for(k=0;k<ns;k++){
  i=sr[k];if(i<0||i>=q->w->ns){q->failure=1;return 0;}
  if(q->surfaceseen[i])continue;q->surfaceseen[i]=1;
  if(!(q->w->surfaces[i].contents&q->mask)||!C1VSBounds(&q->shape,q->w->surfaces[i].bounds))continue;
  q->surfaces++;blocked=-1;
  if(!q->surfaceQuery||q->surfaceQuery(q->context,&q->shape,q->w->surfaces+i,&blocked)!=1||(blocked!=0&&blocked!=1)){
   q->failure=1;return 0;
  }
  if(blocked)return q->w->nb+i+1;
 }return 0;
}
static int C1VSWalk(C1VSQuery*q,int index,float f1,float f2,const float*a,const float*b,unsigned depth){
 const int*n;const float*p;double d1,offset,inv,nearPart;float d2,farPart,span,mid[3];int k,axis,side,result;
 /* Loader validates at most 1024 levels; its maxdepth field is temporary
  * recursion scratch and returns to zero after construction. */
 if(depth>1024u){q->failure=1;return 0;}
 if(index<0){const int*l;index=-1-index;if(index<0||index>=q->w->nl){q->failure=1;return 0;}
  l=q->w->leaves[index].v;q->leaves++;
  return C1VSVisit(q,q->w->brushrefs+l[4],l[5],q->w->surfacerefs+l[2],l[3]);
 }
 if(index>=q->w->nn){q->failure=1;return 0;}n=q->w->nodes[index].v;p=q->w->planes[n[0]];
 for(axis=0;axis<3;axis++)if(p[axis]==1)break;
 if(axis<3){d1=(double)a[axis]-p[3];d2=C1PS_Store((double)b[axis]-p[3]);offset=q->shape.extents[axis];}
 else{d1=((double)a[1]*p[1]+(double)a[2]*p[2])+(double)a[0]*p[0]-p[3];
  d2=C1PS_Store(((double)b[1]*p[1]+(double)b[2]*p[2])+(double)b[0]*p[0]-p[3]);offset=2048;}
 if(d1>=offset+1&&d2>=offset+1)return C1VSWalk(q,n[1],f1,f2,a,b,depth+1);
 if(d1< -offset-1&&d2< -offset-1)return C1VSWalk(q,n[2],f1,f2,a,b,depth+1);
 if(d1<d2){side=1;inv=1/(d1-d2);farPart=C1PS_Store((d1+offset+.125)*inv);nearPart=(d1-offset+.125)*inv;}
 else if(d1>d2){side=0;inv=1/(d1-d2);farPart=C1PS_Store((d1-offset-.125)*inv);nearPart=(d1+offset+.125)*inv;}
 else{side=0;nearPart=1;farPart=0;}
 if(nearPart<0)nearPart=0;else if(nearPart>1)nearPart=1;
 for(k=0;k<3;k++)mid[k]=C1PS_Store(((double)b[k]-a[k])*nearPart+a[k]);
 span=C1PS_Store((double)f2-f1);
 result=C1VSWalk(q,n[1+side],f1,C1PS_Store((double)span*nearPart+f1),a,mid,depth+1);
 if(result||q->failure)return result;
 nearPart=farPart;if(nearPart<0)nearPart=0;else if(nearPart>1)nearPart=1;
 for(k=0;k<3;k++)mid[k]=C1PS_Store(((double)b[k]-a[k])*nearPart+a[k]);
 return C1VSWalk(q,n[1+(side^1)],C1PS_Store((double)span*nearPart+f1),f2,mid,b,depth+1);
}
/* Caller supplies disjoint scratch for immutable, loader-validated world.
 * Returns 1 only with a native result; failure leaves output unchanged. */
static int C1VSWorld(C1VSQuery*q,const float*s,const float*e,const float*mins,const float*maxs,int*out){
 static const float zero[3]={0,0,0};int result,k;
 if(!q||!q->w||!q->brushseen||!q->surfaceseen||!s||!e||!mins||!maxs||!out||q->w->nn<1)return 0;
 for(k=0;k<3;k++)if(!isfinite(s[k])||!isfinite(e[k])||!isfinite(mins[k])||!isfinite(maxs[k])||mins[k]>=maxs[k])return 0;
 if(maxs[0]-mins[0]!=maxs[1]-mins[1])return 0;
 C1PS_Prepare(&q->shape,s,e,mins,maxs,zero);q->failure=0;q->leaves=q->brushes=q->surfaces=0;
 memset(q->brushseen,0,q->w->nb);memset(q->surfaceseen,0,q->w->ns);
 result=C1VSWalk(q,0,0,1,q->shape.start,q->shape.end,0);
 if(q->failure)return 0;*out=result;return 1;
}
#endif
