#ifndef G_COD1_NAVIGATION_PROBE_H
#define G_COD1_NAVIGATION_PROBE_H
/* Diagnostic read-only native nearest-node selection. No movement capability. */
typedef struct {int start,goal;unsigned candidates[2],traces[2];float origin[3],target[3],startNode[3],goalNode[3];uint64_t assignment;int routeAvailable,routeResult,team;unsigned routeTraces;unsigned char path[984];int category,rootAvailable,deltaAvailable,physicsAvailable,physicsResult;unsigned physicsTraces;float root[3],delta[3],physicsEnd[3],physicsVelocity[3];} cod1NavigationProbe_t;
int G_Cod1NavigationProbe(GscSession*,uint64_t,const GscEntityRef*,int,cod1NavigationProbe_t*);
void G_Cod1NavigationDump(int,uint64_t,const unsigned char*);
#endif
