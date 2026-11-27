#ifndef COD1_ACTOR_CLONE_STORAGE45_H
#define COD1_ACTOR_CLONE_STORAGE45_H
/* GAME owner thread. All owned records (fresh, stage, baseline, expected,
 * live and idle) hold the same indivisible triple. The nodes pointer is
 * the allocation base. Non-owning projections must never be released. */
typedef struct {
 c1atl14Node_t nodes[C1GA_COUNT];
 C1GANotifyState notify;
 C1GANameEntry names[C1GA_COUNT];
} ActorCloneBlock45;
/* The existing three range guards must cover the entire allocation, with
 * no unprotected padding between arrays or after the last one. */
typedef char ActorCloneBlock45Layout[(offsetof(ActorCloneBlock45,nodes)==0 &&
 offsetof(ActorCloneBlock45,notify)==sizeof(((ActorCloneBlock45*)0)->nodes) &&
 offsetof(ActorCloneBlock45,names)==offsetof(ActorCloneBlock45,notify)+sizeof(C1GANotifyState) &&
 sizeof(ActorCloneBlock45)==offsetof(ActorCloneBlock45,names)+sizeof(((ActorCloneBlock45*)0)->names))?1:-1];
typedef struct {
 c1atl14Node_t *nodes;
 C1GANotifyState *notify;C1GANameEntry *names;
} ActorCloneStorage;
static ActorCloneStorage actorClonePool[2];
static void ActorClonePoolClear(void){
 unsigned i;
 for(i=0;i<2;i++)free(actorClonePool[i].nodes);
 memset(actorClonePool,0,sizeof(actorClonePool));
}
/* Only called on an empty triple. A miss either sets all three pointers
 * or leaves them all null. Fresh does not consume the idle clone pool. */
static int ActorGenericAllocateBlock45(C1GARecord*r,int fresh){
 ActorCloneBlock45*b=malloc(sizeof(*b));
 if(!b)return 0;
 r->nodes=b->nodes;r->notify=&b->notify;r->names=b->names;
 if(fresh){memset(r->nodes,0,sizeof(b->nodes));memset(r->notify,0,sizeof(b->notify));}
 return 1;
}
static void ActorGenericFreeOwned(C1GARecord*r){
 unsigned i;
 if(r->nodes && r->notify && r->names){
  for(i=0;i<2;i++)if(!actorClonePool[i].nodes){
   actorClonePool[i].nodes=r->nodes;actorClonePool[i].notify=r->notify;actorClonePool[i].names=r->names;
   r->nodes=NULL;r->notify=NULL;r->names=NULL;return;
  }
 }
 free(r->nodes);r->nodes=NULL;r->notify=NULL;r->names=NULL;
}
static int ActorGenericClone(C1GARecord*stage,const C1GARecord*old){
 unsigned i;
 *stage=*old;stage->nodes=NULL;stage->notify=NULL;stage->names=NULL;
 if(!old->nodes||!old->notify||!old->names||old->notify->nameCount>C1GA_COUNT)return 0;
 for(i=0;i<2;i++)if(actorClonePool[i].nodes){
  stage->nodes=actorClonePool[i].nodes;stage->notify=actorClonePool[i].notify;stage->names=actorClonePool[i].names;
  actorClonePool[i].nodes=NULL;actorClonePool[i].notify=NULL;actorClonePool[i].names=NULL;break;
 }
 if(!stage->nodes&&!ActorGenericAllocateBlock45(stage,0))return 0;
 if(!stage->nodes||!stage->notify||!stage->names){ActorGenericFreeOwned(stage);return 0;}
 memcpy(stage->nodes,old->nodes,C1GA_COUNT*sizeof(*stage->nodes));memcpy(stage->notify,old->notify,sizeof(*stage->notify));memcpy(stage->names,old->names,(size_t)old->notify->nameCount*sizeof(*stage->names));return 1;
}
#endif
