/* A direct authored negotiation edge is distinct from a general A* path.
 * Read physical indices only after the complete published graph producer.
 * End+68 is a temporary search parent in native code, not persistent graph
 * identity; never interpret its cleared staging bytes as a parent pointer. */
static int Cod1GraphNegotiationCopy(const GscNodeRef*b,const GscNodeRef*e,cod1NegotiationPair_t*out){
 const C1DPGraphCandidate*g=cod1GraphStaged;const unsigned char*br,*er;const C1DPNode*n;
 unsigned bp,ep,i,links=0;cod1NegotiationPair_t v;
 if(cod1NodeBusy||cod1GraphCancelled||!Cod1NodeAvailable()||!TrainingGraphCurrent(cod1GraphReceipt)||
    !Cod1NodeRefCurrent(b)||!Cod1NodeRefCurrent(e)||b->index==e->index||
    !cod1GraphPrepared->scripts||cod1GraphPrepared->count!=g->count||!g->count||g->count>2048||
    !g->records||!g->nodes||!g->authoredToPhysical||!g->links||!g->owners||!g->ownerCount)return 0;
 bp=C1NAV14_SortU16(g->authoredToPhysical+b->index*2);ep=C1NAV14_SortU16(g->authoredToPhysical+e->index*2);
 if(bp>=g->count||ep>=g->count||bp==ep)return 0;br=g->records+bp*116;er=g->records+ep*116;
 if(C1NAV14_SortU16(br+0x28)!=b->index||C1NAV14_SortU16(er+0x28)!=e->index||
    C1NAV14_SortU32(br+0x24)!=16||C1NAV14_SortU32(er+0x24)!=17||
    !C1NAV14_SortU16(br+0x30)||C1NAV14_SortU16(br+0x30)!=C1NAV14_SortU16(er+0x2c)||
    !C1NAV14_SortU16(br+0x32)||!cod1GraphPrepared->scripts[b->index].cache||!cod1GraphPrepared->scripts[b->index].entry)return 0;
 n=g->nodes+bp;if(n->active>n->count||n->first>g->linkCount||n->count>g->linkCount-n->first)return 0;
 for(i=0;i<n->active;i++){const C1DPLink*l=g->links+n->first+i;if(l->blockers||l->target>=g->count)return 0;if(l->target==ep)++links;}
 if(links!=1)return 0;
 memset(&v,0,sizeof(v));v.receipt=cod1GraphReceipt;v.sessionId=cod1GraphPrepared->sessionId;v.begin=*b;v.end=*e;
 v.script=cod1GraphPrepared->scripts[b->index];v.beginPhysical=bp;v.endPhysical=ep;
 memcpy(v.start,br+0x38,12);memcpy(v.finish,er+0x38,12);memcpy(&v.yaw,br+0x44,4);
 for(i=0;i<3;i++)if(!isfinite(v.start[i])||!isfinite(v.finish[i]))return 0;
 if(!isfinite(v.yaw)||v.yaw<0||v.yaw>360||!v.sessionId)return 0;
 *out=v;return 1;
}
int G_Cod1NodeGraphNegotiationCurrent(const cod1NegotiationPair_t*input){
 cod1NegotiationPair_t v;
 if(!input||Cod1GraphTargetPrivate(input,sizeof(*input))||cod1GraphBusy)return 0;
 return Cod1GraphNegotiationCopy(&input->begin,&input->end,&v)&&!memcmp(input,&v,sizeof(v));
}
int G_Cod1NodeGraphNegotiationPair(GscSession*s,uint64_t id,const GscNodeRef*begin,const GscNodeRef*end,cod1NegotiationPair_t*out){
 GscNodeRef b,e;cod1NegotiationPair_t value;uint64_t receipt=cod1GraphReceipt;int ok,found=0;
 if(!s||!id||!begin||!end||!out||Cod1GraphTargetPrivate(begin,sizeof(*begin))||Cod1GraphTargetPrivate(end,sizeof(*end))||
    Cod1GraphTargetPrivate(out,sizeof(*out))||Cod1NodeOverlap(begin,sizeof(*begin),out,sizeof(*out))||
    Cod1NodeOverlap(end,sizeof(*end),out,sizeof(*out))||cod1GraphBusy||cod1GraphCancelled||cod1NodeBusy)return 0;
 b=*begin;e=*end;
 if(!gsc_session_node_graph_check_begin(s,id,receipt))return 0;
 cod1GraphBusy=1;cod1GraphCancelled=0;
 ok=Cod1GraphPublishedOwnerCurrent(s,id,receipt);
 if(ok)found=Cod1GraphNegotiationCopy(&b,&e,&value)&&value.sessionId==id;
 ok=ok&&Cod1GraphPublishedOwnerCurrent(s,id,receipt)&&
    !memcmp(begin,&b,sizeof(b))&&!memcmp(end,&e,sizeof(e));
 if(!gsc_session_node_graph_end(s,id,receipt,ok))ok=0;cod1GraphBusy=0;
 if(!ok||!found)return 0;*out=value;return 1;
}
