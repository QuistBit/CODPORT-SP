#ifndef G_COD1_WEAPON_MODEL_CW_H
#define G_COD1_WEAPON_MODEL_CW_H
#include "g_cod1_weapon_model.h"
#include "bg_cod1_weapondef.h"
/* Explicit port registration order, not original numeric IDs. The adapter
 * copies exact worldModel bytes and never registers or prefixes models. */
static cod1WeaponModelHost_t cod1CWWeaponModelStage;
static int Cod1CWWeaponModelCurrent(uint64_t receipt){unsigned revision=CW_RegistryRevision();return revision&&receipt==(uint64_t)revision;}
static int G_Cod1SelectCWWeaponModels(char *error,size_t size){
    int i,n=CW_Count(),field=CW_FieldIndex("worldModel");unsigned revision=CW_RegistryRevision();cod1WeaponModelHost_t *host=&cod1CWWeaponModelStage;
    if(!revision||n<0||n>CW_MAX_DEFS||field<0)return 0;
    memset(host,0,sizeof(*host));host->receipt=revision;host->current=Cod1CWWeaponModelCurrent;host->rowCount=1;
    host->rows[0].namePresent=host->rows[0].modelPresent=1;strcpy(host->rows[0].name,"none");
    for(i=0;i<n;++i){const cwDef_t *def=CW_Get(i);cod1WeaponModelRow_t *row;const char *model;
        if(!def||!memchr(def->name,0,sizeof(def->name)))return 0;
        /* A native semantic none row must not become an ordinary CW weapon. */
        if((def->name[0]=='n'||def->name[0]=='N')&&(def->name[1]=='o'||def->name[1]=='O')&&(def->name[2]=='n'||def->name[2]=='N')&&(def->name[3]=='e'||def->name[3]=='E')&&!def->name[4])continue;
        row=&host->rows[host->rowCount++];row->namePresent=1;strcpy(row->name,def->name);row->resourceReceipt=def->hash;model=def->value[field];
        /* Normal native loader 20009ac0 initializes type0 strings to the
         * owned nonNULL empty seed; 2004b540 skips absent info keys. Actual
         * CW parsed definitions use that proved semantic empty default. */
        if(!model)model="";
        if(model){size_t len=strlen(model);if(len>=sizeof(row->model))return 0;row->modelPresent=1;memcpy(row->model,model,len+1);}
    }
    if(revision!=CW_RegistryRevision())return 0;
    return G_Cod1SelectWeaponModelTable(host,error,size);
}
#endif
