/* SPDX-License-Identifier: GPL-3.0-or-later
 * Capsule narrow phases reconstructed and verified against CoD1 SP.
 * Facet clipping follows the RTCW-derived reference; see LICENSE_RTCW.txt. */
#include "v59_internal.h"
#include "v59_box_brush.h"
static int brush_inside(const brush*b,const trace_work*t){int k,j;float p[3],d;const brush_plane*s;
 for(k=0;k<3;k++)if(t->bmins[k]>b->maxs[k]||t->bmaxs[k]<b->mins[k])return 0;
 for(j=6;j<b->n;j++){s=&b->planes[j];d=fa(s->d,t->radius);for(k=0;k<3;k++)p[k]=dp(s->n,t->offset,1,0,2)>0?fs(t->start[k],t->offset[k]):fa(t->start[k],t->offset[k]);
  if(fs(dp(p,s->n,0,2,1),d)>0)return 0;}return 1;
}
static void brush_sweep(const v59_world*w,const brush*b,trace_work*t){int j,k,end_inside=1;float enter=0,leave=t->fraction;const brush_plane*selected=NULL;
 for(j=0;j<b->n;j++){
  const brush_plane*s=&b->planes[j];float d1,d2,dd,num,p[3],q[3],dist;
  if(s->axis>=0){k=s->axis;d1=fs(fm(fs(t->start[k],s->bound),s->sign),t->extents[k]);d2=fs(fm(fs(t->end[k],s->bound),s->sign),t->extents[k]);}
  else {int subtract=dp(s->n,t->offset,2,0,1)>0;dist=fa(s->d,t->radius);
   for(k=0;k<3;k++){p[k]=subtract?fs(t->start[k],t->offset[k]):fa(t->start[k],t->offset[k]);q[k]=subtract?fs(t->end[k],t->offset[k]):fa(t->end[k],t->offset[k]);}
   d1=fs(dot(p,s->n),dist);d2=fs(dot(q,s->n),dist);
  }
  if(d1>0){dd=fs(d1,d2);if(d2>0){if(dd<=0||d2>=.125f)return;end_inside=0;}num=fs(d1,.125f);
   if(num>fm(dd,enter)){enter=fd(num,dd);if(enter>=leave)return;selected=s;}else if(!selected)selected=s;
  }else if(d2>0){end_inside=0;dd=fs(d1,d2);if(fm(leave,dd)<d1){leave=fd(d1,dd);if(enter>=leave)return;}}
 }
 t->contents=b->contents;if(!selected){t->startsolid=1;if(end_inside){t->fraction=0;t->allsolid=1;}}
 else {t->fraction=enter;cp(selected->n,t->normal);t->flags=w->materials[selected->material].flags;t->material=72u*(selected->material+1);}
}
static int plane_inside(const float*p,const trace_work*t){float q[3],dist=fa(p[3],t->radius);int k,subtract=dp(p,t->offset,0,1,2)>0;
 for(k=0;k<3;k++)q[k]=subtract?fs(t->start[k],t->offset[k]):fa(t->start[k],t->offset[k]);return fs(dot(q,p),dist)<=0;
}
static int curve_inside(const curve*c,const trace_work*t){int i,j,k;float p[4];
 for(i=0;i<c->nf;i++){const facet*f=&c->facets[i];if(!plane_inside(c->planes[f->plane].p,t))continue;
  for(j=0;j<f->n;j++){for(k=0;k<4;k++)p[k]=f->inward[j]?-c->planes[f->planes[j]].p[k]:c->planes[f->planes[j]].p[k];if(!plane_inside(p,t))break;}if(j==f->n)return 1;}
 return 0;
}
static int facet_clip(const float*p,const trace_work*t,float*enter,float*leave,int*hit){float a[3],b[3],dist=fa(p[3],t->radius),d1,d2,f;int k,subtract=dp(p,t->offset,0,1,2)>0;*hit=0;
 for(k=0;k<3;k++){a[k]=subtract?fs(t->start[k],t->offset[k]):fa(t->start[k],t->offset[k]);b[k]=subtract?fs(t->end[k],t->offset[k]):fa(t->end[k],t->offset[k]);}
 d1=fs(dot(a,p),dist);d2=fs(dot(b,p),dist);if(d1>0&&(d2>=.125f||d2>=d1))return 0;if(d1<=0&&d2<=0)return 1;
 if(d1>d2){f=fmaxf(0,fd(fs(d1,.125f),fs(d1,d2)));if(f>*enter){*enter=f;*hit=1;}}
 else {*leave=fminf(*leave,fminf(1,fd(fa(d1,.125f),fs(d1,d2))));}return 1;
}
static void curve_sweep(const curve*c,trace_work*t){int i,j,k,hit,hitnum,ok;float p[4],best[3],enter,leave;
 for(i=0;i<c->nf;i++){
  const facet*f=&c->facets[i];enter=-1;leave=1;hitnum=-1;ok=facet_clip(c->planes[f->plane].p,t,&enter,&leave,&hit);if(!ok)continue;if(hit)cp(c->planes[f->plane].p,best);
  for(j=0;j<f->n;j++){for(k=0;k<4;k++)p[k]=f->inward[j]?-c->planes[f->planes[j]].p[k]:c->planes[f->planes[j]].p[k];ok=facet_clip(p,t,&enter,&leave,&hit);if(!ok)break;if(hit){hitnum=j;cp(p,best);}}
  if(!ok||hitnum==f->n-1)continue;if(enter>=0&&enter<leave&&enter<t->fraction){t->fraction=enter;cp(best,t->normal);}
 }
}
static int across_axis(const triangle*tri,const float*start,float radius,float span,float d){
 const float*n=tri->plane,*bu=tri->bary[0],*bv=tri->bary[1];float far=fa(fm(span,n[2]),d),inv,at,u,v,u1,v1;
 if(far<=-radius)return 0;inv=fd(1,n[2]);at=fm(fs(-radius,d),inv);
 u=fs(dp(bu,start,1,0,2),bu[3]);v=fs(dp(bv,start,1,0,2),bv[3]);u1=fa(fm(at,bu[2]),u);v1=fa(fm(at,bv[2]),v);
 if(u1>=0&&v1>=0&&fa(u1,v1)<=1)return 1;at=far<radius?span:fm(fs(radius,d),inv);
 u1=fa(fm(at,bu[2]),u);v1=fa(fm(at,bv[2]),v);return u1>0&&v1>0&&fa(u1,v1)<=1;
}
static int mesh_inside(const mesh*m,const trace_work*t){
 float start[3],axis=fs(t->halfheight,t->radius),span;int i,j,k;cp(t->start,start);
 if(m->down){start[2]=fa(start[2],axis);span=fm(-2,axis);}else{start[2]=fs(start[2],axis);span=fa(axis,axis);}
 for(i=0;i<m->nt;i++){
  const triangle*tri=&m->tri[i];const float*n=tri->plane,*bu=tri->bary[0],*bv=tri->bary[1];float d=fs(dot(n,start),n[3]),p[3],u,v,s[3],along;int mask;
  if(d>=t->radius)continue;if(d<=-t->radius){if(across_axis(tri,start,t->radius,span,d))return 1;continue;}
  for(k=0;k<3;k++)p[k]=fa(fm(-d,n[k]),start[k]);u=fs(dot(bu,p),bu[3]);v=fs(dot(bv,p),bv[3]);mask=(fa(u,v)>1)|((u<0)<<1)|((v<0)<<2);if(!mask)return 1;
  for(j=0;j<3;j++)if(mask&(1<<j)){
   const edge*e;if(tri->edges[j]<0)continue;e=&m->edges[tri->edges[j]];minus(start,e->origin,s);along=dot(s,e->direction);if(along<0||along>e->length)continue;
   for(k=0;k<3;k++)p[k]=fa(fm(-along,e->direction[k]),s[k]);if(dot(p,p)<fm(t->radius,t->radius))return 1;
  }else{if(tri->vertices[j]<0)continue;minus(start,m->vertices[tri->vertices[j]],s);if(dot(s,s)<fm(t->radius,t->radius))return 1;}
 }return 0;
}
static void solid(trace_work*t,const float*n){t->fraction=0;cp(n,t->normal);t->startsolid=1;}
static void mesh_sweep(v59_context*c,const mesh*m){
 trace_work*t=&c->t;float start[3],end[3],delta[3],axis=fs(t->halfheight,t->radius),span,radius=fa(t->radius,.125f),eps=.00001f;int i,j,k;
 cp(t->start,start);cp(t->end,end);minus(end,start,delta);
 if(m->down){start[2]=fa(start[2],axis);end[2]=fa(end[2],axis);span=fm(-2,axis);}else{start[2]=fs(start[2],axis);end[2]=fs(end[2],axis);span=fa(axis,axis);}
 memset(c->edgeseen,0,m->ne);memset(c->vertexseen,0,m->nv);
 for(i=0;i<m->nt;i++){
  const triangle*tri=&m->tri[i];const float*n=tri->plane,*bu=tri->bary[0],*bv=tri->bary[1];float d2=fs(dot(n,end),n[3]),d1,dd,at,p[3],u,v;int mask;
  if(d2>=radius)continue;d1=fs(dot(n,start),n[3]);dd=fs(d1,d2);if(dd<=0)continue;
  if(d1<=-radius){if(across_axis(tri,start,radius,span,d1)){solid(t,n);return;}continue;}
  if(d1<radius){at=0;cp(start,p);}else{at=fd(fs(d1,radius),dd);if(at>t->fraction)continue;for(k=0;k<3;k++)p[k]=fa(fm(at,delta[k]),start[k]);}
  u=fs(dp(bu,p,1,0,2),bu[3]);v=fs(dot(bv,p),bv[3]);mask=(fa(u,v)>1)|((u<0)<<1)|((v<0)<<2);
  if(!mask){cp(n,t->normal);if(at<=eps){solid(t,n);return;}t->fraction=fs(at,eps);continue;}
  for(j=0;j<3;j++)if(mask&(1<<j)){
   int ei=tri->edges[j];const edge*e;float s[3],sx,sy,along,cc,vx,vy,bb,aa,disc,pos,inv,nx,ny;
   if(ei<0||c->edgeseen[ei])continue;c->edgeseen[ei]=1;e=&m->edges[ei];minus(start,e->origin,s);
   sx=dp(s,e->basis[0],0,2,1);sy=dp(s,e->basis[1],0,2,1);along=dot(s,e->direction);cc=fs(fa(fm(sy,sy),fm(sx,sx)),fm(radius,radius));
   if(cc<=0){if(along>=0&&along<=e->length){solid(t,n);return;}continue;}
   vx=dp(e->basis[0],delta,0,1,2);vy=dp(e->basis[1],delta,1,0,2);bb=fa(fm(sy,vy),fm(sx,vx));if(bb>=0)continue;
   aa=fa(fm(vy,vy),fm(vx,vx));disc=fs(fm(bb,bb),fm(aa,cc));if(disc<=0)continue;at=fd(fs(-sqrtf(disc),bb),aa);if(at>=t->fraction)continue;
   pos=fa(fm(dp(e->direction,delta,2,0,1),at),along);if(pos<0||pos>e->length)continue;inv=fd(1,radius);nx=fm(fa(fm(vx,at),sx),inv);ny=fm(fa(fm(vy,at),sy),inv);
   for(k=0;k<3;k++)t->normal[k]=fa(fm(ny,e->basis[1][k]),fm(nx,e->basis[0][k]));
   if(t->fraction<=eps){t->fraction=0;t->startsolid=1;return;}t->fraction=fs(at,eps);
  }else{
   int vi=tri->vertices[j];float s[3],cc,bb,disc,inv;if(vi<0||c->vertexseen[vi])continue;c->vertexseen[vi]=1;minus(start,m->vertices[vi],s);
   cc=fs(dot(s,s),fm(radius,radius));if(cc<=0){solid(t,n);return;}bb=dp(s,delta,1,0,2);if(bb>=0)continue;
   disc=fs(fm(bb,bb),fm(t->delta2,cc));if(disc<0)continue;at=fd(fs(-sqrtf(disc),bb),t->delta2);if(at>=t->fraction)continue;inv=fd(1,radius);
   for(k=0;k<3;k++)t->normal[k]=fm(fa(fm(at,delta[k]),s[k]),inv);if(t->fraction<=eps){t->fraction=0;t->startsolid=1;return;}t->fraction=fs(at,eps);
  }
 }
}
void v59_narrow(v59_context*c,int kind,int index,int stationary){trace_work*t=&c->t;const surface*s;float previous;int hit,k;
 if(kind==1){const brush*b=&c->w->brushes[index];if(!stationary){if(!t->capsule&&!t->point)C1BB14_Sweep(c->w,b,t);else brush_sweep(c->w,b,t);return;}
  if(brush_inside(b,t)){t->fraction=0;t->allsolid=t->startsolid=1;t->contents=b->contents;}return;}
 s=&c->w->surfaces[index];
 if(!t->capsule&&!t->point){
  /* Only a proved miss may skip an unimplemented surface primitive. Include
   * native contact epsilon in the conservative broadphase comparison. */
  for(k=0;k<3;k++)if(fs(t->bmins[k],.125f)>s->bounds[k+3]||fa(t->bmaxs[k],.125f)<s->bounds[k])return;
  c->unsupported=1;return;
 }
 if(stationary){hit=s->type?mesh_inside(&s->m,t):curve_inside(&s->c,t);if(hit){t->fraction=0;t->allsolid=t->startsolid=1;}return;}
 for(k=0;k<3;k++)if(t->bmins[k]>s->bounds[k+3]||t->bmaxs[k]<s->bounds[k])return;
 previous=t->fraction;if(s->type)mesh_sweep(c,&s->m);else curve_sweep(&s->c,t);
 if(t->fraction<previous){t->flags=c->w->materials[s->material].flags;t->contents=s->contents;t->material=72u*(s->material+1);}
}
