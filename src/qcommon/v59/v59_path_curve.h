/* Original 415F40 capsule sight and 415910 facet clipping. Unlike a full
 * trace, this publishes only whether any facet obstructs the segment. */
#ifndef V59_PATH_CURVE_H
#define V59_PATH_CURVE_H
#include "v59_path_sight.h"
static double C1VCDot(const float*a,const float*b){return ((double)a[2]*b[2]+(double)a[1]*b[1])+(double)a[0]*b[0];}
static int C1VCClip(const float*s,const float*e,const float*p,float*enter,float*leave,int*hit){
 float d1=C1PS_Store(C1VCDot(s,p)-p[3]),d2=C1PS_Store(C1VCDot(e,p)-p[3]);double f;*hit=0;
 if(d1>0&&(d2>=.125f||d2>=d1))return 0;
 if(d1<=0&&d2<=0)return 1;
 if(d1>d2){f=((double)d1-.125)/((double)d1-d2);if(f<0)f=0;if(f>*enter){*enter=C1PS_Store(f);*hit=1;}}
 else{f=((double)d1+.125)/((double)d1-d2);if(f>1)f=1;if(f<*leave)*leave=C1PS_Store(f);}
 return 1;
}
static int C1VCPlane(const C1PathSight*t,const float*plane,int inward,float*enter,float*leave,int*hit){
 float p[4],s[3],e[3],offset;int k;
 for(k=0;k<4;k++)p[k]=inward?-plane[k]:plane[k];
 p[3]=C1PS_Store((double)p[3]+t->radius);
 offset=(double)p[2]*t->offset>0?-t->offset:t->offset;
 memcpy(s,t->start,12);memcpy(e,t->end,12);s[2]=C1PS_Store((double)s[2]+offset);e[2]=C1PS_Store((double)e[2]+offset);
 return C1VCClip(s,e,p,enter,leave,hit);
}
static int C1VCCurve(const C1PathSight*t,const curve*c){
 int i,j,hit,last;float enter,leave;
 for(i=0;i<c->nf;i++){
  const facet*f=c->facets+i;enter=-1;leave=1;last=-1;
  if(!C1VCPlane(t,c->planes[f->plane].p,0,&enter,&leave,&hit))continue;
  for(j=0;j<f->n;j++){
   if(!C1VCPlane(t,c->planes[f->planes[j]].p,f->inward[j],&enter,&leave,&hit))break;
   if(hit)last=j;
  }
  if(j==f->n&&last!=f->n-1&&enter<leave&&enter>=0)return 1;
 }
 return 0;
}
#endif
