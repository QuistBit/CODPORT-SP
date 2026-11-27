static cgfa4Resource_t*CGFA4_Resource(const cgfa4Lease_t*l){cgfa4Resource_t staged,*slot=NULL;cod1TurretActorResourceState_t def;c1at14Input_t inputs[8];void*models[8]={0},*parts[8]={0};char names[8][128],path[128];unsigned i;int n,ok=0;
 const char *composition=l->config;
 int recyclable=0,hit=0;
 int version=l->actor.cod1TurretActorVersion==COD1_SOURCE_POSE_WIRE_VERSION?COD1_SCRIPTED_ACTOR_WIRE_VERSION:l->actor.cod1TurretActorVersion;
 if(l->actor.cod1TurretActorConfig==CS_COD1_JUMPER){
    char prefix[64];
    int count=snprintf(prefix,sizeof(prefix),"j:%u:%u:%u:%u|",(unsigned)l->actor.cod1TurretEpochLo,
        (unsigned)l->actor.cod1TurretEpochHi,(unsigned)l->actor.number,(unsigned)l->actor.cod1TurretActorGeneration);
    if(count<=0||(size_t)count>=sizeof(prefix)||strncmp(composition,prefix,count))return NULL;
    composition+=count;
    recyclable=1;
 }
 if(!C1TAW_Composition(composition,&def)||!CGFA4_LoadClips(l))return NULL;
 if(recyclable){
    slot=CGFA4_RecyclableSlot(&def,version,&hit);
    if(hit)return slot;
 }else{
    for(i=0;i<CGTA_CACHE;i++){
        if(cgfa4Resources[i].handle&&cgfa4Resources[i].version==version&&
           !memcmp(&cgfa4Resources[i].definition,&def,sizeof(def)))return cgfa4Resources+i;
        if(!cgfa4Resources[i].resource&&!slot)slot=cgfa4Resources+i;
    }
    if(!slot){static int shown; if(!shown++){CG_Printf("[378 cache full] owner=%d version=%d slots=%d\n",l->actor.number,l->actor.cod1TurretActorVersion,CGTA_CACHE);}return NULL;}
 }
 memset(&staged,0,sizeof(staged));memset(inputs,0,sizeof(inputs));staged.definition=def;
 for(i=0;i<(unsigned)def.partCount;i++){c1mrModelHeader_t model;snprintf(path,sizeof(path),"xmodel/%s",def.model[i]);models[i]=CGFA4_Read(l,path,&n);if(!models[i]||!C1MR_DecodeModel(models[i],n,&model)||!C1TAW_Name(model.lodName[0]))goto done;inputs[i].model=models[i];inputs[i].modelBytes=n;inputs[i].modelName=def.model[i];strcpy(names[i],model.lodName[0]);inputs[i].partsName=names[i];inputs[i].tag=def.tag[i];snprintf(path,sizeof(path),"xmodelparts/%s",names[i]);parts[i]=CGFA4_Read(l,path,&n);if(!parts[i])goto done;inputs[i].parts=parts[i];inputs[i].partsBytes=n;}
 if(!C1AD14_Create(inputs,def.partCount,&staged.resource)||!CGFA4_BuildBank(l,&staged)||!CGFA4_Current(l))goto done;
 staged.handle=trap_R_RegisterCod1TurretActor(&def);if(!staged.handle||!CGFA4_Current(l)||(!recyclable&&slot->resource))goto done;
 C1AD14_Free(slot->resource);*slot=staged;staged.resource=NULL;
 if(recyclable)CGFA4_RecyclablePublished(slot);
 ok=1;
 done:for(i=0;i<8;i++){free(models[i]);free(parts[i]);}C1AD14_Free(staged.resource);return ok?slot:NULL;
}
