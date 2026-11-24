#ifndef G_COD1_VEHICLE_NODES_H
#define G_COD1_VEHICLE_NODES_H
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
#include "cod1_gsc/gsc_fixture.h"
#include "bg_cod1_vehicle14.h"
typedef struct {uint64_t epoch,owner;unsigned count;} cod1VehiclePathLease_t;
int G_Cod1VehiclePathCopy(const GscVehicleNodeRef*,C1V14PathNode*,unsigned,cod1VehiclePathLease_t*);
int G_Cod1VehiclePathCurrent(const cod1VehiclePathLease_t*,const GscVehicleNodeRef*);
void G_Cod1VehicleNodesClear(void);
const GscVehicleNodeHost*G_Cod1VehicleNodeHost(void);
#endif
#endif
