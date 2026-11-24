/* GAME56CC0: the once-per-server-tick gate belongs to the Actor, not
 * the weapon or script task. GSC owns bulletsInClip and firing cadence.
 * This first domain is a genuine no-enemy bullet shot. Enemy aiming, forced
 * vector clamping, projectiles and all dynamic entity hits remain explicit
 * refusals, never an acknowledged shot without discharge. */
static int ActorShoot(C1ALCall*c,const GscActorLookRequest*q,GscActorLookReply*out,char*error,size_t cap){
 C1AShotClock old=cod1ActorShotClocks[c->lease.actorIndex],next;
 cod1ActorRecord_t actor=cod1Actors[c->lease.actorIndex];cod1SentientRecord_t sentient=cod1Sentients[c->lease.sentientIndex];
 gentity_t ownerBody=g_entities[c->entity.slot];uint32_t oldSeed=cod1TurretShotRandom.crt;
 C1AMuzzle muzzle;const cwDef_t*def;uint32_t revision=CW_RegistryRevision(),seed=cod1TurretShotRandom.crt;
 cod1SpatialTraceQuery_t query;trace_t trace;float basis[3][3],end[3];int index,weapon=WP_NONE,i;
 GscEntityRef events[2]={{0}};gentity_t*shotEvent=NULL,*impactEvent=NULL;int ok=0;
 if(!ActorLookFence(c))return 0;
 if(old.valid&&old.epoch==c->entity.epoch&&old.lease==c->lease.serial&&old.tick==c->now){out->kind=GSC_ACTOR_LOOK_RESULT_VOID;return 1;}
 if(q->tag[0]||!sentient.enemyKnown||sentient.enemy.epoch||sentient.enemy.slot||sentient.enemy.generation){if(error&&cap)snprintf(error,cap,"shoot: enemy/forced-target aiming not qualified");return 0;}
 if(!actor.scriptStrings.weaponPresent||(index=CW_Find(actor.scriptStrings.weapon))<0||!(def=CW_Get(index))||strcmp(CW_String(def,"weaponType"),"bullet")){if(error&&cap)snprintf(error,cap,"shoot: registered bullet WeaponDef required");return 0;}
 for(i=1;i<WP_NUM_WEAPONS;i++)if(CW_WeaponRoot(i)==CW_FamilyRoot(index)){weapon=i;break;}
 if(weapon==WP_NONE||!ActorMuzzle(c,&muzzle,error,cap))return 0;
 /* Retail no-enemy path uses the muzzle's forward direction. Zero spread
  * still consumes the two native CRT draws in the existing48AC0 kernel. */
 memcpy(basis[0],muzzle.forward,12);memcpy(basis[1],actor.nativeData.orientation.lookright,12);memcpy(basis[2],actor.nativeData.orientation.lookup,12);
 if(!C1TB_EndPoint(basis[0],basis[1],basis[2],muzzle.origin,0,&seed,end))return 0;
 memset(&query,0,sizeof(query));query.version=COD1_SPATIAL_VERSION;query.bytes=sizeof(query);query.contentsMask=C1TS_TRACE_MASK;
 query.passEntity[0]=c->entity.slot;query.passEntity[1]=ENTITYNUM_NONE;memcpy(query.start,muzzle.origin,12);memcpy(query.end,end,12);
 /* Actor generic damage/penetration and hit-location contracts are not
  * the mounted MG42 damage contract. World/clear shots are the admitted
  * domain; dynamic entity hits fail before discharge or damage is committed. */
 if(trap_Cod1SpatialPointTrace(&query,&trace)!=COD1_SPATIAL_OK||!ActorLookFence(c)||!isfinite(trace.fraction)||trace.fraction<0||trace.fraction>1||trace.startsolid||trace.allsolid||trace.contents&0x10)goto done;
 if(trace.fraction<1&&trace.entityNum!=ENTITYNUM_WORLD)goto done;
 shotEvent=TurretShotAllocate(muzzle.origin,&events[0]);if(!shotEvent)goto done;
 if(trace.fraction<1){impactEvent=TurretShotAllocate(trace.endpos,&events[1]);if(!impactEvent)goto done;}
 if(!ActorLookFence(c)||oldSeed!=cod1TurretShotRandom.crt||memcmp(&ownerBody,g_entities+c->entity.slot,sizeof(ownerBody))||revision!=CW_RegistryRevision()||memcmp(&actor,cod1Actors+c->lease.actorIndex,sizeof(actor))||
    memcmp(&sentient,cod1Sentients+c->lease.sentientIndex,sizeof(sentient))||memcmp(&old,cod1ActorShotClocks+c->lease.actorIndex,sizeof(old)))goto done;
 next.epoch=c->entity.epoch;next.lease=c->lease.serial;next.tick=c->now;next.valid=1;
 cod1ActorShotClocks[c->lease.actorIndex]=next;cod1TurretShotRandom.crt=seed;
 /* Publish the real discharge after all fallible preparation. No ammo field
  * mutation: combat::ShootRunningVolley decrements its own script counter. */
 out->kind=GSC_ACTOR_LOOK_RESULT_VOID;ok=1;
 if(G_Cod1EntitySameAllocation(&events[0])&&G_Cod1EntitySameAllocation(&c->entity)){
  shotEvent=g_entities+events[0].slot;shotEvent->s.eType=ET_EVENTS+EV_FIRE_WEAPON;shotEvent->s.weapon=weapon;shotEvent->s.eventParm=index+1;
  shotEvent->s.clientNum=c->entity.slot;shotEvent->s.otherEntityNum=c->entity.slot;shotEvent->r.svFlags&=~SVF_NOCLIENT;trap_LinkEntity(shotEvent);
 }
 if(impactEvent&&G_Cod1EntitySameAllocation(&events[1])){
  impactEvent=g_entities+events[1].slot;impactEvent->s.eType=ET_EVENTS+EV_COD1_TURRET_IMPACT;
  CI_WriteEvent(&impactEvent->s,&trace,trace.endpos,muzzle.origin,end,weapon,CW_Number(def,"rifleBullet")!=0);
  impactEvent->s.otherEntityNum=c->entity.slot;impactEvent->r.svFlags&=~SVF_NOCLIENT;trap_LinkEntity(impactEvent);
 }
 G_Printf("[actor shoot] owner=%u tick=%d weapon=%s hit=%d\n",c->entity.slot,c->now,actor.scriptStrings.weapon,trace.entityNum);
done:
 if(!ok){TurretShotDrop(&events[0]);TurretShotDrop(&events[1]);if(error&&cap)snprintf(error,cap,"shoot: physical discharge ownership/domain unavailable");}
 return ok;
}
