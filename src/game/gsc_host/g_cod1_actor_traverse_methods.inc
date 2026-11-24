/* Original GAME68BD0 and69C80. The native path producer is a separate
 * capability: an unqualified nonempty path must never use the origin branch. */
static int ActorTraverseMethod(C1ALCall*c,const GscActorLookRequest*q,GscActorLookReply*out,char*error,size_t cap){
 GscActorLookRequest canonical;cod1ActorRecord_t*a;cod1ActorLocalDefaults_t prior;
 float origin[3];uint32_t mode;int16_t count,cursor;unsigned i;
 memset(&canonical,0,sizeof(canonical));canonical.entity=q->entity;canonical.method=q->method;
 canonical.runtime_identity=q->runtime_identity;canonical.catalog_identity=q->catalog_identity;
 canonical.now_ms=q->now_ms;canonical.precision_bits=53;
 if(q->method==GSC_ACTOR_TRAVERSE_MODE){
  const char*end=memchr(q->tag,0,sizeof(q->tag));if(!end)return 0;
  memcpy(canonical.tag,q->tag,(size_t)(end-q->tag));
 }
 if(memcmp(q,&canonical,sizeof(canonical))||!ActorLookFence(c))return 0;
 a=cod1Actors+c->lease.actorIndex;prior=a->defaults;memcpy(origin,a->nativeData.entity.origin,12);
 if(memcmp(origin,g_entities[c->entity.slot].r.currentOrigin,12))return 0;
 if(q->method==GSC_ACTOR_TRAVERSE_MODE){
  mode=!strcmp(q->tag,"gravity")?1:!strcmp(q->tag,"nogravity")?2:!strcmp(q->tag,"noclip")?3:0;
  if(!mode){EntityError(error,cap,"traverseMode must be 'gravity', 'nogravity', or 'noclip'");return 0;}
  if(!ActorLookFence(c)||memcmp(&prior,&a->defaults,sizeof(prior)))return 0;
  a->defaults.allocation.cell024c=mode;out->kind=GSC_ACTOR_LOOK_RESULT_VOID;
  if(g_debugMove.integer)G_Printf("[467 traverse mode] owner=%u tick=%d mode=%u\n",c->entity.slot,c->now,mode);
  return 1;
 }
 if(q->method!=GSC_ACTOR_NEGOTIATION_END)return 0;
 count=(int16_t)prior.pathWords[(0x6f4-0x374)/4];cursor=(int16_t)(prior.pathWords[(0x6f8-0x374)/4]>>16);
 if(count>0&&cursor){EntityError(error,cap,"getNegotiationEndPos requires the owned native path/node producer");return 0;}
 for(i=0;i<3;i++)if(!isfinite(origin[i]))return 0;
 if(!ActorLookFence(c)||memcmp(&prior,&a->defaults,sizeof(prior))||
    memcmp(origin,a->nativeData.entity.origin,12)||memcmp(origin,g_entities[c->entity.slot].r.currentOrigin,12))return 0;
 out->kind=GSC_ACTOR_LOOK_RESULT_VECTOR;memcpy(out->vector_bits,origin,12);return 1;
}
