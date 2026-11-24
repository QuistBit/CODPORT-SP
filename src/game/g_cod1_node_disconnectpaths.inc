/* Original reconnect -> retrace -> block transitions on private scratch.
 * Only a current published graph and the exact live target can commit it. */
typedef struct {
 GscSession*session;uint64_t id,receipt;C1NPPrepared*prepared;C1DPGraphCandidate*graph;
 cod1NavigationResource_t resource;GscEntityRef owner;const GscEntityRef*borrowed;
 uint32_t flags;int now;int(*current)(void*);void*lease;
}C1DisconnectContext;
static int Cod1DisconnectState(const C1DisconnectContext*c){
 uint32_t flags;const GscEntityRef*bound;
 if(!c||!cod1GraphBusy||cod1GraphCancelled||!cod1GraphPublished||cod1GraphReceipt!=c->receipt||
    cod1GraphStaged!=c->graph||cod1GraphPrepared!=c->prepared||!c->receipt||
    memcmp(&cod1GraphResource,&c->resource,sizeof(c->resource))||!c->resource.incarnation||
    c->prepared->sessionId!=c->id||level.time!=c->now||memcmp(c->borrowed,&c->owner,sizeof(c->owner))||
    !G_Cod1EntitySameAllocation(&c->owner)||!G_Cod1PathEntityRead(&c->owner,&flags)||flags!=c->flags||!(flags&0x1000u))return 0;
 bound=&cod1PathOwnerRefs[c->owner.slot];
 return bound->slot==c->owner.slot&&bound->generation==c->owner.generation&&bound->epoch==c->owner.epoch&&
  c->graph->owners[c->owner.slot].entity==c->owner.slot&&
  (c->graph->owners[c->owner.slot].flags&0x1000u)&&
  gsc_session_running_path_graph(c->session,c->id,c->receipt)&&Cod1NodePreparedOwner(c->prepared->registryEpoch)&&
  Cod1GraphResourceCurrent(&c->resource)&&!cod1GraphCancelled;
}
static int Cod1DisconnectCurrent(const C1DisconnectContext*c){
 return Cod1DisconnectState(c)&&c->current(c->lease)==1&&Cod1DisconnectState(c);
}
static int Cod1DisconnectTrace(void*ctx,unsigned from,unsigned to,const float*start,const float*end,const float*mins,const float*maxs,unsigned entity,unsigned mask){
 C1DisconnectContext*c=ctx;GscEntityRef ref;int hit;
 if(!Cod1DisconnectCurrent(c))return -1;ref=c->owner;
 hit=G_Cod1PathCandidateTrace(&ref,from,to,start,end,mins,maxs,entity,mask);
 return !memcmp(&ref,&c->owner,sizeof(ref))&&Cod1DisconnectCurrent(c)?hit:-1;
}
int G_Cod1NodeGraphDisconnectPaths(GscSession*session,uint64_t id,const GscEntityRef*input,int now,char*error,size_t cap,int(*current)(void*),void*lease){
 C1DisconnectContext c;C1DPState live,next;int result=C1DP_BAD_DATA;
 if((cap&&!error)||!session||!id||!input||!current||now<0)return 0;
 if(cod1GraphBusy){cod1GraphCancelled=1;return 0;}
 memset(&c,0,sizeof(c));memset(&live,0,sizeof(live));memset(&next,0,sizeof(next));
 c.owner=*input;c.borrowed=input;c.session=session;c.id=id;c.now=now;c.current=current;c.lease=lease;
 c.receipt=cod1GraphReceipt;c.graph=cod1GraphStaged;c.prepared=cod1GraphPrepared;c.resource=cod1GraphResource;
 if(!cod1GraphPublished||!c.graph||!c.prepared||!c.graph->owners||c.owner.slot>=c.graph->ownerCount||c.owner.slot>=MAX_GENTITIES||!G_Cod1PathEntityRead(&c.owner,&c.flags)||!(c.flags&0x1000u))return 0;
 cod1GraphBusy=1;
 if(!Cod1DisconnectCurrent(&c))goto done;
 live.nodes=c.graph->nodes;live.links=c.graph->links;live.records=c.graph->blockRecords;live.owners=c.graph->owners;
 live.node_count=c.graph->count;live.link_count=c.graph->linkCount;live.record_count=C1NS_POOL_COUNT;live.owner_count=c.graph->ownerCount;
 if(!C1DP_Layout(&live))goto done;next=live;
 next.nodes=malloc(sizeof(*next.nodes)*next.node_count);next.links=malloc(sizeof(*next.links)*next.link_count);
 next.records=malloc(sizeof(*next.records)*next.record_count);next.owners=malloc(sizeof(*next.owners)*next.owner_count);
 if(!next.nodes||!next.links||!next.records||!next.owners)goto done;
 memcpy(next.nodes,live.nodes,sizeof(*next.nodes)*next.node_count);memcpy(next.links,live.links,sizeof(*next.links)*next.link_count);
 memcpy(next.records,live.records,sizeof(*next.records)*next.record_count);memcpy(next.owners,live.owners,sizeof(*next.owners)*next.owner_count);
 if(!Cod1DisconnectCurrent(&c))goto done;
 result=C1DP_CandidateTransition(&next,c.owner.slot,1,now,Cod1DisconnectTrace,&c);
 if(result!=C1DP_OK||!C1DP_Layout(&next)||!Cod1DisconnectCurrent(&c)){result=C1DP_BAD_DATA;goto done;}
 memcpy(live.nodes,next.nodes,sizeof(*next.nodes)*next.node_count);memcpy(live.links,next.links,sizeof(*next.links)*next.link_count);
 memcpy(live.records,next.records,sizeof(*next.records)*next.record_count);live.owners[c.owner.slot]=next.owners[c.owner.slot];
done:
 free(next.nodes);free(next.links);free(next.records);free(next.owners);cod1GraphBusy=0;
 if(cod1GraphCancelled){Cod1GraphRelease();return 0;}
 if(result!=C1DP_OK&&error&&cap)snprintf(error,cap,"disconnectpaths graph/target/trace transaction refused");
 return result==C1DP_OK;
}
