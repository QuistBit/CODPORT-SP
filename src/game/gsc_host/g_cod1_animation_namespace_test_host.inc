/* R211 selected metadata namespace: real winning VFS resources, no playback. */
static uint32_t GscTestAnimationNamespaceHash(const void *data,size_t size) {
    const unsigned char *p=(const unsigned char *)data;uint32_t hash=2166136261u;
    while(size--)hash=(hash^*p++)*16777619u;
    return hash;
}
static int GscTestAnimationNamespaceResources(char *error,size_t size,int joined) {
    static const char *trees[3]={"generic_human","animation_rig_gear_trainingjump","animation_rig_wire_trainingjump"};
    static const char *names[3]={"training_guy_practicejump","training_gear_practicejump","training_wire_practicejump"};
    static const char *atrs[3]={"animtrees/generic_human.atr","animtrees/animation_rig_gear_trainingjump.atr","animtrees/animation_rig_wire_trainingjump.atr"};
    static const char *clips[3]={"xanim/training_guy_practicejump","xanim/training_gear_practicejump","xanim/training_wire_practicejump"};
    void *atr[3]={NULL,NULL,NULL},*clip[3]={NULL,NULL,NULL};
    size_t atrSize[3]={0,0,0},clipSize[3]={0,0,0};
    cod1StartOriginResourceInput_t inputs[3];
    GscSessionAnimationNamespaceSelection selection;
    int i,ok=0;
    memset(inputs,0,sizeof(inputs));memset(&selection,0,sizeof(selection));
    for(i=0;i<3;++i) {
        if(!GscTestStartOriginRead(atrs[i],131072,&atr[i],&atrSize[i],error,size) ||
           !GscTestStartOriginRead(clips[i],16777216,&clip[i],&clipSize[i],error,size))goto done;
        inputs[i].tree=trees[i];inputs[i].name=names[i];
        inputs[i].atr_bytes=atr[i];inputs[i].atr_size=atrSize[i];
        inputs[i].xanim_bytes=clip[i];inputs[i].xanim_size=clipSize[i];
        selection.catalogs[i].atr_bytes=atr[i];selection.catalogs[i].atr_size=atrSize[i];
        G_Printf("[R211] RESOURCE_BYTES path=%s bytes=%u fnv=%08x\n",atrs[i],(unsigned)atrSize[i],GscTestAnimationNamespaceHash(atr[i],atrSize[i]));
        G_Printf("[R211] RESOURCE_BYTES path=%s bytes=%u fnv=%08x\n",clips[i],(unsigned)clipSize[i],GscTestAnimationNamespaceHash(clip[i],clipSize[i]));
    }
    if(!G_Cod1SelectStartOriginResources(inputs,3,error,size) ||
       !GscTestModuleReadCurrent() ||
       !G_Cod1StartOriginMapCertificate(&selection.map_certificate,error,size) ||
       !(joined?gsc_session_training_rigs(gscTestSession,&selection,1,error,size):gsc_session_select_r211_animation_namespace(gscTestSession,&selection,error,size)) ||
       !GscTestModuleReadCurrent())goto done;
    G_Printf("[R211] ANIMATION_NAMESPACE_RESOURCES catalogs=3 ATR=%u,%u,%u XAnim=%u,%u,%u map-receipt=%llu\n",
        (unsigned)atrSize[0],(unsigned)atrSize[1],(unsigned)atrSize[2],
        (unsigned)clipSize[0],(unsigned)clipSize[1],(unsigned)clipSize[2],selection.map_certificate.receipt);
    ok=1;
done:
    for(i=0;i<3;++i) {
        if(atr[i])COD1_GSC_VFS_FREE(atr[i]);
        if(clip[i])COD1_GSC_VFS_FREE(clip[i]);
    }
    if(!ok)GscTestError(error,size,"actual VFS/animation namespace Session selection refused");
    return ok;
}

static int GscTestAnimationNamespaceSelect(char *error,size_t size){
 return GscTestAnimationNamespaceResources(error,size,0);
}
static int GscTestTrainingRigDeclarations(const GscSessionTurretProgramPlan *plan,char *error,size_t size){
 static const char *const paths[2]={"animtrees/animation_rig_gear_trainingjump.atr","animtrees/animation_rig_wire_trainingjump.atr"};
 GscSessionAnimationNamespaceSelection selection={0};
 void *bytes[2]={NULL,NULL};size_t n;int i,ok=0;
 selection.catalogs[0]=plan->catalogs[0];
 for(i=0;i<2;i++){
  if(!GscTestStartOriginRead(paths[i],131072,&bytes[i],&n,error,size))goto done;
  selection.catalogs[i+1].atr_bytes=bytes[i];selection.catalogs[i+1].atr_size=n;
 }
 ok=gsc_session_training_rigs(gscTestSession,&selection,0,error,size);
done:
 for(i=0;i<2;i++)if(bytes[i])COD1_GSC_VFS_FREE(bytes[i]);
 return ok;
}
