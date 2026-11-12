/* R200 explicit script-brush collision removal. Visibility remains separate.
 * Clear the server collision mask and set the existing prediction bmodel flag;
 * an EF_NODRAW change alone cannot open the original training wall passage. */
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "g_cod1_entity_host.h"
#include "g_cod1_r200_collision.h"
#include <math.h>
static int R200BrushLease(const GscEntityRef *ref,int time,int (*current)(void *),void *lease) {
    return current&&current(lease)&&G_Cod1EntitySameAllocation(ref)&&!level.spawning&&
        !level.restarted&&level.time==time&&G_Cod1MapEntities()&&
        g_gametype.integer==GT_SINGLE_PLAYER&&R200BrushShape(&g_entities[ref->slot]);
}
int G_Cod1ScriptBrushNotSolid(const GscEntityRef *input,int now,char *error,size_t cap,
    int (*current)(void *),void *lease) {
    GscEntityRef ref;gentity_t before,*ent;int time;
    if(!input||now<0||!current)goto refuse;
    ref=*input;time=level.time;
    if(!R200BrushLease(&ref,time,current,lease)||memcmp(input,&ref,sizeof(ref)))goto refuse;
    ent=&g_entities[ref.slot];before=*ent;
    if(!ent->r.linked)goto refuse;
    if(!R200BrushLease(&ref,time,current,lease)||memcmp(input,&ref,sizeof(ref))||
       memcmp(ent,&before,sizeof(before)))goto refuse;
    if(!ent->r.contents&&!ent->clipmask&&
       (!ent->s.solid||(ent->s.eFlags&EF_NONSOLID_BMODEL))){
        if(G_Cod1ScriptBrushNonsolidOwned(&ref))return 1;
        goto refuse;
    }
    trap_UnlinkEntity(ent);
    before.r.linked=qfalse;
    if(!R200BrushLease(&ref,time,current,lease)||memcmp(input,&ref,sizeof(ref))||
       memcmp(ent,&before,sizeof(before)))goto refuse;
    ent->r.contents=0;ent->clipmask=0;
    ent->s.eFlags|=EF_NONSOLID_BMODEL;
    trap_LinkEntity(ent);
    if(!R200BrushLease(&ref,time,current,lease)||memcmp(input,&ref,sizeof(ref))||
       !ent->r.linked||ent->r.contents||ent->clipmask||
       ent->s.solid!=SOLID_BMODEL||
       ent->classname!=before.classname||ent->s.modelindex!=before.s.modelindex||
       ent->r.svFlags!=before.r.svFlags||ent->s.eFlags!=(before.s.eFlags|EF_NONSOLID_BMODEL)||
       !VectorCompare(ent->r.currentOrigin,before.r.currentOrigin)||
       !VectorCompare(ent->r.currentAngles,before.r.currentAngles))goto refuse;
    /* SV_LinkEntity retains SOLID_BMODEL. CG_BuildSolidList honors the existing
     * EF_NONSOLID_BMODEL flag, including after any later legitimate relink. */
    return G_Cod1ScriptBrushNonsolidCommit(&ref);
refuse:
    if(error&&cap)snprintf(error,cap,"notsolid requires a current linked script brush; committed unlink/collision changes are retained");
    return 0;
}
#endif
