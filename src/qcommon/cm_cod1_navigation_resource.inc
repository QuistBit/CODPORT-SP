/* Original ENGINE4129A0/trap56 reads header lump31 (length then offset),
 * GAME6F7A0 consumes v7 and trap57 releases its copy. Here the immutable
 * resource is captured from the very CM_LoadV59 input, before FS frees it.
 * Only the stable loaded-BSP profile is admitted; external file replacement
 * and the original absent-lump fallback are not implemented by this owner. */
static unsigned char *cod1NavigationBytes;
static uint32_t cod1NavigationLength;
static uint64_t cod1NavigationSerial,cod1NavigationIncarnation;
static void (*cod1NavigationObserver)(uint64_t);
void CM_Cod1NavigationSetObserver(void (*observer)(uint64_t)){
 cod1NavigationObserver=observer;
}
static void CM_Cod1NavigationClear(void){
 uint64_t retired=cod1NavigationIncarnation;unsigned char*bytes=cod1NavigationBytes;
 /* Revoke identity before notifying. Receiver only cancels private GAME
  * ownership; it must not reload or query CM from this callback. */
 cod1NavigationBytes=NULL;cod1NavigationLength=0;cod1NavigationIncarnation=0;
 free(bytes);
 if(retired&&cod1NavigationObserver)cod1NavigationObserver(retired);
}
static uint32_t CM_Cod1NavigationLE32(const unsigned char*p){
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void CM_Cod1NavigationCapture(const void*data,int bytes){
 const unsigned char*p=(const unsigned char*)data;uint32_t length,offset;
 if(!p||bytes<272||!codWorld||!codContext||cod1NavigationIncarnation||cod1NavigationSerial==UINT64_MAX)return;
 if(memcmp(p,"IBSP",4)||CM_Cod1NavigationLE32(p+4)!=59)return;
 length=CM_Cod1NavigationLE32(p+8+31*8);offset=CM_Cod1NavigationLE32(p+12+31*8);
 if(length>COD1_NAV_RESOURCE_LIMIT||offset>(uint32_t)bytes||length>(uint32_t)bytes-offset)return;
 if(length){
  cod1NavigationBytes=(unsigned char*)malloc(length);if(!cod1NavigationBytes)return;
  memcpy(cod1NavigationBytes,p+offset,length);
 }
 cod1NavigationLength=length;cod1NavigationIncarnation=++cod1NavigationSerial;
}
static int CM_Cod1NavigationOverlap(const void*a,size_t an,const void*b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(!an||!bn)return 0;
 if(!a||!b||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;
 return x<y+bn&&y<x+an;
}
int CM_Cod1NavigationResource(const cod1NavigationRequest_t*request,cod1NavigationResource_t*out,void*buffer,uint32_t capacity){
 cod1NavigationRequest_t q;cod1NavigationResource_t value;
 if(!request||!out||!codWorld||!codContext||!cod1NavigationIncarnation)return 0;
 if(CM_Cod1NavigationOverlap(request,sizeof(*request),out,sizeof(*out)))return 0;
 q=*request;
 if(q.version!=COD1_NAV_RESOURCE_VERSION||q.bytes!=sizeof(q))return 0;
 if(q.operation==COD1_NAV_DESCRIBE){if(q.incarnation||q.length||buffer||capacity)return 0;}
 else if(q.operation==COD1_NAV_COPY||q.operation==COD1_NAV_CURRENT){
  if(q.incarnation!=cod1NavigationIncarnation||q.length!=cod1NavigationLength)return 0;
  if(q.operation==COD1_NAV_CURRENT){if(buffer||capacity)return 0;}
  else {
   if(capacity!=q.length||(!buffer&&capacity)||(!capacity&&buffer))return 0;
   if(CM_Cod1NavigationOverlap(buffer,capacity,request,sizeof(*request))||
      CM_Cod1NavigationOverlap(buffer,capacity,out,sizeof(*out))||
      CM_Cod1NavigationOverlap(buffer,capacity,cod1NavigationBytes,cod1NavigationLength))return 0;
  }
 }else return 0;
 memset(&value,0,sizeof(value));value.version=COD1_NAV_RESOURCE_VERSION;value.bytes=sizeof(value);
 value.incarnation=cod1NavigationIncarnation;value.length=cod1NavigationLength;
 if(q.operation==COD1_NAV_COPY&&capacity)memcpy(buffer,cod1NavigationBytes,capacity);
 *out=value;return 1;
}
