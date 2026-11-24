/* Native count row2008a2e0: entity+1f8, type0, no custom setter. The
 * stalingrad factory reads this same live cell, including after spawned.
 * No factory admission or successful spawn is supplied by field access. */
static int Cod1SpawnerCountPrivate(const void *p,size_t n) {
    return Cod1SpawnerOverlap(p,n,&cod1SpawnerStorage,sizeof(cod1SpawnerStorage))||
        Cod1SpawnerOverlap(p,n,g_entities,sizeof(g_entities))||
        Cod1SpawnerOverlap(p,n,&level,sizeof(level));
}
static int Cod1SpawnerCountAccess(const GscEntityRef *input,
    const GscStringCastValue *value,GscStringCastValue *out,char *error,size_t size) {
    GscEntityRef ref;GscStringCastValue copy,result;int32_t desired=0;
    const cod1SpawnerRecord_t *record;const GscEntityHost *entities;
    double number;int write=value!=NULL;
    if(!input||(!write&&!out)||
       Cod1SpawnerCountPrivate(input,sizeof(*input))||
       Cod1SpawnerCountPrivate(value,value?sizeof(*value):0)||
       Cod1SpawnerCountPrivate(out,out?sizeof(*out):0)||Cod1SpawnerCountPrivate(error,size)||
       Cod1SpawnerOverlap(input,sizeof(*input),out,out?sizeof(*out):0)||
       Cod1SpawnerOverlap(input,sizeof(*input),error,size)||
       Cod1SpawnerOverlap(value,value?sizeof(*value):0,error,size)||
       Cod1SpawnerOverlap(out,out?sizeof(*out):0,error,size))return 0;
    ref=*input;entities=G_Cod1EntityHost();
    if(!entities||!entities->validate||!entities->validate(entities->user,&ref))return 0;
    record=Cod1SpawnerFind(&ref);
    if(!record||!record->intakeIdentity||level.spawning||level.restarted)
        return Cod1SpawnerError(error,size,"count requires a current typed spawner in a running map");
    if(write) {
        copy=*value;
        if(copy.type==5)desired=copy.integer;
        else if(copy.type==4) {
            number=copy.number;
            if(!isfinite(number)||number < -2147483648.0||number >= 2147483648.0)
                return Cod1SpawnerError(error,size,"count requires admitted numeric int32");
            desired=(int32_t)number;
        } else return Cod1SpawnerError(error,size,"count requires admitted numeric int32");
        g_entities[ref.slot].count=desired;
    } else {
        memset(&result,0,sizeof(result));result.type=5;
        result.integer=g_entities[ref.slot].count;*out=result;
    }
    return 1;
}
int G_Cod1SpawnerCountRead(const GscEntityRef *r,GscStringCastValue *v,char *e,size_t n) {
    return Cod1SpawnerCountAccess(r,NULL,v,e,n);
}
int G_Cod1SpawnerCountWrite(const GscEntityRef *r,const GscStringCastValue *v,char *e,size_t n) {
    if(!v)return 0;return Cod1SpawnerCountAccess(r,v,NULL,e,n);
}

#include "g_cod1_actor.h"
int G_Cod1SpawnerCommitFactory(const GscActorSpawnTicket*input,uint64_t intake){
 GscActorSpawnTicket ticket;const cod1SpawnerRecord_t*found;cod1SpawnerRecord_t*record;int count;
 if(!input||Cod1SpawnerCountPrivate(input,sizeof(*input)))return 0;ticket=*input;
 found=Cod1SpawnerFind(&ticket.spawner);
 if(!found||!intake||found->intakeIdentity!=intake||level.spawning||level.restarted||level.time<0||
    !G_Cod1ActorFactoryCommitCurrent(&ticket,intake))return 0;
 record=cod1Spawners+ticket.spawner.slot;
 /* No callback, trap or refusal below. Reload AFTER actual VM spawned.
  * The original wrapper reads the live clock after factory return. */
 count=g_entities[ticket.spawner.slot].count;
 if(count>0)g_entities[ticket.spawner.slot].count=count-1;
 record->snapshot.lastSpawnTime=level.time;return 1;
}
