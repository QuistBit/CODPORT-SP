#ifndef G_COD1_WEAPON_MODEL_H
#define G_COD1_WEAPON_MODEL_H
#include "cod1_gsc/gsc_fixture.h"
#define COD1_WEAPON_MODELS_MAX 256
typedef struct { int namePresent,modelPresent; char name[128],model[128]; unsigned int resourceReceipt; } cod1WeaponModelRow_t;
typedef struct { unsigned int rowCount;cod1WeaponModelRow_t rows[COD1_WEAPON_MODELS_MAX];uint64_t receipt;int (*current)(uint64_t); } cod1WeaponModelHost_t;
/* Complete nonNULL definitions in scan order; absent name is skipped. A matched
 * absent model is held, whereas a present empty model is an owned empty result.
 * Index0 is the explicit semantic none row. No native index/resource parity. */
int G_Cod1SelectWeaponModelTable(const cod1WeaponModelHost_t *,char *,size_t);
int G_Cod1QueryWeaponModel(const char *,char [128],char *,size_t);
const GscWeaponModelHost *G_Cod1WeaponModelHost(void);
#endif
