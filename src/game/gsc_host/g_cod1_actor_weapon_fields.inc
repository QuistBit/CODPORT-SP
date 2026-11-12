/* Selected registered-table semantics, independent of CW/CI/WP numbering.
 * No caller table, resource pointer or callback request is retained. */
static cod1ActorWeaponFieldHost_t cod1ActorWeaponFields;
static uint64_t cod1ActorWeaponFieldsEpoch;
static int cod1ActorWeaponFieldsBusy;
static void ActorWeaponFieldsReset(void) {
    memset(&cod1ActorWeaponFields,0,sizeof(cod1ActorWeaponFields));
    cod1ActorWeaponFieldsEpoch=0;
    /* An old callback may be crossing this reset: its busy scope ends only
     * when it returns, preventing a nested binding from replacing its scope. */
}
static int ActorWeaponTableValid(const cod1ActorRegisteredWeaponTable_t *table) {
    uint32_t i;
    if(!table->rowCount||table->rowCount>COD1_ACTOR_REGISTERED_WEAPONS_MAX)return 0;
    for(i=0;i<table->rowCount;++i) {
        const cod1ActorRegisteredWeaponRow_t *row=&table->rows[i];
        if((row->definitionPresent!=0&&row->definitionPresent!=1)||
           (row->namePresent!=0&&row->namePresent!=1)||
           (!row->definitionPresent&&row->namePresent)||
           (row->namePresent&&!memchr(row->name,0,sizeof(row->name))))return 0;
    }
    return 1;
}
int G_Cod1ActorSelectWeaponFieldTable(const cod1ActorWeaponFieldHost_t *input,char *error,size_t size) {
    cod1ActorWeaponFieldHost_t copy;uint64_t epoch;int current;
    if(!input||!ActorArguments(input,sizeof(*input),NULL,0,error,size))return 0;
    if(cod1ActorWeaponFieldsBusy||cod1ActorWeaponFieldsEpoch||!ActorMapAvailable()||
       !cod1EntityReady||level.spawning||level.restarted) {
        EntityError(error,size,"weapon field table requires an unbound ready map");return 0;
    }
    copy=*input;
    if(!copy.receipt||!copy.current||!ActorWeaponTableValid(&copy.table)) {
        EntityError(error,size,"weapon field table authority is malformed");return 0;
    }
    epoch=cod1EntityEpoch;cod1ActorWeaponFieldsBusy=1;current=copy.current(copy.receipt);
    cod1ActorWeaponFieldsBusy=0;
    if(current!=1||epoch!=cod1EntityEpoch||!ActorMapAvailable()||!cod1EntityReady||
       level.spawning||level.restarted||cod1ActorWeaponFieldsEpoch) {
        EntityError(error,size,"weapon field table authority changed during binding");return 0;
    }
    cod1ActorWeaponFields=copy;cod1ActorWeaponFieldsEpoch=epoch;return 1;
}
int G_Cod1ActorReadTrainingFields(const cod1ActorLease_t *input,
    cod1ActorTrainingFields_t *out,char *error,size_t size) {
    cod1ActorLease_t lease;
    if(!input||!out||!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    lease=*input;
    if(!ActorLeaseValid(&lease)||lease.kind!=COD1_ACTOR_LEASE_ACTOR) {
        EntityError(error,size,"training field snapshot requires a current actor lease");return 0;
    }
    *out=cod1Actors[lease.actorIndex].trainingFields;return 1;
}
static int ActorTrainingCurrent(const cod1ActorLease_t *lease) {
    return EntityValidate(NULL,&lease->entity)&&ActorTopologyStageValid(lease,0)&&
        !cod1Actors[lease->actorIndex].modelBusy&&!cod1Actors[lease->actorIndex].modelMethodBusy&&
        !level.spawning&&!level.restarted;
}
static int ActorWeaponAuthorityCurrent(const cod1ActorLease_t *lease,char *error,size_t size) {
    uint64_t epoch=cod1ActorWeaponFieldsEpoch,receipt=cod1ActorWeaponFields.receipt;int current;
    if(!epoch||epoch!=cod1EntityEpoch||!cod1ActorWeaponFields.current) {
        EntityError(error,size,"registered weapon field table is unavailable");return 0;
    }
    current=cod1ActorWeaponFields.current(receipt);
    if(!ActorTrainingCurrent(lease)) {
        EntityError(error,size,"training field owner changed during registry validation");return 0;
    }
    if(current!=1||epoch!=cod1EntityEpoch||epoch!=cod1ActorWeaponFieldsEpoch||
       receipt!=cod1ActorWeaponFields.receipt) {
        EntityError(error,size,"registered weapon field table authority changed");return 0;
    }
    return 1;
}
static int ActorWeaponTextEqual(const char *a,const char *b) {
    unsigned char x,y;
    do {
        x=(unsigned char)*a++;y=(unsigned char)*b++;
        if(x>='a'&&x<='z')x=(unsigned char)(x-'a'+'A');
        if(y>='a'&&y<='z')y=(unsigned char)(y-'a'+'A');
        if(x!=y)return 0;
    }while(x);
    return 1;
}
static int ActorTrainingRead(void *user,const GscEntityRef *input,const char *name,
    GscStringCastValue *out,char *error,size_t size) {
    cod1ActorLease_t lease;GscStringCastValue copy;const cod1ActorRegisteredWeaponRow_t *row;
    uint32_t index;int ok=0;(void)user;
    if(cod1ActorWeaponFieldsBusy) {EntityError(error,size,"training field callback reentered");return 0;}
    if(!name)return 0;
    if(Q_stricmp(name,"grenadeweapon")&&Q_stricmp(name,"bravery")&&Q_stricmp(name,"grenadeammo"))
        return ActorScriptFieldRead(user,input,name,out,error,size);
    if(!input||!out||!ActorArguments(input,sizeof(*input),out,sizeof(*out),error,size)||
       ActorOverlap(input,sizeof(*input),out,sizeof(*out)))return 0;
    if(!ActorScriptLease(input,&lease)) {EntityError(error,size,"training field read requires a current native-data actor");return 0;}
    memset(&copy,0,sizeof(copy));
    if(!Q_stricmp(name,"bravery")) {
        copy.type=4;copy.number=cod1Actors[lease.actorIndex].trainingFields.bravery;
        if(!isfinite(copy.number))return 0;
    } else if(!Q_stricmp(name,"grenadeammo")) {
        copy.type=5;copy.integer=cod1Actors[lease.actorIndex].trainingFields.grenadeAmmo;
    } else {
        cod1ActorWeaponFieldsBusy=1;
        if(!ActorWeaponAuthorityCurrent(&lease,error,size))goto done;
        index=cod1Actors[lease.actorIndex].trainingFields.grenadeWeaponIndex;
        if(index>=cod1ActorWeaponFields.table.rowCount) {
            EntityError(error,size,"grenade weapon index is outside the selected table");goto done;
        }
        row=&cod1ActorWeaponFields.table.rows[index];
        if(row->definitionPresent) {
            if(!row->namePresent) {EntityError(error,size,"registered weapon getter has no admitted name");goto done;}
            copy.type=1;strcpy(copy.string,row->name);
        }
        ok=1;
done:
        cod1ActorWeaponFieldsBusy=0;if(!ok)return 0;
    }
    *out=copy;return 1;
}
static int ActorTrainingWrite(void *user,const GscEntityRef *input,const char *name,
    const GscStringCastValue *value,char *error,size_t size) {
    cod1ActorLease_t lease;GscStringCastValue copy;cod1ActorModelString_t text;
    float number;int32_t integer;uint32_t i,index=0;uint64_t epoch,receipt;int found=0,ok=0;(void)user;
    if(cod1ActorWeaponFieldsBusy) {EntityError(error,size,"training field callback reentered");return 0;}
    if(!name)return 0;
    if(Q_stricmp(name,"grenadeweapon")&&Q_stricmp(name,"bravery")&&Q_stricmp(name,"grenadeammo"))
        return ActorScriptFieldWrite(user,input,name,value,error,size);
    if(!input||!value||!ActorArguments(input,sizeof(*input),NULL,0,error,size)||
       !ActorArguments(value,sizeof(*value),NULL,0,error,size))return 0;
    copy=*value;
    if(!ActorScriptLease(input,&lease)) {EntityError(error,size,"training field write requires a current native-data actor");return 0;}
    if(!Q_stricmp(name,"bravery")) {
        if(copy.type==4){number=copy.number;if(!isfinite(number))return 0;}
        else if(copy.type==5&&copy.integer>=-16777216&&copy.integer<=16777216)number=(float)copy.integer;
        else {EntityError(error,size,"bravery requires an admitted finite float or exact integer");return 0;}
        cod1Actors[lease.actorIndex].trainingFields.bravery=number;return 1;
    }
    if(!Q_stricmp(name,"grenadeammo")) {
        if(!ActorScriptInteger(&copy,&integer)) {EntityError(error,size,"grenadeAmmo requires admitted numeric int32");return 0;}
        cod1Actors[lease.actorIndex].trainingFields.grenadeAmmo=integer;return 1;
    }
    if(G_Cod1ActorConvertModelValue(&copy,&text,error,size)!=1)return 0;
    cod1ActorWeaponFieldsBusy=1;
    if(!ActorWeaponAuthorityCurrent(&lease,error,size))goto done;
    /* The native lookup dereferences every definition in the inclusive scan.
     * NULL definitions are admitted for getter-only use, never silently skipped. */
    for(i=0;i<cod1ActorWeaponFields.table.rowCount;++i)
        if(!cod1ActorWeaponFields.table.rows[i].definitionPresent) {
            EntityError(error,size,"registered weapon lookup requires complete definitions");goto done;
        }
    for(i=0;i<cod1ActorWeaponFields.table.rowCount;++i) {
        const cod1ActorRegisteredWeaponRow_t *row=&cod1ActorWeaponFields.table.rows[i];
        if(row->namePresent&&ActorWeaponTextEqual(text.string,row->name)){index=i;found=1;break;}
    }
    epoch=cod1ActorWeaponFieldsEpoch;receipt=cod1ActorWeaponFields.receipt;
    if(!found) {
        G_Printf("Couldn't find weapon \"%s\"\n",text.string);
        if(!ActorTrainingCurrent(&lease)||epoch!=cod1EntityEpoch||epoch!=cod1ActorWeaponFieldsEpoch||
           receipt!=cod1ActorWeaponFields.receipt) {
            EntityError(error,size,"training field ownership changed during weapon warning");goto done;
        }
        /* Warning callbacks may reset/reload the provider without changing
         * the actor. Revalidate its receipt before publishing the default0. */
        if(!ActorWeaponAuthorityCurrent(&lease,error,size))goto done;
    }
    cod1Actors[lease.actorIndex].trainingFields.grenadeWeaponIndex=index;ok=1;
done:
    cod1ActorWeaponFieldsBusy=0;return ok;
}
const GscEntityFieldHost *G_Cod1ActorTrainingFieldHost(void) {
    static const GscEntityFieldHost host={NULL,ActorScriptClassify,ActorTrainingRead,ActorTrainingWrite};return &host;
}
