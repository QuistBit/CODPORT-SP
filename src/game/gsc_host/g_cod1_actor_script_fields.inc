/* R168 native field namespaces: actor table20089760 (43 rows), sentient
 * table20089200 (9), entity table2008a200 (38). VM transport is explicitly
 * selected; this file never grants initialized/spawned/AI membership. */
static int ActorScriptLease(const GscEntityRef *input,cod1ActorLease_t *out) {
    GscEntityRef ref;cod1ActorLease_t lease;const cod1ActorRecord_t *a;uint32_t index;
    if(!input||!out)return 0;ref=*input;
    if(!EntityValidate(NULL,&ref)||!ActorMapAvailable()||level.spawning||level.restarted||
       !cod1ActorByEntity[ref.slot])return 0;
    index=cod1ActorByEntity[ref.slot]-1;if(index>=COD1_ACTOR_CAPACITY)return 0;
    a=&cod1Actors[index];
    memset(&lease,0,sizeof(lease));lease.entity=ref;lease.serial=a->serial;
    lease.actorIndex=index;lease.sentientIndex=a->sentientIndex;lease.kind=COD1_ACTOR_LEASE_ACTOR;
    if(!ActorTopologyStageValid(&lease,0)||a->modelBusy||a->modelMethodBusy)return 0;
    *out=lease;return 1;
}
static int ActorScriptClassify(void *user,const GscEntityRef *ref,const char *name,char *error,size_t size) {
    static const char *const nativeNames[]={
        "accuracy","accuracystationarymod","lookforward","lookright","lookup","fovcosine",
        "maxsightdistsqrd","visibilitythreshold","defaultsightlatency","followmin","followmax",
        "chainfallback","interval","personalspace","damagetaken","damagedir","damageyaw",
        "damagelocation","proneok","walkdist","desiredangle","bravery","pacifist","pacifistwait",
        "suppressionwait","name","weapon","dontavoidplayer","grenadeawareness","grenade",
        "grenadeweapon","grenadeammo","favoriteenemy","allowdeath","useable","goalradiusonly",
        "dropweapon","drawoncompass","scriptstate","lastscriptstate","statechangereason",
        "groundtype","anim_pose","team","threatbias","scariness","node","chainnode",
        "goalradius","goalangletolerance","enemy","ignoreme","classname","model","spawnflags",
        "speed","closespeed","message","popup","book","teamname","wait","random","count",
        "health","light","dmg","duration","rotate","degrees","time","_color","color","key",
        "delay","radius","start_size","end_size","shard","spawnitem","track","groupname",
        "export","script_noteworthy","maxhealth","anglelerprate"
    };
    size_t n;(void)user;
    if(!ref||!name||!EntityValidate(NULL,ref)){EntityError(error,size,"native field namespace requires current ready-map identity");return -1;}
    /* origin/angles/target/targetname retain their existing checked providers. */
    for(n=0;n<sizeof(nativeNames)/sizeof(nativeNames[0]);++n)if(!Q_stricmp(name,nativeNames[n]))return 1;
    return 0;
}
static float *ActorScriptFloat(const cod1ActorLease_t *lease,const char *name) {
    if(!Q_stricmp(name,"maxsightdistsqrd"))return &cod1Actors[lease->actorIndex].defaults.allocation.maxsightdistsqrd;
    if(!Q_stricmp(name,"grenadeawareness"))return &cod1Actors[lease->actorIndex].defaults.allocation.grenadeawareness;
    if(!Q_stricmp(name,"walkdist"))return &cod1Actors[lease->actorIndex].defaults.allocation.walkdist;
    if(!Q_stricmp(name,"interval"))return &cod1Actors[lease->actorIndex].defaults.allocation.interval;
    if(!Q_stricmp(name,"accuracy"))return &cod1Actors[lease->actorIndex].nativeData.actor.accuracy;
    if(!Q_stricmp(name,"accuracystationarymod"))return &cod1Actors[lease->actorIndex].nativeData.actor.accuracystationarymod;
    if(!Q_stricmp(name,"scariness"))return &cod1Sentients[lease->sentientIndex].defaults.scariness;
    return NULL;
}
/* Plain native type0 cells. Caller validates the existing Actor lease first;
 * use the pointer immediately, without callbacks or cached ownership. The
 * uint32 dropweapon cell carries the same four native bits as signed INT32. */
static void *ActorScriptInt32Cell(const cod1ActorLease_t *lease,const char *name) {
    if(!Q_stricmp(name,"followmin"))return &cod1Actors[lease->actorIndex].defaults.allocation.followmin;
    if(!Q_stricmp(name,"followmax"))return &cod1Actors[lease->actorIndex].defaults.allocation.followmax;
    if(!Q_stricmp(name,"dontavoidplayer"))return &cod1Actors[lease->actorIndex].nativeData.actor.dontavoidplayer;
    if(!Q_stricmp(name,"dropweapon"))return &cod1Actors[lease->actorIndex].nativeData.actor.dropweapon;
    if(!Q_stricmp(name,"drawoncompass"))return &cod1Actors[lease->actorIndex].nativeData.actor.drawoncompass;
    if(!Q_stricmp(name,"pacifist"))return &cod1Actors[lease->actorIndex].nativeData.actor.pacifist;
    return NULL;
}
/* Type1 uses GetInt0 then one WORD store; its getter sign-extends WORD.
 * Keep raw16 storage separate from the type0 DWORD provider and use the
 * checked lease only within this pure field access. */
static uint16_t *ActorScriptShort16Cell(const cod1ActorLease_t *lease,const char *name) {
    if(!Q_stricmp(name,"chainfallback"))return &cod1Actors[lease->actorIndex].nativeData.actor.chainfallback;
    return NULL;
}
/* Plain native type2 BYTE. Access only immediately under the checked lease. */
static uint8_t *ActorScriptByteCell(const cod1ActorLease_t *lease,const char *name) {
    if(!Q_stricmp(name,"useable"))return &cod1Actors[lease->actorIndex].nativeData.actor.useable;
    return NULL;
}
/* R169 GetInt's admitted numeric domain. Original463040/52e218 truncates
 * float toward zero; native string parsing/ref release is not selected here.
 * In-range C conversion is independent of the process FPU rounding mode. */
static int ActorScriptInteger(const GscStringCastValue *value,int32_t *out) {
    double number;
    if(value->type==5){*out=value->integer;return 1;}
    if(value->type!=4)return 0;
    number=value->number;
    if(!isfinite(number)||number < -2147483648.0||number >= 2147483648.0)return 0;
    *out=(int32_t)number;return 1;
}
static int ActorScriptHealthWrite(const cod1ActorLease_t *lease,int maximum,
    const GscStringCastValue *value,char *error,size_t size) {
    int32_t desired,oldHealth,oldMaximum;cod1ActorNativeEntityDefaults_t *data;
    char name[128];const char *borrowed;gentity_t *entity;
    if(!ActorScriptInteger(value,&desired)){
        EntityError(error,size,"native integer field requires admitted numeric int32 or finite in-range float");return 0;}
    data=&cod1Actors[lease->actorIndex].nativeData.entity;entity=&g_entities[lease->entity.slot];
    if(maximum){data->maxhealth=desired;return 1;}
    /* Entity+1d8 is the live entity health, not actor's separate life word.
     * RTCW has no corresponding entity maxhealth cell; its typed sidecar is
     * used, without borrowing client maxHealth or an unrelated entity count. */
    if(desired<=0){EntityError(error,size,"self.health must be greater than 0");return 0;}
    oldHealth=entity->health;oldMaximum=data->maxhealth;
    if(oldHealth<=0&&oldMaximum!=0) {
        borrowed=entity->targetname;
        if(borrowed){const char *end=memchr(borrowed,0,sizeof(name));if(!end)return 0;
            memcpy(name,borrowed,(size_t)(end-borrowed)+1);}
        else strcpy(name,"<not set>");
        G_Printf("^2Cannot set health on dead entities (health %i, max %i, ent %u, name %s)\n",
            oldHealth,oldMaximum,(unsigned)lease->entity.slot,name);
        /* The diagnostic crosses a callback. Never adopt a replacement
         * component/slot. Entity retirement is accepted so VM cancels self. */
        if(!G_Cod1EntitySameAllocation(&lease->entity))return 1;
        if(!ActorLeaseValid(lease)||!ActorTopologyStageValid(lease,0)||
           cod1Actors[lease->actorIndex].modelBusy||cod1Actors[lease->actorIndex].modelMethodBusy)return 0;
        return 1;
    }
    /* Pure commit. Reflect the proved native pair in owned defaults as well
     * as the live GAME health. No life/readiness/AI/model flags are changed. */
    entity->health=desired;data->health=desired;data->maxhealth=desired;return 1;
}
static int ActorPoseLease(const GscEntityRef*,cod1ActorLease_t*);
static int ActorScriptFieldReadSelected(int runtime,void *user,const GscEntityRef *input,const char *name,
    GscStringCastValue *out,char *error,size_t size) {
    cod1ActorLease_t lease;GscStringCastValue copy;float *number;void *cell;uint16_t *shortCell;uint8_t *byteCell;uint32_t team;(void)user;
    if(!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out))||!out||!name)return 0;
    if(!(runtime?ActorPoseLease(input,&lease):ActorScriptLease(input,&lease))){EntityError(error,size,"native field read requires a current owned actor in a ready map");return 0;}
    memset(&copy,0,sizeof(copy));
    if(!Q_stricmp(name,"damagetaken")){if(!ActorDamageTakenRead(cod1Actors+lease.actorIndex,&copy)){EntityError(error,size,"native damageTaken has no current admitted hit");return 0;}*out=copy;return 1;}
    if(!Q_stricmp(name,"damageyaw")){if(!ActorDamageYawRead(cod1Actors+lease.actorIndex,&copy)){EntityError(error,size,"native damageYaw has no current admitted hit");return 0;}*out=copy;return 1;}
    if(!Q_stricmp(name,"damagedir")){if(!ActorDamageDirectionRead(cod1Actors+lease.actorIndex,&copy)){EntityError(error,size,"native damageDir has no current admitted direction");return 0;}*out=copy;return 1;}
    if(!Q_stricmp(name,"lookforward")||!Q_stricmp(name,"lookright")||!Q_stricmp(name,"lookup")){
        /* Saved retail rows89788/8979C/897B0: readonly type5 vectors
         * at FC/108/114. Read the existing native orientation cells;
         * do not substitute body angles, a tag, or a fabricated enemy. */
        const cod1ActorNativeData_t *data=&cod1Actors[lease.actorIndex].nativeData;
        const float *v=!Q_stricmp(name,"lookforward")?data->orientation.lookforward:
            !Q_stricmp(name,"lookright")?data->orientation.lookright:data->orientation.lookup;
        if(!isfinite(v[0])||!isfinite(v[1])||!isfinite(v[2]))return 0;
        copy.type=3;memcpy(copy.vector,v,12);*out=copy;return 1;
    }
    if(!Q_stricmp(name,"grenade")){
        const GscEntityRef *grenade=&cod1Actors[lease.actorIndex].grenade1164;
        if(grenade->epoch||grenade->slot||grenade->generation){
            EntityError(error,size,"grenade reference producer/transport not qualified");return 0;
        }
        /* Native type6 null pointer returns undefined, not integer zero. */
        *out=copy;return 1;
    }
    if(runtime && !Q_stricmp(name,"groundtype")){
        /* GAME 5B960 leaves the result undefined for the zero phys cell.
         * Read the owned +294 word, shared with the actor physics record. */
        if(cod1Actors[lease.actorIndex].defaults.lifeWords[(0x294-0x26c)/4]){
            EntityError(error,size,"ground material configstring lookup unavailable");return 0;
        }
        *out=copy;return 1;
    }
    if(runtime && !Q_stricmp(name,"desiredangle")){
        if(!ActorDesiredAngleRead(cod1Actors+lease.actorIndex,&copy)){
            EntityError(error,size,"native desiredAngle producer is not currently admitted");return 0;}
        *out=copy;return 1;
    }
    if(runtime && (!Q_stricmp(name,"node") || !Q_stricmp(name,"chainnode"))) {
        const cod1SentientRecord_t *sentient=&cod1Sentients[lease.sentientIndex];
        /* Retail sentient rows 8923C and 89250 are type 10. A fresh
         * sentient owns both null pointers independently of its script goal. */
        uint32_t node=!Q_stricmp(name,"node")?sentient->rawGoal.node0040:sentient->rawGoal.node004c;
        if(node){
            EntityError(error,size,"sentient node requires typed map-owned node transport");
            return 0;
        }
        *out=copy;
        return 1;
    }
    if(!Q_stricmp(name,"name")||!Q_stricmp(name,"weapon")) {
        const cod1ActorScriptStrings_t *strings=&cod1Actors[lease.actorIndex].scriptStrings;
        const char *text=!Q_stricmp(name,"name")?strings->name:strings->weapon;
        int present=!Q_stricmp(name,"name")?strings->namePresent:strings->weaponPresent;
        if(present){if(!memchr(text,0,sizeof(copy.string)))return 0;
            copy.type=1;strcpy(copy.string,text);}
    } else if(!Q_stricmp(name,"health")||!Q_stricmp(name,"maxhealth")) {
        copy.type=5;copy.integer=!Q_stricmp(name,"health")?g_entities[lease.entity.slot].health:
            cod1Actors[lease.actorIndex].nativeData.entity.maxhealth;
    } else if(!Q_stricmp(name,"suppressionwait")) {
        copy.type=4;
        copy.number=(float)((double)cod1Actors[lease.actorIndex].defaults.allocation.suppressionwait * (double)0.001f);
    } else if((byteCell=ActorScriptByteCell(&lease,name))!=NULL) {
        copy.type=5;copy.integer=(int32_t)*byteCell;
    } else if((shortCell=ActorScriptShort16Cell(&lease,name))!=NULL) {
        copy.type=5;copy.integer=(int32_t)*shortCell;
        if(copy.integer>=32768)copy.integer-=65536;
    } else if((cell=ActorScriptInt32Cell(&lease,name))!=NULL) {
        copy.type=5;memcpy(&copy.integer,cell,sizeof(copy.integer));
    } else if(!Q_stricmp(name,"team")) {
        team=cod1Sentients[lease.sentientIndex].defaults.team;copy.type=1;
        if(team==1)strcpy(copy.string,"axis");else if(team==2)strcpy(copy.string,"allies");
        else if(team==3)strcpy(copy.string,"neutral");else return 0;
    } else {
        number=ActorScriptFloat(&lease,name);if(!number||!isfinite(*number)){
            EntityError(error,size,"native actor field has no admitted readable provider");return 0;}
        copy.type=4;copy.number=*number;
    }
    *out=copy;return 1;
}
static int ActorScriptFieldRead(void *u,const GscEntityRef*r,const char*n,GscStringCastValue*v,char*e,size_t z){return ActorScriptFieldReadSelected(0,u,r,n,v,e,z);}
int G_Cod1ActorReadRuntimeField(const GscEntityRef*r,const char*n,GscStringCastValue*v,char*e,size_t z){return ActorScriptFieldReadSelected(1,NULL,r,n,v,e,z);}
static int ActorScriptFieldWrite(void *user,const GscEntityRef *input,const char *name,
    const GscStringCastValue *value,char *error,size_t size) {
    cod1ActorLease_t lease;GscStringCastValue copy;cod1ActorModelString_t text;
    float *number,converted;void *cell;uint16_t *shortCell;uint8_t *byteCell;uint32_t team;size_t n;int cast;(void)user;
    if(!ActorArguments(input,sizeof(*input),NULL,0,error,size)||
       !ActorArguments(value,sizeof(*value),NULL,0,error,size)||!value||!name)return 0;
    copy=*value;
    /* Plain maxsight storage also accepts the existing checked current-motion lease.
     * Other field writes retain their constructor-only authority. */
    if(!Q_stricmp(name,"useable")) {
        if(!ActorPoseLease(input,&lease)){EntityError(error,size,"useable write requires a current owned actor field lease");return 0;}
    } else if(!Q_stricmp(name,"chainfallback")) {
        if(!ActorPoseLease(input,&lease)){EntityError(error,size,"chainfallback write requires a current owned actor field lease");return 0;}
    } else if(!Q_stricmp(name,"dontavoidplayer")||!Q_stricmp(name,"dropweapon")||!Q_stricmp(name,"pacifist")||!Q_stricmp(name,"drawoncompass")) {
        if(!ActorPoseLease(input,&lease)){EntityError(error,size,!Q_stricmp(name,"dontavoidplayer")?
            "dontavoidplayer write requires a current owned actor field lease":!Q_stricmp(name,"dropweapon")?
            "dropweapon write requires a current owned actor field lease":!Q_stricmp(name,"drawoncompass")?
            "drawoncompass write requires a current owned actor field lease":"pacifist write requires a current owned actor field lease");return 0;}
    } else if(!Q_stricmp(name,"grenadeawareness") || !Q_stricmp(name,"suppressionwait")) {
        if(!ActorPoseLease(input,&lease)){EntityError(error,size,"AI tuning requires a current actor");return 0;}
    } else if(!Q_stricmp(name,"interval")) {
        if(!ActorPoseLease(input,&lease)){EntityError(error,size,"interval write requires a current owned actor field lease");return 0;}
    } else if(!Q_stricmp(name,"walkdist")) {
        if(!ActorPoseLease(input,&lease)){EntityError(error,size,"walkdist write requires a current owned actor field lease");return 0;}
    } else if(!Q_stricmp(name,"maxsightdistsqrd")) {
        if(!ActorPoseLease(input,&lease)){EntityError(error,size,"maxsightdistsqrd write requires a current owned actor field lease");return 0;}
    } else if(!ActorScriptLease(input,&lease)){EntityError(error,size,"native field write requires a hidden native-data actor in a ready map");return 0;}
    if(!Q_stricmp(name,"name")||!Q_stricmp(name,"weapon")) {
        cod1ActorScriptStrings_t *strings=&cod1Actors[lease.actorIndex].scriptStrings;
        char *target=!Q_stricmp(name,"name")?strings->name:strings->weapon;
        int *present=!Q_stricmp(name,"name")?&strings->namePresent:&strings->weaponPresent;
        cast=G_Cod1ActorConvertModelValue(&copy,&text,error,size);if(cast!=1)return 0;
        memset(target,0,sizeof(strings->name));strcpy(target,text.string);*present=1;return 1;
    }
    if(!Q_stricmp(name,"health")||!Q_stricmp(name,"maxhealth"))
        return ActorScriptHealthWrite(&lease,!Q_stricmp(name,"maxhealth"),&copy,error,size);
    byteCell=ActorScriptByteCell(&lease,name);
    if(byteCell) {
        if(copy.type!=5){EntityError(error,size,"useable requires an admitted INT32 field value");return 0;}
        *byteCell=(uint8_t)(uint32_t)copy.integer;return 1;
    }
    shortCell=ActorScriptShort16Cell(&lease,name);
    if(shortCell) {
        if(copy.type!=5){EntityError(error,size,"chainfallback requires an admitted INT32 field value");return 0;}
        *shortCell=(uint16_t)(uint32_t)copy.integer;return 1;
    }
    cell=ActorScriptInt32Cell(&lease,name);
    if(cell) {
        if(copy.type!=5){EntityError(error,size,
              (!Q_stricmp(name,"followmin")||!Q_stricmp(name,"followmax"))?"follow range requires an admitted INT32 field value":
              !Q_stricmp(name,"dontavoidplayer")?
            "dontavoidplayer requires an admitted INT32 field value":!Q_stricmp(name,"dropweapon")?
            "dropweapon requires an admitted INT32 field value":!Q_stricmp(name,"drawoncompass")?
            "drawoncompass requires an admitted INT32 field value":"pacifist requires an admitted INT32 field value");return 0;}
        memcpy(cell,&copy.integer,sizeof(copy.integer));return 1;
    }
    if(!Q_stricmp(name,"team")) {
        cast=G_Cod1ActorConvertModelValue(&copy,&text,error,size);if(cast!=1)return 0;
        for(n=0;text.string[n];++n)if((unsigned char)text.string[n]>127)return 0;
        if(!Q_stricmp(text.string,"axis"))team=1;else if(!Q_stricmp(text.string,"allies"))team=2;
        else if(!Q_stricmp(text.string,"neutral"))team=3;
        else{EntityError(error,size,"unknown team; expected axis, allies, or neutral");return 0;}
        return G_Cod1ActorSetTeam(&lease,team,error,size);
    }
    if(!Q_stricmp(name,"grenadeawareness") || !Q_stricmp(name,"suppressionwait")) {
        if(copy.type==4)converted=copy.number;
        else if(copy.type==5)converted=(float)copy.integer;
        else {EntityError(error,size,"actor field requires a numeric value");return 0;}
        if(!isfinite(converted))return 0;
        if(!Q_stricmp(name,"grenadeawareness")){
            if(converted<0.0f || converted>1.0f){
                if(error&&size)snprintf(error,size,"actor field %s clamped from %g to %d",name,(double)converted,converted>1.0f);
                return 0;
            }
            cod1Actors[lease.actorIndex].defaults.allocation.grenadeawareness=converted;
        }else{
            /* GAME 5B890 spills seconds*1000 to float32 before the biased
             * FISTP. Invalid integer conversion produces x87 indefinite. */
            float scaled=(float)((double)converted*1000.0);
            double ms=floor((double)scaled+0x1p-30+0.5);
            cod1Actors[lease.actorIndex].defaults.allocation.suppressionwait=
                (ms>=INT32_MIN && ms<=INT32_MAX)?(int32_t)ms:INT32_MIN;
        }
        return 1;
    }
    number=ActorScriptFloat(&lease,name);if(!number){EntityError(error,size,"native actor field has no admitted writable provider");return 0;}
    if(copy.type==4){converted=copy.number;if(!isfinite(converted))return 0;}
    else if(copy.type==5){if(copy.integer < -16777216 || copy.integer > 16777216)return 0;converted=(float)copy.integer;}
    else{EntityError(error,size,"native floating field requires an admitted float or exact integer");return 0;}
    *number=converted;return 1;
}
const GscEntityFieldHost *G_Cod1ActorConstructorFieldHost(void) {
    static const GscEntityFieldHost host={NULL,ActorScriptClassify,ActorScriptFieldRead,ActorScriptFieldWrite};return &host;
}
/* Original gamex86 2005b990: GetConstString, native prone predicate,
 * AssignString(WORD+178), or ScriptError. Lease-owned semantic string copy;
 * this does not publish/initialize an actor or start a prone animation. */
/* Constructor fields retain their hidden/zero-collision scope. Native pose
 * remains writable after our real motion service installs its owned collider.
 * Original setter2005B990 checks the native prone predicate, not contents0. */
static int ActorGoalLease(const GscEntityRef*,cod1ActorLease_t*);
static int ActorPoseLease(const GscEntityRef*ref,cod1ActorLease_t*out){
 cod1ActorLease_t lease;const cod1ActorRecord_t*a;const gentity_t*e;
 if(ActorScriptLease(ref,out))return 1;
 if(!ref||!out||!cod1EntityReady||!ActorMapAvailable()||level.spawning||level.restarted||!ActorGoalLease(ref,&lease)||cod1ActorEngineSceneBusyEpoch==ref->epoch)return 0;
 a=cod1Actors+lease.actorIndex;e=g_entities+ref->slot;
 if(!a->motion.active||a->motion.busy||!a->spatial.linked||!e->r.linked||e->client||e->r.bmodel||e->physicsObject||e->neverFree||e->r.contents!=CONTENTS_BODY||e->clipmask!=MASK_PLAYERSOLID||(e->r.svFlags&(SVF_CASTAI|SVF_BOT)))return 0;
 *out=lease;return 1;
}
/* Selected current owned strings remain readable after motion/collision starts.
 * This reads the string assigned by original aitype/script, never weapon slots. */
int G_Cod1ActorReadRuntimeStringField(const GscEntityRef*ref,const char*name,GscStringCastValue*out,char*error,size_t size){
 cod1ActorLease_t lease;const cod1ActorScriptStrings_t*strings;const char*text;int present;GscStringCastValue value;
 if(!ref||!name||!out||(Q_stricmp(name,"weapon")&&Q_stricmp(name,"name"))||!ActorArguments(ref,sizeof(*ref),out,sizeof(*out),error,size)||ActorOverlap(ref,sizeof(*ref),out,sizeof(*out))||!ActorPoseLease(ref,&lease))return 0;
 strings=&cod1Actors[lease.actorIndex].scriptStrings;text=!Q_stricmp(name,"weapon")?strings->weapon:strings->name;present=!Q_stricmp(name,"weapon")?strings->weaponPresent:strings->namePresent;
 memset(&value,0,sizeof(value));if(present){if(!memchr(text,0,sizeof(value.string)))return 0;value.type=1;strcpy(value.string,text);}*out=value;return 1;
}
static int ActorPoseRead(void *user,const GscEntityRef *input,const char *name,
    GscStringCastValue *out,char *error,size_t size) {
    cod1ActorLease_t lease;GscStringCastValue value;const cod1ActorScriptStrings_t *strings;int selected;(void)user;
    if(!name||Q_stricmp(name,"anim_pose")||!input||!out||
       !ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    selected=ActorFacePoseRead(input,out,error,size);if(selected>=0)return selected;
    if(!ActorPoseLease(input,&lease))return 0;
    strings=&cod1Actors[lease.actorIndex].scriptStrings;
    if(!strings->posePresent){EntityError(error,size,"native anim_pose getter requires an assigned owned string");return 0;}
    memset(&value,0,sizeof(value));value.type=1;strcpy(value.string,strings->pose);*out=value;return 1;
}
static int ActorPoseWrite(void *user,const GscEntityRef *input,const char *name,
    const GscStringCastValue *source,char *error,size_t size) {
    cod1ActorLease_t lease;GscStringCastValue value;cod1ActorScriptStrings_t *strings;
    const uint32_t *cells;int32_t phase;int active;(void)user;
    if(!name||Q_stricmp(name,"anim_pose")||!input||!source||
       !ActorArguments(input,sizeof(*input),NULL,0,error,size)||
       !ActorArguments(source,sizeof(*source),NULL,0,error,size)||!ActorPoseLease(input,&lease))return 0;
    value=*source;
    if(value.type!=1||!memchr(value.string,0,sizeof(value.string))){EntityError(error,size,"native anim_pose requires an admitted plain string");return 0;}
    cells=cod1Actors[lease.actorIndex].nativeData.actor.zeroCells;
    memcpy(&phase,&cells[5],sizeof(phase));active=cells[4]!=0&&cells[3]==0&&phase>=0;
    if((!strcmp(value.string,"prone"))!=active){EntityError(error,size,"native anim_pose conflicts with the current prone state");return 0;}
    strings=&cod1Actors[lease.actorIndex].scriptStrings;
    memset(strings->pose,0,sizeof(strings->pose));strcpy(strings->pose,value.string);strings->posePresent=1;return 1;
}
const GscEntityFieldHost *G_Cod1ActorPoseFieldHost(void) {
    static const GscEntityFieldHost host={NULL,ActorScriptClassify,ActorPoseRead,ActorPoseWrite};return &host;
}
/* Original entity table2008a200 row2: BYTE+12d, typeC, read-only.
 * Generic getter20041b60 typeC reads CS model name; do not use the hidden
 * actor's unpublished network modelindex or synthesize a model path. */
static int ActorReadModelFieldConstructor(const GscEntityRef *input,GscStringCastValue *out,char *error,size_t size) {
    GscEntityRef ref;cod1ActorLease_t lease;cod1ActorAssignedModel_t assigned;
    GscStringCastValue result;char text[MAX_STRING_CHARS];uint32_t index;int spawning,ok=0;
    if(!input||!out||!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    /* Native field getter20041B60 has no constructor-only collider test.
     * Keep the current runtime actor lease and model transaction checks. */
    ref=*input;if(!ActorPoseLease(&ref,&lease))return 0;
    assigned=cod1Actors[lease.actorIndex].assignedModel;index=assigned.registration.modelIndex;
    if(index>255||(index&&memcmp(&assigned.registration.lease,&lease,sizeof(lease))))return 0;
    if(!ActorModelBegin(&lease,&spawning))return 0;
    if(ActorModelReadCS(&lease,spawning,index,text)&&ActorLeaseValid(&lease)&&
       !memcmp(input,&ref,sizeof(ref))&&
       !memcmp(&cod1Actors[lease.actorIndex].assignedModel,&assigned,sizeof(assigned))&&
       (!index||!strcmp(text,assigned.registration.nativeName))&&strlen(text)<sizeof(result.string)) {
        memset(&result,0,sizeof(result));result.type=1;strcpy(result.string,text);ok=1;
    }
    ActorModelEnd(&lease);if(!ok)return 0;*out=result;return 1;
}
/* Original entity field WORD+12e/type4: zero -> undefined, otherwise
 * AddConstString. The live port classname is its owned semantic text.
 * Ordinary entities (including MG42) do not require an Actor component. */
int G_Cod1EntityReadClassnameField(const GscEntityRef *input,GscStringCastValue *out,char *error,size_t size) {
    GscEntityRef ref;GscStringCastValue value;cod1NativeStaticText_t text;
    if(!input||!out||!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    ref=*input;if(!EntityValidate(NULL,&ref)||!NativeStaticTextCapture(g_entities[ref.slot].classname,&text))return 0;
    memset(&value,0,sizeof(value));if(text.present){value.type=1;strcpy(value.string,text.text);}
    *out=value;return 1;
}
static int ActorScriptSetModel(void *user,const GscEntityRef *ref,const char *model,char *error,size_t size) {
    cod1ActorLease_t lease;cod1ActorModelValue_t value;cod1ActorModelMethodResult_t result;(void)user;
    if(!ref||!model||!EntityValidate(NULL,ref))return 0;
    if(!cod1ActorByEntity[ref->slot]&&!cod1SentientByEntity[ref->slot])return EntitySetModel(NULL,ref,model,error,size);
    if(!ActorScriptLease(ref,&lease)||strlen(model)>=sizeof(value.string))return 0;
    memset(&value,0,sizeof(value));value.type=1;strcpy(value.string,model);
    return G_Cod1ActorInvokeTypedModelMethod(&lease,COD1_ACTOR_METHOD_SETMODEL,&value,1,&result,error,size)==COD1_ACTOR_METHOD_VOID;
}
const GscEntityAccessHost *G_Cod1ActorConstructorAccessHost(void) {
    static const GscEntityAccessHost host={NULL,ActorScriptSetModel,EntityGetOrigin,EntityGetAngles,EntitySetAngles};return &host;
}
static int ActorScriptMethodSelected(int runtime,void *user,const GscEntityRef *input,uint32_t method,
    const GscStringCastValue *args,int argc,char *error,size_t size) {
    cod1ActorLease_t lease;cod1ActorModelMethodResult_t result;
    cod1ActorModelValue_t copied[GSC_NATIVE_MAX_ARGS];uint32_t nativeMethod;int disposition,consumed;
    (void)user;
    if(argc<0||argc>GSC_NATIVE_MAX_ARGS||!input||(argc&&!args)||
       !ActorArguments(input,sizeof(*input),NULL,0,error,size)||
       !ActorArguments(args,(size_t)argc*sizeof(*args),NULL,0,error,size))return 0;
    if(method==GSC_ENTITY_METHOD_ATTACH)nativeMethod=COD1_ACTOR_METHOD_ATTACH;
    else if(method==GSC_ENTITY_METHOD_DETACH)nativeMethod=COD1_ACTOR_METHOD_DETACH;
    else if(method==GSC_ENTITY_METHOD_DETACH_ALL)nativeMethod=COD1_ACTOR_METHOD_DETACH_ALL;
    else return 0;
    if(!ActorScriptLease(input,&lease)&&(!runtime||!ActorPoseLease(input,&lease))){EntityError(error,size,"actor script method requires a hidden native-data actor in a ready map");return 0;}
    memset(copied,0,sizeof(copied));memset(&result,0,sizeof(result));
    consumed=nativeMethod==COD1_ACTOR_METHOD_DETACH_ALL?0:(argc>2?2:argc);
    if(consumed)memcpy(copied,args,(size_t)consumed*sizeof(*args));
    disposition=G_Cod1ActorInvokeTypedModelMethod(&lease,nativeMethod,copied,argc,&result,error,size);
    /* Retirement through a model callback cancels the old script owner.
     * A replacement actor on the same allocation is never adopted. */
    if(!G_Cod1EntitySameAllocation(&lease.entity))return 1;
    if(!ActorLeaseValid(&lease)){EntityError(error,size,"actor script method owner component changed");return 0;}
    if(disposition==COD1_ACTOR_METHOD_VOID)return 1;
    if(disposition==COD1_ACTOR_METHOD_SCRIPT_ERROR||disposition==COD1_ACTOR_METHOD_PARAMETER_ERROR)
        EntityError(error,size,result.message);
    return 0;
}
static int ActorScriptMethod(void*u,const GscEntityRef*r,uint32_t m,const GscStringCastValue*a,int n,char*e,size_t z){return ActorScriptMethodSelected(0,u,r,m,a,n,e,z);}
static int ActorRuntimeScriptMethod(void*u,const GscEntityRef*r,uint32_t m,const GscStringCastValue*a,int n,char*e,size_t z){return ActorScriptMethodSelected(1,u,r,m,a,n,e,z);}
const GscEntityMethodHost *G_Cod1ActorRuntimeMethodHost(void){static const GscEntityMethodHost h={NULL,ActorRuntimeScriptMethod};return &h;}
const GscEntityMethodHost *G_Cod1ActorConstructorMethodHost(void) {
    static const GscEntityMethodHost host={NULL,ActorScriptMethod};return &host;
}
