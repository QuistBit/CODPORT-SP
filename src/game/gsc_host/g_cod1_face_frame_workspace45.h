#ifndef COD1_FACE_FRAME_WORKSPACE45_H
#define COD1_FACE_FRAME_WORKSPACE45_H
/* Main GAME thread only. Checkout is independent of published operation
 * state: admission can be reentered before that state is set. Retain at
 * most one idle shell; every active shell has independent snapshots. */
typedef struct C1FaceFrameAllocation {
 C1FaceFrameCall call;
 cod1ActorRecord_t actor;
 gentity_t entity;
 struct C1FaceFrameAllocation *next;
} C1FaceFrameAllocation;
static C1FaceFrameAllocation *actorFaceIdle45,*actorFaceActive45;
static C1FaceFrameCall *ActorFaceFrameAllocate(void){
 C1FaceFrameAllocation *a=actorFaceIdle45;
 if(a)actorFaceIdle45=NULL;
 else a=malloc(sizeof(*a));
 if(!a)return NULL;
 memset(&a->call,0,sizeof(a->call));
 a->call.actor=&a->actor;
 a->call.entity=&a->entity;
 a->next=actorFaceActive45;actorFaceActive45=a;
 return &a->call;
}
static void ActorFaceFrameFree(C1FaceFrameCall *c){
 C1FaceFrameAllocation **link=&actorFaceActive45,*a;
 if(!c)return;
 while(*link&& &(*link)->call!=c)link=&(*link)->next;
 a=*link;if(!a)return;
 *link=a->next;a->next=NULL;
 /* Idle bytes carry no semantic state. Checkout zeros the call and the
  * original Acquire overwrites both snapshots before using them. */
 if(!actorFaceIdle45)actorFaceIdle45=a;
 else free(a);
}
static void ActorFaceFrameWorkspaceClear(void){
 C1FaceFrameAllocation *a=actorFaceIdle45;
 actorFaceIdle45=NULL;free(a);
 /* Active allocations survive reset until their original owners unwind. */
}
static int ActorFaceFrameWorkspacePrivate(const void*p,size_t n){
 C1FaceFrameAllocation *a;
 if(ActorOverlap(p,n,&actorFaceIdle45,sizeof(actorFaceIdle45))||
    ActorOverlap(p,n,&actorFaceActive45,sizeof(actorFaceActive45))||
    (actorFaceIdle45&&ActorOverlap(p,n,actorFaceIdle45,sizeof(*actorFaceIdle45))))return 1;
 for(a=actorFaceActive45;a;a=a->next)if(ActorOverlap(p,n,a,sizeof(*a)))return 1;
 return 0;
}
#endif
