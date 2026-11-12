/* SPDX-License-Identifier: GPL-3.0-or-later
 * MUZZLE_R3: inspected local-view muzzle definitions, including heavy_view. Included by cg_cod1_efx.h.
 * Inspected local-view resources only; unsupported definitions stay disabled.
 * World flashes, brass, impact FX and missing tags are NOT approximated.
 */
#ifndef CG_COD1_MUZZLE_H
#define CG_COD1_MUZZLE_H
static int ceMuzzleRendererReady(void) {
    char value[32];trap_Cvar_VariableStringBuffer("r_cod1MuzzleQuads",value,sizeof(value));
    if(atoi(value)>=1)return ceRendererReady();
    if(!ceMuzzle.warnedRenderer) {
        CG_Printf("CoD1 MUZZLE_R3: update WolfSP.exe with the client DLL; native view flash disabled (no oriented-quad renderer).\n");
        ceMuzzle.warnedRenderer=1;
    }
    return 0;
}
int CG_Cod1EfxRegisterMuzzle(const char *path) {
    static const char *const resources[]={
        "fx/muzzleflashes/thompson_view.efx",
        "fx/muzzleflashes/german_mg_view.efx",
        "fx/muzzleflashes/russian_mg_view.efx",
        "fx/muzzleflashes/standardflashview.efx",
        "fx/muzzleflashes/heavy_view.efx"
    };
    char canonical[CEFX_PATH];unsigned i;
    if(!path || !*path || !cePath(path,canonical,1))return 0;
    /* Follow the actual WeaponDef resource, not a guessed weapon name.
     * Only these five fully parsed definitions are enabled, not arbitrary mods. */
    for(i=0;i<sizeof(resources)/sizeof(resources[0]);++i)
        if(!strcmp(canonical,resources[i]))break;
    if(i==sizeof(resources)/sizeof(resources[0]))return 0;
    if(!ceMuzzleRendererReady())return 0;
    return ceRegisterMode(canonical,0,1);
}
void CG_Cod1EfxMuzzleShot(int effect,int key) {
    int n;
    if(effect<=0 || effect>ceState.cacheCount || ceState.cache[effect-1].state!=2 ||
        !ceState.cache[effect-1].mode)return;
    ++ceMuzzle.events;
    trap_Cvar_Update(&ceEnabled);trap_Cvar_Update(&ceMuzzleEnabled);
    if(!ceEnabled.integer || !ceMuzzleEnabled.integer) {++ceMuzzle.discarded;return;}
    if(ceMuzzle.count==CE_SHOTS) {++ceMuzzle.discarded;return;}
    n=ceMuzzle.count++;ceMuzzle.shot[n].effect=effect;ceMuzzle.shot[n].key=key;ceMuzzle.shot[n].time=cg.time;
}
void CG_Cod1EfxBeginView(void) {
    /* Every scene, not every timestamp: stereo views may share cg.time but
     * have distinct camera transforms. Old hidden tags must never be reused. */
    ceCheckClock();ceMuzzle.view.valid=0;
    trap_Cvar_Update(&ceEnabled);trap_Cvar_Update(&ceMuzzleEnabled);
}
void CG_Cod1EfxMuzzleViewPose(int effect,int key,const refEntity_t *gun,
                              const orientation_t *poseTag,qboolean nativePose) {
    orientation_t tag;ceViewAttachment *v=&ceMuzzle.view;int i,j,k,n;float delay;
    ceCached *c;
    if(cg.hyperspace || !gun || !ceEnabled.integer || !ceMuzzleEnabled.integer || effect<=0 ||
        effect>ceState.cacheCount || !ceMuzzleRendererReady())return;
    c=&ceState.cache[effect-1];if(c->state!=2 || !c->mode)return;
    if(nativePose ? poseTag==NULL : trap_R_LerpTag(&tag,gun,"tag_flash",0)<0) {
        if(v->model!=gun->hModel || v->key!=key) {
            CG_Printf("CoD1 MUZZLE_R3: model %d has no tag_flash; no guessed muzzle offset. Rebuild WolfSP.exe and check the original XModel.\n",gun->hModel);
            ++ceMuzzle.missingTags;v->model=gun->hModel;v->key=key;++v->generation;
        }
        return;
    }
    if(nativePose)tag=*poseTag;
    if(v->model!=gun->hModel || v->key!=key || !v->generation) {
        v->model=gun->hModel;v->key=key;if(++v->generation==0)++v->generation;
    }
    VectorCopy(gun->origin,v->tag.origin);
    for(i=0;i<3;i++)VectorMA(v->tag.origin,tag.origin[i],gun->axis[i],v->tag.origin);
    for(i=0;i<3;i++)for(j=0;j<3;j++) {
        v->tag.axis[i][j]=0;
        for(k=0;k<3;k++)v->tag.axis[i][j]+=tag.axis[i][k]*gun->axis[k][j];
    }
    v->valid=1;
    /* Fire events are queued by the existing prediction-deduplicated event
     * path. Never create a shot merely because the fire button is held. */
    for(n=0;n<ceMuzzle.count;n++) {
        double at=ceMuzzle.shot[n].time;
        if(ceMuzzle.shot[n].effect!=effect || ceMuzzle.shot[n].key!=key ||
           (double)cg.time-at<0 || (double)cg.time-at>150) {++ceMuzzle.discarded;continue;}
        for(i=0;i<c->def->count;i++) {
            const cePrimitive *p=&c->def->prim[i];int count=(int)floorf(ceSample(p->count)+0.5f);
            for(j=0;j<count;j++) {
                if(p->spawn&CE_EVEN)delay=fminf(p->delay.a,p->delay.b)+fabsf(p->delay.b-p->delay.a)*j/count;
                else delay=ceSample(p->delay);
                ceMake(effect,i,v->tag.origin,v->tag.axis,at+delay,0,v);
            }
        }
        ++ceMuzzle.played;
    }
    ceMuzzle.count=0;
}
void CG_Cod1EfxMuzzleView(int effect,int key,const refEntity_t *gun) {
    CG_Cod1EfxMuzzleViewPose(effect,key,gun,NULL,qfalse);
}
void CG_Cod1EfxAddView(void) {
    int i;double now=(double)cg.time;
    /* The old world/grenade pass is kept in its original place and explicitly
     * skips muzzle entries. This pass runs AFTER the final weapon pose. */
    for(i=0;i<CE_LIVE;i++) {
        ceLive *l=&ceState.live[i];
        if(!l->used || !l->muzzle)continue;
        if(cg.hyperspace || !ceEnabled.integer || !ceMuzzleEnabled.integer || now>=l->end ||
            (l->relative && (!ceMuzzle.view.valid || l->attachment!=ceMuzzle.view.generation))) {
            l->used=0;continue;
        }
        if(now<l->start)continue;
        if(ceMove(l,now))ceDraw(l);
    }
    /* No visible viewmodel (scope/ladder/third-person/drawGun=0): discard
     * pending shots instead of replaying them when the gun appears again. */
    ceMuzzle.discarded+=(unsigned)ceMuzzle.count;ceMuzzle.count=0;
}
void CG_Cod1MuzzleInfo_f(void) {
    char renderer[32];trap_Cvar_VariableStringBuffer("r_cod1MuzzleQuads",renderer,sizeof(renderer));
    CG_Printf("CoD1 MUZZLE_R3: enabled=%d native=%d renderer=%s key=%d model=%d tag=%d generation=%u events=%u played=%u discarded=%u missingTags=%u\n",
        ceMuzzleEnabled.integer,ceEnabled.integer,renderer,ceMuzzle.view.key,ceMuzzle.view.model,
        ceMuzzle.view.valid,ceMuzzle.view.generation,ceMuzzle.events,ceMuzzle.played,ceMuzzle.discarded,ceMuzzle.missingTags);
    CG_Printf("  resources: thompson_view, german_mg_view, russian_mg_view, standardflashview, heavy_view (Line/origin2)\n");
    if(ceMuzzle.view.valid)CG_Printf("  tag_flash world: %.3f %.3f %.3f\n",ceMuzzle.view.tag.origin[0],ceMuzzle.view.tag.origin[1],ceMuzzle.view.tag.origin[2]);
}
#endif
