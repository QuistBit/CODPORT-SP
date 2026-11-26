#include "../qcommon/cod1_actor_prone_pose_apply14.h"
static int CGFA4_SourceSampleImpl(const cgfa4Lease_t*l,cgfa4Resource_t*r,c1ab14Accumulator_t*out){
 c1asp14Packet_t packet;c1asp14Decoded_t decoded;c1ast14Bank_t bank;unsigned i;
 if(!C1SourcePoseWireRead(&l->actor,&packet)||!C1ASP14_Decode(&packet,&decoded)||!CGFA4_Current(l))return 0;
 CGAP32_Decoded((unsigned)l->actor.number,&decoded);
 memset(&bank,0,sizeof(bank));bank.bones=r->resource->assembly->topology.boneCount;bank.count=decoded.leaves;
 for(i=0;i<decoded.leaves;i++){
  unsigned id=decoded.resource[i];c1ab14Clip_t*clip=cgSourcePoseClips[id];
  if(!clip){char path[128];int size=0;void*raw;c1ab14Clip_t*staged=NULL;
   uint64_t start=CGAP32_Start();int ok;
   snprintf(path,sizeof(path),"xanim/%s",c1asp14Name[id]);raw=CGFA4_Read(l,path,&size);
   ok=raw&&C1AB14_Decode(raw,size,&staged);free(raw);
   ok=ok&&CGFA4_Current(l);
   if(start)CGAP32_End(C1AP32_CLIP,start,ok,(unsigned)l->actor.number);
   if(!ok){C1AB14_Free(staged);return 0;}
   cgSourcePoseClips[id]=clip=staged;
  }
  if(!r->sourceMapped[id]){
   unsigned char mapped[128]={0};
   uint64_t start=CGAP32_Start();int ok=C1AD14_Map(r->resource,clip,mapped)&&CGFA4_Current(l);
   if(start)CGAP32_End(C1AP32_MAP,start,ok,(unsigned)l->actor.number);
   if(!ok){static int shown;if(!shown++)CG_Printf("[378 source map refusal] owner=%d clip=%s bones=%u\n",l->actor.number,c1asp14Name[id],clip->bones);return 0;}
   memcpy(r->sourceMap[id],mapped,sizeof(mapped));r->sourceMapped[id]=1;
  }
  bank.clip[i]=clip;memcpy(bank.map[i],r->sourceMap[id],128);
 }
 if(!CGFA4_Current(l))return 0;
 if(!decoded.leaves)memset(out,0,bank.bones*sizeof(*out));
 else if(!CGAP32_Tree(&bank,decoded.node,decoded.count,out,128))return 0;
 return C1APP14_Apply(r->resource,packet.controller,out,bank.bones)&&CGFA4_Current(l);
}
static int CGFA4_SourceSample(const cgfa4Lease_t*l,cgfa4Resource_t*r,c1ab14Accumulator_t*out){
 uint64_t start=CGAP32_Start();int ok=CGFA4_SourceSampleImpl(l,r,out);
 if(start)CGAP32_End(C1AP32_SOURCE,start,ok,(unsigned)l->actor.number);
 return ok;
}
