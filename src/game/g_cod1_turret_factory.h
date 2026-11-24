#ifndef G_COD1_TURRET_FACTORY_H
#define G_COD1_TURRET_FACTORY_H
#include "cod1_gsc/gsc_fixture.h"
int G_Cod1TurretFactoryMapCleanup(void);
int G_Cod1TurretFactoryBeforeFree(const GscEntityRef*);
int G_Cod1TurretFactoryThreadRead(const GscEntityRef*,GscEntityExecutionCertificate*);
int G_Cod1TurretFactoryPrivate(const void*,size_t);
#endif
