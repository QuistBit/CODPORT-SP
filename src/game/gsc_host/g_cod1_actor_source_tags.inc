/* Same reachable current graph and ordinary sampling kernel as wire26.
 * Resources remain borrowed from the map bank, fenced by revision. */
#include "../../qcommon/cod1_actor_prone_pose_apply14.h"
static uint64_t ActorSourceTagRevision(uint64_t epoch){return ActorLocomotionReady(epoch)?cod1LocoResources.revision:0;}
static int ActorSourceTagSample(C1FActorTagCall*c,C1FActorTagResource*r,c1ab14Accumulator_t*out,unsigned*bones){
 c1asp14Packet_t packet;c1asp14Decoded_t decoded;c1ast14Bank_t bank;unsigned i;
 if(!c||!r||!out||!bones||!c->sourcePose||!ActorFaceTagFence(c)||
    !C1ASP14_CollectDecoded(cod1GenericDefinitions,c->nodesBefore,C1GA_COUNT,&packet,&decoded))return 0;
 memset(&bank,0,sizeof(bank));bank.bones=r->full->assembly->topology.boneCount;bank.count=decoded.leaves;
 if(r->sourceRevision!=c->sourceRevision){memset(r->sourceMapped,0,sizeof(r->sourceMapped));r->sourceRevision=c->sourceRevision;}
 for(i=0;i<decoded.leaves;i++){
  unsigned id=decoded.resource[i],source=c1asp14Source[id];const c1ab14Clip_t*clip=ActorLocomotionBody(source);
  if(!clip||strcmp(cod1GenericNames[source].name,c1asp14Name[id]))return 0;
  if(!r->sourceMapped[id]){unsigned char map[128]={0};if(!C1AD14_Map(r->full,clip,map)||!ActorFaceTagFence(c))return 0;memcpy(r->sourceMap[id],map,128);r->sourceMapped[id]=1;}
  bank.clip[i]=clip;memcpy(bank.map[i],r->sourceMap[id],128);
 }
 if(!ActorFaceTagFence(c))return 0;
 if(!decoded.leaves)memset(out,0,bank.bones*sizeof(*out));
 else if(!C1AST14_Sample(&bank,decoded.node,decoded.count,out,128))return 0;
 if(!ActorPronePosePacket(&c->actorBefore,c->now,packet.controller)||!C1APP14_Apply(r->full,packet.controller,out,bank.bones))return 0;
 if(!ActorFaceTagFence(c))return 0;*bones=bank.bones;return 1;
}
