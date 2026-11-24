/* R175 allocation-owned semantic native cells. No native pointer overlay,
 * intern IDs, actor-pool buffer pointers or map allocator is used. */
typedef struct {
    GscEntityRef owner;
    int extrasPresent,sourceCertified,everChanged,published,projected;
    cod1EntityNativeStaticSnapshot_t stored;
} cod1EntityNativeStaticRecord_t;
static cod1EntityNativeStaticRecord_t cod1EntityNativeStatic[MAX_GENTITIES];
typedef char NativeStaticFloat32[sizeof(float)==sizeof(uint32_t)?1:-1];
typedef char NativeStaticInt32[sizeof(int)==sizeof(int32_t)?1:-1];
static cod1EntityNativeStaticRecord_t *NativeStaticFind(const GscEntityRef *ref) {
    cod1EntityNativeStaticRecord_t *record;
    if(!ref||ref->slot>=MAX_GENTITIES||!G_Cod1EntitySameAllocation(ref))return NULL;
    record=&cod1EntityNativeStatic[ref->slot];
    return ActorRefEqual(&record->owner,ref)?record:NULL;
}
static int NativeStaticUnreserved(const GscEntityRef *ref) {
    return ref&&ActorOwnerAllowed(ref,0);
}
static void ActorNativeStaticInvalidateSource(const GscEntityRef *ref) {
    cod1EntityNativeStaticRecord_t *record;
    if(!ref||!G_Cod1EntitySameAllocation(ref))return;
    record=&cod1EntityNativeStatic[ref->slot];
    if(!ActorRefEqual(&record->owner,ref)) {
        memset(record,0,sizeof(*record));record->owner=*ref;
    }
    record->sourceCertified=0;record->everChanged=1;
}
static int NativeStaticTextValid(const cod1NativeStaticText_t *text) {
    return (text->present==0||text->present==1)&&
        (!text->present||memchr(text->text,0,sizeof(text->text))!=NULL);
}
static void NativeStaticTextCopy(cod1NativeStaticText_t *out,const cod1NativeStaticText_t *input) {
    memset(out,0,sizeof(*out));out->present=input->present;
    if(input->present)memcpy(out->text,input->text,strlen(input->text)+1);
}
static int NativeStaticTextCapture(const char *source,cod1NativeStaticText_t *out) {
    size_t n;memset(out,0,sizeof(*out));
    if(!source)return 1;
    for(n=0;n<sizeof(out->text)&&source[n];++n){}
    if(n==sizeof(out->text))return 0;
    out->present=1;memcpy(out->text,source,n+1);return 1;
}
int G_Cod1EntityNativeStaticInitialAuthorityAvailable(const GscEntityRef *ref) {
    cod1EntityNativeStaticRecord_t *record;
    if(!NativeStaticUnreserved(ref))return 0;
    record=NativeStaticFind(ref);
    return !record||(!record->everChanged&&!record->published&&!record->projected);
}
int G_Cod1EntityPublishNativeStaticExtras(const GscEntityRef *input,
    const cod1EntityNativeStaticExtras_t *extras_input,char *error,size_t size) {
    GscEntityRef ref;cod1EntityNativeStaticExtras_t extras;
    cod1EntityNativeStaticRecord_t copy,*old;
    if(!input||!extras_input||!ActorArguments(input,sizeof(*input),NULL,0,error,size)||
       !ActorArguments(extras_input,sizeof(*extras_input),NULL,0,error,size))return 0;
    ref=*input;extras=*extras_input;
    if(!NativeStaticUnreserved(&ref)||extras.modelByteAuthoritative!=1||extras.modelByte>255||
       !NativeStaticTextValid(&extras.teamname)||!NativeStaticTextValid(&extras.groupname)||
       !NativeStaticTextValid(&extras.exportName)||!NativeStaticTextValid(&extras.scriptNoteworthy)) {
        EntityError(error,size,"native static extras require complete current unreserved authority");return 0;
    }
    old=NativeStaticFind(&ref);
    if(old)copy=*old;else memset(&copy,0,sizeof(copy));
    copy.owner=ref;copy.stored.entity=ref;
    NativeStaticTextCopy(&copy.stored.teamname,&extras.teamname);
    NativeStaticTextCopy(&copy.stored.groupname,&extras.groupname);
    NativeStaticTextCopy(&copy.stored.exportName,&extras.exportName);
    NativeStaticTextCopy(&copy.stored.scriptNoteworthy,&extras.scriptNoteworthy);
    copy.stored.maxhealth=extras.maxhealth;copy.stored.anglelerprateBits=extras.anglelerprateBits;
    copy.stored.modelByte=extras.modelByte;copy.stored.modelByteAuthoritative=1;
    copy.extrasPresent=copy.sourceCertified=copy.published=1;
    cod1EntityNativeStatic[ref.slot]=copy;return 1;
}
static int NativeStaticCaptureLive(const gentity_t *e,cod1EntityNativeStaticSnapshot_t *out,char *error,size_t size) {
    cod1EntityNativeStaticSnapshot_t copy=*out;
    if(!NativeStaticTextCapture(e->classname,&copy.classname)||
       !NativeStaticTextCapture(e->target,&copy.target)||
       !NativeStaticTextCapture(e->targetname,&copy.targetname)||
       !NativeStaticTextCapture(e->message,&copy.message)||
       !NativeStaticTextCapture(e->spawnitem,&copy.spawnitem)||
       !NativeStaticTextCapture(e->track,&copy.track)) {
        EntityError(error,size,"native static source text exceeds bounded127-byte domain");return 0;
    }
    memcpy(copy.originBits,e->r.currentOrigin,sizeof(copy.originBits));
    memcpy(copy.anglesBits,e->r.currentAngles,sizeof(copy.anglesBits));
    memcpy(copy.rotateBits,e->rotate,sizeof(copy.rotateBits));memcpy(copy.colorBits,e->dl_color,sizeof(copy.colorBits));
    memcpy(&copy.spawnflags,&e->spawnflags,sizeof(copy.spawnflags));
    copy.count=e->count;copy.health=e->health;copy.damage=e->damage;copy.key=e->key;
    copy.radius=e->radius;copy.start_size=e->start_size;copy.end_size=e->end_size;
    memcpy(&copy.speedBits,&e->speed,sizeof(copy.speedBits));
    memcpy(&copy.closespeedBits,&e->closespeed,sizeof(copy.closespeedBits));
    memcpy(&copy.waitBits,&e->wait,sizeof(copy.waitBits));memcpy(&copy.randomBits,&e->random,sizeof(copy.randomBits));
    memcpy(&copy.durationBits,&e->duration,sizeof(copy.durationBits));memcpy(&copy.degreesBits,&e->angle,sizeof(copy.degreesBits));
    memcpy(&copy.delayBits,&e->delay,sizeof(copy.delayBits));
    *out=copy;return 1;
}
int G_Cod1EntityCaptureNativeStaticSource(const GscEntityRef *input,
    cod1EntityNativeStaticSnapshot_t *out,char *error,size_t size) {
    GscEntityRef ref;cod1EntityNativeStaticSnapshot_t copy;cod1EntityNativeStaticRecord_t *record;
    if(!input||!out||!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    ref=*input;record=NativeStaticFind(&ref);
    if(!EntityValidate(NULL,&ref)||!NativeStaticUnreserved(&ref)||!record||
       !record->extrasPresent||!record->sourceCertified||
       record->stored.modelByteAuthoritative!=1||record->stored.modelByte>255) {
        EntityError(error,size,"native static source lacks current complete authority");return 0;
    }
    copy=record->stored;copy.entity=ref;
    if(!NativeStaticCaptureLive(&g_entities[ref.slot],&copy,error,size))return 0;
    *out=copy;return 1;
}
int G_Cod1EntityReadNoteworthy(const GscEntityRef *ref,GscStringCastValue *out,char *error,size_t size) {
    const cod1EntityNativeStaticRecord_t *record;
    const cod1NativeStaticText_t *text;
    GscStringCastValue result={0};
    if(!ref || !out || !ActorArguments(ref,sizeof(*ref),out,sizeof(*out),error,size) ||
       ActorOverlap(ref,sizeof(*ref),out,sizeof(*out)) || !EntityValidate(NULL,ref))return 0;
    record=NativeStaticFind(ref);
    if(!record || !record->extrasPresent){
        EntityError(error,size,"script_noteworthy requires captured or projected entity fields");
        return 0;
    }
    /* The generic native type4 getter returns undefined for token zero. */
    text=&record->stored.scriptNoteworthy;
    if(!NativeStaticTextValid(text))return 0;
    if(text->present){
        if(strlen(text->text)>=sizeof(result.string))return 0;
        result.type=1;
        strcpy(result.string,text->text);
    }
    *out=result;
    return 1;
}
static void NativeStaticDetachAndRetire(const GscEntityRef *ref) {
    cod1EntityNativeStaticRecord_t *record;gentity_t *e;
    if(!ref||ref->slot>=MAX_GENTITIES)return;
    record=&cod1EntityNativeStatic[ref->slot];
    if(!ActorRefEqual(&record->owner,ref))return;
    if(G_Cod1EntitySameAllocation(ref)) {
        e=&g_entities[ref->slot];
        if(e->classname==record->stored.classname.text)e->classname=NULL;
        if(e->target==record->stored.target.text)e->target=NULL;
        if(e->targetname==record->stored.targetname.text)e->targetname=NULL;
        if(e->message==record->stored.message.text)e->message=NULL;
        if(e->spawnitem==record->stored.spawnitem.text)e->spawnitem=NULL;
        if(e->track==record->stored.track.text)e->track=NULL;
    }
    memset(record,0,sizeof(*record));
}
static void NativeStaticMapReset(void) {
    int n;
    for(n=0;n<MAX_GENTITIES;++n) {
        GscEntityRef old=cod1EntityNativeStatic[n].owner;
        if(old.epoch)NativeStaticDetachAndRetire(&old);
    }
}
static int NativeStaticProject(const cod1EntityNativeStaticSnapshot_t *snapshot,
    const GscEntityRef *destination,char *error,size_t size) {
    cod1EntityNativeStaticRecord_t copy,*record;gentity_t *e;
    if(!G_Cod1EntityNativeStaticInitialAuthorityAvailable(destination)) {
        EntityError(error,size,"static projection requires an untouched destination allocation");return 0;
    }
    memset(&copy,0,sizeof(copy));copy.owner=*destination;copy.stored=*snapshot;
    copy.stored.entity=*destination;copy.extrasPresent=copy.sourceCertified=copy.projected=1;
    record=&cod1EntityNativeStatic[destination->slot];*record=copy;e=&g_entities[destination->slot];
    e->classname=record->stored.classname.present?record->stored.classname.text:NULL;
    e->target=record->stored.target.present?record->stored.target.text:NULL;
    e->targetname=record->stored.targetname.present?record->stored.targetname.text:NULL;
    e->message=record->stored.message.present?record->stored.message.text:NULL;
    e->spawnitem=record->stored.spawnitem.present?record->stored.spawnitem.text:NULL;
    e->track=record->stored.track.present?record->stored.track.text:NULL;
    memcpy(e->r.currentOrigin,snapshot->originBits,sizeof(snapshot->originBits));
    memcpy(e->r.currentAngles,snapshot->anglesBits,sizeof(snapshot->anglesBits));
    memcpy(e->rotate,snapshot->rotateBits,sizeof(snapshot->rotateBits));memcpy(e->dl_color,snapshot->colorBits,sizeof(snapshot->colorBits));
    memcpy(&e->spawnflags,&snapshot->spawnflags,sizeof(snapshot->spawnflags));
    e->count=snapshot->count;e->health=snapshot->health;e->damage=snapshot->damage;e->key=snapshot->key;
    e->radius=snapshot->radius;e->start_size=snapshot->start_size;e->end_size=snapshot->end_size;
    memcpy(&e->speed,&snapshot->speedBits,sizeof(snapshot->speedBits));
    memcpy(&e->closespeed,&snapshot->closespeedBits,sizeof(snapshot->closespeedBits));
    memcpy(&e->wait,&snapshot->waitBits,sizeof(snapshot->waitBits));memcpy(&e->random,&snapshot->randomBits,sizeof(snapshot->randomBits));
    memcpy(&e->duration,&snapshot->durationBits,sizeof(snapshot->durationBits));memcpy(&e->angle,&snapshot->degreesBits,sizeof(snapshot->degreesBits));
    memcpy(&e->delay,&snapshot->delayBits,sizeof(snapshot->delayBits));return 1;
}
