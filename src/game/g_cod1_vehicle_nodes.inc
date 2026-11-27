/* Original class3 map catalog. No GAME allocation or infantry node identity.
 * Native constructor/field types: Campaign-415 vehicle evidence. Finite
 * authored numeric intake is scoped; malformed CRT conversions refuse. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_vehicle_nodes.h"
#include "bg_cod1_vehicle_graph14.h"
#include "cod1_gsc/gsc_vehicle_field_types.inc"
#define C1VN_ARENA (1024u*1024u)
typedef struct {
 unsigned fields;uint32_t key[MAX_SPAWN_VARS],value[MAX_SPAWN_VARS];
 float origin[3],angles[3],speed,lookahead;unsigned rotate;
} C1VNRecord;
static C1VNRecord vehicleNodes[GSC_VEHICLE_NODE_CAPACITY];
static C1V14PathNode vehiclePathInput[GSC_VEHICLE_NODE_CAPACITY],vehiclePathScratch[GSC_VEHICLE_NODE_CAPACITY],vehiclePath[GSC_VEHICLE_NODE_CAPACITY];
static char vehicleArena[C1VN_ARENA];static size_t vehicleUsed;
static uint64_t vehicleEpoch,vehicleOwner;static unsigned vehicleCount;
static int vehicleReady,vehicleInvalid,vehicleConsume,vehicleBusy,vehicleCancelled;
static GscVehicleNodeHost vehicleFacade;
void G_Cod1VehicleNodesClear(void){
 if(vehicleBusy)vehicleCancelled=1;
 if(vehicleEpoch!=UINT64_MAX)vehicleEpoch++;
 vehicleOwner=0;vehicleCount=vehicleReady=vehicleInvalid=vehicleConsume=0;vehicleUsed=0;
 memset(vehiclePathInput,0,sizeof(vehiclePathInput));memset(vehiclePathScratch,0,sizeof(vehiclePathScratch));memset(vehiclePath,0,sizeof(vehiclePath));
 memset(vehicleNodes,0,sizeof(vehicleNodes));memset(vehicleArena,0,sizeof(vehicleArena));
}
static int VehicleOwner(void){return vehicleEpoch&&vehicleEpoch!=UINT64_MAX&&vehicleOwner&&vehicleOwner==G_Cod1EntityMapEpoch()&&G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER;}
static int VehicleAvailable(void){return VehicleOwner()&&vehicleReady&&!vehicleInvalid&&!level.spawning&&!level.restarted&&G_Cod1EntityCollectionReady(vehicleOwner,level.num_entities);}
static void VehicleBegin(void){
 uint64_t owner;int enabled,mode;G_Cod1VehicleNodesClear();owner=G_Cod1EntityMapEpoch();
 if(!owner||vehicleEpoch==UINT64_MAX||!G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER)return;
 enabled=G_Cod1GscFeature47(C1CP47_F_ENABLE);mode=G_Cod1GscFeature47(C1CP47_F_MODE);
 if(owner!=G_Cod1EntityMapEpoch()||!G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER)return;
 vehicleOwner=owner;vehicleConsume=enabled==1&&(mode==42||mode==43);
}
static int VehicleFloat(const char*text,float*out){
 unsigned char env[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;float value;int ok;
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=Cod1NodeFloat(&text,&value);while(*text==' '||*text=='\t')text++;ok=ok&&!*text;
 __asm ldmxcsr mx
 __asm frstor env
 if(ok)*out=value;return ok;
}
static int VehicleVector(const char*text,float*out){
 unsigned char env[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;float value[3];int ok;
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 ok=Cod1NodeVector(text,value);
 __asm ldmxcsr mx
 __asm frstor env
 if(ok)memcpy(out,value,12);return ok;
}
static int VehicleScaleSpeed(float*speed){
 unsigned char env[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;float value;int ok;
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 value=*speed;if(value>=0)value=(float)((double)value*17.600000381469727);ok=isfinite(value);
 __asm ldmxcsr mx
 __asm frstor env
 if(ok)*speed=value;return ok;
}
static int VehicleCustomKind(const char*name){unsigned i;for(i=0;i<sizeof(vehicleCustomFields)/sizeof(vehicleCustomFields[0]);i++)if(!Q_stricmp(name,vehicleCustomFields[i].name))return vehicleCustomFields[i].kind;return 0;}
static const char*VehicleRaw(const C1VNRecord*node,const char*name){unsigned i=node->fields;while(i--)if(!Q_stricmp(vehicleArena+node->key[i],name))return vehicleArena+node->value[i];return NULL;}
static int VehicleIntake(void){
 C1VNRecord node;size_t bytes[MAX_SPAWN_VARS][2],needed=0,used;const char*classname=NULL;unsigned i,j;int isNode=0;uint32_t angleBits=0x40490fdb;
 if(!VehicleOwner()||!level.spawning)return 0;
 if(level.numSpawnVars<0||level.numSpawnVars>MAX_SPAWN_VARS){vehicleInvalid=1;return 0;}
 for(i=0;i<(unsigned)level.numSpawnVars;i++){
  for(j=0;j<2;j++)if(!Cod1NodeToken(level.spawnVars[i][j],&bytes[i][j])){vehicleInvalid=1;return 0;}
  if(!strcmp(level.spawnVars[i][0],"classname"))classname=level.spawnVars[i][1];
 }
 if(!classname||(strcmp(classname,"info_vehicle_node")&&strcmp(classname,"info_vehicle_node_rotate")))return 0;isNode=1;
 if(vehicleInvalid||vehicleCount>=GSC_VEHICLE_NODE_CAPACITY)goto invalid;
 memset(&node,0,sizeof(node));node.fields=level.numSpawnVars;node.speed=node.lookahead=-1;node.rotate=!strcmp(classname,"info_vehicle_node_rotate");
 for(i=0;i<3;i++)memcpy(node.angles+i,&angleBits,4);
 for(i=0;i<node.fields;i++){
  const char*key=level.spawnVars[i][0],*value=level.spawnVars[i][1];int kind;float f;int32_t n;
  needed+=bytes[i][0]+bytes[i][1];
  if(!Q_stricmp(key,"origin")){if(!VehicleVector(value,node.origin))goto invalid;}
  else if(!Q_stricmp(key,"angles")){if(!VehicleVector(value,node.angles))goto invalid;}
  else if(!Q_stricmp(key,"speed")){if(!VehicleFloat(value,&node.speed))goto invalid;}
  else if(!Q_stricmp(key,"lookahead")){if(!VehicleFloat(value,&node.lookahead))goto invalid;}
  else {kind=VehicleCustomKind(key);if((kind==GSC_VEHICLE_VALUE_INT32&&!Cod1NodeInteger(value,&n))||(kind==GSC_VEHICLE_VALUE_FLOAT&&!VehicleFloat(value,&f))||(kind==GSC_VEHICLE_VALUE_STRING&&bytes[i][1]>128))goto invalid;}
 }
 if(needed>sizeof(vehicleArena)-vehicleUsed||!VehicleScaleSpeed(&node.speed))goto invalid;
 used=vehicleUsed;for(i=0;i<node.fields;i++){
  node.key[i]=(uint32_t)used;memcpy(vehicleArena+used,level.spawnVars[i][0],bytes[i][0]);used+=bytes[i][0];
  node.value[i]=(uint32_t)used;memcpy(vehicleArena+used,level.spawnVars[i][1],bytes[i][1]);used+=bytes[i][1];
 }
 if(!VehicleRaw(&node,"targetname")||!VehicleRaw(&node,"targetname")[0])goto invalid;
 vehicleUsed=used;vehicleNodes[vehicleCount++]=node;return vehicleConsume;
invalid:vehicleInvalid=1;return isNode&&vehicleConsume;
}
/* Class3 string identities are map-local, exact case and shared by both keys.
 * Runtime refs remain epoch/index/generation; these tokens never escape GAME. */
static uint16_t VehiclePathString(const char*value,uint32_t*offsets,unsigned*count){
 unsigned i;if(!value||!*value)return 0;
 for(i=0;i<*count;i++)if(!strcmp(vehicleArena+offsets[i],value))return (uint16_t)(i+1);
 if(*count>=2*GSC_VEHICLE_NODE_CAPACITY)return 0;offsets[*count]=(uint32_t)(value-vehicleArena);return (uint16_t)(++*count);
}
static int VehiclePreparePath(void){
 uint32_t offsets[2*GSC_VEHICLE_NODE_CAPACITY];unsigned count=0,i;
 for(i=0;i<vehicleCount;i++){
  const C1VNRecord*row=vehicleNodes+i;C1V14PathNode*n=vehiclePathInput+i;
  C1V14_NodeInit(n);n->id=(int16_t)i;n->rotate=row->rotate;
  n->targetname=VehiclePathString(VehicleRaw(row,"targetname"),offsets,&count);
  n->target=VehiclePathString(VehicleRaw(row,"target"),offsets,&count);
  n->speed=row->speed;n->lookahead=row->lookahead;memcpy(n->origin,row->origin,12);memcpy(n->angles,row->angles,12);
 }
 return C1VG14_Prepare(vehiclePathInput,vehicleCount,vehiclePathScratch,vehiclePath);
}
static void VehicleFinish(void){
 vehicleReady=0;if(VehicleOwner()&&!vehicleInvalid&&!level.spawning){if(VehiclePreparePath())vehicleReady=1;else vehicleInvalid=1;}
 if(vehicleOwner)G_Printf("CoD1 vehicle node catalog: %u class3 nodes; %s\n",vehicleCount,vehicleReady?"ready":"unavailable");
}
static int VehiclePrivate(const void*p,size_t n){
#define VP(v) if(Cod1NodeOverlap(p,n,&v,sizeof(v)))return 1
 VP(vehiclePathInput);VP(vehiclePathScratch);VP(vehiclePath);
 VP(vehicleNodes);VP(vehicleArena);VP(vehicleFacade);VP(vehicleUsed);VP(vehicleEpoch);VP(vehicleOwner);VP(vehicleCount);
 VP(vehicleReady);VP(vehicleInvalid);VP(vehicleConsume);VP(vehicleBusy);VP(vehicleCancelled);
#undef VP
 return 0;
}
/* All borrowed inputs, writable results and diagnostics are distinct. Reject
 * aliases before invoking a callback or writing even an error message. */
typedef struct {const void*p;size_t n;} C1VNBuffer;
static int VehicleBuffers(const C1VNBuffer*b,unsigned count){unsigned i,j;
 for(i=0;i<count;i++){if((b[i].n&&!b[i].p)||VehiclePrivate(b[i].p,b[i].n))return 0;
  for(j=0;j<i;j++)if(Cod1NodeOverlap(b[i].p,b[i].n,b[j].p,b[j].n))return 0;
 }return 1;
}
typedef struct {uint64_t epoch,owner;unsigned count;size_t used;int now;int(*current)(void*);void*ctx;} C1VNCall;
static int VehicleScope(C1VNCall*c){return vehicleBusy&&!vehicleCancelled&&VehicleAvailable()&&c->epoch==vehicleEpoch&&c->owner==vehicleOwner&&c->count==vehicleCount&&c->used==vehicleUsed&&c->now==level.time;}
static int VehicleFence(C1VNCall*c){return VehicleScope(c)&&c->current(c->ctx)==1&&VehicleScope(c);}
static int VehicleCallBegin(C1VNCall*c,int now,int(*current)(void*),void*ctx){
 if(vehicleBusy){vehicleCancelled=1;return 0;}if(!current||now!=level.time||!VehicleAvailable())return 0;
 c->epoch=vehicleEpoch;c->owner=vehicleOwner;c->count=vehicleCount;c->used=vehicleUsed;c->now=now;c->current=current;c->ctx=ctx;
 vehicleBusy=1;vehicleCancelled=0;if(!VehicleFence(c)){vehicleBusy=0;return 0;}return 1;
}
static int VehicleRefCurrent(const GscVehicleNodeRef*n){return n&&n->epoch==vehicleEpoch&&n->generation==1&&n->index<vehicleCount;}
static int VehicleValidate(void*u,const GscVehicleNodeRef*input,int now,int*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 C1VNCall c;GscVehicleNodeRef n;int alive,ok=0;C1VNBuffer b[]={{input,sizeof(*input)},{out,sizeof(*out)},{error,cap}};(void)u;
 if(!VehicleBuffers(b,3))return 0;n=*input;
 if(!VehicleCallBegin(&c,now,current,ctx))return 0;alive=VehicleRefCurrent(&n);
 if(VehicleFence(&c)&&!memcmp(input,&n,sizeof(n))){*out=alive;ok=1;}vehicleBusy=0;return ok;
}
static int VehicleQuery(void*u,const GscVehicleNodeQuery*input,GscVehicleNodeRef*out,size_t capacity,size_t*count,int*defined,char*error,size_t cap,int(*current)(void*),void*ctx){
 C1VNCall c;GscVehicleNodeQuery q;GscVehicleNodeRef staged[GSC_VEHICLE_NODE_CAPACITY];size_t used=0;unsigned i;int def=1,ok=0;const char*name;(void)u;
 if(!capacity||capacity>GSC_VEHICLE_NODE_CAPACITY)return 0;
 {C1VNBuffer b[]={{input,sizeof(*input)},{out,capacity*sizeof(*out)},{count,sizeof(*count)},{defined,sizeof(*defined)},{error,cap}};if(!VehicleBuffers(b,5))return 0;}
 q=*input;if((q.kind!=GSC_VEHICLE_QUERY_ONE&&q.kind!=GSC_VEHICLE_QUERY_ARRAY)||!Cod1NodeText(q.value,sizeof(q.value),NULL)||!Cod1NodeText(q.field,sizeof(q.field),NULL))return 0;
 if(!VehicleCallBegin(&c,q.now_ms,current,ctx))return 0;
 if(!Q_stricmp(q.field,"targetname"))name="targetname";else if(!Q_stricmp(q.field,"target"))name="target";
 else if(!Q_stricmp(q.field,"origin")||!Q_stricmp(q.field,"angles")||!Q_stricmp(q.field,"speed")||!Q_stricmp(q.field,"lookahead")){Cod1NodeError(error,cap,"vehicle node query key is not a string field");goto done;}
 else {def=0;goto publish;}
 for(i=0;i<vehicleCount;i++){
  const char*value=VehicleRaw(vehicleNodes+i,name);
  if(!value||!*value||strcmp(value,q.value))continue;
  if(used>=capacity||(q.kind==GSC_VEHICLE_QUERY_ONE&&used)){Cod1NodeError(error,cap,"vehicle node query duplicate or capacity exceeded");goto done;}
  staged[used++]=(GscVehicleNodeRef){vehicleEpoch,i,1};
 }
publish:if(!VehicleFence(&c)||memcmp(input,&q,sizeof(q)))goto done;memcpy(out,staged,used*sizeof(*out));*count=used;*defined=def;ok=1;
done:vehicleBusy=0;return ok;
}
static int VehicleRead(void*u,const GscVehicleNodeRef*input,const char*name,int now,GscVehicleNodeField*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 C1VNCall c;GscVehicleNodeRef n;GscVehicleNodeField value;char field[128];size_t len;const C1VNRecord*row;const char*raw;int kind,ok=0;(void)u;
 if(!Cod1NodeText(name,sizeof(field),&len)||!len)return 0;
 {C1VNBuffer b[]={{input,sizeof(*input)},{name,len+1},{out,sizeof(*out)},{error,cap}};if(!VehicleBuffers(b,4))return 0;}
 n=*input;memset(field,0,sizeof(field));memcpy(field,name,len+1);memset(&value,0,sizeof(value));
 if(!VehicleCallBegin(&c,now,current,ctx))return 0;if(!VehicleRefCurrent(&n))goto done;row=vehicleNodes+n.index;
 if(!Q_stricmp(field,"origin")||!Q_stricmp(field,"angles")){value.kind=GSC_VEHICLE_VALUE_VECTOR;memcpy(value.vector,!Q_stricmp(field,"origin")?vehiclePath[n.index].origin:vehiclePath[n.index].angles,12);}
 else if(!Q_stricmp(field,"speed")||!Q_stricmp(field,"lookahead")){value.kind=GSC_VEHICLE_VALUE_FLOAT;value.number=!Q_stricmp(field,"speed")?vehiclePath[n.index].speed:vehiclePath[n.index].lookahead;}
 else {kind=VehicleCustomKind(field);raw=VehicleRaw(row,field);if(kind&&raw){
  value.kind=kind;if(kind==GSC_VEHICLE_VALUE_STRING){if(strlen(raw)>=sizeof(value.text))goto done;strcpy(value.text,raw);}
  else if(kind==GSC_VEHICLE_VALUE_INT32){if(!Cod1NodeInteger(raw,&value.integer))goto done;}
  else if(!VehicleFloat(raw,&value.number))goto done;
 }}
 if(!VehicleFence(&c)||memcmp(input,&n,sizeof(n))||strcmp(name,field)||!VehicleRefCurrent(&n))goto done;*out=value;ok=1;
done:vehicleBusy=0;return ok;
}
const GscVehicleNodeHost*G_Cod1VehicleNodeHost(void){vehicleFacade=(GscVehicleNodeHost){NULL,VehicleQuery,VehicleValidate,VehicleRead};return &vehicleFacade;}
/* Copy the immutable processed graph under a separate map-catalog lease.
 * No callback/allocator occurs between checks and publication. Node custom
 * GSC fields do not mutate these native route records. */
int G_Cod1VehiclePathCurrent(const cod1VehiclePathLease_t*lease,const GscVehicleNodeRef*node){
 if(!lease||VehiclePrivate(lease,sizeof(*lease))||(node&&VehiclePrivate(node,sizeof(*node)))||vehicleBusy||!VehicleAvailable())return 0;
 return lease->epoch==vehicleEpoch&&lease->owner==vehicleOwner&&lease->count==vehicleCount&&(!node||VehicleRefCurrent(node));
}
int G_Cod1VehiclePathCopy(const GscVehicleNodeRef*node,C1V14PathNode*out,unsigned capacity,cod1VehiclePathLease_t*lease){
 cod1VehiclePathLease_t copy;C1VNBuffer b[]={{node,sizeof(*node)},{out,(size_t)capacity*sizeof(*out)},{lease,sizeof(*lease)}};
 if(!capacity||capacity>GSC_VEHICLE_NODE_CAPACITY||!VehicleBuffers(b,3)||vehicleBusy||!VehicleAvailable()||!VehicleRefCurrent(node)||vehicleCount>capacity)return 0;
 memset(&copy,0,sizeof(copy));copy.epoch=vehicleEpoch;copy.owner=vehicleOwner;copy.count=vehicleCount;
 memcpy(out,vehiclePath,vehicleCount*sizeof(*out));*lease=copy;return 1;
}
#endif
