#ifndef COD1_SOURCE_WALK_WIRE_H
#define COD1_SOURCE_WALK_WIRE_H
/* Additive metadata in existing transmitted effect fields. The source-pose
 * packet, actor identity, lifetime and schema remain unchanged. */
#define C1_SOURCE_WALK_TAG 0x574b01
static void C1SourceWalkClear(entityState_t*s){
 if(s->effect1Time==C1_SOURCE_WALK_TAG){s->effect1Time=0;s->effect2Time=0;}
}
/* 0 absent, 1 current gait, -1 malformed extension. Caller validates the
 * enclosing source-pose snapshot, including epoch/generation and tick. */
static int C1SourceWalkRead(const entityState_t*s,float*out){
 float p;if(!s||!out)return -1;if(s->effect1Time!=C1_SOURCE_WALK_TAG)return 0;
 if(s->cod1TurretActorVersion!=COD1_SOURCE_POSE_WIRE_VERSION)return -1;
 memcpy(&p,&s->effect2Time,sizeof(p));if(!(p>=0&&p<1))return -1;*out=p;return 1;
}
static int C1SourceWalkWrite(entityState_t*s,float phase){
 if(!s||s->cod1TurretActorVersion!=COD1_SOURCE_POSE_WIRE_VERSION||!(phase>=0&&phase<1))return 0;
 s->effect1Time=C1_SOURCE_WALK_TAG;memcpy(&s->effect2Time,&phase,sizeof(phase));return 1;
}
#endif
