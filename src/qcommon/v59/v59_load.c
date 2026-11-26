/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "v59_internal.h"
#include "v59_mesh_build_math.h"
typedef struct {const unsigned char *data;size_t size;v59_world*w;jmp_buf fail;char *err;size_t errsize;} loader;
static void bad(loader*l,const char*s){if(l->errsize)snprintf(l->err,l->errsize,"%s",s);longjmp(l->fail,1);}
static void *take(loader*l,size_t n,size_t size){allocation*a;void*p;
 if(n && size>SIZE_MAX/n)bad(l,"allocation overflow");
 a=malloc(sizeof(*a));if(!a)bad(l,"allocation failed");p=calloc(n?n:1,size);
 if(!p){free(a);bad(l,"allocation failed");}a->ptr=p;a->next=l->w->allocs;l->w->allocs=a;return p;}
static uint32_t u32(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t u16(const unsigned char*p){return (uint16_t)(p[0]|p[1]<<8);}
static float fl(loader*l,const unsigned char*p){uint32_t v=u32(p);float f;memcpy(&f,&v,4);if(!isfinite(f))bad(l,"nonfinite BSP geometry");return f;}
static const unsigned char *lump(loader*l,int i,size_t stride,int *count){uint32_t n=u32(l->data+8+i*8),p=u32(l->data+12+i*8);
 if(p>l->size || n>l->size-p || n%stride || n/stride>INT_MAX)bad(l,"invalid BSP lump range or stride");*count=(int)(n/stride);return l->data+p;}
static void range(loader*l,uint32_t first,uint32_t count,int limit){if(first>(unsigned)limit || count>(unsigned)limit-first)bad(l,"BSP index range outside lump");}
static void finite_vec(loader*l,const unsigned char*p,float *v,int n){int k;for(k=0;k<n;k++)v[k]=fl(l,p+k*4);}
static void build_mesh(loader*l,surface*s,const float(*v)[3],int nv,const uint16_t*indices,int ni){
 typedef struct {int a,b,opposite;} edge_index;
 edge_index*ei;int *remap,ne=0,i,j,k,a,b,c,d,found,down=0,up=0;float A[3],B[3],N[3],D[3],D2[3];mesh*m=&s->m;
 /* Stock Berlin/Red Square/Train Station/MP Brecourt contain empty mesh
  * records. Keep their surface IDs but do not invent collision triangles. */
 if(ni%3)bad(l,"invalid triangle index count");
 for(i=0;i<ni;i++)if(indices[i]>=nv)bad(l,"mesh vertex outside slice");
 m->nt=ni/3;ei=take(l,ni,sizeof(*ei));remap=take(l,nv,sizeof(*remap));for(i=0;i<nv;i++)remap[i]=-1;
 for(i=0;i<ni;i+=3)for(j=0;j<3;j++){
  a=indices[i+j];b=indices[i+(j+1)%3];c=indices[i+(j+2)%3];if(a==b)bad(l,"zero length mesh edge");
  for(found=0;found<ne;found++)if((ei[found].a==a&&ei[found].b==b)||(ei[found].a==b&&ei[found].b==a))break;
  if(found==ne){ei[ne].a=a;ei[ne].b=b;ei[ne++].opposite=c;continue;}
  d=ei[found].opposite;minus(v[b],v[c],A);minus(v[a],v[c],B);cross(A,B,N);minus(v[d],v[c],D);
  if(dot(N,D)<=0)continue;minus(v[d],v[a],D);minus(v[d],v[b],D2);
  if(dot(N,N)>=fm(fmaxf(dot(D,D),dot(D2,D2)),6.4e-9f))continue;
  ei[found]=ei[--ne];
 }
 m->ne=ne;for(i=0;i<ne;i++){if(remap[ei[i].a]<0)remap[ei[i].a]=m->nv++;if(remap[ei[i].b]<0)remap[ei[i].b]=m->nv++;}
 m->tri=take(l,m->nt,sizeof(*m->tri));m->edges=take(l,ne,sizeof(*m->edges));m->vertices=take(l,m->nv,sizeof(*m->vertices));
 for(i=0;i<nv;i++)if(remap[i]>=0)cp(v[i],m->vertices[remap[i]]);
 for(i=0;i<m->nt;i++){
  triangle*t=&m->tri[i];float u[3],q[3],us[3],vs[3],lu,lv,uv,factor,length;int tri[3];
  for(j=0;j<3;j++)tri[j]=indices[i*3+j];a=tri[0];b=tri[1];c=tri[2];
  minus(v[c],v[a],A);minus(v[b],v[a],B);C1MLCross(A,B,N);length=C1MLNorm(N);if(!length||!isfinite(length))bad(l,"degenerate or overflowing triangle");
  cp(N,t->plane);t->plane[3]=C1MLStore(C1MLDot(N,v[a],0,1,2));if(N[2]<-.001f)down=1;if(N[2]>.001f)up=1;
  minus(v[b],v[a],u);lu=C1MLNorm(u);minus(v[c],v[a],q);lv=C1MLNorm(q);
  C1MLBary(v[a],u,q,lu,lv,t->bary);
  for(j=0;j<2;j++)for(k=0;k<4;k++)if(!isfinite(t->bary[j][k]))bad(l,"singular mesh barycentric basis");
  for(j=0;j<3;j++){
   int e0=tri[j==0?1:0],e1=tri[j==2?1:2];t->vertices[j]=remap[tri[j]];t->edges[j]=-1;
   /* Dictionary semantics from the reference: the last duplicate wins. */
   for(k=0;k<ne;k++)if((ei[k].a==e0&&ei[k].b==e1)||(ei[k].a==e1&&ei[k].b==e0))t->edges[j]=k;
  }
 }
 m->down=down&&!up;
 for(i=0;i<ne;i++){
  edge*e=&m->edges[i];float axis[3]={0},inv,projection[3],d0;cp(v[ei[i].a],e->origin);
  minus(v[ei[i].b],e->origin,e->direction);e->length=C1MLNorm(e->direction);if(!e->length||!isfinite(e->length))bad(l,"zero or overflowing mesh edge");
  C1MLBasis(e->direction,e->basis);
 }
 for(k=0;k<3;k++){s->bounds[k]=262144;s->bounds[k+3]=-262144;for(i=0;i<nv;i++){if(v[i][k]<s->bounds[k])s->bounds[k]=v[i][k];if(v[i][k]>s->bounds[k+3])s->bounds[k+3]=v[i][k];}}
 if(ne>l->w->maxedges)l->w->maxedges=ne;if(m->nv>l->w->maxvertices)l->w->maxvertices=m->nv;
}
/* Each job owns its failure boundary and CRT allocation list. In particular it
 * never links allocations into the shared world and never calls the legacy
 * curve builder. Keep build_mesh itself unchanged: topology, edge ordering and
 * original floating-point operation order are identical in both modes. */
#define V59_MESH_BATCH 64
typedef struct {
 loader load;v59_world owned;surface *out;const unsigned char *record;
 const float (*vertices)[3];const uint16_t *indices;const material *materials;
 int nv,ni,nm,done,failed;char error[128];
} mesh_job;
static void build_mesh_job(mesh_job *job){
 loader *l=&job->load;surface *s=job->out;const unsigned char *p=job->record;
 int a,b;uint32_t d,e;
 if(setjmp(l->fail)){job->failed=1;job->done=1;return;}
 a=u16(p+4);b=u16(p+6);d=u32(p+8);e=u32(p+12);
 s->material=u16(p);s->type=p[2];
 if(s->material>=job->nm)bad(l,"surface material outside lump");
 s->contents=job->materials[s->material].contents;
 range(l,d,a,job->nv);range(l,e,b,job->ni);
 build_mesh(l,s,job->vertices+d,a,job->indices+e,b);
 job->done=1;
}
static int build_mesh_jobs(void *context,unsigned first,unsigned end){
 mesh_job *jobs=(mesh_job*)context;unsigned i;
 for(i=first;i<end;i++)if(!jobs[i].done)build_mesh_job(jobs+i);
 /* A malformed mesh is recorded in its own slot. Let every dispatched mesh
  * finish so the loading thread can select the first error in source order. */
 return 1;
}
static void build_mesh_batch(loader *l,const unsigned char *records,int first,int count,
 const float (*vertices)[3],int nv,const uint16_t *indices,int ni,v59_parallel_fn dispatch){
 mesh_job jobs[V59_MESH_BATCH];int i;allocation *tail;
 memset(jobs,0,sizeof(jobs));
 for(i=0;i<count;i++){
  mesh_job *j=jobs+i;j->load.w=&j->owned;j->load.err=j->error;j->load.errsize=sizeof(j->error);
  j->out=l->w->surfaces+first+i;j->record=records+(first+i)*16;
  j->vertices=vertices;j->indices=indices;j->materials=l->w->materials;
  j->nv=nv;j->ni=ni;j->nm=l->w->nm;
 }
 (void)dispatch((unsigned)count,1,build_mesh_jobs,jobs);
 /* Synchronous dispatch may decline or finish only part of the batch. Retain
  * completed work and finish missing slots exactly once; never replay an
  * owned allocation list or a partially constructed mesh. */
 build_mesh_jobs(jobs,0,(unsigned)count);
 for(i=0;i<count;i++){
  v59_world *owned=&jobs[i].owned;
  if(owned->allocs){
   for(tail=owned->allocs;tail->next;tail=tail->next);
   tail->next=l->w->allocs;l->w->allocs=owned->allocs;owned->allocs=NULL;
  }
  if(owned->maxedges>l->w->maxedges)l->w->maxedges=owned->maxedges;
  if(owned->maxvertices>l->w->maxvertices)l->w->maxvertices=owned->maxvertices;
 }
 /* Ownership has already moved to l->w, including partially built failures,
  * before its normal longjmp cleanup can free the complete world. */
 for(i=0;i<count;i++)if(jobs[i].failed)bad(l,jobs[i].error);
}
static int depth_check(loader*l,int i,unsigned char*color,int *depth){int j,d=0;if(i<0)return 0;
 if(color[i]==1)bad(l,"cyclic BSP tree");if(color[i]==2)return depth[i];color[i]=1;
 /* Limit recursion before descending: cycles and pathological chains rejected. */
 if(++l->w->maxdepth>1024)bad(l,"BSP tree deeper than 1024");
 for(j=1;j<=2;j++){int v=depth_check(l,l->w->nodes[i].v[j],color,depth);if(v>d)d=v;}
 --l->w->maxdepth;color[i]=2;return depth[i]=d+1;
}
V59_API void v59_free(v59_world*w){allocation*a;int i;if(!w)return;
 if(w->surfaces)for(i=0;i<w->ns;i++){free(w->surfaces[i].c.planes);free(w->surfaces[i].c.facets);}
 while(w->allocs){a=w->allocs;w->allocs=a->next;free(a->ptr);free(a);}free(w);}
static v59_world *v59_load_native(const void *data,size_t size,char *error,size_t error_size,v59_parallel_fn dispatch){
 loader*l=calloc(1,sizeof(*l));v59_world*w;const unsigned char *p,*bsides,*bbrushes;int i,j,k,count,nside,nv,ni;uint32_t first=0;float(*verts)[3];uint16_t*indices;
 if(error&&error_size)error[0]=0;
 if(!l)return NULL;l->data=data;l->size=size;l->err=error;l->errsize=error?error_size:0;
 l->w=calloc(1,sizeof(*l->w));if(!l->w){free(l);return NULL;}
 if(setjmp(l->fail)){v59_free(l->w);free(l);return NULL;}w=l->w;
 if(!data||size<272||memcmp(data,"IBSP",4)||u32(l->data+4)!=59)bad(l,"expected complete IBSP v59 header");
 for(i=0;i<33;i++)lump(l,i,1,&count);
 p=lump(l,0,72,&w->nm);w->materials=take(l,w->nm,sizeof(*w->materials));for(i=0;i<w->nm;i++){w->materials[i].flags=u32(p+i*72+64);w->materials[i].contents=u32(p+i*72+68);}
 p=lump(l,2,16,&w->np);w->planes=take(l,w->np,sizeof(*w->planes));for(i=0;i<w->np;i++)finite_vec(l,p+i*16,w->planes[i],4);
 bsides=lump(l,3,8,&nside);bbrushes=lump(l,4,4,&w->nb);w->brushes=take(l,w->nb,sizeof(*w->brushes));
 for(i=0;i<w->nb;i++){
  brush*b=&w->brushes[i];int material_id=u16(bbrushes+i*4+2);b->n=u16(bbrushes+i*4);
  range(l,first,b->n,nside);if(b->n<6||material_id>=w->nm)bad(l,"invalid brush");b->contents=w->materials[material_id].contents&0xdfff7ffbu;
  b->planes=take(l,b->n,sizeof(*b->planes));
  for(j=0;j<6;j++){
   int axis=j%3,parity=j/3;const unsigned char*side=bsides+(first+2*axis+parity)*8;brush_plane*t=&b->planes[j];
   t->bound=fl(l,side);t->axis=axis;t->sign=parity?1:-1;t->n[axis]=t->sign;t->d=fm(t->sign,t->bound);t->material=u32(side+4);
   if(t->material<0||t->material>=w->nm)bad(l,"brush side material outside lump");if(parity)b->maxs[axis]=t->bound;else b->mins[axis]=t->bound;
  }
  for(k=0;k<3;k++)if(b->mins[k]>b->maxs[k])bad(l,"inverted brush bounds");
  for(j=6;j<b->n;j++){
   const unsigned char*side=bsides+(first+j)*8;uint32_t pi=u32(side),mi=u32(side+4);brush_plane*t=&b->planes[j];
   if(pi>=(unsigned)w->np||mi>=(unsigned)w->nm)bad(l,"brush side outside lump");cp(w->planes[pi],t->n);t->d=w->planes[pi][3];t->material=mi;t->axis=-1;
  }first+=b->n;
 }if(first!=(unsigned)nside)bad(l,"unclaimed brush sides");
 p=lump(l,25,12,&nv);verts=take(l,nv,sizeof(*verts));for(i=0;i<nv;i++)finite_vec(l,p+i*12,verts[i],3);
 p=lump(l,26,2,&ni);indices=take(l,ni,sizeof(*indices));for(i=0;i<ni;i++)indices[i]=u16(p+i*2);
 p=lump(l,24,16,&w->ns);w->surfaces=take(l,w->ns,sizeof(*w->surfaces));
 for(i=0;i<w->ns;i++){
  if(dispatch && p[i*16+2]==1){
   int end=i+1;
   while(end<w->ns && end-i<V59_MESH_BATCH && p[end*16+2]==1)end++;
   if(end-i>1){
    build_mesh_batch(l,p,i,end-i,(const float(*)[3])verts,nv,indices,ni,dispatch);
    i=end-1;continue;
   }
  }
  surface*s=&w->surfaces[i];int a=u16(p+i*16+4),b=u16(p+i*16+6);uint32_t d=u32(p+i*16+8),e=u32(p+i*16+12);s->material=u16(p+i*16);s->type=p[i*16+2];
  if(s->material>=w->nm)bad(l,"surface material outside lump");s->contents=w->materials[s->material].contents;
  if(s->type==1){range(l,d,a,nv);range(l,e,b,ni);build_mesh(l,s,(const float(*)[3])(verts+d),a,indices+e,b);}
  else if(s->type==0){range(l,e,(uint32_t)a*b,nv);if(d>INT_MAX)bad(l,"invalid curve subdivision tolerance");
   if(v59_curve_build(a,b,(int)d,(const float*)(verts+e),&s->c,s->bounds,error,error_size))longjmp(l->fail,1);}
  else bad(l,"unknown collision surface type");
 }
 p=lump(l,22,4,&w->nbr);w->brushrefs=take(l,w->nbr,4);for(i=0;i<w->nbr;i++){w->brushrefs[i]=u32(p+i*4);if(w->brushrefs[i]>=(unsigned)w->nb)bad(l,"leaf brush outside lump");}
 p=lump(l,23,4,&w->nsr);w->surfacerefs=take(l,w->nsr,4);for(i=0;i<w->nsr;i++){w->surfacerefs[i]=u32(p+i*4);if(w->surfacerefs[i]>=(unsigned)w->ns)bad(l,"leaf surface outside lump");}
 p=lump(l,21,36,&w->nl);w->leaves=take(l,w->nl,sizeof(node));for(i=0;i<w->nl;i++){for(j=0;j<9;j++)w->leaves[i].v[j]=(int32_t)u32(p+i*36+j*4);
  range(l,w->leaves[i].v[4],w->leaves[i].v[5],w->nbr);range(l,w->leaves[i].v[2],w->leaves[i].v[3],w->nsr);}
 p=lump(l,20,36,&w->nn);w->nodes=take(l,w->nn,sizeof(node));if(w->nn==0)bad(l,"missing BSP root node");
 for(i=0;i<w->nn;i++){for(j=0;j<9;j++)w->nodes[i].v[j]=(int32_t)u32(p+i*36+j*4);if(w->nodes[i].v[0]<0||w->nodes[i].v[0]>=w->np)bad(l,"node plane outside lump");
  for(j=1;j<3;j++){int n=w->nodes[i].v[j];if(n>=w->nn||n==INT_MIN||(n<0&&-1-n>=w->nl))bad(l,"node child outside lump");}}
 {unsigned char *color=take(l,w->nn,1);int*depth=take(l,w->nn,sizeof(int));int*parents=take(l,w->nn,sizeof(int));
  for(i=0;i<w->nn;i++)for(j=1;j<3;j++){int n=w->nodes[i].v[j];if(n>=0&&++parents[n]>1)bad(l,"shared BSP node is not a tree");}
  for(i=0;i<w->nn;i++)depth_check(l,i,color,depth);}

 p=lump(l,27,48,&w->nmodels);w->models=take(l,w->nmodels,sizeof(*w->models));if(!w->nmodels)bad(l,"missing world model");
 for(i=0;i<w->nmodels;i++){model*m=&w->models[i];finite_vec(l,p+i*48,m->bounds,6);for(j=0;j<6;j++)m->ranges[j]=u32(p+i*48+24+j*4);
  range(l,m->ranges[4],m->ranges[5],w->nb);range(l,m->ranges[2],m->ranges[3],w->nsr);}
 free(l);return w;
}
/* Construction is original engine PC53, round-to-nearest. The caller may
 * be a renderer with PC24/FTZ or retain live x87 values. Preserve its complete
 * x87 state and MXCSR on success and on every checked loader failure. */
V59_API v59_world *v59_load_parallel(const void *data,size_t size,char *error,size_t error_size,v59_parallel_fn dispatch){
 v59_world *result;
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char saved[108],discard[108];unsigned mx,nearMx=0x1f80;unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 result=v59_load_native(data,size,error,error_size,dispatch);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
#else
 result=v59_load_native(data,size,error,error_size,dispatch);
#endif
 return result;
}
V59_API v59_world *v59_load(const void *data,size_t size,char *error,size_t error_size){
 return v59_load_parallel(data,size,error,error_size,NULL);
}
V59_API int v59_model_count(const v59_world*w){return w?w->nmodels:0;}

V59_API int v59_point_contents(const v59_world*w,const float point[3],int model_id,uint32_t *contents){
 int node_id=0,first=0,count=0,i,j;const brush*b;const node*n;
 if(!w||!point||!contents||model_id<0||model_id>=w->nmodels||model_id==510)return -1;
 for(i=0;i<3;i++)if(!isfinite(point[i]))return -1;
 *contents=0;
 if(model_id){first=(int)w->models[model_id].ranges[4];count=(int)w->models[model_id].ranges[5];}
 else{
  while(node_id>=0){const float*p;
   n=&w->nodes[node_id];p=w->planes[n->v[0]];
   node_id=n->v[dp(point,p,0,1,2)<p[3]?2:1];
  }
  n=&w->leaves[-1-node_id];first=n->v[4];count=n->v[5];
 }
 for(i=0;i<count;i++){
  int brush_id=model_id?first+i:(int)w->brushrefs[first+i];
  b=&w->brushes[brush_id];
  for(j=0;j<b->n;j++)if(dp(point,b->planes[j].n,0,1,2)>b->planes[j].d)break;
  if(j==b->n)*contents|=b->contents;
 }
 return 0;
}

/* Read-only export: use the loader's checked tables, never parse BSP a second time. */
V59_API int v59_model_brush_range(const v59_world *w,int id,int *first,int *count,int *surfaces) {
 if(!w||id<0||id>=w->nmodels||!first||!count||!surfaces)return -1;
 *first=(int)w->models[id].ranges[4];*count=(int)w->models[id].ranges[5];
 *surfaces=(int)w->models[id].ranges[3];return 0;
}
V59_API int v59_get_brush(const v59_world *w,int id,v59_brush_info *out) {
 const brush *b;if(!w||id<0||id>=w->nb||!out)return -1;b=&w->brushes[id];
 cp(b->mins,out->mins);cp(b->maxs,out->maxs);out->contents=b->contents;out->sides=b->n;return 0;
}
V59_API int v59_get_brush_side(const v59_world *w,int id,int side,v59_brush_side *out) {
 const brush_plane *p;if(!w||id<0||id>=w->nb||!out||side<0||side>=w->brushes[id].n)return -1;
 p=&w->brushes[id].planes[side];cp(p->n,out->normal);out->distance=p->d;
 out->surface_flags=w->materials[p->material].flags;return 0;
}

V59_API int v59_model_surface(const v59_world *w,int id,int ordinal) {
 const model *m;if(!w||id<0||id>=w->nmodels||ordinal<0)return -1;m=&w->models[id];
 if((uint32_t)ordinal>=m->ranges[3])return -1;return (int)w->surfacerefs[m->ranges[2]+ordinal];
}
V59_API int v59_get_surface(const v59_world *w,int id,v59_surface_info *out) {
 const surface *s;if(!w||id<0||id>=w->ns||!out)return -1;s=&w->surfaces[id];
 memcpy(out->bounds,s->bounds,sizeof(out->bounds));out->contents=s->contents;
 out->flags=w->materials[s->material].flags;out->type=s->type;
 out->planes=s->c.np;out->facets=s->c.nf;return 0;
}
V59_API int v59_get_curve_plane(const v59_world *w,int id,int i,v59_curve_plane *out) {
 const curve_plane *p;if(!w||id<0||id>=w->ns||!out||w->surfaces[id].type||i<0||i>=w->surfaces[id].c.np)return -1;
 p=&w->surfaces[id].c.planes[i];memcpy(out->plane,p->p,sizeof(out->plane));out->signbits=p->signbits;return 0;
}
V59_API int v59_get_curve_facet(const v59_world *w,int id,int i,v59_curve_facet *out) {
 const facet *f;if(!w||id<0||id>=w->ns||!out||w->surfaces[id].type||i<0||i>=w->surfaces[id].c.nf)return -1;
 f=&w->surfaces[id].c.facets[i];out->surface=f->plane;out->numBorders=f->n;
 memcpy(out->border,f->planes,sizeof(out->border));memcpy(out->inward,f->inward,sizeof(out->inward));
 memcpy(out->noAdjust,f->noadjust,sizeof(out->noAdjust));return 0;
}
