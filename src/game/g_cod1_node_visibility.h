#ifndef G_COD1_NODE_VISIBILITY_H
#define G_COD1_NODE_VISIBILITY_H
#include <math.h>
#include <stdlib.h>
#include "g_cod1_dynamic_path_core.h"
/* Original6E800/6E6C0/6E750/6E8C0. Private candidate only. The two
 * triangles are DIFFERENT: direct visibility and visibility through a
 * depth-first, first-visit link neighborhood of budget256. This is not
 * shortest-path reachability. Failed queries never become clear traces.
 * Input nodes/links must be finalized physical order. PC53/nearest numeric
 * profile; no world-query, actor-perception or graph-publication authority. */
typedef int (*C1NVQuery)(void*,const float*,const float*,uint32_t,int*);
typedef struct {unsigned count,bytes;unsigned char*bits;} C1NVisibility;
static void C1NV_Free(C1NVisibility*v){if(v){free(v->bits);free(v);}}
static unsigned C1NV_Size(unsigned n){return (n*(n-1)+7)/8;}
static int C1NV_Get(const C1NVisibility*v,unsigned a,unsigned b,int neighborhood){
 unsigned bit,t;if(!v||!v->bits||a>=v->count||b>=v->count)return -1;
 if(a==b)return 1;if(a>b){t=a;a=b;b=t;}
 bit=neighborhood?(b-1)*v->count+a:a*v->count+b;
 return (v->bits[bit/8]>>(bit%8))&1;
}
static void C1NV_Set(C1NVisibility*v,unsigned bit){v->bits[bit/8]|=(unsigned char)(1u<<(bit%8));}
static int C1NV_Eye(const unsigned char*r,float*out){
 uint32_t type;uint16_t flags;float p[3],yaw,angleOffset=0;
 memcpy(&type,r+0x24,4);memcpy(&flags,r+0x2a,2);memcpy(p,r+0x38,12);memcpy(&yaw,r+0x44,4);
 if(type>17||!isfinite(yaw)||fabsf(yaw)>360000)return 0;
 if(!isfinite(p[0])||!isfinite(p[1])||!isfinite(p[2])||fabsf(p[0])>1e7f||fabsf(p[1])>1e7f||fabsf(p[2])>1e7f)return 0;
 if(flags&0x8000){if(type==5||type==7)angleOffset=-90;else if(type==6||type==8)angleOffset=90;}
 if(angleOffset){
#if defined(_MSC_VER)&&defined(_M_IX86)
  float factor=0.01745329238474369f,radius=44,sv,cv,radians;
  /* Keep the original intermediate stores and x87 multiply/add ordering. */
  __asm fld yaw
  __asm fadd angleOffset
  __asm fmul factor
  __asm fstp radians
  __asm fld radians
  __asm fsincos
  __asm fstp cv
  __asm fstp sv
  __asm fld radius
  __asm fmul cv
  __asm fadd p[0]
  __asm fstp p[0]
  __asm fld radius
  __asm fmul sv
  __asm fadd p[4]
  __asm fstp p[4]
#else
  return 0; /* Unproved sincos provider is not silently substituted. */
#endif
 }
 p[2]=(float)((double)p[2]+64.0);memcpy(out,p,12);return 1;
}
typedef struct {unsigned node,next;float budget;} C1NVFrame;
/* Iterative equivalent of original recursive traversal, preserving link order
 * and the global first-visit set. Zero length edges cannot create recursion. */
static unsigned C1NV_Neighbors(unsigned root,const C1DPNode*nodes,const C1DPLink*links,unsigned n,unsigned*out,C1NVFrame*stack){
 unsigned used=1,depth=1;unsigned char seen[2048]={0};out[0]=root;seen[root]=1;stack[0].node=root;stack[0].next=0;stack[0].budget=256;
 while(depth){C1NVFrame*f=&stack[depth-1];const C1DPNode*node=&nodes[f->node];const C1DPLink*l;float distance,remaining;
  if(f->next>=node->count){depth--;continue;}l=&links[node->first+f->next++];memcpy(&distance,&l->payload[1],4);
  if(distance>f->budget||seen[l->target])continue;
  remaining=(float)((double)f->budget-distance);seen[l->target]=1;out[used++]=l->target;
  if(used>n||depth>=n)return 0;stack[depth].node=l->target;stack[depth].next=0;stack[depth].budget=remaining;depth++;
 }return used;
}
static C1NVisibility*C1NV_Build(const unsigned char*records,size_t bytes,const C1DPNode*nodes,unsigned n,const C1DPLink*links,unsigned nl,C1NVQuery query,void*ctx){
 C1NVisibility*v=NULL;float(*eyes)[3]=NULL;C1NVFrame*stack=NULL;unsigned*a=NULL,*b=NULL,i,j,k,end=0,an,bn;int blocked,visible;
 if(!records||!nodes||!query||!n||n>2048||bytes<(size_t)n*116||(nl&&!links))return NULL;
 for(i=0;i<n;i++){if(nodes[i].first!=end||nodes[i].active>nodes[i].count||nodes[i].count>32767||nodes[i].count>nl-end)return NULL;
  for(j=0;j<nodes[i].count;j++){float d;const C1DPLink*l=&links[end+j];memcpy(&d,&l->payload[1],4);if(l->target>=n||!isfinite(d)||d<0)return NULL;}end+=nodes[i].count;
 }if(end!=nl)return NULL;
 v=calloc(1,sizeof(*v));if(!v)return NULL;v->count=n;v->bytes=C1NV_Size(n);v->bits=calloc(v->bytes?v->bytes:1,1);
 eyes=malloc(n*sizeof(*eyes));stack=malloc(n*sizeof(*stack));a=malloc(n*sizeof(*a));b=malloc(n*sizeof(*b));
 if(!v->bits||!eyes||!stack||!a||!b)goto fail;
 for(i=0;i<n;i++)if(!C1NV_Eye(records+i*116,eyes[i]))goto fail;
 for(i=0;i<n;i++)for(j=i+1;j<n;j++){
  blocked=-1;if(!query(ctx,eyes[i],eyes[j],0x0280d003u,&blocked)||(blocked!=0&&blocked!=1))goto fail;
  if(!blocked){C1NV_Set(v,i*n+j);C1NV_Set(v,(j-1)*n+i);}
 }
 for(i=0;i<n;i++){
  an=C1NV_Neighbors(i,nodes,links,n,a,stack);if(!an)goto fail;
  for(j=i+1;j<n;j++){
   if(C1NV_Get(v,i,j,1))continue;bn=C1NV_Neighbors(j,nodes,links,n,b,stack);if(!bn)goto fail;
   visible=0;for(k=0;k<an&&!visible;k++){unsigned z;for(z=0;z<bn;z++)if(a[k]==b[z]||C1NV_Get(v,a[k],b[z],0)){visible=1;break;}}
   if(visible)C1NV_Set(v,(j-1)*n+i);
  }
 }
 free(eyes);free(stack);free(a);free(b);return v;
fail:free(eyes);free(stack);free(a);free(b);C1NV_Free(v);return NULL;
}
#endif
