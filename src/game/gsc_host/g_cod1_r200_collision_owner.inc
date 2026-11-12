/* Pure generation/epoch/model keyed ownership of actually converted brushes.
 * No traps or calls to mutation helpers, including in retained host users. */
#include "../g_cod1_r200_collision.h"
static GscEntityRef r200NonsolidOwner[MAX_GENTITIES];
static int r200NonsolidModel[MAX_GENTITIES];
int G_Cod1ScriptBrushNonsolidOwned(const GscEntityRef *ref) {
    const gentity_t *ent;
    if(!ref||ref->slot>=MAX_GENTITIES||!G_Cod1EntitySameAllocation(ref)||
       memcmp(ref,&r200NonsolidOwner[ref->slot],sizeof(*ref)))return 0;
    ent=&g_entities[ref->slot];
    return R200BrushShape(ent)&&ent->s.modelindex==r200NonsolidModel[ref->slot]&&
        !ent->r.contents&&!ent->clipmask&&ent->s.solid==SOLID_BMODEL&&
        (ent->s.eFlags&EF_NONSOLID_BMODEL);
}
int G_Cod1ScriptBrushNonsolidCommit(const GscEntityRef *ref) {
    const gentity_t *ent;
    if(!ref||ref->slot>=MAX_GENTITIES||!G_Cod1EntitySameAllocation(ref))return 0;
    ent=&g_entities[ref->slot];
    if(!R200BrushShape(ent)||!ent->r.linked||ent->r.contents||ent->clipmask||
       ent->s.solid!=SOLID_BMODEL||!(ent->s.eFlags&EF_NONSOLID_BMODEL))return 0;
    r200NonsolidOwner[ref->slot]=*ref;r200NonsolidModel[ref->slot]=ent->s.modelindex;
    return 1;
}
