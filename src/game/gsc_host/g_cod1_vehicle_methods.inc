/* GAME state methods. C47 has no wheel tags; query that real topology rather
 * than assuming that an arbitrary plane shares the resource's skeleton. */
typedef struct {
 GscVehicleMethodRequest q;const GscVehicleMethodRequest*input;
 GscVehicleRow*row,expected;cod1VehiclePathLease_t graph;
 int time;uint64_t epoch;int(*current)(void*);void*context;
} GscVehicleMethodLease;
static int GscVehicleMethodBase(const GscVehicleMethodLease*l){
 return gscVehicleBusy&&!gscVehicleCancelled&&level.time==l->time&&G_Cod1EntityMapEpoch()==l->epoch&&
 !level.spawning&&!level.restarted&&!memcmp(l->input,&l->q,sizeof(l->q))&&!memcmp(l->row,&l->expected,sizeof(l->expected))&&
 GscVehiclePrepared(l->row)&&(!l->graph.epoch||G_Cod1VehiclePathCurrent(&l->graph,(l->q.kind==1||l->q.kind==3)?&l->q.node:NULL));
}
static int GscVehicleMethodCurrent(GscVehicleMethodLease*l){return GscVehicleMethodBase(l)&&l->current(l->context)==1&&GscVehicleMethodBase(l);}
static int GscVehicleNoWheels(GscVehicleMethodLease*l){
 static const char*names[]={"tag_wheel_front_left","tag_wheel_front_right","tag_wheel_back_left","tag_wheel_back_right","tag_wheel_middle_left","tag_wheel_middle_right"};
 cod1DObjQuery_t q,expected;cod1DObjQueryResult_t result;unsigned i;int status;
 for(i=0;i<6;i++){
  if(!GscVehicleMethodCurrent(l))return 0;memset(&q,0,sizeof(q));memset(&result,0,sizeof(result));
  q.version=COD1_DOBJ_QUERY_VERSION;q.bytes=sizeof(q);q.key=GscVehicleKey(l->row);q.topologySerial=l->expected.topology.topologySerial;q.phase=COD1_DOBJ_QUERY_BONE;strcpy(q.tag,names[i]);expected=q;
  status=trap_Cod1DObjQuery(&q,&result);
  if(!GscVehicleMethodCurrent(l)||memcmp(&q,&expected,sizeof(q))||status!=COD1_DOBJ_OK||result.version!=COD1_DOBJ_QUERY_VERSION||result.bytes!=sizeof(result)||
   result.topologySerial!=q.topologySerial||result.modelCount!=1||result.boneCount!=l->expected.topology.boneCount||!memchr(result.model,0,sizeof(result.model))||strcmp(result.model,l->expected.model)||result.boneIndex!=-1)return 0;
 }return 1;
}
static int GscVehicleMethodPublish(GscVehicleMethodLease*l,const C1V14Record*record){
 gentity_t*e;unsigned i;int changed=0;GscVehicleRow*r=l->row;
 if(!GscVehicleMethodCurrent(l))return 0;e=g_entities+r->owner.slot;
 for(i=0;i<3;i++)if(e->r.currentOrigin[i]!=record->origin[i]||e->s.pos.trBase[i]!=record->origin[i]||e->r.currentAngles[i]!=record->angles[i]||e->s.apos.trBase[i]!=record->angles[i])changed=1;
 r->record=*record;
 if(changed){G_SetOrigin(e,r->record.origin);G_SetAngle(e,r->record.angles);e->s.pos.trType=e->s.apos.trType=TR_INTERPOLATE;}
 l->expected=*r;
 if(!changed)return GscVehicleMethodCurrent(l);
 trap_LinkEntity(e);if(!GscVehicleMethodCurrent(l))return 0;
 for(i=0;i<2;i++)if(r->children[i].epoch){
  GscEntityRef owner=r->children[i];e=g_entities+owner.slot;
  if(!GscVehicleMethodCurrent(l)||!G_Cod1EntitySameAllocation(&owner))return 0;
  G_SetOrigin(e,r->record.origin);G_SetAngle(e,r->record.angles);e->s.pos.trType=e->s.apos.trType=TR_INTERPOLATE;
  memcpy(r->childOrigin[i],r->record.origin,12);memcpy(r->childAngles[i],r->record.angles,12);r->childLinked[i]=1;l->expected=*r;
  trap_LinkEntity(e);
  if(!GscVehicleMethodCurrent(l)||!G_Cod1EntitySameAllocation(&owner)||!e->r.linked||memcmp(e->r.currentOrigin,r->record.origin,12)||memcmp(e->r.currentAngles,r->record.angles,12))return 0;
 }
 return 1;
}
static int GscVehicleMethod(void*u,const GscVehicleMethodRequest*input,char*error,size_t cap,int(*current)(void*),void*context){
 GscVehicleMethodLease l;GscVehicleRow*r;C1V14Record staged;unsigned mode,count=0;int ok=0;(void)u;
 if(gscVehicleBusy){gscVehicleCancelled=1;return 0;}
 if(!input||!current||(cap&&!error)||GscVehiclePrivate(input,sizeof(*input))||GscVehiclePrivate(error,cap)||C1V14_Overlap(input,sizeof(*input),error,cap))return 0;
 memset(&l,0,sizeof(l));l.q=*input;l.input=input;l.current=current;l.context=context;l.time=level.time;l.epoch=G_Cod1EntityMapEpoch();
 r=GscVehicleFind(&l.q.owner);if(!r||!r->committed||r->freeing||r->broken||r->program!=l.q.program||l.q.kind<1||l.q.kind>6||!GscVehiclePrepared(r))return 0;
 l.row=r;l.expected=*r;gscVehicleBusy=1;gscVehicleCancelled=0;
 if(!GscVehicleMethodCurrent(&l))goto done;
 if(l.q.kind==1||l.q.kind==3){
  if(!G_Cod1VehiclePathCopy(&l.q.node,gscVehicleGraph,GSC_VEHICLE_NODE_CAPACITY,&l.graph)||!GscVehicleMethodCurrent(&l))goto done;count=l.graph.count;
 }else if(l.q.kind==2){l.graph=r->pathLease;if(!G_Cod1VehiclePathCurrent(&l.graph,NULL)||!GscVehicleMethodCurrent(&l))goto done;count=l.graph.count;}
 if(!C1VM14_Apply(&r->record,r->mode,l.q.kind,gscVehicleGraph,count,(int)l.q.node.index,l.q.values[0],l.q.values[1],&staged,&mode))goto done;
 if(l.q.kind==1){
  if(!GscVehicleMethodPublish(&l,&staged)||!GscVehicleNoWheels(&l))goto done;
  r->pathLease=l.graph;l.expected=*r;
 }else{r->record=staged;r->mode=mode;l.expected=*r;}
 ok=GscVehicleMethodCurrent(&l);
done:
 /* A failed world callback is terminal for this exact owner. Never adopt
  * the state supplied by that callback as a new expected allocation/pose. */
 if(!ok&&GscVehicleSameRow(r,&l.expected))r->broken=1;
 gscVehicleBusy=0;if(!ok)GscTestError(error,cap,"vehicle method owner/path/topology boundary refused");return ok;
}
static const GscVehicleMethodHost gscVehicleMethods={NULL,GscVehicleMethod};
