/* No graph/Session capability is manufactured here. Capture the actual
 * typed allocation, query one target, and publish only while it is current. */
int G_Cod1PathSight(const GscEntityRef*input,const float*start,const float*end,int*out){
    GscEntityRef ref;cod1PathSightQuery_t q;cod1ActorPathView_t a,after;
    entityShared_t shared;uint32_t flags,afterFlags;int nativeActor,result,status,model,type;
    uintptr_t p=(uintptr_t)out,base=(uintptr_t)g_entities;
    if(!input||!start||!end||!out||Cod1PathEntityOverlap(out,sizeof(*out))||
       (p<=base?base-p<sizeof(*out):p-base<sizeof(g_entities)))return 0;
    ref=*input;if(!G_Cod1PathEntityRead(&ref,&flags))return 0;
    /* Native44EDD7/44EF4D returns clear on C4&02820011==0 before
     * touching model/bounds. The typed model owner supplies C4=2080;
     * no generic physical0 inference or DObj/bbox substitution. */
    nativeActor=G_Cod1ActorPathRead(&ref,&a);
    if(!nativeActor&&(cod1PathEntities[ref.slot].kind==7||cod1PathEntities[ref.slot].kind==8||cod1PathEntities[ref.slot].kind==9)){
        if((cod1PathEntities[ref.slot].kind==7?!Cod1PathEntityModelCurrent(&ref):!Cod1HealthCurrent(&ref))||
           (cod1PathEntities[ref.slot].nativeContents&0x02820011u))return 0;
        *out=0;return 1;
    }
    memset(&q,0,sizeof(q));q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);q.slot=ref.slot;
    VectorCopy(start,q.start);VectorCopy(end,q.end);
    shared=g_entities[ref.slot].r;model=g_entities[ref.slot].s.modelindex;type=g_entities[ref.slot].s.eType;
    /* A selected interaction brush represents different native contents.
     * Only its current world projection can be sent to the SERVER query. */
    if(!nativeActor&&cod1PathEntities[ref.slot].projected&&
       !Cod1PathProjectionActive(&ref,(uint32_t)shared.contents))return 0;
    if(nativeActor){
        if(!a.linked)return 0;
        q.actor.epoch=ref.epoch;q.actor.generation=ref.generation;q.actor.slot=ref.slot;q.actor.lease=a.lease.serial;
        q.expectedContents=a.contents;
    }else q.expectedContents=(uint32_t)shared.contents;
    status=trap_Cod1PathSight(&q,&result);
    if(status!=COD1_SPATIAL_OK||(result!=0&&result!=-1)||
       !G_Cod1PathEntityRead(&ref,&afterFlags)||afterFlags!=flags||
       memcmp(&shared,&g_entities[ref.slot].r,sizeof(shared))||
       model!=g_entities[ref.slot].s.modelindex||type!=g_entities[ref.slot].s.eType)return 0;
    if(nativeActor){
        if(!G_Cod1ActorPathRead(&ref,&after)||memcmp(&a,&after,sizeof(a)))return 0;
    }else if(G_Cod1ActorPathRead(&ref,&after))return 0;
    *out=result;return 1;
}
/* C1DPTrace adapter. The context is an allocation ref owned by the candidate
 * caller, not a slot-only snapshot. Candidate mutation failure still requires
 * discarding/rolling back that private candidate; no live publication here. */
int G_Cod1PathCandidateTrace(void*context,unsigned from,unsigned to,
    const float*start,const float*end,const float*mins,const float*maxs,
    unsigned entity,unsigned mask){
    GscEntityRef ref;int result;uint32_t flags;
    (void)from;(void)to;
    if(!context||!mins||!maxs)return -1;ref=*(const GscEntityRef*)context;
    if(entity!=ref.slot||mask!=0x02820011u||
       mins[0]!=-15||mins[1]!=-15||mins[2]!=18||maxs[0]!=15||maxs[1]!=15||maxs[2]!=72||
       !G_Cod1PathEntityRead(&ref,&flags)||!(flags&0x1000u)||
       !G_Cod1PathSight(&ref,start,end,&result))return -1;
    return result!=0;
}
