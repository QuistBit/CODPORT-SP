/* Fixed original6FAD0 zero-transition input, NOT a spatial world receipt.
 * It reads native active/path bits only. Every live slot is represented;
 * unknown owners (including the currently unsupported player) refuse.
 * 1000 alone and2000 alone do not reach native726C0. Both remain unsupported
 * here until that candidate/trace contract is separately implemented. */
typedef struct {cod1PathNodeOwner_t owner;cod1ActorLease_t actor;int inuse,reserve,helper,turret,spawner,isActor;} C1LateDisconnectRow;
static struct {
 uint64_t serial,receipt,epoch;int count,time,busy,cancelled;
 C1LateDisconnectRow rows[MAX_GENTITIES];
} cod1LateDisconnectWorld;
int G_Cod1LateDisconnectPrivate(const void*p,size_t n){
 return Cod1PathWorldOverlap(p,n,&cod1LateDisconnectWorld,sizeof(cod1LateDisconnectWorld));
}
static void Cod1LateDisconnectReset(void){
 cod1LateDisconnectWorld.receipt=0;cod1LateDisconnectWorld.epoch=0;
 cod1LateDisconnectWorld.count=cod1LateDisconnectWorld.time=0;
 memset(cod1LateDisconnectWorld.rows,0,sizeof(cod1LateDisconnectWorld.rows));
 cod1LateDisconnectWorld.cancelled=cod1LateDisconnectWorld.busy?1:0;
 /* Serial is never reset by map/birth/cleanup. No token ABA revival. */
}
static int Cod1LateDisconnectOutput(const void*p,size_t n){
 return p&&n&&(uintptr_t)p<=UINTPTR_MAX-(n-1)&&
  !G_Cod1LateDisconnectPrivate(p,n)&&!G_Cod1EntityStorageProtected(p,n)&&!G_Cod1LateDisconnectInputsPrivate(p,n)&&
  !G_Cod1LateDisconnectGraphPrivate(p,n)&&
  !Cod1PathEntityOverlap(p,n)&&!Cod1PathWorldOverlap(p,n,&g_cod1MapEntities,sizeof(g_cod1MapEntities))&&
  !Cod1PathWorldOverlap(p,n,cod1PathWorldRows,sizeof(cod1PathWorldRows))&&
  !Cod1PathWorldOverlap(p,n,&cod1PathWorldSerial,sizeof(cod1PathWorldSerial))&&
  !Cod1PathWorldOverlap(p,n,&cod1PathWorldReceipt,sizeof(cod1PathWorldReceipt))&&
  !Cod1PathWorldOverlap(p,n,&cod1PathWorldEpoch,sizeof(cod1PathWorldEpoch))&&
  !Cod1PathWorldOverlap(p,n,&cod1PathWorldPhase,sizeof(cod1PathWorldPhase))&&
  !Cod1PathWorldOverlap(p,n,&cod1PathWorldCount,sizeof(cod1PathWorldCount))&&
  !Cod1PathWorldOverlap(p,n,&cod1PathWorldUsable,sizeof(cod1PathWorldUsable))&&
  !Cod1PathWorldOverlap(p,n,cod1NodeWorldRows,sizeof(cod1NodeWorldRows))&&
  !Cod1PathWorldOverlap(p,n,&cod1NodeWorldSerial,sizeof(cod1NodeWorldSerial))&&
  !Cod1PathWorldOverlap(p,n,&cod1NodeWorldReceipt,sizeof(cod1NodeWorldReceipt))&&
  !Cod1PathWorldOverlap(p,n,&cod1NodeWorldEpoch,sizeof(cod1NodeWorldEpoch))&&
  !Cod1PathWorldOverlap(p,n,&cod1NodeWorldCount,sizeof(cod1NodeWorldCount))&&
  !Cod1PathWorldOverlap(p,n,&cod1NodeWorldTime,sizeof(cod1NodeWorldTime));
}
static int Cod1LateDisconnectRowRead(unsigned slot,C1LateDisconnectRow*out){
 C1LateDisconnectRow value;C1SpawnTraceRow typed;cod1ActorPathView_t actor;GscEntityRef ref;
 memset(&value,0,sizeof(value));value.inuse=!!g_entities[slot].inuse;
 if(!value.inuse){*out=value;return 1;}
 if(!G_Cod1EntityCapture(g_entities+slot,&ref)||ref.slot!=slot)return 0;
 value.owner.ref=ref;value.reserve=G_Cod1BodyReserveCurrent(&ref);value.helper=G_Cod1ScriptHelperCurrent(&ref);
 value.turret=Cod1PathEntityTurretCurrent(&ref);value.spawner=Cod1PathEntitySpawnerCurrent(&ref);
 value.isActor=G_Cod1ActorPathRead(&ref,&actor);if(value.isActor)value.actor=actor.lease;
 if(value.reserve||value.helper){
  if(value.reserve+value.helper!=1||value.isActor||value.turret||value.spawner)return 0;
  value.owner.adapter=1;*out=value;return 1;
 }
 memset(&typed,0,sizeof(typed));typed.ref=ref;typed.isTurret=value.turret;typed.isSpawner=value.spawner;
 if((typed.isTurret&&typed.isSpawner)||((typed.isTurret||typed.isSpawner)&&value.isActor))return 0;
 if(!Cod1PathNodeOwnerFlags(&typed,&value.owner.flags)||
    (value.owner.flags&~0x3000u)||(value.owner.flags&0x3000u)==0x3000u)return 0;
 value.owner.active=1;*out=value;return 1;
}
int G_Cod1LateDisconnectHeld(uint64_t receipt){
 return receipt&&receipt==cod1LateDisconnectWorld.receipt&&
  !cod1LateDisconnectWorld.busy&&!cod1LateDisconnectWorld.cancelled;
}
int G_Cod1LateDisconnectCurrent(uint64_t receipt){
 int i,ok=0;C1LateDisconnectRow current;
 if(!receipt||receipt!=cod1LateDisconnectWorld.receipt)return 0;
 if(cod1LateDisconnectWorld.busy){cod1LateDisconnectWorld.cancelled=1;return 0;}
 cod1LateDisconnectWorld.busy=1;
 if(!G_Cod1EntityMapEpoch()||!g_cod1MapEntities||cod1PathWorldPhase||
    cod1LateDisconnectWorld.epoch!=G_Cod1EntityMapEpoch()||
    cod1LateDisconnectWorld.count!=level.num_entities||cod1LateDisconnectWorld.time!=level.time)goto done;
 for(i=0;i<cod1LateDisconnectWorld.count;i++){
  if(!Cod1LateDisconnectRowRead((unsigned)i,&current)||
     memcmp(&current,&cod1LateDisconnectWorld.rows[i],sizeof(current))||cod1LateDisconnectWorld.cancelled)goto done;
 }
 ok=!cod1LateDisconnectWorld.cancelled&&g_cod1MapEntities&&!cod1PathWorldPhase&&
  cod1LateDisconnectWorld.epoch==G_Cod1EntityMapEpoch()&&cod1LateDisconnectWorld.count==level.num_entities&&
  cod1LateDisconnectWorld.time==level.time;
done:
 cod1LateDisconnectWorld.busy=0;if(!ok)Cod1LateDisconnectReset();return ok;
}
int G_Cod1LateDisconnectBegin(uint64_t*out){
 int i,ok=0;uint64_t receipt=0;
 if(!Cod1LateDisconnectOutput(out,sizeof(*out)))return 0;
 if(cod1LateDisconnectWorld.busy){cod1LateDisconnectWorld.cancelled=1;return 0;}
 if(cod1LateDisconnectWorld.receipt||cod1LateDisconnectWorld.serial==UINT64_MAX||
    !G_Cod1EntityMapEpoch()||!g_cod1MapEntities||cod1PathWorldPhase||
    level.num_entities<0||level.num_entities>MAX_GENTITIES)return 0;
 cod1LateDisconnectWorld.busy=1;cod1LateDisconnectWorld.cancelled=0;
 cod1LateDisconnectWorld.epoch=G_Cod1EntityMapEpoch();cod1LateDisconnectWorld.count=level.num_entities;
 cod1LateDisconnectWorld.time=level.time;memset(cod1LateDisconnectWorld.rows,0,sizeof(cod1LateDisconnectWorld.rows));
 for(i=0;i<cod1LateDisconnectWorld.count;i++)if(!Cod1LateDisconnectRowRead((unsigned)i,cod1LateDisconnectWorld.rows+i)||
    cod1LateDisconnectWorld.cancelled)goto done;
 if(cod1LateDisconnectWorld.epoch!=G_Cod1EntityMapEpoch()||cod1LateDisconnectWorld.count!=level.num_entities||
    cod1LateDisconnectWorld.time!=level.time||cod1LateDisconnectWorld.cancelled)goto done;
 receipt=++cod1LateDisconnectWorld.serial;cod1LateDisconnectWorld.receipt=receipt;ok=1;
done:
 cod1LateDisconnectWorld.busy=0;
 if(!ok||!G_Cod1LateDisconnectCurrent(receipt)){Cod1LateDisconnectReset();return 0;}
 *out=receipt;return 1;
}
void G_Cod1LateDisconnectEnd(uint64_t receipt){
 if(receipt&&receipt==cod1LateDisconnectWorld.receipt)Cod1LateDisconnectReset();
}
int G_Cod1LateDisconnectOwners(uint64_t receipt,cod1PathNodeOwners_t*out){
 cod1PathNodeOwners_t value;int i;
 if(!Cod1LateDisconnectOutput(out,sizeof(*out))||!G_Cod1LateDisconnectCurrent(receipt))return 0;
 memset(&value,0,sizeof(value));value.count=(unsigned)cod1LateDisconnectWorld.count;value.time=cod1LateDisconnectWorld.time;
 for(i=0;i<cod1LateDisconnectWorld.count;i++)value.rows[i]=cod1LateDisconnectWorld.rows[i].owner;
 if(!G_Cod1LateDisconnectCurrent(receipt))return 0;*out=value;return 1;
}
