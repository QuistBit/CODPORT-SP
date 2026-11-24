/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "v59_internal.h"
#include "v59_path_query.inc"
static void event(v59_context*c,int kind,int i){if(c->nevents<c->eventcap){c->events[c->nevents][0]=kind;c->events[c->nevents++][1]=i;}}
static void visit(v59_context*c,const uint32_t*br,int nb,int firstbr,const uint32_t*sr,int ns,int stationary){int k,i;
 for(k=0;k<nb;k++){i=br?(int)br[k]:firstbr+k;if(c->brushseen[i])continue;c->brushseen[i]=1;
  if(!(c->w->brushes[i].contents&c->t.mask))continue;event(c,1,i);v59_narrow(c,1,i,stationary);if(c->unsupported||(stationary?c->t.allsolid:c->t.fraction==0))return;}
 for(k=0;k<ns;k++){i=(int)sr[k];if(c->surfaceseen[i])continue;c->surfaceseen[i]=1;
  if(!(c->w->surfaces[i].contents&c->t.mask))continue;event(c,2,i);v59_narrow(c,2,i,stationary);if(c->unsupported||(stationary?c->t.allsolid:c->t.fraction==0))return;}
}
static void leaf(v59_context*c,int i,int stationary){const int*l=c->w->leaves[i].v;event(c,0,i);
 visit(c,c->w->brushrefs+l[4],l[5],0,c->w->surfacerefs+l[2],l[3],stationary);}
static void walk(v59_context*c,int index,float f1,float f2,const float*p1,const float*p2){
 const int*n;const float*p;float d1,d2,offset,inv,near,far,span,mid[3];int axis,side,k;
 if(c->unsupported||c->t.fraction<=f1)return;if(index<0){leaf(c,-1-index,0);return;}
 n=c->w->nodes[index].v;p=c->w->planes[n[0]];for(axis=0;axis<3;axis++)if(p[axis]==1)break;
 if(axis<3){d1=fs(p1[axis],p[3]);d2=fs(p2[axis],p[3]);offset=c->t.size1[axis];}
 else {d1=fs(dp(p1,p,1,2,0),p[3]);d2=fs(dp(p2,p,1,2,0),p[3]);offset=2048;}
 if(d1>=fa(offset,1)&&d2>=fa(offset,1)){walk(c,n[1],f1,f2,p1,p2);return;}
 if(d1<fs(-offset,1)&&d2<fs(-offset,1)){walk(c,n[2],f1,f2,p1,p2);return;}
 if(d1<d2){side=1;inv=fd(1,fs(d1,d2));near=fm(fa(fs(d1,offset),.125f),inv);far=fm(fa(fa(d1,offset),.125f),inv);}
 else if(d1>d2){side=0;inv=fd(1,fs(d1,d2));near=fm(fa(fa(d1,offset),.125f),inv);far=fm(fs(fs(d1,offset),.125f),inv);}
 else {side=0;near=1;far=0;}
 near=fmaxf(0,fminf(1,near));far=fmaxf(0,fminf(1,far));span=fs(f2,f1);
 for(k=0;k<3;k++)mid[k]=fa(p1[k],fm(near,fs(p2[k],p1[k])));walk(c,n[1+side],f1,fa(f1,fm(span,near)),p1,mid);
 for(k=0;k<3;k++)mid[k]=fa(p1[k],fm(far,fs(p2[k],p1[k])));walk(c,n[1+(side^1)],fa(f1,fm(span,far)),f2,mid,p2);
}
static void collect(v59_context*c,int index,const float*mins,const float*maxs){
 const int*n;const float*p;float hi[3],lo[3];int k;
 if(index<0){c->gathered[c->nleaves++]=-1-index;return;}n=c->w->nodes[index].v;p=c->w->planes[n[0]];
 for(k=0;k<3;k++){hi[k]=p[k]<0?mins[k]:maxs[k];lo[k]=p[k]<0?maxs[k]:mins[k];}
 if(dp(hi,p,0,1,2)>=p[3])collect(c,n[1],mins,maxs);
 if(dp(lo,p,0,1,2)<p[3])collect(c,n[2],mins,maxs);
}
V59_API v59_context *v59_context_create(const v59_world*w){v59_context*c;if(!w)return NULL;c=calloc(1,sizeof(*c));if(!c)return NULL;c->w=w;
 c->brushseen=calloc(w->nb?w->nb:1,1);c->surfaceseen=calloc(w->ns?w->ns:1,1);c->edgeseen=calloc(w->maxedges?w->maxedges:1,1);c->vertexseen=calloc(w->maxvertices?w->maxvertices:1,1);
 c->eventcap=(size_t)w->nn*2+1+w->nb+w->ns;c->events=calloc(c->eventcap,sizeof(*c->events));c->gathered=calloc((size_t)w->nn*2+1,sizeof(int));
 if(!c->brushseen||!c->surfaceseen||!c->edgeseen||!c->vertexseen||!c->events||!c->gathered){v59_context_free(c);return NULL;}return c;
}
V59_API void v59_context_free(v59_context*c){if(!c)return;free(c->brushseen);free(c->surfaceseen);free(c->edgeseen);free(c->vertexseen);free(c->events);free(c->gathered);free(c);}
static int v59_query_shape(v59_context*c,const float*start,const float*end,const float*mins,const float*maxs,uint32_t mask,int model_id,float fraction,int box){
 trace_work*t;float center[3],lo[3],hi[3];int i,k,point=1;if(!c||!start||!end||!mins||!maxs||!isfinite(fraction)||fraction<0||fraction>1||model_id<0||model_id>=c->w->nmodels||model_id==510)return -1;
 for(k=0;k<3;k++){if(!isfinite(start[k])||!isfinite(end[k])||!isfinite(mins[k])||!isfinite(maxs[k])||mins[k]>maxs[k])return -1;if(mins[k]!=0||maxs[k]!=0)point=0;}
 if(!point){for(k=0;k<3;k++)if(mins[k]>=maxs[k])return -1;if(!box&&fs(maxs[0],mins[0])!=fs(maxs[1],mins[1]))return -1;}
 t=&c->t;memset(t,0,sizeof(*t));c->stationary=1;c->unsupported=0;
 for(k=0;k<3;k++){center[k]=fm(fa(mins[k],maxs[k]),.5f);t->size0[k]=fs(mins[k],center[k]);t->size1[k]=fs(maxs[k],center[k]);
  t->start[k]=fa(start[k],center[k]);t->end[k]=fa(end[k],center[k]);t->delta[k]=fs(t->end[k],t->start[k]);if(start[k]!=end[k])c->stationary=0;}
 t->delta2=dot(t->delta,t->delta);t->radius=fminf(t->size1[0],t->size1[2]);t->halfheight=t->size1[2];t->offset[2]=fs(t->halfheight,t->radius);
 for(i=0;i<8;i++)for(k=0;k<3;k++)t->corners[i][k]=(i&(1<<k))?t->size1[k]:t->size0[k];
 t->max_offset=fa(fa(t->size1[1],t->size1[2]),t->size1[0]);t->mask=mask;t->fraction=fraction;t->capsule=!box&&!point;t->point=point;
 for(k=0;k<3;k++){
  float a=t->start[k],b=t->end[k];lo[k]=b<a?b:a;hi[k]=b>a?b:a;
  t->bmins[k]=fs(fs(lo[k],fabsf(t->offset[k])),t->radius);t->bmaxs[k]=fa(fa(hi[k],fabsf(t->offset[k])),t->radius);
  if(!c->stationary)t->extents[k]=fa(fabsf(t->offset[k]),t->radius);
  if(box){t->bmins[k]=fa(lo[k],t->size0[k]);t->bmaxs[k]=fa(hi[k],t->size1[k]);t->extents[k]=t->size1[k];}
  if(!isfinite(t->delta[k])||!isfinite(t->bmins[k])||!isfinite(t->bmaxs[k]))return -1;
 }
 if(!isfinite(t->delta2)||!isfinite(t->max_offset)||(!point&&t->radius<=0)||t->radius<0)return -1;
 if(box&&(c->stationary||point))return -2;
 if(!c->stationary)t->point=point||t->max_offset==0;c->prep=*t;c->nevents=c->nleaves=0;
 memset(c->brushseen,0,c->w->nb);memset(c->surfaceseen,0,c->w->ns);
 if(model_id){const model*m=&c->w->models[model_id];event(c,3,model_id);visit(c,NULL,m->ranges[5],m->ranges[4],c->w->surfacerefs+m->ranges[2],m->ranges[3],c->stationary);}
 else if(c->stationary){for(k=0;k<3;k++){lo[k]=fs(fa(t->start[k],t->size0[k]),1);hi[k]=fa(fa(t->start[k],t->size1[k]),1);}collect(c,0,lo,hi);
  for(i=0;i<(int)c->nleaves&&i<1024;i++){leaf(c,c->gathered[i],1);if(t->allsolid)break;}}
 else walk(c,0,0,t->fraction,t->start,t->end);
 if(c->unsupported)return -2;
 for(k=0;k<3;k++)t->endpos[k]=fa(start[k],fm(t->delta[k],t->fraction));return 0;
}
V59_API int v59_query(v59_context*c,const float*s,const float*e,const float*mi,const float*ma,uint32_t mask,int model,float fraction){return v59_query_shape(c,s,e,mi,ma,mask,model,fraction,0);}
V59_API int v59_query_box(v59_context*c,const float*s,const float*e,const float*mi,const float*ma,uint32_t mask,int model,float fraction){return v59_query_shape(c,s,e,mi,ma,mask,model,fraction,1);}
V59_API const void *v59_prepared(const v59_context*c){return &c->prep;}
V59_API const void *v59_result(const v59_context*c){return &c->t;}
V59_API int v59_stationary(const v59_context*c){return c->stationary;}
V59_API size_t v59_event_count(const v59_context*c){return c->nevents;}
V59_API const int *v59_events(const v59_context*c){return (const int*)c->events;}
V59_API size_t v59_leaf_count(const v59_context*c){return c->nleaves;}
V59_API const int *v59_leaves(const v59_context*c){return c->gathered;}

V59_API void v59_read_hit(const v59_context *c,v59_hit *h) {
 const trace_work *t=&c->t;
 h->fraction=t->fraction;cp(t->endpos,h->end);cp(t->normal,h->normal);
 h->surface_flags=t->flags;h->contents=t->contents;
 h->startsolid=t->startsolid;h->allsolid=t->allsolid;
}
