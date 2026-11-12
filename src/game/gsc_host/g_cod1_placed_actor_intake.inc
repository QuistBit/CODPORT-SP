/* Explicit original-allocation BSP intake. No automatic actor dispatch. */
typedef struct {
    cod1PlacedActorSnapshot_t snapshot;
    unsigned short keyOffset[MAX_SPAWN_VARS],valueOffset[MAX_SPAWN_VARS];
    unsigned short privateDecl[MAX_SPAWN_VARS],privateValue[MAX_SPAWN_VARS];
    char raw[MAX_SPAWN_VARS_CHARS];
    char originalClassname[128];
    uint64_t declarationReceipt;
} Cod1PlacedActorRecord;
static Cod1PlacedActorRecord cod1PlacedActors[MAX_GENTITIES];
static cod1BspFieldDeclarationHost_t cod1BspDeclarations;
static uint64_t cod1BspDeclarationEpoch;
static int cod1PlacedBusy;
static uint64_t cod1BspTextReceipt,cod1BspTextEpoch,cod1BspTextPending,cod1BspTextPendingEpoch;
static uint64_t cod1BspTextSerial;
static int cod1BspTextBusy;
static void ActorAitypeResourcesReset(void);
static void ActorPlacedFieldsReset(void) {
    memset(cod1PlacedActors,0,sizeof(cod1PlacedActors));
    memset(&cod1BspDeclarations,0,sizeof(cod1BspDeclarations));cod1BspDeclarationEpoch=0;
    cod1BspTextReceipt=cod1BspTextEpoch=cod1BspTextPending=cod1BspTextPendingEpoch=0;
    ActorAitypeResourcesReset();
}
static int PlacedTextEqual(const char *a,const char *b) {
    unsigned char x,y;do{x=(unsigned char)*a++;y=(unsigned char)*b++;
        if(x>='A'&&x<='Z')x=(unsigned char)(x+'a'-'A');
        if(y>='A'&&y<='Z')y=(unsigned char)(y+'a'-'A');
        if(x!=y)return 0;}while(x);return 1;
}
static int PlacedPlainText(const char *text) {
    size_t n;for(n=0;text[n];++n)if(n>=127||(unsigned char)text[n]<32||(unsigned char)text[n]>126||text[n]=='\\')return 0;
    return 1;
}
static int PlacedTokenBytes(const char *text,size_t *bytes) {
    uintptr_t base=(uintptr_t)level.spawnVarChars,p=(uintptr_t)text;size_t at,n;
    if(!text||level.numSpawnVarChars<0||level.numSpawnVarChars>MAX_SPAWN_VARS_CHARS||p<base)return 0;
    at=(size_t)(p-base);if(at>=(size_t)level.numSpawnVarChars)return 0;
    for(n=0;at+n<(size_t)level.numSpawnVarChars;++n)if(!text[n]){*bytes=n+1;return 1;}
    return 0;
}
static int PlacedRawEligible(const GscEntityRef *ref) {
    const gentity_t *e;
    if(!NativeStaticUnreserved(ref))return 0;
    e=&g_entities[ref->slot];
    return e->classname&&!strncmp(e->classname,"actor_",6)&&!(e->spawnflags&1)&&
        !e->r.linked&&!e->r.bmodel&&!e->r.contents&&!e->s.solid&&!e->neverFree;
}
static Cod1PlacedActorRecord *PlacedFind(const GscEntityRef *ref) {
    Cod1PlacedActorRecord *record;
    if(!ref||ref->slot>=MAX_GENTITIES||!ActorMapAvailable()||!G_Cod1EntitySameAllocation(ref))return NULL;
    record=&cod1PlacedActors[ref->slot];return ActorRefEqual(&record->snapshot.entity,ref)?record:NULL;
}
static int PlacedOriginalRaw(const GscEntityRef *ref) {
    Cod1PlacedActorRecord *record=PlacedFind(ref);cod1NativeStaticText_t classname;
    return record&&PlacedRawEligible(ref)&&NativeStaticTextCapture(g_entities[ref->slot].classname,&classname)&&
        classname.present&&!strcmp(classname.text,record->originalClassname);
}
static int PlacedDeclarationsCurrent(uint64_t receipt,uint64_t epoch) {
    int current;
    if(!epoch||epoch!=cod1EntityEpoch||epoch!=cod1BspDeclarationEpoch||receipt!=cod1BspDeclarations.receipt||!cod1BspDeclarations.current)return 0;
    current=cod1BspDeclarations.current(receipt);
    return current==1&&epoch==cod1EntityEpoch&&epoch==cod1BspDeclarationEpoch&&receipt==cod1BspDeclarations.receipt;
}
int G_Cod1EntitySelectBspFieldDeclarations(const cod1BspFieldDeclarationHost_t *input,char *error,size_t size) {
    cod1BspFieldDeclarationHost_t copy;uint64_t epoch;uint32_t i,j;int current;
    if(!input||!ActorArguments(input,sizeof(*input),NULL,0,error,size))return 0;
    if(cod1PlacedBusy||cod1BspDeclarationEpoch||!ActorMapAvailable()) {EntityError(error,size,"BSP declarations require an unbound current map");return 0;}
    copy=*input;
    if(copy.count>COD1_BSP_FIELD_DECLARATIONS_MAX||!copy.receipt||!copy.current)goto malformed;
    for(i=0;i<copy.count;++i){size_t n;cod1BspFieldDeclaration_t *field=&copy.fields[i];
        if(!memchr(field->name,0,sizeof(field->name))||!field->name[0]||(field->type!=1&&field->type!=4&&field->type!=5))goto malformed;
        for(n=0;field->name[n];++n)if(!((field->name[n]>='a'&&field->name[n]<='z')||(field->name[n]>='0'&&field->name[n]<='9')||field->name[n]=='_'))goto malformed;
        for(j=0;j<i;++j)if(!strcmp(field->name,copy.fields[j].name))goto malformed;
    }
    epoch=cod1EntityEpoch;cod1PlacedBusy=1;current=copy.current(copy.receipt);cod1PlacedBusy=0;
    if(current!=1||epoch!=cod1EntityEpoch||!ActorMapAvailable()||cod1BspDeclarationEpoch) {EntityError(error,size,"BSP declaration authority changed");return 0;}
    cod1BspDeclarations=copy;cod1BspDeclarationEpoch=epoch;return 1;
malformed:
    EntityError(error,size,"BSP declaration authority is malformed");return 0;
}
#include "g_cod1_bsp_declarations.inc"
static int PlacedNativeKey(const char *key) {
    static const char *const names[]={"classname","origin","spawnflags","model","speed","closespeed","target","targetname","message","popup","book","teamname","wait","random","count","health","light","dmg","angles","duration","rotate","degrees","time","_color","color","key","delay","radius","start_size","end_size","shard","spawnitem","track","groupname","export","script_noteworthy","maxhealth","anglelerprate"};
    size_t i;for(i=0;i<sizeof(names)/sizeof(names[0]);++i)if(PlacedTextEqual(key,names[i]))return 1;return 0;
}
static int PlacedInt32(const char *text,int32_t *out) {
    const unsigned char *p=(const unsigned char *)text;uint32_t n=0,limit;int negative=0;
    if(*p=='-'){negative=1;++p;}if(!*p||(*p=='0'&&p[1]))return 0;
    limit=negative?UINT32_C(2147483648):UINT32_C(2147483647);
    for(;*p;++p){uint32_t digit;if(*p<'0'||*p>'9')return 0;digit=(uint32_t)(*p-'0');if(n>(limit-digit)/10)return 0;n=n*10+digit;}
    *out=negative?(n==UINT32_C(2147483648)?INT32_MIN:-(int32_t)n):(int32_t)n;return 1;
}
int G_Cod1PlacedActorIntake(struct gentity_s *ent,char *error,size_t size) {
    Cod1PlacedActorRecord copy;cod1EntityNativeStaticRecord_t native;GscEntityRef ref;
    size_t used=0,kb,vb;int i,n;uint32_t d;uint64_t epoch;int native_ok=1;
    if(ActorOverlap(ent,ent?sizeof(*ent):0,error,size)||ActorOverlap(&level,sizeof(level),error,size))return 0;
    if(!ent||cod1PlacedBusy||!ActorMapAvailable()||!level.spawning||level.numSpawnVars<0||level.numSpawnVars>MAX_SPAWN_VARS||
       !G_Cod1EntityCapture(ent,&ref)||!PlacedRawEligible(&ref)||PlacedFind(&ref)||!G_Cod1EntityNativeStaticInitialAuthorityAvailable(&ref)) {
        EntityError(error,size,"placed intake requires a fresh actor BSP allocation");return 0;
    }
    memset(&copy,0,sizeof(copy));memset(&native,0,sizeof(native));copy.snapshot.entity=ref;
    {cod1NativeStaticText_t classname;if(!NativeStaticTextCapture(ent->classname,&classname)||!classname.present)return 0;strcpy(copy.originalClassname,classname.text);}
    native.owner=ref;native.stored.entity=ref;native.stored.modelByteAuthoritative=1;native.stored.anglelerprateBits=UINT32_C(0x44070000);
    for(i=0;i<level.numSpawnVars;++i){const char *key=level.spawnVars[i][0],*value=level.spawnVars[i][1];
        if(!PlacedTokenBytes(key,&kb)||!PlacedTokenBytes(value,&vb)||kb+vb>sizeof(copy.raw)-used) {
            EntityError(error,size,"placed BSP token ownership or capacity is invalid");return 0;
        }
        copy.keyOffset[i]=(unsigned short)used;memcpy(copy.raw+used,key,kb);used+=kb;
        copy.valueOffset[i]=(unsigned short)used;memcpy(copy.raw+used,value,vb);used+=vb;
    }
    copy.snapshot.rawFieldCount=(uint32_t)level.numSpawnVars;copy.snapshot.rawBytes=(uint32_t)used;
    if(!NativeStaticCaptureLive(ent,&native.stored,error,size))native_ok=0;
    memcpy(native.stored.originBits,ent->s.origin,12);memcpy(native.stored.anglesBits,ent->s.angles,12);
    for(i=0;i<level.numSpawnVars;++i){const char *key=copy.raw+copy.keyOffset[i],*value=copy.raw+copy.valueOffset[i];cod1NativeStaticText_t *text=NULL;
        if(PlacedTextEqual(key,"teamname"))text=&native.stored.teamname;else if(PlacedTextEqual(key,"groupname"))text=&native.stored.groupname;
        else if(PlacedTextEqual(key,"export"))text=&native.stored.exportName;else if(PlacedTextEqual(key,"script_noteworthy"))text=&native.stored.scriptNoteworthy;
        else if(PlacedTextEqual(key,"maxhealth")){if(!PlacedInt32(value,&native.stored.maxhealth))native_ok=0;}
        else if(PlacedTextEqual(key,"anglelerprate"))native_ok=0;
        if(text&&(!PlacedPlainText(value)||!NativeStaticTextCapture(value,text)))native_ok=0;
    }
    copy.snapshot.privateFieldsAvailable=1;copy.declarationReceipt=cod1BspDeclarations.receipt;epoch=cod1EntityEpoch;
    for(i=0;i<level.numSpawnVars;++i){const char *key=copy.raw+copy.keyOffset[i],*value=copy.raw+copy.valueOffset[i];
        if(PlacedNativeKey(key))continue;
        for(d=0;d<cod1BspDeclarations.count;++d)if(PlacedTextEqual(key,cod1BspDeclarations.fields[d].name))break;
        if(d==cod1BspDeclarations.count)continue;
        if(cod1BspDeclarations.fields[d].type!=1||!PlacedPlainText(value)){copy.snapshot.privateFieldsAvailable=0;continue;}
        for(n=0;n<(int)copy.snapshot.privateFieldCount;++n)if(copy.privateDecl[n]==d)break;
        if(n==(int)copy.snapshot.privateFieldCount)++copy.snapshot.privateFieldCount;
        copy.privateDecl[n]=(unsigned short)d;copy.privateValue[n]=copy.valueOffset[i];
    }
    cod1PlacedBusy=1;
    if(!PlacedDeclarationsCurrent(copy.declarationReceipt,epoch))copy.snapshot.privateFieldsAvailable=0;
    if(epoch!=cod1EntityEpoch||!PlacedRawEligible(&ref)||PlacedFind(&ref)||!G_Cod1EntityNativeStaticInitialAuthorityAvailable(&ref)) {
        cod1PlacedBusy=0;EntityError(error,size,"placed allocation changed during intake");return 0;
    }
    if(native_ok){cod1EntityNativeStaticSnapshot_t check=native.stored;
        if(!NativeStaticCaptureLive(ent,&check,error,size)){cod1PlacedBusy=0;return 0;}
        memcpy(check.originBits,ent->s.origin,12);memcpy(check.anglesBits,ent->s.angles,12);
        if(memcmp(&check,&native.stored,sizeof(check))){cod1PlacedBusy=0;EntityError(error,size,"placed native fields changed during intake");return 0;}
    }
    copy.snapshot.nativeStaticAvailable=native_ok;
    if(native_ok){cod1EntityNativeStaticRecord_t *record=&cod1EntityNativeStatic[ref.slot];native.extrasPresent=native.sourceCertified=native.published=1;*record=native;
        ent->classname=record->stored.classname.present?record->stored.classname.text:NULL;ent->target=record->stored.target.present?record->stored.target.text:NULL;
        ent->targetname=record->stored.targetname.present?record->stored.targetname.text:NULL;ent->message=record->stored.message.present?record->stored.message.text:NULL;
        ent->spawnitem=record->stored.spawnitem.present?record->stored.spawnitem.text:NULL;ent->track=record->stored.track.present?record->stored.track.text:NULL;
    }
    ent->s.eType=ET_GENERAL;ent->s.modelindex=0;ent->s.groundEntityNum=ENTITYNUM_NONE;
    ent->r.svFlags=SVF_USE_CURRENT_ORIGIN|SVF_NOCLIENT;ent->r.contents=0;ent->takedamage=qfalse;
    G_SetOrigin(ent,ent->s.origin);G_SetAngle(ent,ent->s.angles);
    cod1PlacedActors[ref.slot]=copy;cod1PlacedBusy=0;return 1;
}
int G_Cod1PlacedActorCapture(const struct gentity_s *ent,GscEntityRef *out) {
    GscEntityRef ref;if(!out||!G_Cod1EntityCapture(ent,&ref)||!PlacedFind(&ref))return 0;*out=ref;return 1;
}
int G_Cod1PlacedActorRead(const GscEntityRef *input,cod1PlacedActorSnapshot_t *out,char *error,size_t size) {
    GscEntityRef ref;Cod1PlacedActorRecord *record;
    if(!input||!out||!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size))return 0;
    ref=*input;record=PlacedFind(&ref);if(!record){EntityError(error,size,"placed allocation is unavailable");return 0;}
    *out=record->snapshot;return 1;
}
int G_Cod1PlacedActorRawField(const GscEntityRef *input,size_t index,char *key,size_t key_size,char *value,size_t value_size,char *error,size_t size) {
    GscEntityRef ref;Cod1PlacedActorRecord *record;const char *k,*v;
    if(!input||!key||!value||ActorOverlap(key,key_size,value,value_size)||
       !ActorArguments(input,sizeof(*input),key,key_size,error,size)||!ActorArguments(NULL,0,value,value_size,error,size))return 0;
    ref=*input;record=PlacedFind(&ref);if(!record||index>=record->snapshot.rawFieldCount)return 0;
    k=record->raw+record->keyOffset[index];v=record->raw+record->valueOffset[index];
    if(strlen(k)+1>key_size||strlen(v)+1>value_size){EntityError(error,size,"placed raw output capacity is unavailable");return 0;}
    memcpy(key,k,strlen(k)+1);memcpy(value,v,strlen(v)+1);return 1;
}
