/* Original23A20/233D0 sets C4=00200004. The admitted setmode/target/fire
 * and mount controls change turret/actor state, not this filter cell.
 * This is a mask-only capability: it grants no turret collision geometry. */
int G_Cod1TurretNavigationMask(const GscEntityRef*ref,unsigned*out){
 C1TRecord*r;const gentity_t*e;uint64_t actor;
 if(!ref||!out||!cod1EntityReady||level.spawning||level.restarted||cod1TurretBusy||cod1TurretFrameBusyEpoch||
  TurretStoragePrivate(out,sizeof(*out))||G_Cod1EntityStorageProtected(out,sizeof(*out))||
  !(r=TurretFind(ref))||r->nativeContents!=0x200004||cod1TurretResourceEpoch!=ref->epoch||!cod1TurretAnimatedSkeleton||
  !G_Cod1ActorPresenceRead(ref,&actor,NULL,0)||actor)return 0;
 e=g_entities+ref->slot;
 if(e->client||e->aiCharacter||e->r.bmodel||!e->r.linked||e->r.contents||e->s.eType!=ET_GENERAL||e->s.modelindex!=r->modelIndex||
  !e->classname||strcmp(e->classname,"misc_mg42")||e->think||e->touch||e->use||e->die||e->nextthink||e->takedamage||
  e->scriptEvents||e->numScriptEvents||e->scriptStatus.scriptEventIndex!=-1||r->poseTick>level.time||e->s.cod1TurretTick!=r->poseTick)return 0;
 *out=r->nativeContents;return 1;
}
