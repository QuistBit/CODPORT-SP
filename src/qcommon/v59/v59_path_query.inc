/* Scoped path-query entry: original vertical path hulls, immutable world,
 * private caller-owned context. No movement-trace result substitution. */
#include "v59_path_mesh.h"
#include "v59_path_curve.h"
static int v59_path_surface(void*context,const C1PathSight*t,const surface*s,int*out){
 v59_context*c=context;
 *out=s->type?C1VMMesh(t,&s->m,c->edgeseen,c->vertexseen):C1VCCurve(t,&s->c);
 return 1;
}
static int v59_path_value(const float*v){
 unsigned k,u;for(k=0;k<3;k++){memcpy(&u,v+k,4);u&=0x7fffffffu;if(u>0x49742400u||(u&&u<0x00800000u))return 0;}return 1;
}
static int v59_path_alias(const void*a,size_t n,const void*b,size_t m){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<n:x-y<m;
}
static int v59_path_native(v59_context*c,const float*s,const float*e,const float*mins,const float*maxs,const float*origin,uint32_t mask,int model_id,int*out){
 C1VSQuery q;int result=0,k;const model*m;
 if(!c||!c->w||!s||!e||!mins||!maxs||!origin||!out||model_id<0||model_id>=c->w->nmodels||model_id==510)return 0;
 if(!v59_path_value(s)||!v59_path_value(e)||!v59_path_value(origin))return 0;
 if(mins[0]!=-15||mins[1]!=-15||(mins[2]!=17&&mins[2]!=18)||maxs[0]!=15||maxs[1]!=15||maxs[2]!=72)return 0;
 if(mask!=0x820011u&&mask!=0x02820011u)return 0;
 if(!model_id&&(origin[0]!=0||origin[1]!=0||origin[2]!=0))return 0;
 if(v59_path_alias(out,4,s,12)||v59_path_alias(out,4,e,12)||v59_path_alias(out,4,mins,12)||v59_path_alias(out,4,maxs,12)||v59_path_alias(out,4,origin,12)||v59_path_alias(out,4,c,sizeof(*c)))return 0;
 memset(&q,0,sizeof(q));q.w=c->w;q.mask=mask;q.brushseen=c->brushseen;q.surfaceseen=c->surfaceseen;q.surfaceQuery=v59_path_surface;q.context=c;
 if(!model_id){if(!C1VSWorld(&q,s,e,mins,maxs,&result))return 0;}
 else{
  C1PS_Prepare(&q.shape,s,e,mins,maxs,origin);m=c->w->models+model_id;
  memset(q.brushseen,0,c->w->nb);memset(q.surfaceseen,0,c->w->ns);
  for(k=0;k<(int)m->ranges[5];k++){uint32_t id=m->ranges[4]+k;result=C1VSVisit(&q,&id,1,NULL,0);if(result||q.failure)break;}
  if(!result&&!q.failure)result=C1VSVisit(&q,NULL,0,c->w->surfacerefs+m->ranges[2],m->ranges[3]);
  if(q.failure)return 0;
 }
 *out=result;return 1;
}
V59_API int v59_path_sight(v59_context*c,const float*s,const float*e,const float*mins,const float*maxs,const float*origin,uint32_t mask,int model_id,int*out){
 int status;
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char saved[108],discard[108];unsigned mx,nearMx=0x1f80;unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 status=v59_path_native(c,s,e,mins,maxs,origin,mask,model_id,out);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
#else
 status=v59_path_native(c,s,e,mins,maxs,origin,mask,model_id,out);
#endif
 return status;
}
