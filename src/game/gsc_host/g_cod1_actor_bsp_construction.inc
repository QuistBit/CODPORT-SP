/* Exact early-record scope. Ordinary EntityValidate/ActorInheritedRaw and
 * ActorArguments/ActorLeaseValid remain unchanged. No script entry here. */
#include "../g_cod1_actor_bootstrap.h"
typedef struct {
 ActorPlacedEntryContext placed;cod1ActorBspConstructionOwner_t owner;
 GscSession *session;uint64_t id;GscPreparedEntityEntry entry;
} ActorBspConstructionContext;
static ActorBspConstructionContext *cod1ActorBspContext;
static int ActorBspCurrent(ActorBspConstructionContext*c){GscEntityRef copy;int ok;
 if(!c||cod1ActorBspContext!=c||!level.spawning||level.restarted||!ActorMapAvailable()||
    c->owner.epoch!=cod1EntityEpoch||!G_Cod1EntitySameAllocation(&c->owner.entity)||!c->owner.current)return 0;
 copy=c->owner.entity;ok=c->owner.current(c->owner.user,&copy);
 return ok==1&&ActorRefEqual(&copy,&c->owner.entity)&&cod1ActorBspContext==c&&c->owner.epoch==cod1EntityEpoch&&
  gsc_session_prepared_program_identity(c->session,c->id)==c->owner.program&&
  level.spawning&&!level.restarted&&G_Cod1EntitySameAllocation(&c->owner.entity);
}
static int ActorBspRaw(ActorBspConstructionContext*c){const gentity_t*e;cod1EntityNativeStaticRecord_t*n;
 if(!ActorBspCurrent(c)||!PlacedOriginalRaw(&c->owner.entity)||!ActorOwnerAllowed(&c->owner.entity,0))return 0;
 n=NativeStaticFind(&c->owner.entity);if(!n||!n->sourceCertified)return 0;
 e=&g_entities[c->owner.entity.slot];return !e->client&&!e->r.bmodel&&!e->r.linked&&!e->r.contents&&!e->s.solid&&
  !e->neverFree&&(e->r.svFlags&SVF_NOCLIENT)&&!(e->r.svFlags&(SVF_CASTAI|SVF_BOT));
}
static int ActorBspPrepareCurrent(ActorPlacedEntryContext*base,const GscEntityScriptEntry*q){
 ActorBspConstructionContext*c=base?base->bsp:NULL;Cod1PlacedActorRecord*r;
 if(!c||&c->placed!=base||!q||!ActorRefEqual(&q->entity,&c->owner.entity)||!ActorBspCurrent(c)||
    !gsc_session_bsp_fields_current(c->session,c->id,&c->entry))return 0;
 r=PlacedFind(&c->owner.entity);if(!r||!r->snapshot.startupConsumed||r->declarationReceipt!=base->declarationReceipt)return 0;
 return base->result.lease.serial?ActorLeaseValid(&base->result.lease):ActorBspRaw(c);
}
static int ActorBspCertificate(const GscEntityRef*ref,uint64_t serial){ActorBspConstructionContext*c=cod1ActorBspContext;
 return c&&ref&&serial==c->placed.result.lease.serial&&ActorRefEqual(ref,&c->owner.entity)&&
  ActorBspCurrent(c)&&ActorLeaseValid(&c->placed.result.lease);
}
static int ActorBspSessionCurrent(void*u,const GscEntityRef*ref){ActorBspConstructionContext*c=u;
 return c&&ref&&ActorRefEqual(ref,&c->owner.entity)&&ActorBspCurrent(c);
}
static int ActorBspCommit(void*u,const GscEntityScriptEntry*q,char*e,size_t n){ActorBspConstructionContext*c=u;Cod1PlacedActorRecord*r;int current;
 if(!q||!ActorRefEqual(&q->entity,&c->owner.entity)||!ActorBspRaw(c)||cod1PlacedBusy)return 0;
 r=PlacedFind(&c->owner.entity);if(!r||r->snapshot.startupConsumed||r->declarationReceipt!=c->placed.declarationReceipt)return 0;
 cod1PlacedBusy=1;current=PlacedDeclarationsCurrent(c->placed.declarationReceipt,c->owner.epoch);cod1PlacedBusy=0;
 if(!current||!ActorBspRaw(c))return 0;
 r=PlacedFind(&c->owner.entity);if(!r||r->snapshot.startupConsumed)return 0;
 r->snapshot.startupConsumed=1;c->placed.result.startupConsumed=1;return 1;
}
static int ActorBspPrepare(void*u,const GscEntityScriptEntry*q,GscEntityExecutionCertificate*out,char*e,size_t n){
 ActorBspConstructionContext*c=u;return ActorPlacedPrepare(&c->placed,q,out,e,n);
}
int G_Cod1ActorConstructBspPlaced(GscSession*s,uint64_t id,const cod1ActorBspConstructionOwner_t*input,
 cod1ActorPlacedEntryResult_t*out,char*e,size_t n){ActorBspConstructionContext c;GscSessionBspConstructionHost host;
 GscPreparedEntityEntryRun run;Cod1PlacedActorRecord*r;cod1NativeStaticText_t classname;uint32_t i;int ok;
 if(!s||!id||!input||!out||!ActorArguments(input,sizeof(*input),out,sizeof(*out),e,n)||
    ActorOverlap(input,sizeof(*input),out,sizeof(*out))||cod1ActorBspContext||cod1PlacedBusy||cod1ActorInheritedEntryBusy)return 0;
 memset(&c,0,sizeof(c));c.owner=*input;c.session=s;c.id=id;c.placed.bsp=&c;c.placed.request.entity=c.owner.entity;
 memset(out,0,sizeof(*out));cod1ActorBspContext=&c;
 if(!c.owner.allocation||!c.owner.program||!c.owner.ordinal||c.owner.receipt==0||
    gsc_session_prepared_program_identity(s,id)!=c.owner.program||!ActorBspRaw(&c))goto refused;
 r=PlacedFind(&c.owner.entity);
 if(!r||r->snapshot.startupConsumed||!r->snapshot.privateFieldsAvailable||!r->snapshot.nativeStaticAvailable||
    !NativeStaticTextCapture(g_entities[c.owner.entity.slot].classname,&classname)||!classname.present||strncmp(classname.text,"actor_",6))goto refused;
 c.placed.declarationReceipt=r->declarationReceipt;c.entry.entry.entity=c.owner.entity;
 Com_sprintf(c.entry.entry.module,sizeof(c.entry.entry.module),"aitype/%s",classname.text+6);strcpy(c.entry.entry.function,"main");
 c.entry.field_count=r->snapshot.privateFieldCount;
 for(i=0;i<c.entry.field_count;i++){
  strcpy(c.entry.fields[i].name,cod1BspDeclarations.fields[r->privateDecl[i]].name);
  strcpy(c.entry.fields[i].string,r->raw+r->privateValue[i]);
 }
 host.user=&c;host.current=ActorBspSessionCurrent;host.native.user=&c;
 host.native.commit_fields=ActorBspCommit;
 /* Native body consumes the same exact placed context as the late wrapper. */
 host.native.prepare=ActorBspPrepare;
 memset(&run,0,sizeof(run));cod1ActorInheritedEntryBusy=1;
 ok=gsc_session_construct_bsp_actor(s,id,&c.entry,&host,&run,e,n);cod1ActorInheritedEntryBusy=0;
 c.placed.result.initialFields=run.fields;
 if(run.stage>=GSC_PREPARED_ENTRY_FIELDS&&c.placed.result.stage<COD1_ACTOR_ENTRY_FIELDS)c.placed.result.stage=COD1_ACTOR_ENTRY_FIELDS;
 c.placed.result.leaseRetained=c.placed.result.lease.serial&&ActorLeaseValid(&c.placed.result.lease);
 *out=c.placed.result;cod1ActorBspContext=NULL;return ok;
refused:
 cod1ActorBspContext=NULL;EntityError(e,n,"BSP native constructor requires current exact authored allocation/program/declaration");return 0;
}

int G_Cod1ActorAdoptBspPlaced(const GscActorBootstrapServices*input,const GscEntityRef*ref,char*error,size_t size){
 GscActorBootstrapServices api;GscPreparedEntityEntry q;Cod1PlacedActorRecord*r;uint32_t j;uint64_t receipt;
 if(!input||!ref||!input->adopt_bsp_entity||!ActorServicesValid(input)||
    !input->storage_separate(input->context,ref,sizeof(*ref))||!input->storage_separate(input->context,error,size)||
    !ActorArguments(ref,sizeof(*ref),NULL,0,error,size))return 0;
 api=*input;r=PlacedFind(ref);if(!r||!r->snapshot.startupConsumed||!r->snapshot.privateFieldsAvailable||!r->snapshot.nativeStaticAvailable)return 0;
 receipt=r->declarationReceipt;
 if(!PlacedDeclarationsCurrent(receipt,ref->epoch)||api.current(api.context)!=1)return 0;
 r=PlacedFind(ref);if(!r||r->declarationReceipt!=receipt||!r->snapshot.startupConsumed)return 0;
 memset(&q,0,sizeof(q));q.entry.entity=*ref;Com_sprintf(q.entry.module,sizeof(q.entry.module),"aitype/%s",g_entities[ref->slot].classname+6);strcpy(q.entry.function,"main");q.field_count=r->snapshot.privateFieldCount;
 for(j=0;j<q.field_count;j++){strcpy(q.fields[j].name,cod1BspDeclarations.fields[r->privateDecl[j]].name);strcpy(q.fields[j].string,r->raw+r->privateValue[j]);}
 return api.adopt_bsp_entity(api.context,&q,error,size)&&api.current(api.context)==1;
}
