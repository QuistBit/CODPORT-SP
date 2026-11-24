/* Original53: world sight first, then current linked entities with the native
 * two-pass owner exclusions. A trace fraction is not a sight result.
 * Rotated inline models and target capsules remain explicit unavailable
 * domains; no such candidate is silently treated as empty space. */
#include "../qcommon/cod1_path_sight.h"
static int CS_PathWorldNative(const cod1SpatialTraceQuery_t*query,int*out){
 cod1SpatialTraceQuery_t q;cod1SpatialArea_t area;float low[3],high[3];
 int result=0,status,i,k;
 if(!query||!out)return COD1_SPATIAL_INVALID;
 {uintptr_t a=(uintptr_t)query,b=(uintptr_t)out;if(a<=b?b-a<sizeof(*query):a-b<sizeof(*out))return COD1_SPATIAL_INVALID;}
 q=*query;
 if(q.version!=COD1_SPATIAL_VERSION||q.bytes!=sizeof(q)||!CS_Finite(q.start)||!CS_Finite(q.end)||
    q.mins[0]!=-15||q.mins[1]!=-15||(q.mins[2]!=17&&q.mins[2]!=18)||
    q.maxs[0]!=15||q.maxs[1]!=15||q.maxs[2]!=72||
    (q.contentsMask!=0x00820011u&&q.contentsMask!=0x02820011u))return COD1_SPATIAL_INVALID;
 if(!CS_TableReady()||!CM_HasV59()||!sv_numworldSectors)return COD1_SPATIAL_UNAVAILABLE;
 if(!CS_Passes(q.passEntity))return COD1_SPATIAL_INVALID;
 for(k=0;k<3;k++)if(fabs(q.start[k])>1000000||fabs(q.end[k])>1000000)return COD1_SPATIAL_INVALID;
 if(!CM_Cod1PathNativeSight(q.start,q.end,q.mins,q.maxs,vec3_origin,0,q.contentsMask,&result))return COD1_SPATIAL_UNAVAILABLE;
 if(result){*out=result;return COD1_SPATIAL_OK;}
 memset(&area,0,sizeof(area));
 for(k=0;k<3;k++){
  low[k]=C1PS_Store((double)(q.start[k]<q.end[k]?q.start[k]:q.end[k])+q.mins[k]-1.0);
  high[k]=C1PS_Store((double)(q.start[k]>q.end[k]?q.start[k]:q.end[k])+q.maxs[k]+1.0);
 }
 area.mins=low;area.maxs=high;area.mask=q.contentsMask;
 status=CS_Area_r(sv_worldSectors,&area);if(status!=COD1_SPATIAL_OK)return status;
 for(i=0;i<area.count;i++){
  int slot=area.slots[i];sharedEntity_t*ent=SV_GentityNum(slot);uint32_t contents;
  if(CS_Ignored(slot,ent,q.passEntity))continue;
  status=CS_Contents(slot,ent,&contents);if(status!=COD1_SPATIAL_OK)return status;
  if(!(contents&q.contentsMask))continue;
  if(!ent->r.linked||!CS_Finite(ent->r.currentOrigin))return COD1_SPATIAL_STALE;
  for(k=0;k<3;k++)if(fabs(ent->r.currentOrigin[k])>1000000)return COD1_SPATIAL_UNAVAILABLE;
  if(ent->r.bmodel){
   if(ent->s.modelindex<=0||ent->s.modelindex>=CM_NumInlineModels())return COD1_SPATIAL_INVALID;
   if(!CS_Finite(ent->r.currentAngles)||!VectorCompare(ent->r.currentAngles,vec3_origin))return COD1_SPATIAL_UNAVAILABLE;
   if(!CM_Cod1PathNativeSight(q.start,q.end,q.mins,q.maxs,ent->r.currentOrigin,ent->s.modelindex,q.contentsMask,&result))return COD1_SPATIAL_UNAVAILABLE;
  }else{
   C1PathSight shape;float enter,leave;
   int capsule=cod1SpatialActors[slot].active?(cod1SpatialActors[slot].flags&COD1_SPATIAL_NATIVE_CAPSULE_FLAG):(ent->r.svFlags&SVF_CAPSULE);
   if(capsule)return COD1_SPATIAL_UNAVAILABLE;
   if(!CS_Bounds(ent->r.mins,ent->r.maxs))return COD1_SPATIAL_INVALID;
   for(k=0;k<3;k++)if(fabs(ent->r.mins[k])>1000000||fabs(ent->r.maxs[k])>1000000)return COD1_SPATIAL_UNAVAILABLE;
   C1PS_Prepare(&shape,q.start,q.end,q.mins,q.maxs,ent->r.currentOrigin);
   result=C1PS_Axes(&shape,ent->r.mins,ent->r.maxs,&enter,&leave);
  }
  if(result){*out=-1;return COD1_SPATIAL_OK;}
 }
 *out=0;return COD1_SPATIAL_OK;
}
int SV_Cod1PathWorldSight(const cod1SpatialTraceQuery_t*q,int*out){
 int status;
#if defined(_MSC_VER) && defined(_M_IX86)
 unsigned char saved[108],discard[108];unsigned mx,nearMx=0x1f80;unsigned short cw=0x027f;
 __asm fnsave saved
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 status=CS_PathWorldNative(q,out);
 __asm fnsave discard
 __asm ldmxcsr mx
 __asm frstor saved
#else
 status=CS_PathWorldNative(q,out);
#endif
 return status;
}
