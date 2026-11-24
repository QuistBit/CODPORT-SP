/* Native32C00 death ->46200 unlink/model ->3E830 helpers ->entity retirement.
 * The real G_FreeEntity owns each allocation. Its explicit phase hooks fence
 * every callback before a memset; no ordinary/actor delete predicate changes. */
typedef struct {
 int active,checking,time,phase[3],resources;GscVehicleRow*row;GscVehicleRow expected;
 GscEntityRef refs[3];gentity_t bodies[3];
 const GscDeleteTransactionRequest*input;GscDeleteTransactionRequest request;
 const GscDeleteTransactionServices*borrowed;GscDeleteTransactionServices api;
}GscVehicleDeleteLease;
static GscVehicleDeleteLease gscVehicleDelete;
static int GscVehicleDeleteStoragePrivate(const void*p,size_t n){return C1V14_Overlap(p,n,&gscVehicleDelete,sizeof(gscVehicleDelete));}
static int GscVehicleDeleteInProgress(void){return gscVehicleDelete.active;}
static int GscVehicleDeleteSlot(const GscEntityRef*ref){int i;for(i=0;i<3;i++)if(gscVehicleDelete.refs[i].epoch&&GscVehicleRef(ref,gscVehicleDelete.refs+i))return i;return -1;}
static int GscVehicleDeleteScope(void){GscVehicleDeleteLease*l=&gscVehicleDelete;int i;
 if(!l->active||!gscVehicleBusy||gscVehicleCancelled||l->time!=level.time||!l->refs[0].epoch||l->refs[0].epoch!=G_Cod1EntityMapEpoch()||
    level.spawning||level.restarted||g_gametype.integer!=GT_SINGLE_PLAYER||!G_Cod1MapEntities()||
    memcmp(l->input,&l->request,sizeof(l->request))||memcmp(l->borrowed,&l->api,sizeof(l->api))||memcmp(l->row,&l->expected,sizeof(l->expected)))return 0;
 for(i=0;i<3;i++)if(l->refs[i].epoch){
  if(l->phase[i]==4){if(!G_Cod1EntityDeleteRetiredIdentity(l->refs+i,NULL))return 0;}
  else if(!G_Cod1EntitySameAllocation(l->refs+i))return 0;
  if(memcmp(g_entities+l->refs[i].slot,l->bodies+i,sizeof(gentity_t)))return 0;
 }
 return 1;
}
static int GscVehicleDeleteCurrent(void){GscVehicleDeleteLease*l=&gscVehicleDelete;int ok;
 if(!GscVehicleDeleteScope())return 0;if(l->checking){gscVehicleCancelled=1;return 0;}
 l->checking=1;ok=l->api.current(l->api.context);l->checking=0;
 return ok==1&&GscVehicleDeleteScope();
}
int G_Cod1VehicleFreePhase(const GscEntityRef*ref,int phase){GscVehicleDeleteLease*l=&gscVehicleDelete;int n,i;gentity_t retired;
 if(!l->active)return 1;
 n=GscVehicleDeleteSlot(ref);if(n<0||phase<0||phase>3||l->checking){gscVehicleCancelled=1;return 0;}
 if(phase==0){
  if(l->phase[n]||!GscVehicleDeleteCurrent())return 0;l->phase[n]=1;return 1;
 }
 if(phase==1){
  if(l->phase[n]!=1)return 0;l->bodies[n].r.linked=qfalse;l->phase[n]=2;
  return GscVehicleDeleteCurrent();
 }
 if(phase==2){
  if(l->phase[n]!=2||(!n&&!l->resources)||!GscVehicleDeleteCurrent())return 0;
  l->phase[n]=3;return 1;
 }
 if(l->phase[n]!=3)return 0;
 memset(&retired,0,sizeof(retired));retired.classname=g_entities[ref->slot].classname;retired.freetime=l->time;
 if(!retired.classname||strcmp(retired.classname,"freed")||memcmp(g_entities+ref->slot,&retired,sizeof(retired))||!G_Cod1EntityDeleteRetiredIdentity(ref,NULL))return 0;
 l->bodies[n]=retired;l->phase[n]=4;
 if(!GscVehicleDeleteCurrent())return 0;
 if(!n){
  for(i=1;i<3;i++)if(l->refs[i].epoch&&l->phase[i]!=4)return 0;
  memset(l->row,0,sizeof(*l->row));l->expected=*l->row;
 }
 return GscVehicleDeleteCurrent();
}
static int GscVehicleDeleteBeforeFree(const GscEntityRef*ref){GscVehicleDeleteLease*l=&gscVehicleDelete;cod1DObjFree_t q,seal;int n,i,k,status;
 n=GscVehicleDeleteSlot(ref);if(n<0||l->phase[n]!=2||!GscVehicleDeleteCurrent())return 0;
 if(n)return 1;
 if(l->resources||!l->row->dobj||l->row->freeing)return 0;
 l->row->freeing=1;l->expected=*l->row;
 memset(&q,0,sizeof(q));q.version=COD1_DOBJ_VERSION;q.bytes=sizeof(q);q.key=GscVehicleKey(l->row);seal=q;
 status=trap_Cod1DObjFree(&q);
 /* A successful release is irreversible even if the outer Session expired.
  * Preserve that exact resource receipt for subsequent owned cleanup. */
 if(status==COD1_DOBJ_OK&&!memcmp(&q,&seal,sizeof(q))&&GscVehicleSameRow(l->row,&l->expected)){
  l->row->dobj=0;l->expected=*l->row;
 }
 if(status!=COD1_DOBJ_OK||memcmp(&q,&seal,sizeof(q))||!GscVehicleDeleteCurrent())return 0;
 /* Original3D4F0 health0 silently publishes both helpers before freeing.
  * Expected ordinary point-hull link side effects are derived before trap. */
 for(i=1;i<3;i++)if(l->refs[i].epoch){gentity_t*e=g_entities+l->refs[i].slot;gentity_t expected=l->bodies[i];
  if(expected.r.bmodel||expected.r.contents||expected.r.linkcount==INT_MAX||expected.s.loopSound)return 0;
  expected.s.solid=0;expected.r.linked=qtrue;expected.r.linkcount++;
  for(k=0;k<3;k++){expected.r.absmin[k]=(expected.r.currentOrigin[k]+expected.r.mins[k])-1.0f;expected.r.absmax[k]=(expected.r.currentOrigin[k]+expected.r.maxs[k])+1.0f;}
  l->bodies[i]=expected;l->row->childLinked[i-1]=1;l->expected=*l->row;
  trap_LinkEntity(e);if(!GscVehicleDeleteCurrent())return 0;
 }
 for(i=1;i<3;i++)if(l->refs[i].epoch){
  G_FreeEntity(g_entities+l->refs[i].slot);
  if(l->phase[i]!=4||!GscVehicleDeleteCurrent())return 0;
  l->row->record.soundEntities[i-1]=2047;l->expected=*l->row;
 }
 l->resources=1;return GscVehicleDeleteCurrent();
}
static int GscVehicleDeleteTopology(const GscVehicleRow*r){int i,k;const gentity_t*e;
 if(!r||!r->committed||r->freeing||r->deleteAttempted||!GscVehiclePrepared((GscVehicleRow*)r))return 0;
 for(k=0;k<3;k++){const GscEntityRef*ref=k?r->children+k-1:&r->owner;if(!ref->epoch)continue;e=g_entities+ref->slot;
  if(e->teamchain||e->teammaster||e->tagParent||e->tagName||(e->team&&e->team[0])||e->neverFree)return 0;
  for(i=0;i<level.num_entities;i++)if(g_entities[i].inuse&&g_entities[i].tagParent==e)return 0;
 }
 /* Qualified path-only vehicles carry no driver/target or external helper
  * ownership. Future targeted vehicles need native3E940 reference cleanup. */
 for(i=0;i<32;i++)if(gscVehicles[i].present&&gscVehicles[i].record.targetEntity!=2047)return 0;
 return 1;
}
static int GscVehicleDelete(void*u,const GscDeleteTransactionRequest*input,const GscDeleteTransactionServices*api,char*error,size_t cap){
 GscVehicleDeleteLease*l=&gscVehicleDelete;GscVehicleRow*r;GscDeleteTransactionRequest q;GscDeleteTransactionServices services;int i,ok=0;(void)u;
 if(!input||!api)return 0;r=GscVehicleFind(&input->entity);
 if(!r)return G_Cod1EntityDeleteTransactionHost()->invoke(NULL,input,api,error,cap);
 if(gscVehicleBusy||l->active){gscVehicleCancelled=1;return 0;}
 if((cap&&!error)||GscVehiclePrivate(input,sizeof(*input))||GscVehiclePrivate(api,sizeof(*api))||GscVehiclePrivate(error,cap)||
  C1V14_Overlap(input,sizeof(*input),api,sizeof(*api))||C1V14_Overlap(input,sizeof(*input),error,cap)||C1V14_Overlap(api,sizeof(*api),error,cap)||
  !api->current||!api->notify_death||input->now_ms<0||!GscVehicleDeleteTopology(r))return 0;
 q=*input;services=*api;memset(l,0,sizeof(*l));l->active=1;l->row=r;l->expected=*r;l->request=q;l->input=input;l->api=services;l->borrowed=api;l->time=level.time;
 l->refs[0]=r->owner;l->refs[1]=r->children[0];l->refs[2]=r->children[1];for(i=0;i<3;i++)if(l->refs[i].epoch)l->bodies[i]=g_entities[l->refs[i].slot];
 gscVehicleBusy=1;gscVehicleCancelled=0;
 if(!GscVehicleDeleteCurrent())goto done;
 r->deleteAttempted=1;l->expected=*r;
 if(services.notify_death(services.context,error,cap)!=1||!GscVehicleDeleteCurrent())goto done;
 G_FreeEntity(g_entities+q.entity.slot);
 ok=l->phase[0]==4&&!l->row->present&&GscVehicleDeleteCurrent();
 done:
 if(!ok&&GscVehicleSameRow(r,&l->expected)){r->broken=1;r->freeing=0;}
 memset(l,0,sizeof(*l));gscVehicleBusy=0;
 if(!ok)GscTestError(error,cap,"vehicle death/free transaction refused; exact owner retained or retired");
 return ok;
}
