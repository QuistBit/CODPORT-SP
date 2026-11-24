#ifndef G_COD1_TURRET_H
#define G_COD1_TURRET_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "../qcommon/cod1_actor_root_interval14.h"
int G_Cod1TurretSpawn(gentity_t *);
/* Current map-owned WeaponDef script dependency; no file reads or fallback. */
int G_Cod1TurretScriptModule(char *,size_t);
int G_Cod1TurretReadFlash(const GscEntityRef *,int,float[3],float[3]);
int G_Cod1TurretFrame(int,int (*)(void *),int (*)(void *,const GscEntityRef *,const char *),void *);
void G_Cod1TurretPoseTrace(const GscEntityRef *,int);
int G_Cod1TurretMountedFrame(const GscEntityRef *,int,int(*)(void*),void*);
const GscTurretHost *G_Cod1TurretControlHost(void);
const GscTurretAnimationHost *G_Cod1TurretAnimationHost(void);
int G_Cod1TurretAnimationCatalogs(const void *[2],size_t[2],GscMapExecutionCertificate *);
int G_Cod1ActorGenericPresent(const GscEntityRef *);
int G_Cod1ActorGenericSample(const GscEntityRef *,int,float[98],int (*)(void *),void *);
int G_Cod1ActorGenericFrame(const GscEntityRef *,int,int (*)(void *),void *);
/* goalByte belongs to the selected original actor script lifecycle. */
int G_Cod1ActorGeneralRootInterval(const GscEntityRef*,int,int,c1ari14Result_t*,int(*)(void*),void*);
int G_Cod1ActorRootInterval(const GscEntityRef *,int,int,c1ari14Result_t *,int (*)(void *),void *);
/* Original unmounted shuffle/Look closure; zero transport tails, no gun. */
int G_Cod1ActorUnmountedSample(const GscEntityRef *,int,float[98],float[2][21],int (*)(void *),void *);
int G_Cod1ActorUnmountedPublish(const GscEntityRef *,int,int (*)(void *),void *);
const GscActorAnimationHost *G_Cod1ActorGenericAnimationHost(void);
#endif
#endif
