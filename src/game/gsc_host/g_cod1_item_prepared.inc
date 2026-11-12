/* Exact attempted construction prefix, independent of failed providers. This
 * record is never a restored initial mask or a borrowed execution permit. */
static int ItemPreparedReceipt(const cod1ItemPreparedReceipt_t*q){
 return q&&cod1PreparedItems.active&&q->epoch&&q->selection&&q->transaction&&
  !memcmp(q,&cod1PreparedItems.receipt,sizeof(*q));
}
static int ItemPreparedSelection(void){
 return cod1LiveItems.selected&&cod1LiveItems.epoch==cod1PreparedItems.receipt.epoch&&
  cod1LiveItems.serial==cod1PreparedItems.receipt.selection;
}
static int ItemPreparedTopology(const GscEntityRef*ref){int i;gentity_t*e;
 if(level.num_entities<MAX_CLIENTS||level.num_entities>ENTITYNUM_MAX_NORMAL||!G_Cod1EntitySameAllocation(ref)||!ActorOwnerAllowed(ref,0)||AmmoCandidateOwned(ref)||
    cod1ActorByEntity[ref->slot]||cod1SentientByEntity[ref->slot])return 0;
 e=g_entities+ref->slot;
 if(e->neverFree||e->client||e->aiCharacter||e->aiName||e->aiAttributes||e->AIScript_AlertEntity||
    e->aiInactive||e->physicsObject||e->r.bmodel||e->teamchain||e->teammaster||e->tagParent||e->tagName||
    e->team)return 0;
 for(i=0;i<level.num_entities;i++)if(g_entities[i].inuse&&g_entities[i].tagParent==e)return 0;
 return 1;
}
static int ItemPreparedBody(unsigned i,int unlinking){
 const Cod1PreparedItem*p;gentity_t expected;const gentity_t*actual;
 if(i>=5)return 0;p=cod1PreparedItems.items+i;
 if(!G_Cod1EntitySameAllocation(&p->ref))return 0;expected=p->installed?p->body:p->before;actual=g_entities+p->ref.slot;
 /* G_RunThink's exact inactive script shadow copy is an engine-owned store,
  * not a new body owner. Accept original bytes or that one canonical copy;
  * retain the captured journal and all initial health snapshots unchanged. */
 if(!memcmp(&actual->scriptStatusCurrent,&expected.scriptStatus,sizeof(expected.scriptStatus)))
  expected.scriptStatusCurrent=expected.scriptStatus;
 if(p->installed||unlinking){if(!LiveItemLinkUnchanged(&expected,actual))return 0;}
 else if(memcmp(&expected,actual,sizeof(expected)))return 0;
 return ItemPreparedTopology(&p->ref);
}
static int ItemPreparedAttempt(const GscEntityRef*ref){unsigned i;
 if(!cod1PreparedItems.active)return 1;
 if(cod1PreparedItems.closed||cod1PreparedItems.executing||!ItemPreparedSelection()||cod1PreparedItems.operation!=-1)return 0;
 for(i=0;i<5;i++)if(LiveItemRefEqual(ref,&cod1PreparedItems.items[i].ref)){
  if(cod1PreparedItems.items[i].attempted||!ItemPreparedBody(i,0))return 0;
  cod1PreparedItems.items[i].attempted=1;cod1PreparedItems.operation=(int)i;return 1;
 }
 return 0;
}
static int ItemPreparedInstall(const GscEntityRef*ref,const Cod1LiveItem*row,const gentity_t*body){
 Cod1PreparedItem*p;int i;
 if(!cod1PreparedItems.active)return 1;i=cod1PreparedItems.operation;
 if(i<0||i>=5||cod1PreparedItems.closed||cod1PreparedItems.executing||!ItemPreparedSelection())return 0;
 p=cod1PreparedItems.items+i;
 if(!p->attempted||p->installed||!LiveItemRefEqual(&p->ref,ref))return 0;
 p->row=*row;p->body=*body;p->installed=1;return 1;
}
static int ItemPreparedInstalledCurrent(const GscEntityRef*ref){int i;
 if(!cod1PreparedItems.active)return 1;i=cod1PreparedItems.operation;
 if(i<0||i>=5||cod1PreparedItems.closed||!ItemPreparedSelection()||
    !cod1PreparedItems.items[i].installed||!LiveItemRefEqual(ref,&cod1PreparedItems.items[i].ref)||!ItemPreparedBody((unsigned)i,0))return 0;
 if(!LiveItemResourceCurrent(&cod1PreparedItems.items[i].row))return 0;
 return cod1PreparedItems.active&&!cod1PreparedItems.closed&&cod1PreparedItems.operation==i&&ItemPreparedSelection()&&
  LiveItemRefEqual(ref,&cod1PreparedItems.items[i].ref)&&ItemPreparedBody((unsigned)i,0);
}
int G_Cod1ItemLifecyclePreparedBegin(const GscEntityRef input[5],cod1ItemPreparedReceipt_t*out,char*error,size_t size){
 GscEntityRef refs[5];gentity_t before[5];unsigned i,j;uint64_t epoch,selection;int begun=0;
 if(!LiveItemBuffers(input,sizeof(refs),out,sizeof(*out),error,size)||!input||!out)return 0;
 if(cod1PreparedBegin.busy){cod1PreparedBegin.cancelled=1;return 0;}
 if(cod1LiveItemGuard.active){cod1LiveItemGuard.cancelled=1;goto refuse;}
 if(cod1PreparedItems.active||!LiveItemMap()||!cod1LiveItems.selected||cod1PreparedItemSerial==UINT64_MAX)goto refuse;
 memcpy(refs,input,sizeof(refs));epoch=cod1LiveItems.epoch;selection=cod1LiveItems.serial;
 for(i=0;i<5;i++){
  if(refs[i].epoch!=epoch||!ItemPreparedTopology(refs+i)||G_Cod1ItemLifecycleOwns(refs+i))goto refuse;
  for(j=0;j<i;j++)if(refs[i].slot==refs[j].slot)goto refuse;
  before[i]=g_entities[refs[i].slot];
 }
 cod1PreparedBegin.busy=1;cod1PreparedBegin.cancelled=0;begun=1;
 for(i=0;i<5;i++)if(!G_Cod1PathHealthPreparedItem(refs+i)||cod1PreparedBegin.cancelled)goto refuse;
 if(cod1PreparedBegin.cancelled||cod1PreparedItems.active||!LiveItemMap()||!cod1LiveItems.selected||cod1LiveItems.epoch!=epoch||cod1LiveItems.serial!=selection||
    memcmp(input,refs,sizeof(refs))||!LiveItemBuffers(input,sizeof(refs),out,sizeof(*out),error,size))goto refuse;
 for(i=0;i<5;i++)if(!ItemPreparedTopology(refs+i)||memcmp(before+i,g_entities+refs[i].slot,sizeof(gentity_t)))goto refuse;
 memset(&cod1PreparedItems,0,sizeof(cod1PreparedItems));cod1PreparedItems.active=1;cod1PreparedItems.operation=-1;
 cod1PreparedItems.receipt.epoch=epoch;cod1PreparedItems.receipt.selection=selection;cod1PreparedItems.receipt.transaction=++cod1PreparedItemSerial;
 for(i=0;i<5;i++){cod1PreparedItems.items[i].ref=refs[i];cod1PreparedItems.items[i].before=before[i];}
 cod1PreparedBegin.busy=0;*out=cod1PreparedItems.receipt;return 1;
refuse:if(begun)cod1PreparedBegin.busy=0;LiveItemError(error,size,"prepared item batch requires five exact initial BSP owners");return 0;
}
int G_Cod1ItemLifecyclePreparedExecute(const cod1ItemPreparedReceipt_t*input,char*error,size_t size){
 cod1ItemPreparedReceipt_t q;unsigned i;
 if(!LiveItemBuffers(input,sizeof(q),NULL,0,error,size)||!input)return 0;q=*input;
 if(!ItemPreparedReceipt(&q)||cod1PreparedItems.closed||cod1PreparedItems.executing||cod1LiveItemGuard.active||!ItemPreparedSelection())goto refuse;
 for(i=0;i<5;i++){GscEntityRef ref=cod1PreparedItems.items[i].ref;
  if(!cod1PreparedItems.items[i].installed||!ItemPreparedBody(i,0)||
     !G_Cod1ItemLifecycleMaskCurrent(&ref,cod1PreparedItems.items[i].row.descriptor.index)||
     !ItemPreparedReceipt(&q)||cod1PreparedItems.closed||!ItemPreparedSelection())goto refuse;
 }
 for(i=0;i<5;i++)if(!ItemPreparedBody(i,0)||memcmp(&cod1LiveItems.rows[cod1PreparedItems.items[i].ref.slot],&cod1PreparedItems.items[i].row,sizeof(Cod1LiveItem)))goto refuse;
 /* No callback between final ownership check and the routing transfer. The
  * journal remains available if actual runtime start subsequently fails. */
 cod1PreparedItems.executing=1;return 1;
refuse:LiveItemError(error,size,"prepared item execution transfer owner retired");return 0;
}
int G_Cod1ItemLifecyclePreparedCommit(const cod1ItemPreparedReceipt_t*input,char*error,size_t size){
 cod1ItemPreparedReceipt_t q;
 if(!LiveItemBuffers(input,sizeof(q),NULL,0,error,size)||!input)return 0;q=*input;
 if(!ItemPreparedReceipt(&q)||cod1PreparedItems.closed||!cod1PreparedItems.executing||cod1LiveItemGuard.active||!ItemPreparedSelection()){
  LiveItemError(error,size,"prepared item runtime commit owner retired");return 0;
 }
 memset(&cod1PreparedItems,0,sizeof(cod1PreparedItems));return 1;
}
int G_Cod1ItemLifecyclePreparedFreeGuard(const GscEntityRef*ref){
 if(!cod1PreparedItems.freeActive)return 1;
 return cod1PreparedItems.active&&cod1PreparedItems.closed&&cod1PreparedItems.aborting&&!cod1PreparedItems.cancelled&&
  ref&&LiveItemRefEqual(ref,&cod1PreparedItems.items[cod1PreparedItems.freeIndex].ref)&&
  ItemPreparedBody((unsigned)cod1PreparedItems.freeIndex,1);
}
int G_Cod1ItemLifecyclePreparedAbort(const cod1ItemPreparedReceipt_t*input,char*error,size_t size){
 cod1ItemPreparedReceipt_t q;unsigned i;int ok=1;GscEntityRef ref;
 if(!LiveItemBuffers(input,sizeof(q),NULL,0,error,size)||!input)return 0;q=*input;
 if(!ItemPreparedReceipt(&q))goto refuse;
 cod1PreparedItems.closed=1;
 if(cod1PreparedItems.aborting||cod1LiveItemGuard.active){cod1PreparedItems.cancelled=1;if(cod1LiveItemGuard.active)cod1LiveItemGuard.cancelled=1;goto refuse;}
 cod1PreparedItems.aborting=1;cod1PreparedItems.cancelled=0;
 for(i=0;i<5;i++){
  Cod1PreparedItem*p=cod1PreparedItems.items+i;
  if(!p->attempted||p->retired)continue;ref=p->ref;
  if(!G_Cod1EntitySameAllocation(&ref)){p->retired=1;continue;}
  if((cod1LiveItems.selected&&!ItemPreparedSelection())||!ItemPreparedBody(i,0)) {ok=0;continue;}
  if(cod1LiveItems.rows[ref.slot].serial&&(!p->installed||
     memcmp(&cod1LiveItems.rows[ref.slot],&p->row,sizeof(p->row)))){ok=0;continue;}
  cod1PreparedItems.freeIndex=(int)i;cod1PreparedItems.freeActive=1;
  G_FreeEntity(g_entities+ref.slot);
  cod1PreparedItems.freeActive=0;
  if(!G_Cod1EntitySameAllocation(&ref))p->retired=1;else ok=0;
  if(cod1PreparedItems.cancelled){ok=0;break;}
 }
 cod1PreparedItems.aborting=0;
 if(ok){memset(&cod1PreparedItems,0,sizeof(cod1PreparedItems));return 1;}
refuse:LiveItemError(error,size,"prepared item abort deferred: exact native prefix owner not retired");return 0;
}
