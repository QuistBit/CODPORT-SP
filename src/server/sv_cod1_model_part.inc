/* Original engine89/8A read LOD0 ordered part names. No resource is borrowed
 * across a callback; the existing map-owned decoded model is reused. */
static int CP_PartOverlap(const void*a,size_t na,const void*b,size_t nb){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);
}
int SV_Cod1ModelPartName(const cod1ModelResourceReference_t*input,int32_t index,cod1ModelPartNameResult_t*out){
 cod1ModelResourceReference_t request,reference;cod1ModelPartNameResult_t result;const c1mrResource_t*resource;unsigned i;
 if(!input||!out||CP_PartOverlap(input,sizeof(*input),out,sizeof(*out))||CP_PartOverlap(out,sizeof(*out),cod1PrecacheBank,sizeof(cod1PrecacheBank))||CP_PartOverlap(out,sizeof(*out),&cod1PrecacheMutation,sizeof(cod1PrecacheMutation))||CP_PartOverlap(out,sizeof(*out),&cod1PrecacheBusy,sizeof(cod1PrecacheBusy))||CP_PartOverlap(out,sizeof(*out),&cod1PrecacheCancelled,sizeof(cod1PrecacheCancelled)))return COD1_PRECACHE_INVALID;
 for(i=0;i<COD1_PRECACHE_MODELS;i++)if(cod1PrecacheBank[i].resource&&CP_PartOverlap(out,sizeof(*out),cod1PrecacheBank[i].resource,sizeof(c1mrResource_t)))return COD1_PRECACHE_INVALID;
 request=*input;request.operation=COD1_MODEL_RESOURCE_VALIDATE;
 if(SV_Cod1ModelResourceReference(&request,&reference)!=1||memcmp(&request,&reference,sizeof(request)))return COD1_PRECACHE_STALE;
 resource=cod1PrecacheBank[reference.slot-1].resource;memset(&result,0,sizeof(result));
 if(resource->skeleton.boneCount>C1MR_BONES_MAX)return COD1_PRECACHE_INVALID;
 result.count=(int32_t)resource->skeleton.boneCount;
 if((uint32_t)index>=resource->skeleton.boneCount){*out=result;return COD1_MODEL_PART_RANGE;}
 if(!memchr(resource->skeleton.boneName[index],0,128)||!resource->skeleton.boneName[index][0])return COD1_MODEL_PART_BAD_MODEL;
 memcpy(result.name,resource->skeleton.boneName[index],128);*out=result;return 1;
}
