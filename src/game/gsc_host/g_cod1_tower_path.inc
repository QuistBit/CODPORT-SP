/* The horizontal branches of original GAME 62040, 62520 and 62770.
 * Training's six mounted path points share a floor. Movement still uses
 * the actor's real collision capsule, floor trace, and goal notification. */
static float TowerAlong(const vec3_t a,const vec3_t b,const vec3_t direction){
 return (a[0]-b[0])*direction[0]+(a[1]-b[1])*direction[1];
}
static void TowerProject(const cod1LocalRoute_t *r,cod1TowerPath_t *p,const vec3_t origin){
 int i;float ahead,behind,used;
 if(TowerAlong(p->base,origin,p->aim)>=0)return;
 for(i=p->length-2;i>=0;i--){
  const float *begin=i==p->length-2?p->base:r->points[i+1];
  float length=i==p->length-2?p->remaining:p->edgeLength[i];
  ahead=TowerAlong(r->points[i],origin,p->aim);
  if(ahead<0)continue;
  behind=TowerAlong(begin,origin,p->aim);
  if(ahead<=behind)return;
  used=fmaxf(0,fminf(length,length*(-behind)/(ahead-behind)));
  VectorMA(begin,used,p->edge[i],p->base);
  p->remaining=length-used;p->length=i+2;
  if(p->remaining<=0&&i>0){VectorCopy(r->points[i],p->base);p->length=i+1;p->remaining=p->edgeLength[i-1];}
  return;
 }
}
static int TowerLook(const cod1LocalRoute_t *r,const cod1TowerPath_t *p,const vec3_t origin,
                     vec3_t target,float *back,float *area){
 int i;float sum=0;
 for(i=p->length-2;i>=0;i--){
  const float *point=r->points[i],*dir=p->edge[i];
  float length=i==p->length-2?p->remaining:p->edgeLength[i];
  float width=fabsf((point[0]-origin[0])*dir[1]-(point[1]-origin[1])*dir[0]);
  sum+=width*length;
  if(sum>p->area&&width>0){
   *back=fminf(length,(sum-p->area)/width);*area=sum;
   VectorMA(point,-*back,dir,target);return i;
  }
 }
 *back=0;*area=sum;VectorCopy(r->points[0],target);return 0;
}
/* Original 61B30 uses a raised 30x30 hull for lookahead rays. The
 * independent feet-and-body sweep is performed when applying displacement. */
static int TowerLookClear(const cod1ActorLease_t *l,uint64_t assignment,int now,
                          const vec3_t origin,const vec3_t target){
 cod1SpatialTraceQuery_t q={0};trace_t tr;vec3_t floor;int n;
 q.version=COD1_SPATIAL_VERSION;q.bytes=sizeof(q);q.contentsMask=0x00820011u;
 q.passEntity[0]=q.passEntity[1]=ENTITYNUM_NONE;
 VectorSet(q.mins,-15,-15,18);VectorSet(q.maxs,15,15,72);
 VectorCopy(origin,q.start);VectorCopy(target,q.end);q.end[2]=q.start[2];
 for(n=0;n<16;n++){
  vec3_t before;VectorCopy(q.start,before);
  if(trap_Cod1SpatialCapsuleTrace(&q,&tr)!=COD1_SPATIAL_OK||!MotionCurrent(l,assignment,now))return -1;
  if(tr.startsolid||tr.allsolid)return 0;
  if(tr.fraction<1&&hypotf(tr.endpos[0]-before[0],tr.endpos[1]-before[1])<15)return 0;
  VectorCopy(tr.endpos,floor);VectorCopy(floor,q.start);VectorCopy(q.start,q.end);q.end[2]-=72;
  if(trap_Cod1SpatialCapsuleTrace(&q,&tr)!=COD1_SPATIAL_OK||!MotionCurrent(l,assignment,now))return -1;
  if(tr.fraction<1&&tr.plane.normal[2]<.7f)return 0;
  VectorCopy(tr.endpos,q.start);q.start[2]+=18;
  if(fabsf(floor[0]-target[0])<.01f&&fabsf(floor[1]-target[1])<.01f)return fabsf(q.start[2]-target[2])<=72;
  VectorCopy(target,q.end);q.end[2]=q.start[2];
 }
 return 0;
}
static int TowerUpdateDirection(const cod1ActorLease_t *l,uint64_t assignment,int now,
        cod1LocalRoute_t *route,const vec3_t origin){
 cod1TowerPath_t *p=&route->tower,forward;vec3_t target,dir,next;float back,sum,len,x;
 int index,clear,end;
 TowerProject(route,p,origin);
 index=TowerLook(route,p,origin,target,&back,&sum);end=sum<=p->area;
 clear=TowerLookClear(l,assignment,now,origin,target);if(clear<0)return 0;
 if(clear){
  if(p->lastClear)p->area=fminf(65536.f,p->area*(1.f/.85f)+6.4f);
 }else{
  p->area=fmaxf(.001f,p->lastClear?(p->area-6.4f)*.75f:(p->area-12.8f)*.5625f);
  if(p->lastClear&&p->lastIndex<p->length){
   x=p->lastIndex==p->length-2?p->remaining:p->edgeLength[p->lastIndex];
   if(x>=p->back){p->lastClear=0;return 1;}
  }
 }
 p->lastClear=clear;p->lastIndex=index;p->back=back;
 if(end)p->area=fminf(p->area,fmaxf(32768.f,sum));
 VectorSubtract(target,origin,p->aim);p->aim[2]=0;
 len=VectorNormalize(p->aim);p->distance=len;
 VectorCopy(p->aim,p->face);
 if(!end&&len>.01f){
  VectorMA(origin,fminf(len,60.f),p->aim,next);
  if(TowerAlong(p->base,next,p->aim)<0){
   forward=*p;TowerProject(route,&forward,next);
   TowerLook(route,&forward,next,target,&back,&sum);
   VectorSubtract(target,next,dir);dir[2]=0;
   if(VectorNormalize(dir)>0)VectorCopy(dir,p->face);
  }
 }
 return 1;
}
int TrainingJumperPath(const GscEntityRef*,const GscNodeRef*,float route[6][3]);
static int MotionJumperRoute(const cod1ActorLease_t *l,const cod1ActorGoalSnapshot_t *goal,
                            int now,int mask,const vec3_t origin,vec3_t target){
 cod1LocalRoute_t *r=&cod1Actors[l->actorIndex].motion.route;cod1TowerPath_t *p=&r->tower;int i;
 (void)mask;
 if(!r->phase){
  if(!TrainingJumperPath(&l->entity,&goal->node,r->points))return -1;
  for(i=0;i<5;i++){
   VectorSubtract(r->points[i],r->points[i+1],p->edge[i]);
   if(fabsf(p->edge[i][2])>.05f)return -1;
   p->edge[i][2]=0;p->edgeLength[i]=VectorNormalize(p->edge[i]);if(!(p->edgeLength[i]>0))return -1;
  }
  p->length=6;p->remaining=p->edgeLength[4];p->lastIndex=5;p->area=32768;
  VectorCopy(r->points[5],p->base);VectorSubtract(p->base,origin,p->aim);p->aim[2]=0;VectorNormalize(p->aim);
  r->phase=2;r->cursor=0;
  if(!TowerUpdateDirection(l,goal->assignment,now,r,origin))return -1;
 }
 if(!TowerUpdateDirection(l,goal->assignment,now,r,origin))return -1;
 VectorMA(origin,p->distance,p->aim,target);target[2]=origin[2];
 if(g_debugMove.integer>1)G_Printf("[tower route] tick=%d n=%d look=%.3f axis=%.5f %.5f face=%.5f %.5f area=%.2f clear=%d\n",
    now,p->length,p->distance,p->aim[0],p->aim[1],p->face[0],p->face[1],p->area,p->lastClear);
 return 1;
}
