/* Registration-owned original MG42 actor/gun clips. Root decoder validates
 * the entire source blob. No ordinal, pose, phase or completion is invented. */
#include "../../qcommon/cod1_actor_root_group14.h"
#include "../../qcommon/cod1_actor_bone14.h"
static c1ar14Root_t *cod1TurretRoots[2][21],*cod1TurretGunRoots[2];
static c1ab14Clip_t *cod1TurretBodies[2][21],*cod1TurretGunBodies[2];
static uint64_t cod1TurretBankEpoch,cod1TurretBankBusyEpoch;
/* Storage is immutable from TurretBankLoad's commit until reset.  The
 * inclusive extent can reject disjoint requests; overlap still requires the
 * original precise per-allocation check. Static bank fields are separate. */
typedef struct {uintptr_t lo,hi;} TurretClipExtent;
static TurretClipExtent cod1TurretExtent;
static int TurretClipRange(const void *p,size_t n,const void *owner,size_t bytes,TurretClipExtent *build){
 uintptr_t lo=(uintptr_t)owner,hi;
 if(!build)return ActorOverlap(p,n,owner,bytes);
 if(!owner||!bytes)return 0;
 hi=bytes-1>UINTPTR_MAX-lo?UINTPTR_MAX:lo+bytes-1;
 if(lo<build->lo)build->lo=lo;if(hi>build->hi)build->hi=hi;
 return 0;
}
static void TurretBankReset(void){int i,j;
 cod1TurretExtent.lo=UINTPTR_MAX;cod1TurretExtent.hi=0;
 for(i=0;i<2;i++){
  for(j=0;j<21;j++){C1AR14_Free(cod1TurretRoots[i][j]);cod1TurretRoots[i][j]=NULL;C1AB14_Free(cod1TurretBodies[i][j]);cod1TurretBodies[i][j]=NULL;}
  C1AR14_Free(cod1TurretGunRoots[i]);cod1TurretGunRoots[i]=NULL;C1AB14_Free(cod1TurretGunBodies[i]);cod1TurretGunBodies[i]=NULL;
 }
 cod1TurretBankEpoch=cod1TurretBankBusyEpoch=0;
}
static int TurretBodyRanges(const void *p,size_t n,const c1ab14Clip_t *c,TurretClipExtent *build){unsigned k;
 if(c && c->packedBytes)return TurretClipRange(p,n,c,c->packedBytes,build);
 if(!c)return 0;if(TurretClipRange(p,n,c,sizeof(*c),build))return 1;
 for(k=0;k<c->bones;k++){const c1ab14Bone_t *b=&c->bone[k];
  if(TurretClipRange(p,n,b->qi,b->nq*sizeof(*b->qi),build)||TurretClipRange(p,n,b->q,b->nq*sizeof(*b->q),build)||
     TurretClipRange(p,n,b->ti,b->nt*sizeof(*b->ti),build)||TurretClipRange(p,n,b->t,b->nt*sizeof(*b->t),build))return 1;
 }return 0;
}
static int TurretBodyPrivate(const void *p,size_t n,const c1ab14Clip_t *c){return TurretBodyRanges(p,n,c,NULL);}
static int TurretBankData(const void*p,size_t n,TurretClipExtent *build){int i,j;
 for(i=0;i<2;i++){
  for(j=0;j<21;j++)if(TurretBodyRanges(p,n,cod1TurretBodies[i][j],build)||TurretClipRange(p,n,cod1TurretRoots[i][j],sizeof(c1ar14Root_t),build))return 1;
  if(TurretBodyRanges(p,n,cod1TurretGunBodies[i],build)||TurretClipRange(p,n,cod1TurretGunRoots[i],sizeof(c1ar14Root_t),build))return 1;
 }
 return 0;
}
static int TurretBankPrivate(const void *p,size_t n){uintptr_t start=(uintptr_t)p;
 if(ActorOverlap(p,n,&cod1TurretExtent,sizeof(cod1TurretExtent))||ActorOverlap(p,n,cod1TurretBodies,sizeof(cod1TurretBodies))||ActorOverlap(p,n,cod1TurretGunBodies,sizeof(cod1TurretGunBodies))||ActorOverlap(p,n,cod1TurretRoots,sizeof(cod1TurretRoots))||
    ActorOverlap(p,n,cod1TurretGunRoots,sizeof(cod1TurretGunRoots))||
    ActorOverlap(p,n,&cod1TurretBankEpoch,sizeof(cod1TurretBankEpoch))||
    ActorOverlap(p,n,&cod1TurretBankBusyEpoch,sizeof(cod1TurretBankBusyEpoch)))return 1;
 if(cod1TurretBankEpoch && cod1TurretExtent.lo<=cod1TurretExtent.hi && p && n &&
    (start>cod1TurretExtent.hi || (start<cod1TurretExtent.lo && n<=cod1TurretExtent.lo-start)))return 0;
 return TurretBankData(p,n,NULL);
}
/* These tracks become immutable before publication. Keeping the track bytes with
 * their owner makes a single range check exact for the entire lifetime. */
static void *TurretCopyTrack(unsigned char **next,const void *src,size_t bytes){
    unsigned char *dst=*next;
    if(!bytes)return NULL;
    memcpy(dst,src,bytes);
    *next+=bytes;
    return dst;
}
static void TurretPackDecodedBody(c1ab14Clip_t **body){
    c1ab14Clip_t *src=*body,*packed;
    size_t bytes=sizeof(*src);
    unsigned i;
    unsigned char *next;
    if(!src || src->packedBytes)return;
    for(i=0;i<src->bones;i++){
        const c1ab14Bone_t *b=&src->bone[i];
        /* nq and nt were decoded from uint16_t. Every bone starts on a
         * four-byte boundary for its translation and quaternion samples. */
        size_t stride=b->nq*(sizeof(*b->q)+sizeof(*b->qi))+b->nt*(sizeof(*b->t)+sizeof(*b->ti));
        stride=(stride+3u)&~(size_t)3u;
        if(stride>SIZE_MAX-bytes)return;
        bytes+=stride;
    }
    packed=malloc(bytes);
    if(!packed)return; /* The decoder's allocations are already usable. */
    *packed=*src;
    next=(unsigned char *)(packed+1);
    for(i=0;i<src->bones;i++){
        const c1ab14Bone_t *b=&src->bone[i];
        c1ab14Bone_t *dst=&packed->bone[i];
        dst->t=TurretCopyTrack(&next,b->t,b->nt*sizeof(*b->t));
        dst->q=TurretCopyTrack(&next,b->q,b->nq*sizeof(*b->q));
        dst->ti=TurretCopyTrack(&next,b->ti,b->nt*sizeof(*b->ti));
        dst->qi=TurretCopyTrack(&next,b->qi,b->nq*sizeof(*b->qi));
        if(((uintptr_t)next&3u)!=0)next+=2;
    }
    packed->packedBytes=bytes;
    *body=packed;
    C1AB14_Free(src);
}
static int TurretBankRead(const char *name,c1ar14Root_t **out,c1ab14Clip_t **body,uint64_t epoch){
 fileHandle_t file=0;int bytes,ok=0;unsigned char *data;
 bytes=trap_FS_FOpenFile(name,&file,FS_READ);
 if(bytes<9||bytes>(int)C1AR14_CLIP_LIMIT||!file||epoch!=cod1EntityEpoch||!level.spawning){if(file)trap_FS_FCloseFile(file);return 0;}
 data=malloc(bytes);if(!data){trap_FS_FCloseFile(file);return 0;}
 trap_FS_Read(data,bytes,file);trap_FS_FCloseFile(file);
 if(epoch==cod1EntityEpoch&&level.spawning&&cod1EntityActive)ok=C1AR14_Decode(data,bytes,out)&&C1AB14_Decode(data,bytes,body);
 if(ok)TurretPackDecodedBody(body);
 free(data);return ok;
}
static int TurretBankLoad(void){
 static const char *yaw[7]={"45left","30left","15left","forward","15right","30right","45right"};
 static const char *height[3]={"15down","level","15up"};
 static const char *role[2]={"aim","fire"};
 c1ar14Root_t *stage[2][21]={{0}},*gun[2]={0};c1ab14Clip_t *body[2][21]={{0}},*gunBody[2]={0};char path[128];uint64_t epoch=cod1EntityEpoch;int i,j,k,ok=0;
 if(epoch&&cod1TurretBankEpoch==epoch)return 1;
 if(!epoch||!level.spawning||!cod1EntityActive||cod1TurretBankBusyEpoch)return 0;
 cod1TurretBankBusyEpoch=epoch;
 for(i=0;i<2;i++){
  for(j=0;j<3;j++)for(k=0;k<7;k++){
   snprintf(path,sizeof(path),"xanim/standMG42gunner_%s_%s_%s",role[i],yaw[k],height[j]);
   if(!TurretBankRead(path,&stage[i][j*7+k],&body[i][j*7+k],epoch)||stage[i][j*7+k]->bones!=55||
      stage[i][j*7+k]->fps!=30||stage[i][j*7+k]->flags!=(unsigned)(i?3:2))goto done;
  }
  snprintf(path,sizeof(path),"xanim/standMG42gun_%s_foward",role[i]);
  if(!TurretBankRead(path,&gun[i],&gunBody[i],epoch)||gun[i]->bones!=9||gun[i]->fps!=30||gun[i]->flags!=(unsigned)(i?3:2))goto done;
 }
 if(epoch!=cod1EntityEpoch||!level.spawning||!cod1EntityActive||cod1TurretBankEpoch)goto done;
 memcpy(cod1TurretRoots,stage,sizeof(stage));memcpy(cod1TurretGunRoots,gun,sizeof(gun));
 memcpy(cod1TurretBodies,body,sizeof(body));memcpy(cod1TurretGunBodies,gunBody,sizeof(gunBody));
 memset(body,0,sizeof(body));memset(gunBody,0,sizeof(gunBody));
 memset(stage,0,sizeof(stage));memset(gun,0,sizeof(gun));
 cod1TurretExtent.lo=UINTPTR_MAX;cod1TurretExtent.hi=0;
 TurretBankData(NULL,0,&cod1TurretExtent);cod1TurretBankEpoch=epoch;ok=1;
done:
 for(i=0;i<2;i++){for(j=0;j<21;j++){C1AR14_Free(stage[i][j]);C1AB14_Free(body[i][j]);}C1AR14_Free(gun[i]);C1AB14_Free(gunBody[i]);}
 if(cod1TurretBankBusyEpoch==epoch)cod1TurretBankBusyEpoch=0;
 return ok;
}
