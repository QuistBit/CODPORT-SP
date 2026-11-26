/* The model-light implementation captures settings before dispatching its
 * immutable jobs. Keep the original per-vertex fallback boundary here. */
static qboolean R_Cod1ShadeParallel(const trRefEntity_t *ent,int count,const vec4_t *xyz,const vec4_t *normal,byte *rgba){
 c1jFp before;int i;
 /* A batch reuses only immutable geometry and the caller's rounding mode.
  * Failure replays the scalar route from the exact original FP environment. */
 C1J_FpGet(&before);
 if(R_Cod1ShadeEntityVertices(ent,count,xyz,normal,rgba))return qtrue;
 C1J_FpRestore(&before,0,0);
 for(i=0;i<count;i++)if(!R_Cod1ShadeEntityVertex(ent,xyz[i],normal[i],rgba+i*4))break;
 return i==count;
}
