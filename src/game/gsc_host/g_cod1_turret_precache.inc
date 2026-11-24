/* Native311D0 -> AF90 -> existing native item registrar1C490. This shares
 * GscTestItem state/configstring/model dependencies; no second precache bitset.
 * Only the actual loaded CW definitions are searchable, as for weapon queries.
 * Expanding the initial CW resource table remains a separate dependency. */
static int gscTurretPrecacheBusy,gscTurretPrecacheCancelled;
typedef struct {GscTurretPrecacheRequest q;const GscTurretPrecacheRequest*borrowed;GscTestItemScene owner;int time;unsigned registry;int(*current)(void*);void*context;} GscTurretPrecacheLease;
static int GscTurretPrecachePrivate(const void*p,size_t n){int i,count=CW_Count();
 if(GscVehiclePrivate(p,n)||C1V14_Overlap(p,n,&gscTestItem,sizeof(gscTestItem))||C1V14_Overlap(p,n,&gscTestSession,sizeof(gscTestSession))||C1V14_Overlap(p,n,&gscTestId,sizeof(gscTestId))||C1V14_Overlap(p,n,&gscTurretPrecacheBusy,sizeof(gscTurretPrecacheBusy))||C1V14_Overlap(p,n,&gscTurretPrecacheCancelled,sizeof(gscTurretPrecacheCancelled)))return 1;
 if(count<0||count>CW_MAX_DEFS)return n!=0;for(i=0;i<count;i++)if(C1V14_Overlap(p,n,CW_Get(i),sizeof(cwDef_t)))return 1;return 0;
}
static int GscTurretPrecacheDirect(const GscTurretPrecacheLease*l){
 return gscTurretPrecacheBusy&&!gscTurretPrecacheCancelled&&!gscTestStopRequested&&!gscTestCompleted&&gscTestItemInitialization&&!gscTestItem.failed&&
  gscTestSession==l->owner.session&&gscTestId==l->owner.id&&gscTestItem.id==l->owner.id&&gscTestItem.incarnation==l->owner.incarnation&&gscTestItem.epoch==l->owner.epoch&&
  G_Cod1EntityMapEpoch()==l->owner.map_epoch&&level.time==l->time&&!level.restarted&&!level.spawning&&l->registry&&l->registry==CW_RegistryRevision()&&!memcmp(l->borrowed,&l->q,sizeof(l->q));
}
static int GscTurretPrecacheCurrent(GscTurretPrecacheLease*l){
 return GscTurretPrecacheDirect(l)&&GscTestItemTime(&l->owner,l->q.now_ms)&&GscTurretPrecacheDirect(l)&&l->current(l->context)==1&&GscTurretPrecacheDirect(l)&&GscTestItemTime(&l->owner,l->q.now_ms)&&GscTurretPrecacheDirect(l);
}
static void GscTurretPrecacheReset(void){if(gscTurretPrecacheBusy)gscTurretPrecacheCancelled=1;else gscTurretPrecacheCancelled=0;}
static int GscTurretPrecacheInvoke(void*u,const GscTurretPrecacheRequest*input,char*error,size_t cap,int(*current)(void*),void*context){
 GscTurretPrecacheLease l;GscPrecacheItemEvent event;cod1ItemDescriptor_t descriptor;int index,ok=0;(void)u;
 if(gscTurretPrecacheBusy){gscTurretPrecacheCancelled=1;return 0;}
 if(!input||!current||(cap&&!error)||GscTurretPrecachePrivate(input,sizeof(*input))||GscTurretPrecachePrivate(error,cap)||C1V14_Overlap(input,sizeof(*input),error,cap)||!input->program||!input->operation||input->operation==UINT64_MAX||!memchr(input->name,0,128))return 0;
 memset(&l,0,sizeof(l));l.q=*input;l.borrowed=input;l.owner=gscTestItem;l.time=level.time;l.registry=CW_RegistryRevision();l.current=current;l.context=context;
 gscTurretPrecacheBusy=1;gscTurretPrecacheCancelled=0;
 if(!GscTurretPrecacheCurrent(&l))goto done;
 index=!Q_stricmp(l.q.name,"none")?-2:CW_Find(l.q.name);
 if(!GscTurretPrecacheCurrent(&l))goto done;
 if(index<0){if(index==-1)G_DPrintf("Couldn't find weapon \"%s\"\n",l.q.name);ok=GscTurretPrecacheCurrent(&l);goto done;}
 if(!CI_Resolve(l.q.name,&descriptor)||!descriptor.dynamic||descriptor.weapon_index!=index||descriptor.index!=index+1||!GscTurretPrecacheCurrent(&l))goto done;
 memset(&event,0,sizeof(event));memcpy(event.name,l.q.name,128);event.operation_id=l.q.operation;
 if(!GscTestItemPublish(NULL,l.owner.id,l.q.now_ms,&event,error,cap)||!GscTurretPrecacheCurrent(&l)||gscTestItem.operation_id!=l.q.operation||!gscTestItem.registered[index+1]||!gscTestItem.complete[index+1])goto done;
 ok=1;
done:gscTurretPrecacheBusy=0;if(!ok&&error&&cap&&!GscTurretPrecachePrivate(error,cap)&&!error[0])GscTestError(error,cap,"precacheTurret shared registration/map/initialization boundary refused");return ok;
}
static const GscTurretPrecacheHost*GscTurretPrecacheProvider(void){static const GscTurretPrecacheHost h={NULL,GscTurretPrecacheInvoke};return &h;}
