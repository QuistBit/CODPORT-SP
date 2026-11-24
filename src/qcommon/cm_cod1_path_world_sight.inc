/* Uses the immutable current CM world and its exclusive main-thread query
 * context. Server queries execute synchronously, with no callback to GAME. */
qboolean CM_Cod1PathNativeSight(const vec3_t start,const vec3_t end,
 const vec3_t mins,const vec3_t maxs,const vec3_t origin,int model,
 unsigned mask,int*out){
 if(!codWorld||!codContext||model<0||model>=BOX_MODEL_HANDLE-1)return qfalse;
 return v59_path_sight(codContext,start,end,mins,maxs,origin,mask,model,out)?qtrue:qfalse;
}
