#ifndef G_COD1_VEHICLE_H
#define G_COD1_VEHICLE_H
#include "cod1_gsc/gsc_fixture.h"
int G_Cod1VehicleMapCleanup(void);
int G_Cod1VehicleBeforeFree(const GscEntityRef*);
/* Ordinary G_FreeEntity phases: entry, unlinked, resources retired, memset. */
int G_Cod1VehicleFreePhase(const GscEntityRef*,int);
int G_Cod1VehicleThreadCurrent(const GscEntityRef*,uint64_t);
int G_Cod1VehicleThreadRead(const GscEntityRef*,GscEntityExecutionCertificate*);
#endif
