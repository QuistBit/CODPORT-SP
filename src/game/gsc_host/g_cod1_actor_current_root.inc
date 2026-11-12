#include "../g_cod1_actor_current_root.h"
/* Source parse order and native compiled indices are separate. Native4548C0
 * inserts named siblings at the head(45E21B), then455270 reserves immediate
 * children contiguously before recursing. This map repeats that actual order,
 * not global reverse-source numbering. No runtime node pointer is invented. */
typedef struct {
    GscEntityRef owner;cod1ActorLease_t lease;C1GARecord generic;
    cod1ActorRecord_t actor;gentity_t entity;C1TRecord gun;C1TARecord mg;
    c1atl14Node_t original[C1GA_COUNT],working[C1GA_COUNT];
    int first[C1GA_COUNT],next[C1GA_COUNT];unsigned order[C1GA_COUNT],frontier;
    unsigned mgMap[2][25],source[128],leafSource[82],count,leaves;
    c1arc14Node_t nodes[128];const c1ar14Root_t *roots[82];const c1ab14Clip_t *bodies[82];
    uint64_t epoch,receipt,lookRevision,faceRevision,scriptedRevision;int now,gunIndex;
} C1ActorCurrentRoot;
static int ActorCurrentRootOrderVisit(C1ActorCurrentRoot *v,unsigned source,unsigned depth) {
    unsigned first=v->frontier,n=0;int child;
    if(depth>64||source>=C1GA_COUNT||cod1GenericDefinitions[source].children>C1GA_COUNT-v->frontier)return 0;
    v->frontier+=cod1GenericDefinitions[source].children;
    for(child=v->first[source];child>=0;child=v->next[child]){
        if(n>=cod1GenericDefinitions[source].children)return 0;
        v->order[child]=first+n++;
        if(!ActorCurrentRootOrderVisit(v,(unsigned)child,depth+1))return 0;
    }
    return n==cod1GenericDefinitions[source].children;
}
static int ActorCurrentRootOrder(C1ActorCurrentRoot *v) {
    unsigned i,parent;
    for(i=0;i<C1GA_COUNT;i++){v->first[i]=v->next[i]=-1;v->order[i]=~0u;}
    if(cod1GenericDefinitions[0].parent)return 0;
    for(i=1;i<C1GA_COUNT;i++){
        parent=cod1GenericDefinitions[i].parent;
        /* Actual catalog parser publishes names in textual preorder. */
        if(parent>=i)return 0;
        v->next[i]=v->first[parent];v->first[parent]=(int)i;
    }
    v->order[0]=0;v->frontier=1;
    if(!ActorCurrentRootOrderVisit(v,0,0)||v->frontier!=C1GA_COUNT)return 0;
    for(i=0;i<C1GA_COUNT;i++)if(v->order[i]>=C1GA_COUNT)return 0;
    return 1;
}
static int ActorCurrentRootResource(const C1ActorCurrentRoot *v,unsigned ordinal,
    const c1ar14Root_t **root,const c1ab14Clip_t **body) {
    const char *name;int index;unsigned t,j;
    *root=NULL;*body=NULL;
    if(!ordinal||ordinal>=C1GA_COUNT||cod1GenericDefinitions[ordinal].children)return 0;
    name=cod1GenericNames[ordinal].name;index=C1AL14_Index(name);
    if(index>=0){*root=cod1GenericLookBank.root[index];*body=cod1GenericLookBank.body[index];}
    else if((index=ActorGenericShuffleIndex(name))>=0){*root=cod1GenericUnmountedBank.root[index];*body=cod1GenericUnmountedBank.body[index];}
    else if(ActorFaceLeafIndex(ordinal)>=0){*root=ActorFaceRoot(ordinal);*body=ActorFaceBody(ordinal);}
    else if(ActorScriptedIndex(ordinal)>=0){*root=ActorScriptedRoot(ordinal);*body=ActorScriptedBody(ordinal);}
    else for(t=0;t<2;t++)for(j=0;j<21;j++)if(v->mgMap[t][4+j]==ordinal){*root=cod1TurretRoots[t][20-j];*body=cod1TurretBodies[t][20-j];}
    return *root&&*body&&(*root)->magic==C1AR14_MAGIC&&(*body)->magic==C1AB14_MAGIC&&
        (*root)->frames==(*body)->frames&&(*root)->flags==(*body)->flags&&
        (*root)->fps==(*body)->fps&&(*root)->intervals==(*body)->intervals&&
        (*root)->intervals==(*root)->frames-1+((*root)->flags&1u)&&(*root)->fps&&(*root)->fps<=120;
}
static int ActorCurrentRootCollect(C1ActorCurrentRoot *v,unsigned source,int parent,unsigned depth) {
    const c1atl14Node_t *live;const c1ar14Root_t *root;const c1ab14Clip_t *body;
    c1arc14Node_t *n;unsigned here,leaf;int child;
    if(source>=C1GA_COUNT||depth>32||v->count>=128)return 0;
    here=v->count++;n=v->nodes+here;memset(n,0,sizeof(*n));live=v->working+source;
    v->source[here]=source;n->ordinal=v->order[source];n->parent=parent;n->leaf=-1;
    n->definition_children=cod1GenericDefinitions[source].children;
    n->phase=live->phase;n->goalWeight=live->goalWeight;n->currentWeight=live->currentWeight;
    /* Native selected composites do not consult their own slot. For a visited
     * non-root positive child, the admitted actual goal setter owns materialized
     * storage before the goal write. Never infer ABSENCE from a zero cell.
     * This projection does not prove full native producer/controller parity. */
    n->runtime_present=parent<0?-1:1;
    if(!n->definition_children){
        /* Actual scripted record111 selects the CLIP leaf, not its clear-root.
         * A current positive admitted non-root goal proves materialization;
         * zero goal does not prove absence in the portable full-sized array.
         * Refuse unknown presence instead of returning an identity sample. */
        if(parent<0){if(!(live->goalWeight>0))return 0;n->runtime_present=1;}
        if(v->leaves>=82||!ActorCurrentRootResource(v,source,&root,&body))return 0;
        leaf=v->leaves++;n->leaf=(int)leaf;v->roots[leaf]=root;v->bodies[leaf]=body;v->leafSource[leaf]=source;
        return 1;
    }
    for(child=v->first[source];child>=0;child=v->next[child]){
        /* Exhaustively inspect EVERY actual definition child. Omit only its
         * genuine zero goal; unknown positive resources are never an identity. */
        if(v->working[child].goalWeight>0&&
           !ActorCurrentRootCollect(v,(unsigned)child,(int)here,depth+1))return 0;
    }
    return 1;
}
static int ActorCurrentRootFence(const C1ActorCurrentRoot *v,const GscEntityRef *input) {
    cod1ActorLease_t lease;
    return cod1GenericOperation.busy&&!cod1GenericOperation.cancelled&&v->now==level.time&&
        v->epoch==cod1EntityEpoch&&v->epoch==cod1GenericEpoch&&v->receipt==cod1GenericReceipt&&
        v->receipt==cod1TurretAnimationReceipt&&v->lookRevision==cod1GenericLookRevision&&
        v->faceRevision==cod1FaceResources.revision&&v->scriptedRevision==cod1ScriptedResources.revision&&cod1EntityReady&&!level.spawning&&!level.restarted&&
        !memcmp(input,&v->owner,sizeof(v->owner))&&ActorGoalLease(&v->owner,&lease)&&
        !memcmp(&lease,&v->lease,sizeof(lease))&&
        !memcmp(&v->generic,cod1GenericActors+lease.actorIndex,sizeof(v->generic))&&
        !memcmp(v->original,v->generic.nodes,sizeof(v->original))&&
        !memcmp(&v->actor,cod1Actors+lease.actorIndex,sizeof(v->actor))&&
        !memcmp(&v->entity,g_entities+v->owner.slot,sizeof(v->entity))&&
        ActorGenericRootMGCurrent(&v->owner,v->gunIndex,&v->gun,&v->mg,v->now)&&
        cod1GenericLookEpoch==v->epoch&&C1AUT14_Ready(&cod1GenericUnmountedBank)&&TurretAnimationBanks();
}
int G_Cod1ActorRootCurrent(const GscEntityRef *input,int now,unsigned sourceOrdinal,
    c1arc14Result_t *output,int (*current)(void *),void *context) {
    C1ActorCurrentRoot *v=NULL;c1arc14Result_t result;cod1ActorLease_t lease;
    const c1ar14Root_t *root;const c1ab14Clip_t *body;unsigned i,t,j;int ok=0;
    if(!input||!output||!current||now!=level.time||!sourceOrdinal||sourceOrdinal>=C1GA_COUNT||
       cod1GenericOperation.busy||!ActorGoalBuffers(input,sizeof(*input),output,sizeof(*output),NULL,0)||
       ActorGenericPrivate(input,sizeof(*input))||ActorGenericPrivate(output,sizeof(*output))||
       TurretAnimationPrivate(input,sizeof(*input))||TurretAnimationPrivate(output,sizeof(*output))||
       TurretBankPrivate(input,sizeof(*input))||TurretBankPrivate(output,sizeof(*output))||
       !ActorGoalLease(input,&lease)||
       G_Cod1ActorGenericPresent(input)!=1)return 0;
    v=calloc(1,sizeof(*v));if(!v)return 0;
    v->owner=*input;v->lease=lease;v->now=now;v->epoch=cod1EntityEpoch;v->receipt=cod1GenericReceipt;
    v->lookRevision=cod1GenericLookRevision;v->faceRevision=cod1FaceResources.revision;v->scriptedRevision=cod1ScriptedResources.revision;v->gunIndex=-1;
    v->generic=cod1GenericActors[lease.actorIndex];
    if(v->generic.tick!=now||!v->generic.nodes||!v->generic.runtime||!v->generic.catalog||
       !ActorGenericMGMap(v->mgMap)||!TurretAnimationBanks()||
       cod1GenericLookEpoch!=v->epoch||!C1AUT14_Ready(&cod1GenericUnmountedBank))goto done;
    v->actor=cod1Actors[lease.actorIndex];v->entity=g_entities[v->owner.slot];
    memcpy(v->original,v->generic.nodes,sizeof(v->original));memcpy(v->working,v->original,sizeof(v->working));
    for(i=0;i<C1T_CAPACITY;i++)if(cod1Turrets[i].used&&ActorRefEqual(&cod1Turrets[i].actor,&v->owner)){
        if(v->gunIndex>=0)goto done;v->gunIndex=(int)i;v->gun=cod1Turrets[i];v->mg=cod1TurretAnimations[i];
    }
    if(!ActorGenericRootMGCurrent(&v->owner,v->gunIndex,&v->gun,&v->mg,now))goto done;
    if(v->actor.turretMount.mounted){
        if(v->gunIndex<0||!v->mg.used||v->mg.selected<0)goto done;
        for(t=0;t<2;t++)for(j=0;j<25;j++)v->working[v->mgMap[t][j]]=v->mg.actorTree[t].node[j];
    }
    if(!cod1GenericDefinitions[sourceOrdinal].children&&
       !(v->working[sourceOrdinal].goalWeight>0))goto done;
    cod1GenericOperation.busy=1;cod1GenericOperation.cancelled=0;
    cod1GenericOperation.stage=v;cod1GenericOperation.bytes=sizeof(*v);
    if(current(context)!=1||!ActorCurrentRootFence(v,input)||
       !C1AGT14_Valid(cod1GenericDefinitions,v->working,C1GA_COUNT)||
       !ActorCurrentRootOrder(v)||!ActorCurrentRootCollect(v,sourceOrdinal,-1,0)||
       !C1ARC14_Current(v->roots,v->leaves,v->nodes,v->count,0,&result))goto unlock;
    if(current(context)!=1||!ActorCurrentRootFence(v,input))goto unlock;
    for(i=0;i<v->leaves;i++)if(!ActorCurrentRootResource(v,v->leafSource[i],&root,&body)||
        root!=v->roots[i]||body!=v->bodies[i])goto unlock;
    *output=result;ok=1;
unlock:
    cod1GenericOperation.stage=NULL;cod1GenericOperation.bytes=0;
    cod1GenericOperation.notifyStage=NULL;cod1GenericOperation.notifyBytes=0;
    cod1GenericOperation.busy=0;
done:
    free(v);return ok;
}
