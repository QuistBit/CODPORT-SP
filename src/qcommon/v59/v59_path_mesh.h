/* Original 41A3B0 -> 41A350 -> 4196F0, vertical capsule sight.
 * Only its boolean result is exposed. Every accepted contact monotonically
 * lowers fraction below 1, so normal/fraction publication is unnecessary here.
 * PC53 intermediate arithmetic, with the original float stores retained. */
#ifndef V59_PATH_MESH_H
#define V59_PATH_MESH_H
#include "v59_path_sight.h"
static double C1VMDot(const float*a,const float*b,int i,int j,int k){
 return ((double)a[i]*b[i]+(double)a[j]*b[j])+(double)a[k]*b[k];
}
static int C1VMSpan(const triangle*t,const float*s,float radius,float span,float d){
 const float*n=t->plane,*bu=t->bary[0],*bv=t->bary[1];
 double farEnd=(double)span*n[2]+d,at,u1,v1;float inv,u,v,storedU;
 if(farEnd<=-radius)return 0;
 inv=C1PS_Store(1.0/n[2]);at=(-(double)radius-d)*inv;
 u=C1PS_Store(C1VMDot(bu,s,1,0,2)-bu[3]);v=C1PS_Store(C1VMDot(bv,s,1,0,2)-bv[3]);
 u1=at*bu[2]+u;storedU=C1PS_Store(u1);v1=at*bv[2]+v;
 if(u1>=0&&v1>=0&&(double)storedU+v1<=1)return 1;
 at=farEnd<radius?span:((double)radius-d)*inv;
 u1=at*bu[2]+u;storedU=C1PS_Store(u1);v1=at*bv[2]+v;
 return u1>0&&v1>0&&(double)storedU+v1<=1;
}
static int C1VMMesh(const C1PathSight*t,const mesh*m,unsigned char*edgeSeen,unsigned char*vertexSeen){
 float s[3],e[3],delta[3],span,radius,delta2;int i,j,k;
 memcpy(s,t->start,12);memcpy(e,t->end,12);
 for(k=0;k<3;k++)delta[k]=C1PS_Store((double)e[k]-s[k]);
 delta2=C1PS_Store(C1VMDot(delta,delta,2,1,0));radius=C1PS_Store((double)t->radius+.125);
 if(m->down){span=C1PS_Store(-2.0*t->offset);s[2]=C1PS_Store((double)s[2]+t->offset);e[2]=C1PS_Store((double)e[2]+t->offset);}
 else{span=C1PS_Store(2.0*t->offset);s[2]=C1PS_Store((double)s[2]-t->offset);e[2]=C1PS_Store((double)e[2]-t->offset);}
 memset(edgeSeen,0,m->ne);memset(vertexSeen,0,m->nv);
 for(i=0;i<m->nt;i++){
  const triangle*tri=m->tri+i;const float*n=tri->plane,*bu=tri->bary[0],*bv=tri->bary[1];
  double d2=C1VMDot(n,e,2,1,0)-n[3],d1,at,p[3],v;float storedD1,dd,u;int mask;
  if(d2>=radius)continue;
  d1=C1VMDot(n,s,2,1,0)-n[3];storedD1=C1PS_Store(d1);dd=C1PS_Store(d1-d2);
  if(dd<=0)continue;
  if(storedD1<=-radius){if(C1VMSpan(tri,s,radius,span,storedD1))return 1;continue;}
  if(storedD1<radius){at=0;for(k=0;k<3;k++)p[k]=s[k];}
  else{at=((double)storedD1-radius)/dd;if(at>1)continue;for(k=0;k<3;k++)p[k]=at*delta[k]+s[k];}
  u=C1PS_Store((p[1]*bu[1]+p[0]*bu[0])+p[2]*bu[2]-bu[3]);
  v=(p[2]*bv[2]+p[1]*bv[1])+p[0]*bv[0]-bv[3];
  mask=((double)u+v>1)|((u<0)<<1)|((v<0)<<2);
  if(!mask)return 1;
  for(j=0;j<3;j++)if(mask&(1<<j)){
   int id=tri->edges[j];const edge*ed;double q[3],cc,bb,aa,disc,pos;float sx,sy,along,vx,vy,storedC,storedB,storedAt;
   if(id<0||edgeSeen[id])continue;edgeSeen[id]=1;ed=m->edges+id;
   for(k=0;k<3;k++)q[k]=(double)s[k]-ed->origin[k];
   sx=C1PS_Store((q[0]*ed->basis[0][0]+q[2]*ed->basis[0][2])+q[1]*ed->basis[0][1]);
   sy=C1PS_Store((q[0]*ed->basis[1][0]+q[2]*ed->basis[1][2])+q[1]*ed->basis[1][1]);
   along=C1PS_Store((q[2]*ed->direction[2]+q[1]*ed->direction[1])+q[0]*ed->direction[0]);
   cc=((double)sy*sy+(double)sx*sx)-(double)radius*radius;storedC=C1PS_Store(cc);
   if(cc<=0){if(along>=0&&along<=ed->length)return 1;continue;}
   vx=C1PS_Store(C1VMDot(ed->basis[0],delta,0,1,2));vy=C1PS_Store(C1VMDot(ed->basis[1],delta,1,0,2));
   bb=(double)sy*vy+(double)sx*vx;storedB=C1PS_Store(bb);if(bb>=0)continue;
   aa=(double)vy*vy+(double)vx*vx;disc=(double)storedB*storedB-aa*storedC;if(disc<=0)continue;
   at=(-sqrt(disc)-storedB)/aa;storedAt=C1PS_Store(at);if(at>=1)continue;
   pos=C1VMDot(ed->direction,delta,2,0,1)*storedAt+along;
   if(pos>=0&&pos<=ed->length)return 1;
  }else{
   int id=tri->vertices[j];float q[3],storedC,storedB;double z,cc,bb,disc;
   if(id<0||vertexSeen[id])continue;vertexSeen[id]=1;
   q[0]=C1PS_Store((double)s[0]-m->vertices[id][0]);q[1]=C1PS_Store((double)s[1]-m->vertices[id][1]);
   z=(double)s[2]-m->vertices[id][2];q[2]=C1PS_Store(z);
   cc=(z*q[2]+(double)q[1]*q[1])+(double)q[0]*q[0]-(double)radius*radius;storedC=C1PS_Store(cc);
   if(cc<=0)return 1;
   bb=C1VMDot(q,delta,1,0,2);storedB=C1PS_Store(bb);if(bb>=0)continue;
   disc=(double)storedB*storedB-(double)delta2*storedC;if(disc<0)continue;
   at=(-sqrt(disc)-storedB)/delta2;if(at<1)return 1;
  }
 }
 return 0;
}
#endif
