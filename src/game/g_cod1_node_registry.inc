/* Included by g_spawn.c. Original DLL SHA27930aa3... class/type pairs
 * 0x2008a470, name table 0x200a0050 and type getter 0x2006d6e0 prove these
 * metadata names. This is authored-node lookup, not AI/path-link simulation.
 * Every source pair is retained so absent optional keys are distinguishable
 * from present metadata whose native value schema is not implemented. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_node_host.h"
#include "g_cod1_vehicle_nodes.h"
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../qcommon/cod1_angle_normalize14.h"
#define COD1_NODE_ARENA_BYTES (4u*1024u*1024u)
typedef struct {
    int type,fields;
    uint32_t key[MAX_SPAWN_VARS],value[MAX_SPAWN_VARS];
    float origin[3],angles[3];
    int32_t spawnflags;
} Cod1NodeRecord;
static const char *const cod1NodeClasses[]={
    "node_pathnode","node_cover_stand","node_cover_crouch","node_cover_prone",
    "node_cover_right","node_cover_left","node_cover_wide_right","node_cover_wide_left",
    "node_concealment_stand","node_concealment_crouch","node_concealment_prone",
    "node_stack","node_reacquire","node_balcony","node_scripted",
    "node_negotiation_begin","node_negotiation_end"};
static const char *const cod1NodeTypes[]={
    "Path","Cover Stand","Cover Crouch","Cover Prone","Cover Right","Cover Left",
    "Cover Wide Right","Cover Wide Left","Conceal Stand","Conceal Crouch",
    "Conceal Prone","Stack","Reacquire","Balcony","Scripted","Begin","End"};
static Cod1NodeRecord cod1Nodes[GSC_NODE_QUERY_CAPACITY];
static char cod1NodeArena[COD1_NODE_ARENA_BYTES];
static uint64_t cod1NodeEpoch,cod1NodeOwnerEpoch;
/* Resource identity captured with this registry intake, not at graph use. */
static cod1NavigationResource_t cod1NodeResource;
/* CM serial is process-monotonic. Keep the high-water mark across registry
 * clears so a DESCRIBE reply retired before capture cannot bind again. */
static uint64_t cod1NodeRetiredResource;
static size_t cod1NodeArenaUsed;
static int cod1NodeCount,cod1NodeReady,cod1NodeInvalid,cod1NodeConsume;
static int cod1NodeBusy,cod1NodeCancelled;
static int Cod1NodeError(char *error,size_t size,const char *text){
    if(error&&size){size_t n=strlen(text);if(n>=size)n=size-1;memcpy(error,text,n);error[n]=0;}return 0;
}
static int Cod1NodeOverlap(const void *a,size_t na,const void *b,size_t nb){
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);
}
static int Cod1NodePrivate(const void *p,size_t n){
    return Cod1NodeOverlap(p,n,cod1Nodes,sizeof(cod1Nodes))||Cod1NodeOverlap(p,n,cod1NodeArena,sizeof(cod1NodeArena));
}
static int Cod1NodeText(const char *text,size_t capacity,size_t *length){
    size_t n;if(!text)return 0;for(n=0;n<capacity;n++)if(!text[n]){if(length)*length=n;return 1;}return 0;
}
/* Only the parser's bounded arena may supply intake strings. */
static int Cod1NodeToken(const char *text,size_t *bytes){
    uintptr_t base=(uintptr_t)level.spawnVarChars,p=(uintptr_t)text;size_t offset,n;
    if(!text||!bytes||level.numSpawnVarChars<0||level.numSpawnVarChars>MAX_SPAWN_VARS_CHARS||p<base)return 0;
    offset=(size_t)(p-base);if(offset>=(size_t)level.numSpawnVarChars)return 0;
    for(n=0;offset+n<(size_t)level.numSpawnVarChars;n++)if(!text[n]){*bytes=n+1;return 1;}return 0;
}
static int Cod1NodeFloat(const char **cursor,float *out){
    char *end;double number;const char *p=*cursor;
    while(*p==' '||*p=='\t')p++;if(!*p)return 0;errno=0;number=strtod(p,&end);
    if(end==p||!isfinite(number)||number>FLT_MAX||number<-FLT_MAX)return 0;
    *out=(float)number;if(!isfinite(*out))return 0;*cursor=end;return 1;
}
static int Cod1NodeVector(const char *text,float out[3]){
    int i;for(i=0;i<3;i++)if(!Cod1NodeFloat(&text,&out[i]))return 0;
    while(*text==' '||*text=='\t')text++;return !*text;
}
static int Cod1NodeInteger(const char *text,int32_t *out){
    uint32_t number=0,limit;int negative=0;const unsigned char *p=(const unsigned char *)text;
    if(*p=='-'){negative=1;p++;}if(!*p)return 0;limit=negative?UINT32_C(2147483648):UINT32_C(2147483647);
    for(;*p;p++){uint32_t digit;if(*p<'0'||*p>'9')return 0;digit=*p-'0';if(number>(limit-digit)/10)return 0;number=number*10+digit;}
    *out=negative?(number==UINT32_C(2147483648)?INT32_MIN:-(int32_t)number):(int32_t)number;return 1;
}
static int Cod1NodeOwner(void){
    return cod1NodeOwnerEpoch&&cod1NodeEpoch&&cod1NodeEpoch!=UINT64_MAX&&
        cod1NodeOwnerEpoch==G_Cod1EntityMapEpoch()&&G_Cod1MapEntities()&&g_gametype.integer==GT_SINGLE_PLAYER;
}
static int Cod1NodeAvailable(void){
    return Cod1NodeOwner()&&cod1NodeReady&&!cod1NodeInvalid&&!level.spawning&&!level.restarted&&
        G_Cod1EntityCollectionReady(cod1NodeOwnerEpoch,level.num_entities);
}
void G_Cod1NodeRegistryClear(void){
    G_Cod1VehicleNodesClear();
    G_Cod1NodeGraphStageClear();
    memset(&cod1NodeResource,0,sizeof(cod1NodeResource));
    if(cod1NodeBusy)cod1NodeCancelled=1;
    cod1NodeOwnerEpoch=0;cod1NodeReady=cod1NodeInvalid=cod1NodeConsume=cod1NodeCount=0;cod1NodeArenaUsed=0;
    memset(cod1Nodes,0,sizeof(cod1Nodes));memset(cod1NodeArena,0,sizeof(cod1NodeArena));
}
void G_Cod1NodeResourceRetire(uint64_t incarnation){
    if(!incarnation)return;
    if(incarnation>cod1NodeRetiredResource)cod1NodeRetiredResource=incarnation;
    if(cod1NodeResource.incarnation&&cod1NodeResource.incarnation<=cod1NodeRetiredResource)G_Cod1NodeRegistryClear();
}
static void Cod1NodeRegistryBegin(void){
    uint64_t owner,epoch;int enabled,mode;cod1NavigationRequest_t q;cod1NavigationResource_t resource;
    G_Cod1NodeRegistryClear();if(cod1NodeEpoch==UINT64_MAX)return;++cod1NodeEpoch;
    owner=G_Cod1EntityMapEpoch();if(!owner||!G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER)return;
    enabled=trap_Cvar_VariableIntegerValue("g_cod1GscTest");mode=trap_Cvar_VariableIntegerValue("g_cod1GscEntityTest");
    if(owner!=G_Cod1EntityMapEpoch()||!G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER)return;
    cod1NodeOwnerEpoch=owner;cod1NodeConsume=enabled==1&&(mode==42||mode==43);
    epoch=cod1NodeEpoch;memset(&q,0,sizeof(q));q.version=COD1_NAV_RESOURCE_VERSION;q.bytes=sizeof(q);q.operation=COD1_NAV_DESCRIBE;
    if(trap_Cod1NavigationResource(&q,&resource,NULL,0)&&epoch==cod1NodeEpoch&&owner==cod1NodeOwnerEpoch&&Cod1NodeOwner()&&
       resource.version==COD1_NAV_RESOURCE_VERSION&&resource.bytes==sizeof(resource)&&resource.incarnation>cod1NodeRetiredResource&&!resource.reserved&&resource.length<=COD1_NAV_RESOURCE_LIMIT)
        cod1NodeResource=resource;
    /* Lookup remains available without navigation data. Only resource-backed
     * graph acquisition requires this captured map identity. */
}
/* Return 1 only for the explicitly selected profile's node records, before
 * allocating a gentity. Other profiles retain their legacy spawn behavior. */
static int Cod1NodeRegistryIntake(void){
    Cod1NodeRecord copy;size_t bytes[MAX_SPAWN_VARS][2],needed=0,used;const char *classname=NULL,*origin=NULL,*angles=NULL,*flags=NULL;
    int i,j,type=0,isNode=0;
    if(!Cod1NodeOwner()||!level.spawning)return 0;
    if(level.numSpawnVars<0||level.numSpawnVars>MAX_SPAWN_VARS){cod1NodeInvalid=1;return 0;}
    for(i=0;i<level.numSpawnVars;i++){
        for(j=0;j<2;j++)if(!Cod1NodeToken(level.spawnVars[i][j],&bytes[i][j])){cod1NodeInvalid=1;return 0;}
        if(!strcmp(level.spawnVars[i][0],"classname"))classname=level.spawnVars[i][1];
    }
    if(!classname||strncmp(classname,"node_",5))return 0;isNode=1;
    for(i=0;i<(int)(sizeof(cod1NodeClasses)/sizeof(cod1NodeClasses[0]));i++)if(!strcmp(classname,cod1NodeClasses[i]))type=i+1;
    if(cod1NodeInvalid||!type||cod1NodeCount>=GSC_NODE_QUERY_CAPACITY)goto invalid;
    memset(&copy,0,sizeof(copy));copy.type=type;copy.fields=level.numSpawnVars;
    for(i=0;i<level.numSpawnVars;i++){
        const char *key=level.spawnVars[i][0],*value=level.spawnVars[i][1];
        for(j=0;j<i;j++)if(!strcmp(key,level.spawnVars[j][0]))goto invalid;
        needed+=bytes[i][0]+bytes[i][1];
        if(!strcmp(key,"origin"))origin=value;else if(!strcmp(key,"angles"))angles=value;
        else if(!strcmp(key,"spawnflags"))flags=value;
        /* GAME6D92B parses all three values but stores only normalized yaw.
         * Keep authored values here for the existing single-pass node graph
         * constructor. Script reads below project the native type11 value. */
        else if(!strcmp(key,"angle"))goto invalid;
    }
    if(!origin||!Cod1NodeVector(origin,copy.origin)||(angles&&(!Cod1NodeVector(angles,copy.angles)||copy.angles[1] < -360000.0f||copy.angles[1] > 360000.0f))||
       (flags&&!Cod1NodeInteger(flags,&copy.spawnflags))||needed>sizeof(cod1NodeArena)-cod1NodeArenaUsed)goto invalid;
    used=cod1NodeArenaUsed;
    for(i=0;i<copy.fields;i++){
        copy.key[i]=(uint32_t)used;memcpy(cod1NodeArena+used,level.spawnVars[i][0],bytes[i][0]);used+=bytes[i][0];
        copy.value[i]=(uint32_t)used;memcpy(cod1NodeArena+used,level.spawnVars[i][1],bytes[i][1]);used+=bytes[i][1];
    }
    cod1NodeArenaUsed=used;cod1Nodes[cod1NodeCount++]=copy;return cod1NodeConsume;
invalid:
    cod1NodeInvalid=1;return isNode&&cod1NodeConsume;
}
static void Cod1NodeRegistryFinish(void){
    cod1NodeReady=Cod1NodeOwner()&&!cod1NodeInvalid&&!level.spawning;
    if(cod1NodeOwnerEpoch)G_Printf("CoD1 authored node catalog: %d nodes; %s; nodes are separate from GAME entities and AI\n",cod1NodeCount,cod1NodeReady?"ready":"unavailable (invalid or incomplete metadata)");
}
static const char *Cod1NodeRaw(const Cod1NodeRecord *record,const char *field){
    int i;for(i=0;i<record->fields;i++)if(!strcmp(cod1NodeArena+record->key[i],field))return cod1NodeArena+record->value[i];return NULL;
}
static int Cod1NodeRefCurrent(const GscNodeRef *ref){
    return ref&&ref->epoch==cod1NodeEpoch&&ref->generation==1&&ref->index<(uint32_t)cod1NodeCount;
}
/* The two subsequent Training assignments must resolve to their authored path nodes. */
int G_Cod1MoodyCourseGoal(uint64_t assignment,const GscNodeRef *node){
    static const char *const names[]={"moody_node1","moody_node2","moody_node3"};
    const char *name;
    unsigned i;
    if(!assignment || !Cod1NodeAvailable() || !Cod1NodeRefCurrent(node) ||
       cod1Nodes[node->index].type!=1)return 0;
    name=Cod1NodeRaw(&cod1Nodes[node->index],"targetname");
    /* Each gate task assigns its own named node. Serial numbers follow
     * committed calls even when a different station is reached first. */
    for(i=0;name && i<3;i++)if(!strcmp(name,names[i]))return 1;
    return 0;
}
int G_Cod1FoleyCourseGoal(uint64_t assignment, const GscNodeRef *node) {
    const char *name;
    if (assignment == 1) return 1;
    if ((assignment != 2 && assignment != 3) || !Cod1NodeAvailable() || !Cod1NodeRefCurrent(node)) return 0;
    if (cod1Nodes[node->index].type != 1 /* node_pathnode */) return 0;
    name = Cod1NodeRaw(&cod1Nodes[node->index], "targetname");
    return name && !strcmp(name, assignment == 2 ? "foley_node2" : "foley_node3");
}
/* A route ends with the name of the next (unadmitted) node, or NULL
 * when the authored chain ends. Each transition checks the actual map handle,
 * node type, traversal script, name and successor. */
int G_Cod1BindTrainingRoute(const char *first, cod1TrainingRoute_t *route){
    /* Ordinary, negotiation Begin, wall End and ladder End respectively.
     * The movement and stance controllers implement exactly this sequence. */
    static const unsigned char kinds[COD1_TRAINING_ROUTE_LENGTH]={
        1,1,1,2,3,1,2,3,1,2,3,1,1,1,1,1,1,2,4,2,4,1
    };
    cod1TrainingRoute_t staged={0};
    const char *name=first;
    int step,i;
    if(!route || !Cod1NodeAvailable())return 0;
    staged.epoch=cod1NodeEpoch;
    for(step=0;step<COD1_TRAINING_ROUTE_LENGTH;step++){
        const Cod1NodeRecord *r;
        const char *script;
        int found=-1,kind=kinds[step];
        if(!name || !*name)return 0;
        for(i=0;i<cod1NodeCount;i++){
            const char *candidate=Cod1NodeRaw(&cod1Nodes[i],"targetname");
            if(candidate && !strcmp(candidate,name)){
                if(found>=0)return 0;
                found=i;
            }
        }
        if(found<0)return 0;
        for(i=0;i<step;i++)if(staged.index[i]==(unsigned)found)return 0;
        r=&cod1Nodes[found];
        if(r->type!=(kind==1?1:kind==2?16:17))return 0;
        if(kind==2){
            script=Cod1NodeRaw(r,"animscript");
            if(!script || strcmp(script,step==17?"ladder_up":step==19?"ladder_down":"wall_dive"))return 0;
        }
        if(step==12 && (strlen(name)!=6 || strncmp(name,"prone",5) || !strchr("1234",name[5])))return 0;
        if(step==16 && (strlen(name)!=6 || strncmp(name,"stand",5) || !strchr("1234",name[5])))return 0;
        staged.index[step]=(unsigned)found;
        staged.kind[step]=(unsigned char)kind;
        name=Cod1NodeRaw(r,"target");
    }
    if(name || !Cod1NodeAvailable() || staged.epoch!=cod1NodeEpoch)return 0;
    *route=staged;
    return 1;
}
extern const cod1TrainingRoute_t *TrainingCurrentRoute(const GscEntityRef *);
/* The goal command serial also counts earlier non-course goals. The
 * authored node and the current runner allocation identify its route step. */
static unsigned Cod1RunnerStep(const cod1TrainingRoute_t *route,const GscNodeRef *node){
    unsigned i;
    if(!route || !Cod1NodeAvailable() || !Cod1NodeRefCurrent(node) || route->epoch!=node->epoch)return 0;
    for(i=0;i<COD1_TRAINING_ROUTE_LENGTH;i++)if(route->index[i]==node->index)return i+1;
    return 0;
}
unsigned G_Cod1RunnerStep(const GscEntityRef *owner,const GscNodeRef *node){
    return Cod1RunnerStep(TrainingCurrentRoute(owner),node);
}
int G_Cod1RunnerCourseGoal(const GscEntityRef *owner,uint64_t assignment,const GscNodeRef *node){
    const cod1TrainingRoute_t *route=TrainingCurrentRoute(owner);
    unsigned step=Cod1RunnerStep(route,node);
    return assignment && step ? route->kind[step-1] : 0;
}
static int Cod1NodeLease(uint64_t epoch,int now,int (*current)(void *),void *lease){
    return current&&current(lease)==1&&!cod1NodeCancelled&&epoch==cod1NodeEpoch&&Cod1NodeAvailable()&&level.time==now;
}
static int Cod1NodeQuery(void *user,const GscNodeQueryRequest *input,GscNodeRef *out,size_t capacity,size_t *count,
    char *error,size_t size,int (*current)(void *),void *lease){
    GscNodeQueryRequest request;GscNodeRef staged[GSC_NODE_QUERY_CAPACITY];uint64_t epoch;size_t found=0,i;int now,ok=0;
    (void)user;
    if(Cod1NodePrivate(error,size)||Cod1NodeOverlap(error,size,input,input?sizeof(*input):0)||
       Cod1NodeOverlap(error,size,out,capacity<=GSC_NODE_QUERY_CAPACITY?capacity*sizeof(*out):0)||
       Cod1NodeOverlap(error,size,count,count?sizeof(*count):0))return 0;
    if(cod1NodeBusy){cod1NodeCancelled=1;return Cod1NodeError(error,size,"node host reentered");}
    if(!input||!count||capacity>GSC_NODE_QUERY_CAPACITY||(capacity&&!out)||!current||
       Cod1NodePrivate(out,capacity*sizeof(*out))||Cod1NodePrivate(count,sizeof(*count))||
       Cod1NodeOverlap(input,sizeof(*input),out,capacity*sizeof(*out))||Cod1NodeOverlap(input,sizeof(*input),count,sizeof(*count))||
       Cod1NodeOverlap(out,capacity*sizeof(*out),count,sizeof(*count)))return Cod1NodeError(error,size,"invalid node query buffers");
    request=*input;
    if(!Cod1NodeText(request.value,sizeof(request.value),NULL)||!Cod1NodeText(request.field,sizeof(request.field),NULL)||
       request.kind<GSC_NODE_QUERY_ONE||request.kind>GSC_NODE_QUERY_ALL||request.now_ms<0||!Cod1NodeAvailable())return Cod1NodeError(error,size,"node query requires complete fresh CoD SP catalog");
    if(request.kind==GSC_NODE_QUERY_ALL){if(request.value[0]||request.field[0])return Cod1NodeError(error,size,"all-node query takes no filter");}
    else if(strcmp(request.field,"targetname")&&strcmp(request.field,"target")&&strcmp(request.field,"animscript")&&strcmp(request.field,"script_noteworthy"))return Cod1NodeError(error,size,"node query requires a supported native string field");
    epoch=cod1NodeEpoch;now=level.time;cod1NodeBusy=1;cod1NodeCancelled=0;
    if(!Cod1NodeLease(epoch,now,current,lease)||memcmp(input,&request,sizeof(request)))goto stale;
    for(i=0;i<(size_t)cod1NodeCount;i++){
        const char *value=request.kind==GSC_NODE_QUERY_ALL?NULL:Cod1NodeRaw(&cod1Nodes[i],request.field);
        if(request.kind!=GSC_NODE_QUERY_ALL&&(!value||strcmp(value,request.value)))continue;
        if(found>=capacity){Cod1NodeError(error,size,"node result capacity exceeded");goto done;}
        staged[found].epoch=epoch;staged[found].generation=1;staged[found++].index=(uint32_t)i;
        if(request.kind==GSC_NODE_QUERY_ONE&&found>1){Cod1NodeError(error,size,"getnode requires a unique authored node");goto done;}
    }
    if(!Cod1NodeLease(epoch,now,current,lease)||memcmp(input,&request,sizeof(request)))goto stale;
    if(found)memcpy(out,staged,found*sizeof(*out));*count=found;ok=1;goto done;
stale:Cod1NodeError(error,size,"node query owner or session changed");
done:cod1NodeBusy=0;return ok;
}
static int TrainingRigNodePosition(const Cod1NodeRecord*,float[3]);
static int TrainingGraphOrigin(const Cod1NodeRecord*,float[3]);
static int Cod1NodeValidate(void *user,const GscNodeRef *input,int now_ms,int *alive,char *error,size_t size,int (*current)(void *),void *lease){
    GscNodeRef ref;uint64_t epoch;int now,value,ok=0;(void)user;
    if(Cod1NodePrivate(error,size)||Cod1NodeOverlap(error,size,input,input?sizeof(*input):0)||Cod1NodeOverlap(error,size,alive,alive?sizeof(*alive):0))return 0;
    if(cod1NodeBusy){cod1NodeCancelled=1;return Cod1NodeError(error,size,"node host reentered");}
    if(!input||!alive||!current||now_ms<0||Cod1NodePrivate(alive,sizeof(*alive))||Cod1NodeOverlap(input,sizeof(*input),alive,sizeof(*alive))||!Cod1NodeAvailable())return Cod1NodeError(error,size,"node validation requires fresh catalog");
    ref=*input;epoch=cod1NodeEpoch;now=level.time;cod1NodeBusy=1;cod1NodeCancelled=0;
    if(!Cod1NodeLease(epoch,now,current,lease)||memcmp(input,&ref,sizeof(ref)))goto stale;
    value=Cod1NodeRefCurrent(&ref);
    if(!Cod1NodeLease(epoch,now,current,lease)||memcmp(input,&ref,sizeof(ref)))goto stale;
    *alive=value;ok=1;goto done;
stale:Cod1NodeError(error,size,"node validation owner or session changed");
done:cod1NodeBusy=0;return ok;
}
static int Cod1NodeValuePolicy(const Cod1NodeRecord *record,const char *field,GscNodeFieldValue *out,char *error,size_t size,int zeroRadius){
    const char *value;size_t n;memset(out,0,sizeof(*out));
    /* radius is a genuine built-in float at node+0x48. Its absence from
     * authored text does not prove that the native initialized value is
     * undefined, so do not confuse it with an absent custom script cell. */
    if(!strcmp(field,"radius")){
        /* Original2006dc90 clears all0x74 bytes before2006d990 applies BSP
         * keys; generic node getter then reads float+48. No authored radius
         * conversion domain is admitted by this selected defaults provider. */
        if(zeroRadius&&!Cod1NodeRaw(record,"radius")){out->kind=GSC_NODE_VALUE_FLOAT;out->number=0.0f;return 1;}
        return Cod1NodeError(error,size,"native node radius value is unavailable");
    }
    if(!strcmp(field,"origin")){
        int placed=TrainingGraphOrigin(record,out->vector);
        if(placed<0)return Cod1NodeError(error,size,"published node origin retired");
        if(placed){out->kind=GSC_NODE_VALUE_VECTOR;return 1;}
    }
    if(!strcmp(field,"origin")&&record->type==15 && Cod1NodeRaw(record,"targetname") && !strcmp(Cod1NodeRaw(record,"targetname"),"jumpnode") &&
       trap_Cvar_VariableIntegerValue("g_cod1GscTrainingRigs")==1){
        if(!TrainingRigNodePosition(record,out->vector))return Cod1NodeError(error,size,"training jump node placement unavailable");
        out->kind=GSC_NODE_VALUE_VECTOR;return 1;
    }
    if(!strcmp(field,"origin")){out->kind=GSC_NODE_VALUE_VECTOR;memcpy(out->vector,record->origin,sizeof(out->vector));return 1;}
    if(!strcmp(field,"angles")){
        float yaw;if(!C1AngleNormalize14(record->angles[1],&yaw))return Cod1NodeError(error,size,"node yaw outside admitted finite conversion");
        out->kind=GSC_NODE_VALUE_VECTOR;out->vector[0]=out->vector[2]=0.0f;out->vector[1]=yaw;return 1;
    }
    if(!strcmp(field,"spawnflags")){out->kind=GSC_NODE_VALUE_INT32;out->integer=record->spawnflags;return 1;}
    value=!strcmp(field,"type")?cod1NodeTypes[record->type-1]:Cod1NodeRaw(record,field);
    if(!value)return 1;
    if(!strcmp(field,"script_delay")||!strcmp(field,"script_fallback")){
        const char *cursor=value;int32_t integer;
        if(Cod1NodeInteger(value,&integer)){out->kind=GSC_NODE_VALUE_INT32;out->integer=integer;return 1;}
        if(!Cod1NodeFloat(&cursor,&out->number))return Cod1NodeError(error,size,"invalid authored node numeric metadata");
        while(*cursor==' '||*cursor=='\t')cursor++;if(*cursor)return Cod1NodeError(error,size,"unsupported authored node numeric metadata");out->kind=GSC_NODE_VALUE_FLOAT;return 1;
    }
    if(strcmp(field,"type")&&strcmp(field,"classname")&&strcmp(field,"targetname")&&strcmp(field,"target")&&strcmp(field,"animscript")&&strcmp(field,"script_noteworthy"))return Cod1NodeError(error,size,"present node field has unsupported native value schema");
    if(!Cod1NodeText(value,sizeof(out->text),&n))return Cod1NodeError(error,size,"authored node text exceeds copied value capacity");
    out->kind=GSC_NODE_VALUE_STRING;memcpy(out->text,value,n+1);return 1;
}
static int Cod1NodeValue(const Cod1NodeRecord *record,const char *field,GscNodeFieldValue *out,char *error,size_t size){return Cod1NodeValuePolicy(record,field,out,error,size,0);}
static int Cod1NodeReadPolicy(void *user,const GscNodeRef *input,const char *inputField,int now_ms,GscNodeFieldValue *out,char *error,size_t size,int (*current)(void *),void *lease,int zeroRadius){
    GscNodeRef ref;GscNodeFieldValue staged;char field[128];size_t length;uint64_t epoch;int now,ok=0;(void)user;
    if(Cod1NodePrivate(error,size)||Cod1NodeOverlap(error,size,input,input?sizeof(*input):0)||Cod1NodeOverlap(error,size,out,out?sizeof(*out):0)||Cod1NodeOverlap(error,size,inputField,inputField?sizeof(field):0))return 0;
    if(cod1NodeBusy){cod1NodeCancelled=1;return Cod1NodeError(error,size,"node host reentered");}
    if(!input||!out||!current||now_ms<0||!Cod1NodeText(inputField,sizeof(field),&length)||!length||
       Cod1NodePrivate(out,sizeof(*out))||Cod1NodeOverlap(input,sizeof(*input),out,sizeof(*out))||Cod1NodeOverlap(inputField,length+1,out,sizeof(*out))||!Cod1NodeAvailable())return Cod1NodeError(error,size,"invalid node field read");
    ref=*input;memcpy(field,inputField,length+1);epoch=cod1NodeEpoch;now=level.time;cod1NodeBusy=1;cod1NodeCancelled=0;
    if(!Cod1NodeLease(epoch,now,current,lease)||memcmp(input,&ref,sizeof(ref))||memcmp(inputField,field,length+1))goto stale;
    if(!Cod1NodeRefCurrent(&ref)){Cod1NodeError(error,size,"stale or invalid authored node reference");goto done;}
    if(!Cod1NodeValuePolicy(&cod1Nodes[ref.index],field,&staged,error,size,zeroRadius))goto done;
    if(!Cod1NodeLease(epoch,now,current,lease)||memcmp(input,&ref,sizeof(ref))||memcmp(inputField,field,length+1))goto stale;
    *out=staged;ok=1;goto done;
stale:Cod1NodeError(error,size,"node field owner or session changed");
done:cod1NodeBusy=0;return ok;
}
static int Cod1NodeRead(void *u,const GscNodeRef *r,const char *f,int now,GscNodeFieldValue *o,char *e,size_t n,int (*current)(void *),void *lease){return Cod1NodeReadPolicy(u,r,f,now,o,e,n,current,lease,0);}
static int Cod1NodeRadiusRead(void *u,const GscNodeRef *r,const char *f,int now,GscNodeFieldValue *o,char *e,size_t n,int (*current)(void *),void *lease){return Cod1NodeReadPolicy(u,r,f,now,o,e,n,current,lease,1);}
static const GscNodeQueryHost cod1NodeHost={NULL,Cod1NodeQuery,Cod1NodeValidate,Cod1NodeRead};
const GscNodeQueryHost *G_Cod1NodeQueryHost(void){return &cod1NodeHost;}
const GscNodeQueryHost *G_Cod1NodeRadiusQueryHost(void){static const GscNodeQueryHost host={NULL,Cod1NodeQuery,Cod1NodeValidate,Cod1NodeRadiusRead};return &host;}
static int Cod1NodeScriptMapCurrent(uint64_t receipt){
    /* Query cancellation belongs to its operation, not the next map's
     * dependency owner. The registry epoch and real GAME owner determine it. */
    return receipt&&receipt==cod1NodeEpoch&&Cod1NodeAvailable();
}
int G_Cod1NodeScriptDependencies(GscNodeScriptNames*out,GscMapExecutionCertificate*map){
    GscNodeScriptNames names;GscMapExecutionCertificate certificate;int i;unsigned j;const char*name;
    if(!out||!map||Cod1NodePrivate(out,sizeof(*out))||Cod1NodePrivate(map,sizeof(*map))||
       Cod1NodeOverlap(out,sizeof(*out),map,sizeof(*map))||cod1NodeBusy||!Cod1NodeAvailable())return 0;
    memset(&names,0,sizeof(names));certificate.receipt=cod1NodeEpoch;certificate.current=Cod1NodeScriptMapCurrent;
    for(i=0;i<cod1NodeCount;i++){
        if(cod1Nodes[i].type!=16)continue;name=Cod1NodeRaw(&cod1Nodes[i],"animscript");
        /* A missing animscript stays a node-initialization failure/type0; do
         * not create a successful cache entry or invented module for it. */
        if(!name||!*name)continue;
        if(!gsc_node_script_name_valid(name))return 0;
        for(j=0;j<names.count;j++)if(!strcmp(name,names.names[j]))break;
        if(j<names.count)continue;if(names.count==GSC_NODE_SCRIPT_CAPACITY)return 0;
        strcpy(names.names[names.count++],name);
    }
    if(!certificate.current(certificate.receipt))return 0;
    *out=names;*map=certificate;return 1;
}
int G_Cod1NodeScriptAdmission(GscSessionNodeScriptAdmission*out){GscSessionNodeScriptAdmission copy;
 if(!out||Cod1NodePrivate(out,sizeof(*out))||cod1NodeBusy||!Cod1NodeAvailable())return 0;
 memset(&copy,0,sizeof(copy));copy.epoch=cod1NodeOwnerEpoch;
 if(!G_Cod1NodeScriptDependencies(&copy.selection.scripts,&copy.selection.map_certificate)||
    copy.epoch!=cod1NodeOwnerEpoch||!Cod1NodeScriptMapCurrent(copy.selection.map_certificate.receipt))return 0;
 *out=copy;return 1;
}
#include "g_cod1_node_prepare.inc"
#include "g_cod1_node_graph_stage.inc"
#include "g_cod1_training_goal_placement.inc"
#include "g_cod1_node_negotiation_pair.inc"
#include "g_cod1_navigation_probe.inc"
#endif
