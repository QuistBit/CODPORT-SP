/* Original trap36 target-only capsule sight. GAME supplies and checks the
 * actual allocation; a native actor additionally requires its exact SERVER
 * key. No global world trace, pass-owner filter, or DObj trace is involved. */
#include "../qcommon/cod1_path_sight.h"
int SV_Cod1PathSight(const cod1PathSightQuery_t*query,int*out){
    cod1PathSightQuery_t q;sharedEntity_t*ent;uint32_t contents;
    C1PathSight t;int i,status,result;float enter,leave;
    if(!query||!out)return COD1_SPATIAL_INVALID;q=*query;
    if(q.version!=COD1_SPATIAL_VERSION||q.bytes!=sizeof(q)||
       !CS_Finite(q.start)||!CS_Finite(q.end)||q.slot>=ENTITYNUM_WORLD)
        return COD1_SPATIAL_INVALID;
    if(!CS_TableReady()||!CM_HasV59()||q.slot>=(unsigned)sv.num_entities)
        return COD1_SPATIAL_UNAVAILABLE;
    ent=SV_GentityNum(q.slot);
    if(ent->s.number!=(int)q.slot||!ent->r.linked)return COD1_SPATIAL_STALE;
    if(cod1SpatialActors[q.slot].active){
        if(!CS_KeyValid(&q.actor)||!CS_KeyEqual(&q.actor,&cod1SpatialActors[q.slot].key))
            return COD1_SPATIAL_STALE;
    }else if(q.actor.epoch||q.actor.lease||q.actor.generation||q.actor.slot)
        return COD1_SPATIAL_STALE;
    status=CS_Contents(q.slot,ent,&contents);if(status!=COD1_SPATIAL_OK)return status;
    if(contents!=q.expectedContents)return COD1_SPATIAL_STALE;
    if(!CS_Finite(ent->r.currentOrigin)||!CS_Bounds(ent->r.absmin,ent->r.absmax))
        return COD1_SPATIAL_INVALID;
    /* Fixed original path hull. Bound intermediate sums before CM. */
    for(i=0;i<3;i++){
        double a=q.start[i],b=q.end[i],o=ent->r.currentOrigin[i];
        if(fabs(a)>1000000||fabs(b)>1000000||fabs(o)>1000000||
           fabs(ent->r.absmin[i])>2000000||fabs(ent->r.absmax[i])>2000000)
            return COD1_SPATIAL_UNAVAILABLE;
    }
    result=0;
    if(!(contents&COD1_SPATIAL_FLOOR_MASK)){*out=0;return COD1_SPATIAL_OK;}
    for(i=0;i<3;i++){
        double lo=(q.start[i]<q.end[i]?q.start[i]:q.end[i])+(i==2?18:-15)-1.0;
        double hi=(q.start[i]>q.end[i]?q.start[i]:q.end[i])+(i==2?72:15)+1.0;
        /* Original retains X on x87, but stores Y/Z swept limits. */
        if(i){lo=C1PS_Store(lo);hi=C1PS_Store(hi);}
        if(hi<ent->r.absmin[i]||lo>ent->r.absmax[i]){*out=0;return COD1_SPATIAL_OK;}
    }
    {
        static const vec3_t mins={-15,-15,18},maxs={15,15,72};
        if(ent->r.bmodel){
            if(ent->s.modelindex<=0||ent->s.modelindex>=CM_NumInlineModels())return COD1_SPATIAL_INVALID;
            if(!CS_Finite(ent->r.currentAngles)||!VectorCompare(ent->r.currentAngles,vec3_origin))
                return COD1_SPATIAL_UNAVAILABLE;
            if(!CM_Cod1PathBrushSight(q.start,q.end,mins,maxs,ent->r.currentOrigin,
                ent->s.modelindex,COD1_SPATIAL_FLOOR_MASK,&result))return COD1_SPATIAL_UNAVAILABLE;
        }else{
            int capsule=cod1SpatialActors[q.slot].active?
                (cod1SpatialActors[q.slot].flags&COD1_SPATIAL_NATIVE_CAPSULE_FLAG):
                (ent->r.svFlags&SVF_CAPSULE);
            if(capsule)return COD1_SPATIAL_UNAVAILABLE;
            if(!CS_Bounds(ent->r.mins,ent->r.maxs))return COD1_SPATIAL_INVALID;
            for(i=0;i<3;i++)if(fabs(ent->r.mins[i])>1000000||fabs(ent->r.maxs[i])>1000000)
                return COD1_SPATIAL_UNAVAILABLE;
            C1PS_Prepare(&t,q.start,q.end,mins,maxs,ent->r.currentOrigin);
            result=C1PS_Axes(&t,ent->r.mins,ent->r.maxs,&enter,&leave)?-1:0;
        }
    }
    *out=result;return COD1_SPATIAL_OK;
}
