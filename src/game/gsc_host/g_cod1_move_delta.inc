#include "../cod1_gsc/move-delta-profile.h"
#include "../cod1_gsc/move-delta-data.h"
#include "../cod1_gsc/move-delta-resources.h"
typedef struct {GscAnimationCatalogEntry declaration;unsigned char*raw;size_t bytes;c1ar14Root_t*root;} R243MoveDeltaRow;
typedef struct {uint64_t epoch,owner;int retired;unsigned char*atr;size_t atrBytes;GscAnimationCatalog*catalog;R243MoveDeltaRow rows[R243_MOVE_DELTA_RESOURCES];} R243MoveDeltaBank;
typedef struct {R243MoveDeltaRequest q,seal;const R243MoveDeltaRequest*source;R243MoveDeltaBank*bank;c1ar14Root_t*root;c1ar14Root_t snapshot;uint64_t epoch,owner;int hostNow;int(*current)(void*);void*context;float result[3];} R243MoveDeltaGameFrame;
static R243MoveDeltaBank*r243MoveDeltaBank;
static uint64_t r243MoveDeltaBankSerial;
static int r243_move_delta_game_query(void*,const R243MoveDeltaRequest*,R243MoveDeltaReply*,char*,size_t,int(*)(void*),void*);
static R243MoveDeltaHost r243MoveDeltaGameHost={NULL,r243_move_delta_game_query,0};
static struct {int busy,cancelled;R243MoveDeltaBank*stage;int mapSelected,checking,hostNow;GscMapExecutionCertificate map;const GscMapExecutionCertificate*source;}r243MoveDeltaLoad;
static R243MoveDeltaGameFrame*r243MoveDeltaGameActive;
static int MoveDeltaBankPrivate(const void*p,size_t n,const R243MoveDeltaBank*b){unsigned i;if(!b)return 0;if(ActorOverlap(p,n,b,sizeof(*b))||ActorOverlap(p,n,b->atr,b->atrBytes)||gsc_animation_catalog_storage_overlap(b->catalog,p,n))return 1;
 for(i=0;i<R243_MOVE_DELTA_RESOURCES;i++)if(ActorOverlap(p,n,b->rows[i].raw,b->rows[i].bytes)||ActorOverlap(p,n,b->rows[i].root,sizeof(c1ar14Root_t)))return 1;return 0;}
static int MoveDeltaGamePrivate(const void*p,size_t n){return ActorOverlap(p,n,&r243MoveDeltaBank,sizeof(r243MoveDeltaBank))||ActorOverlap(p,n,&r243MoveDeltaBankSerial,sizeof(r243MoveDeltaBankSerial))||ActorOverlap(p,n,&r243MoveDeltaGameHost,sizeof(r243MoveDeltaGameHost))||ActorOverlap(p,n,&r243MoveDeltaLoad,sizeof(r243MoveDeltaLoad))||ActorOverlap(p,n,&r243MoveDeltaGameActive,sizeof(r243MoveDeltaGameActive))||MoveDeltaBankPrivate(p,n,r243MoveDeltaBank)||MoveDeltaBankPrivate(p,n,r243MoveDeltaLoad.stage)||(r243MoveDeltaGameActive&&(ActorOverlap(p,n,r243MoveDeltaGameActive,sizeof(*r243MoveDeltaGameActive))||MoveDeltaBankPrivate(p,n,r243MoveDeltaGameActive->bank)));}
static void MoveDeltaBankFree(R243MoveDeltaBank*b){unsigned i;if(!b)return;for(i=0;i<R243_MOVE_DELTA_RESOURCES;i++){free(b->rows[i].raw);C1AR14_Free(b->rows[i].root);}free(b->atr);gsc_animation_catalog_free(b->catalog);free(b);}
static void MoveDeltaBankReset(void){R243MoveDeltaBank*b=r243MoveDeltaBank;r243MoveDeltaBank=NULL;r243MoveDeltaGameHost.resource_owner=0;if(r243MoveDeltaLoad.busy)r243MoveDeltaLoad.cancelled=1;if(b){b->retired=1;if(!r243MoveDeltaGameActive||r243MoveDeltaGameActive->bank!=b)MoveDeltaBankFree(b);}}
static int MoveDeltaLoadCurrent(uint64_t epoch){
 return epoch&&epoch==cod1EntityEpoch&&cod1EntityActive&&r243MoveDeltaLoad.busy&&!r243MoveDeltaLoad.cancelled&&!r243MoveDeltaGameActive&&
  (!r243MoveDeltaBank||(r243MoveDeltaLoad.mapSelected&&!r243MoveDeltaLoad.stage))&&
  (r243MoveDeltaLoad.mapSelected?(cod1EntityReady&&!level.spawning&&!level.restarted&&ActorMapAvailable()&&level.time==r243MoveDeltaLoad.hostNow&&r243MoveDeltaLoad.source&&r243MoveDeltaLoad.map.receipt==epoch&&r243MoveDeltaLoad.map.current&&r243MoveDeltaLoad.source->receipt==r243MoveDeltaLoad.map.receipt&&r243MoveDeltaLoad.source->current==r243MoveDeltaLoad.map.current):level.spawning);
}
static int MoveDeltaLoadFence(uint64_t epoch){GscMapExecutionCertificate copy;const GscMapExecutionCertificate*source;int now,ok;
 if(!MoveDeltaLoadCurrent(epoch))return 0;if(!r243MoveDeltaLoad.mapSelected)return 1;
 if(r243MoveDeltaLoad.checking){r243MoveDeltaLoad.cancelled=1;return 0;}
 copy=r243MoveDeltaLoad.map;source=r243MoveDeltaLoad.source;now=r243MoveDeltaLoad.hostNow;r243MoveDeltaLoad.checking=1;
 ok=copy.current(copy.receipt);r243MoveDeltaLoad.checking=0;
 return ok==1&&MoveDeltaLoadCurrent(epoch)&&r243MoveDeltaLoad.source==source&&r243MoveDeltaLoad.hostNow==now&&r243MoveDeltaLoad.map.receipt==copy.receipt&&r243MoveDeltaLoad.map.current==copy.current;
}
static int MoveDeltaRead(const char*path,unsigned char**out,size_t*size,uint64_t epoch){fileHandle_t file=0;int bytes,got;unsigned char*b;
 if(!MoveDeltaLoadFence(epoch)||*out)return 0;bytes=trap_FS_FOpenFile(path,&file,FS_READ);
 if(bytes<9||bytes>(int)C1AR14_CLIP_LIMIT||!file||!MoveDeltaLoadFence(epoch)){if(file)trap_FS_FCloseFile(file);return 0;}
 b=malloc(bytes);if(!b){trap_FS_FCloseFile(file);return 0;}*out=b;*size=(size_t)bytes;
 got=trap_FS_ReadCount(b,bytes,file);trap_FS_FCloseFile(file);
 return got==bytes&&MoveDeltaLoadFence(epoch);
}
static int MoveDeltaPrepareInternal(const GscMapExecutionCertificate*map){R243MoveDeltaBank*b;unsigned i,j;uint64_t epoch=cod1EntityEpoch;char path[128],error[256];int ok=0;
 if(!map&&r243MoveDeltaBank)return r243MoveDeltaBank->epoch==epoch&&!r243MoveDeltaBank->retired&&r243MoveDeltaBank->owner==r243MoveDeltaGameHost.resource_owner;
 if(r243MoveDeltaLoad.busy){if(map)r243MoveDeltaLoad.cancelled=1;return 0;}
 if(!epoch||!cod1EntityActive||r243MoveDeltaGameActive||r243MoveDeltaBankSerial==UINT64_MAX)return 0;
 if(map){if(MoveDeltaGamePrivate(map,sizeof(*map))||map->receipt!=epoch||!map->current||!cod1EntityReady||level.spawning||level.restarted||!ActorMapAvailable())return 0;}
 else if(!level.spawning)return 0;
 if(map&&r243MoveDeltaBank){R243MoveDeltaBank*old=r243MoveDeltaBank;uint64_t owner=old->owner;
  r243MoveDeltaLoad.busy=1;r243MoveDeltaLoad.mapSelected=1;r243MoveDeltaLoad.source=map;r243MoveDeltaLoad.map=*map;r243MoveDeltaLoad.hostNow=level.time;
  ok=MoveDeltaLoadFence(epoch)&&r243MoveDeltaBank==old&&old->epoch==epoch&&!old->retired&&old->owner==owner&&owner==r243MoveDeltaGameHost.resource_owner;
  memset(&r243MoveDeltaLoad,0,sizeof(r243MoveDeltaLoad));return ok;
 }
 b=calloc(1,sizeof(*b));if(!b)return 0;r243MoveDeltaLoad.busy=1;r243MoveDeltaLoad.cancelled=0;r243MoveDeltaLoad.stage=b;
 if(map){r243MoveDeltaLoad.mapSelected=1;r243MoveDeltaLoad.map=*map;r243MoveDeltaLoad.source=map;r243MoveDeltaLoad.hostNow=level.time;}
 if(!MoveDeltaRead("animtrees/generic_human.atr",&b->atr,&b->atrBytes,epoch)||b->atrBytes>131072)goto done;
 b->catalog=gsc_animation_catalog_parse("generic_human",b->atr,b->atrBytes,error,sizeof(error));if(!b->catalog||!MoveDeltaLoadFence(epoch))goto done;
 for(i=0;i<R243_MOVE_DELTA_RESOURCES;i++){int found=0;GscAnimationCatalogEntry e;
  for(j=0;j<gsc_animation_catalog_count(b->catalog);j++)if(gsc_animation_catalog_entry(b->catalog,j,&e)&&!Q_stricmp(e.name,r243MoveDeltaResourceNames[i])){b->rows[i].declaration=e;found++;}
  if(found!=1||!b->rows[i].declaration.ordinal||b->rows[i].declaration.flags)goto done;
  snprintf(path,sizeof(path),"xanim/%s",r243MoveDeltaResourceNames[i]);
  if(!MoveDeltaRead(path,&b->rows[i].raw,&b->rows[i].bytes,epoch)||!C1AR14_Decode(b->rows[i].raw,b->rows[i].bytes,&b->rows[i].root)||!MoveDeltaLoadFence(epoch))goto done;
  /* Real decoded leaf only. Native nonleaf/no-resource zero paths are not a
   * substitute for publication. Rotation-only leaves may genuinely yield0. */
  if(!(b->rows[i].root->flags&2))goto done;
 }
 if(!MoveDeltaLoadFence(epoch))goto done;
 b->epoch=epoch;b->owner=++r243MoveDeltaBankSerial;r243MoveDeltaBank=b;r243MoveDeltaGameHost.resource_owner=b->owner;r243MoveDeltaLoad.stage=NULL;b=NULL;ok=1;
done:r243MoveDeltaLoad.stage=NULL;MoveDeltaBankFree(b);memset(&r243MoveDeltaLoad,0,sizeof(r243MoveDeltaLoad));return ok;
}
int R243GameMoveDeltaPrepare(void){return MoveDeltaPrepareInternal(NULL);}
int R243GameMoveDeltaPrepareMap(const GscMapExecutionCertificate*map){return map?MoveDeltaPrepareInternal(map):0;}
static int MoveDeltaGameScope(R243MoveDeltaGameFrame*f){R243MoveDeltaBank*b;
 if(!f||r243MoveDeltaGameActive!=f||r243MoveDeltaBank!=f->bank)return 0;b=f->bank;
 return b&&!b->retired&&b->owner&&b->owner==f->owner&&f->owner==f->seal.resource_owner&&f->owner==r243MoveDeltaGameHost.resource_owner&&b->epoch==f->epoch&&f->epoch==cod1EntityEpoch&&cod1EntityReady&&ActorMapAvailable()&&!level.spawning&&!level.restarted&&f->hostNow==level.time&&!memcmp(&f->q,&f->seal,sizeof(f->q))&&!memcmp(f->source,&f->seal,sizeof(f->seal))&&!memcmp(f->root,&f->snapshot,sizeof(f->snapshot));
}
static int MoveDeltaGameFence(R243MoveDeltaGameFrame*f){return MoveDeltaGameScope(f)&&R243MoveDeltaSessionHostTimeCurrent(&f->seal,f->hostNow)&&MoveDeltaGameScope(f)&&R243MoveDeltaExactCallCurrent(&f->seal,f->current,f->context)&&MoveDeltaGameScope(f)&&R243MoveDeltaSessionCatalogCurrent(&f->seal,f->bank->catalog)&&MoveDeltaGameScope(f);}
static int r243_move_delta_game_query(void*u,const R243MoveDeltaRequest*input,R243MoveDeltaReply*out,char*error,size_t cap,int(*current)(void*),void*context){R243MoveDeltaGameFrame*f;R243MoveDeltaBank*b;R243MoveDeltaRequest q;unsigned i;int ok=0;(void)u;
 if(!input||!out||!current||r243MoveDeltaGameActive||r243MoveDeltaLoad.busy||!R243MoveDeltaNativeBuffersCurrent(input,out,error,cap,current,context)||MoveDeltaGamePrivate(input,sizeof(*input))||MoveDeltaGamePrivate(out,sizeof(*out))||MoveDeltaGamePrivate(error,cap))return 0;
 q=*input;b=r243MoveDeltaBank;if(!b||!q.resource_owner||q.resource_owner!=b->owner||b->retired||b->epoch!=cod1EntityEpoch||strcmp(q.tree,"generic_human"))return 0;
 for(i=0;i<R243_MOVE_DELTA_RESOURCES;i++)if(!Q_stricmp(q.name,b->rows[i].declaration.name))break;
 if(i==R243_MOVE_DELTA_RESOURCES||q.ordinal!=b->rows[i].declaration.ordinal||q.flags!=b->rows[i].declaration.flags||!b->rows[i].root)return 0;
 f=malloc(sizeof(*f));if(!f)return 0;memset(f,0,sizeof(*f));f->q=f->seal=q;f->source=input;f->bank=b;f->root=b->rows[i].root;f->snapshot=*f->root;f->epoch=cod1EntityEpoch;f->owner=b->owner;f->hostNow=level.time;f->current=current;f->context=context;r243MoveDeltaGameActive=f;
 if(!MoveDeltaGameFence(f)||!R243MoveDeltaData(&f->snapshot,(unsigned)q.argc,q.start,q.end,f->result)||!R243MoveDeltaNativeBuffersCurrent(input,out,error,cap,current,context)||!MoveDeltaGameFence(f))goto done;
 /* No callback after this final pure scope and staged result derivation. */
 if(!MoveDeltaGameScope(f))goto done;memcpy(out->vector,f->result,12);ok=1;
done:b=f->bank;r243MoveDeltaGameActive=NULL;free(f);if(b->retired)MoveDeltaBankFree(b);return ok;
}
const R243MoveDeltaHost*R243GameMoveDeltaHost(void){if(!r243MoveDeltaBank||!r243MoveDeltaGameHost.resource_owner)return NULL;return &r243MoveDeltaGameHost;}
