#ifndef COD1_FACE_FRAME_STORAGE35_H
#define COD1_FACE_FRAME_STORAGE35_H
/* Included after C1FaceFrameCall. One allocation, identical independent
 * actor/entity snapshots. Only the call header needs zero initialization. */
typedef struct {
 C1FaceFrameCall call;
 cod1ActorRecord_t actor;
 gentity_t entity;
} C1FaceFrameAllocation;
static C1FaceFrameCall *ActorFaceFrameAllocate(void){
 C1FaceFrameAllocation *a=malloc(sizeof(*a));
 if(!a)return NULL;
 memset(&a->call,0,sizeof(a->call));
 a->call.actor=&a->actor;
 a->call.entity=&a->entity;
 return &a->call;
}
static void ActorFaceFrameFree(C1FaceFrameCall *c){free((C1FaceFrameAllocation*)c);}
#endif
