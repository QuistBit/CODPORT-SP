/* Same vertex function and operation order as the serial renderer. The front
 * end, tess and model-light context are immutable until the synchronous join.
 * Workers publish only private RGBA bytes; GL and frame ownership stay here. */
typedef struct {const trRefEntity_t *ent;const vec4_t *xyz,*normal;byte *rgba;} c1LightJob;
static int R_Cod1LightJob(void *context,unsigned first,unsigned end){
 c1LightJob *j=(c1LightJob*)context;
 return R_Cod1ShadeEntityVertices(j->ent,(int)(end-first),j->xyz+first,j->normal+first,j->rgba+first*4);
}
static qboolean R_Cod1ShadeParallel(const trRefEntity_t *ent,int count,const vec4_t *xyz,const vec4_t *normal,byte *rgba){
 byte scratch[SHADER_MAX_VERTEXES*4];c1LightJob job;c1jFp before;int i;
 if(count>=512 && count<=SHADER_MAX_VERTEXES && C1J_Enabled()){
  job.ent=ent;job.xyz=xyz;job.normal=normal;job.rgba=scratch;C1J_FpGet(&before);
  if(C1J_ParallelFor((unsigned)count,256,R_Cod1LightJob,&job)){memcpy(rgba,scratch,(size_t)count*4);return qtrue;}
  /* Preserve the serial first-failure boundary, untouched output suffix and
   * FP flags even if later chunks completed before one worker declined. */
  C1J_FpRestore(&before,0,0);
 }
 /* A batch reuses only immutable geometry and the caller's rounding mode.
  * Failure replays the scalar route from the exact original FP environment. */
 C1J_FpGet(&before);
 if(R_Cod1ShadeEntityVertices(ent,count,xyz,normal,rgba))return qtrue;
 C1J_FpRestore(&before,0,0);
 for(i=0;i<count;i++)if(!R_Cod1ShadeEntityVertex(ent,xyz[i],normal[i],rgba+i*4))break;
 return i==count;
}
