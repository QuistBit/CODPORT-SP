#include "../qcommon/cod1_actor_prone_pose_apply14.h"
static int CGFA4_SourceSample(const cgfa4Lease_t*l,cgfa4Resource_t*r,c1ab14Accumulator_t*out){
 c1asp14Packet_t packet;c1asp14Decoded_t decoded;c1ast14Bank_t bank;unsigned i;
 if(!C1SourcePoseWireRead(&l->actor,&packet)||!C1ASP14_Decode(&packet,&decoded)||!CGFA4_Current(l))return 0;
 memset(&bank,0,sizeof(bank));bank.bones=r->resource->assembly->topology.boneCount;bank.count=decoded.leaves;
 for(i=0;i<decoded.leaves;i++){
  unsigned id=decoded.resource[i];c1ab14Clip_t*clip=cgSourcePoseClips[id];
  if(!clip){char path[128];int size=0;void*raw;c1ab14Clip_t*staged=NULL;
   snprintf(path,sizeof(path),"xanim/%s",c1asp14Name[id]);raw=CGFA4_Read(l,path,&size);
   int ok=raw&&C1AB14_Decode(raw,size,&staged);free(raw);
   if(!ok||!CGFA4_Current(l)){C1AB14_Free(staged);return 0;}
   cgSourcePoseClips[id]=clip=staged;
  }
  if(!r->sourceMapped[id]){
   unsigned char mapped[128]={0};
   if(!C1AD14_Map(r->resource,clip,mapped)||!CGFA4_Current(l)){static int shown;if(!shown++)CG_Printf("[378 source map refusal] owner=%d clip=%s bones=%u\n",l->actor.number,c1asp14Name[id],clip->bones);return 0;}
   memcpy(r->sourceMap[id],mapped,sizeof(mapped));r->sourceMapped[id]=1;
  }
  bank.clip[i]=clip;memcpy(bank.map[i],r->sourceMap[id],128);
 }
 if(!CGFA4_Current(l))return 0;
 if(!decoded.leaves)memset(out,0,bank.bones*sizeof(*out));
 else if(!C1AST14_Sample(&bank,decoded.node,decoded.count,out,128))return 0;
 return C1APP14_Apply(r->resource,packet.controller,out,bank.bones)&&CGFA4_Current(l);
}
