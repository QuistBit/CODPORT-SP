/* Training consumes only these authored IDs. Physical sorting moves complete
 * records and preserves the authored ID and placed goal fields together. */
static const char *const trainingGoalNames[]={"foley_node1","foley_node2","foley_node3","auto509","auto482","auto483","auto486","auto488","auto490","prone1","auto492","auto493","auto494","stand1","auto499","auto457","auto458","auto459","auto462","auto463","auto478","prone2","auto466","auto467","auto468","stand2","auto473","moody_node1","moody_node2","moody_node3"};
#define TRAINING_GOAL_COUNT (sizeof(trainingGoalNames)/sizeof(trainingGoalNames[0]))
static struct {
    uint64_t epoch;
    cod1NavigationResource_t resource;
    int ready, busy, cancelled;
    float ordinary[GSC_NODE_QUERY_CAPACITY][3];
    int ordinaryCount,worldReady;int hasJumpMount;unsigned jumpIndex;float jumpMount[3];
    cod1PlacedGoal_t placedByIndex[GSC_NODE_QUERY_CAPACITY];
    unsigned index[TRAINING_GOAL_COUNT];
    cod1PlacedGoal_t goal[TRAINING_GOAL_COUNT];
} trainingGoals;

static int TrainingGoalIndex(const Cod1NodeRecord *node) {

    const char *name=Cod1NodeRaw(node,"targetname");
    unsigned i;
    if (name) for (i=0;i<TRAINING_GOAL_COUNT;i++) if (!strcmp(name,trainingGoalNames[i])) return (int)i;
    return -1;
}

static int TrainingGoalTrace(void *context,const float *start,const float *mins,const float *maxs,
                             const float *end,unsigned pass,unsigned mask,C1DPPlaceTrace *out) {
    const uint64_t world=*(const uint64_t *)context;
    return !trainingGoals.cancelled && G_Cod1PathNodeWorldCurrent(world) &&
        Cod1NodePlaceTrace(start,mins,maxs,end,pass,mask,out) &&
        G_Cod1PathNodeWorldCurrent(world) && !trainingGoals.cancelled;
}

static int TrainingPlaceGoal(const Cod1NodeRecord *r,unsigned i,uint64_t *world,cod1PlacedGoal_t *goal) {
    C1NPInput input={0};
    unsigned char prepared[116];
    C1DPProjection node={0};
    unsigned reason;
    float yaw;
    input.type=r->type;input.index=i;input.flags=r->spawnflags;
    memcpy(input.origin,r->origin,sizeof(input.origin));
    input.hasYaw=Cod1NodeRaw(r,"angles")!=NULL;input.yaw=r->angles[1];
    if (!C1NP_Prepare(prepared,&input)) return 0;
    memcpy(&node.type,prepared+0x24,4);memcpy(node.origin,prepared+0x38,12);
    if (!C1DP_PlaceCandidate(&node,TrainingGoalTrace,world,&reason) || reason!=C1DP_PLACE_VALID) return 0;
    goal->yaw=-1.0f;
    if (prepared[0x2b]&0x80) {
        memcpy(&yaw,prepared+0x44,4);
        if (!C1NP_Yaw(yaw,&goal->yaw)) return 0;
    }
    memcpy(goal->position,node.origin,sizeof(node.origin));
    return 1;
}

void G_Cod1PrepareTrainingGoals(void) {
    uint64_t world=0;
    unsigned i,seen=0,invalid=0;
    if (trainingGoals.busy) {trainingGoals.cancelled=1;return;}
    if (!cod1NodeConsume || !Cod1NodeAvailable() || trainingGoals.epoch==cod1NodeEpoch ||
        trap_Cvar_VariableIntegerValue("g_cod1GscTrainingOpening")!=1) return;
    memset(&trainingGoals,0,sizeof(trainingGoals));
    trainingGoals.busy=1;
    trainingGoals.epoch=cod1NodeEpoch;
    trainingGoals.resource=cod1NodeResource;
    if (!trainingGoals.resource.incarnation || !Cod1GraphResourceCurrent(&trainingGoals.resource) ||
        !G_Cod1PathNodeWorldBegin(&world)) goto done;
    for (i=0;i<(unsigned)cod1NodeCount;i++) {
        const Cod1NodeRecord *r=&cod1Nodes[i];
        const int slot=TrainingGoalIndex(r);
        unsigned bit;
        cod1PlacedGoal_t placed={0};
        int valid=r->type==1 && TrainingPlaceGoal(r,i,&world,&placed);
        if(r->type==15 && Cod1NodeRaw(r,"targetname") && !strcmp(Cod1NodeRaw(r,"targetname"),"jumpnode") &&
           TrainingPlaceGoal(r,i,&world,&placed)){
           trainingGoals.jumpIndex=i; trainingGoals.hasJumpMount=1;
           VectorCopy(placed.position,trainingGoals.jumpMount);
        }
        if (valid) VectorCopy(placed.position,trainingGoals.ordinary[trainingGoals.ordinaryCount]);
        if (valid) {
            ++trainingGoals.ordinaryCount;
            placed.status=1;
            trainingGoals.placedByIndex[i]=placed;
        }
        if (slot<0) continue;
        bit=1u<<slot;
        trainingGoals.goal[slot]=placed;
        if ((seen&bit) || !valid)
            invalid|=bit;
        trainingGoals.index[slot]=i;seen|=1u<<slot;
    }
    trainingGoals.worldReady=(Cod1GraphResourceCurrent(&trainingGoals.resource) &&
        G_Cod1PathNodeWorldCurrent(world) && trainingGoals.epoch==cod1NodeEpoch && !trainingGoals.cancelled);
    trainingGoals.ready=trainingGoals.worldReady ? (seen & ~invalid) : 0;
    /* All three instructor nodes remain an inseparable set. */
    if ((trainingGoals.ready & 7)!=7) trainingGoals.ready &= ~7u;
done:
    if (world) G_Cod1PathNodeWorldEnd(world);
    trainingGoals.busy=0;
}

/* Local visibility search uses only ordinary placed map nodes. It does not
 * publish physical v7 indices or grant a native graph handle. */
int G_Cod1LocalWaypoints(const GscEntityRef *owner,uint64_t assignment,const GscNodeRef *goal,const float start[3],const float end[3],
                         float points[][3],int capacity) {
    int i,j,n=0;
    cod1PlacedGoal_t placed=G_Cod1TrainingPlacedGoal(owner,assignment,goal);
    if (placed.status!=1) return -1;
    for (i=0;i<trainingGoals.ordinaryCount;i++) {
        const float *p=trainingGoals.ordinary[i];
        for (j=0;j<3;j++) {
            float pad=j==2?18.0f:96.0f;
            if (p[j]<fminf(start[j],end[j])-pad || p[j]>fmaxf(start[j],end[j])+pad) break;
        }
        if (j!=3 || DistanceSquared(start,p)<1 || DistanceSquared(end,p)<1) continue;
        if (n==capacity) return -1;
        VectorCopy(p,points[n]); n++;
    }
    return n;
}

int TrainingJumperOwnerIs(const GscEntityRef*);
int TrainingJumperExitNode(const GscEntityRef *owner,uint64_t assignment,const GscNodeRef *ref){
 const Cod1NodeRecord *node;const char *name;
 if(assignment!=1||!Cod1NodeAvailable()||!Cod1NodeRefCurrent(ref)||!TrainingJumperOwnerIs(owner))return 0;
 node=cod1Nodes+ref->index;name=Cod1NodeRaw(node,"targetname");
 return name&&!strcmp(name,"disappear_node")&&node->type==1;
}
cod1PlacedGoal_t G_Cod1TrainingPlacedGoal(const GscEntityRef *owner,uint64_t assignment,const GscNodeRef *ref) {
    cod1PlacedGoal_t goal={0};
    int i;
    goal.status=-1;
    if (!Cod1NodeAvailable() || !Cod1NodeRefCurrent(ref)) return goal;
    if (G_Cod1RunnerCourseGoal(owner,assignment,ref)==1||TrainingJumperExitNode(owner,assignment,ref)){
        if(trainingGoals.epoch!=cod1NodeEpoch || trainingGoals.busy || !trainingGoals.worldReady ||
           !cod1NodeResource.incarnation || cod1NodeResource.incarnation<=cod1NodeRetiredResource ||
           memcmp(&trainingGoals.resource,&cod1NodeResource,sizeof(cod1NodeResource)) ||
           !trainingGoals.placedByIndex[ref->index].status)return goal;
        return trainingGoals.placedByIndex[ref->index];
    }
    if (trainingGoals.epoch!=cod1NodeEpoch || (i=TrainingGoalIndex(&cod1Nodes[ref->index]))<0) {
        goal.status=0;return goal;
    }
    if (trainingGoals.busy || !trainingGoals.ready || trainingGoals.index[i]!=ref->index ||
        !cod1NodeResource.incarnation || cod1NodeResource.incarnation<=cod1NodeRetiredResource ||
        memcmp(&trainingGoals.resource,&cod1NodeResource,sizeof(cod1NodeResource))) return goal;
    if (!(trainingGoals.ready & (1u<<i))) return goal;
    goal=trainingGoals.goal[i];goal.status=1;return goal;
}

static int TrainingRigNodePosition(const Cod1NodeRecord *record,float out[3]){
 if(!trainingGoals.hasJumpMount||trainingGoals.busy||trainingGoals.cancelled||!trainingGoals.worldReady||
    trainingGoals.epoch!=cod1NodeEpoch||trainingGoals.jumpIndex>=cod1NodeCount||
    cod1Nodes+trainingGoals.jumpIndex!=record||!Cod1GraphResourceCurrent(&trainingGoals.resource))return 0;
 VectorCopy(trainingGoals.jumpMount,out);return 1;
}

/* Each pair is the physical path point read from the original actor and its
 * authored Training node. Edge selection and lengths come from the mounted
 * version-7 lump. The floor-projected map supplies the actual positions. */
static int TrainingJumperBuildPath(float route[6][3]){
 static const struct {int physical,authored;float x,y;} tower[]={
  {125,186,5654,-3878},{122,192,5678,-3702},{131,193,5702,-3510},
  {80,194,5750,-3302},{48,195,5858,-3202},{154,198,5818,-2966}
 };
 cod1NavigationResource_t actual,res=trainingGoals.resource;
 cod1NavigationRequest_t q={0};C1NAV14_V7Layout layout;
 unsigned char *bytes=NULL,*visibility=NULL,visited[GSC_NODE_QUERY_CAPACITY]={0};
 C1NAV14_V7NodeData rows[GSC_NODE_QUERY_CAPACITY];C1NAV14_V7LinkData *links=NULL;
 float distance[GSC_NODE_QUERY_CAPACITY];int parent[GSC_NODE_QUERY_CAPACITY],i,n,count=0,ok=0;
 if(cod1NodeCount!=199||!trainingGoals.worldReady||!res.length||res.length>COD1_NAV_RESOURCE_LIMIT||
    !Cod1GraphResourceCurrent(&res))return 0;
 bytes=malloc(res.length);if(!bytes)return 0;
 q.version=COD1_NAV_RESOURCE_VERSION;q.bytes=sizeof(q);q.operation=COD1_NAV_COPY;
 q.incarnation=res.incarnation;q.length=res.length;
 if(!trap_Cod1NavigationResource(&q,&actual,bytes,res.length)||memcmp(&actual,&res,sizeof(res)))goto done;
 layout=C1NAV14_V7Inspect(bytes,res.length,cod1NodeCount);if(!layout.valid)goto done;
 links=calloc(layout.links,sizeof(*links));visibility=malloc(layout.visibilityBytes);
 if(!links||!visibility||!C1NAV14_V7Decode(bytes,res.length,cod1NodeCount,rows,GSC_NODE_QUERY_CAPACITY,
      links,layout.links,visibility,layout.visibilityBytes))goto done;
 for(i=0;i<cod1NodeCount;i++){distance[i]=FLT_MAX;parent[i]=-1;}distance[tower[0].physical]=0;
 for(;;){
  unsigned k;n=-1;
  for(i=0;i<cod1NodeCount;i++)if(!visited[i]&&distance[i]<FLT_MAX&&(n<0||distance[i]<distance[n]))n=i;
  if(n<0)goto done;
  if(n==tower[5].physical)break;
  visited[n]=1;
  for(k=0;k<rows[n].count;k++){
   const C1NAV14_V7LinkData *link=links+rows[n].first+k;float cost,alt;
   memcpy(&cost,&link->distanceBits,4);if(!isfinite(cost)||cost<0)goto done;
   alt=distance[n]+cost;
   if(alt<distance[link->target]){parent[link->target]=n;distance[link->target]=alt;}
  }
 }
 for(;n>=0;n=parent[n]){
  const cod1PlacedGoal_t *placed;
  for(i=0;i<6&&tower[i].physical!=n;i++);
  if(i==6||count==6)goto done;
  placed=trainingGoals.placedByIndex+tower[i].authored;
  if(placed->status!=1||placed->position[0]!=tower[i].x||placed->position[1]!=tower[i].y)goto done;
  VectorCopy(placed->position,route[count]);++count;
 }
 if(count==6&&Cod1GraphResourceCurrent(&res)){
  if(g_debugMove.integer)G_Printf("[Training jump path] v7 nodes=%d length=%.3f points=%d\n",cod1NodeCount,distance[tower[5].physical],count);
  ok=1;
 }
 done:free(bytes);free(links);free(visibility);return ok;
}
int TrainingJumperPath(const GscEntityRef *owner,const GscNodeRef *node,float route[6][3]){
 if(!TrainingJumperExitNode(owner,1,node))return 0;
 return TrainingJumperBuildPath(route);
}
