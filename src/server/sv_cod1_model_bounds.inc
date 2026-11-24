/* ENGINE syscall142 reads the first model's serialized bounds, independent
 * of animation and orientation. Caller must separately certify which model
 * belongs to its entity. No resource pointer leaves this map-owned bank. */
static int CP_BoundsPrivate(const void*p,size_t bytes){unsigned i;
#define BP(v) if(CP_PartOverlap(p,bytes,&v,sizeof(v)))return 1
 BP(cod1PrecacheBank);BP(cod1PrecacheMutation);BP(cod1PrecacheBusy);BP(cod1PrecacheCancelled);
#undef BP
 for(i=0;i<COD1_PRECACHE_MODELS;i++)if(cod1PrecacheBank[i].resource&&CP_PartOverlap(p,bytes,cod1PrecacheBank[i].resource,sizeof(c1mrResource_t)))return 1;
 return 0;
}
int SV_Cod1ModelBounds(const cod1ModelResourceReference_t*input,cod1ModelBounds_t*out){
 cod1ModelResourceReference_t request,reference;cod1ModelBounds_t result;const c1mrResource_t*resource;unsigned i;
 if(!input||!out||CP_PartOverlap(input,sizeof(*input),out,sizeof(*out))||CP_BoundsPrivate(input,sizeof(*input))||CP_BoundsPrivate(out,sizeof(*out)))return COD1_PRECACHE_INVALID;
 request=*input;request.operation=COD1_MODEL_RESOURCE_VALIDATE;
 if(SV_Cod1ModelResourceReference(&request,&reference)!=1||memcmp(&request,&reference,sizeof(request)))return COD1_PRECACHE_STALE;
 resource=cod1PrecacheBank[reference.slot-1].resource;
 memcpy(result.mins,resource->model.serializedBounds.mins,12);memcpy(result.maxs,resource->model.serializedBounds.maxs,12);
 for(i=0;i<3;i++)if(!isfinite(result.mins[i])||!isfinite(result.maxs[i])||result.mins[i]>result.maxs[i])return COD1_PRECACHE_INVALID;
 *out=result;return 1;
}
