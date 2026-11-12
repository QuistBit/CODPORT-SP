#ifndef G_COD1_ACTOR_WEAPON_FIELDS_CW_H
#define G_COD1_ACTOR_WEAPON_FIELDS_CW_H
/* Explicit opt-in resource-backed adapter. Native fresh default none is row0;
 * loaded CW definitions occupy1..N in this PORT registration order. This does
 * not assert retail map-specific weapon IDs, CI item IDs or WP inventory slots.
 * No files are loaded by binding or field access. */
#include "g_cod1_actor.h"
#include "bg_cod1_weapondef.h"
#include <string.h>
#if defined(COD1_GSC_TEST_WIP) && COD1_GSC_TEST_WIP
static int ActorCWWeaponFieldCurrent(uint64_t receipt) {
    unsigned int revision=CW_RegistryRevision();
    return revision&&receipt==(uint64_t)revision;
}
static int G_Cod1ActorSelectCWWeaponFields(char *error,size_t size) {
    cod1ActorWeaponFieldHost_t host;const cwDef_t *def;int i,n=CW_Count();
    unsigned int revision=CW_RegistryRevision();
    if(!revision||n<0||n>CW_MAX_DEFS)return 0;
    memset(&host,0,sizeof(host));host.table.rowCount=(uint32_t)n+1;
    host.receipt=revision;host.current=ActorCWWeaponFieldCurrent;
    host.table.rows[0].definitionPresent=host.table.rows[0].namePresent=1;
    strcpy(host.table.rows[0].name,"none");
    for(i=0;i<n;++i) {
        cod1ActorRegisteredWeaponRow_t *row=&host.table.rows[i+1];
        def=CW_Get(i);
        if(!def||!memchr(def->name,0,sizeof(def->name)))return 0;
        row->definitionPresent=row->namePresent=1;strcpy(row->name,def->name);
        row->resourceReceipt=def->hash;
    }
    if(CW_RegistryRevision()!=revision)return 0;
    return G_Cod1ActorSelectWeaponFieldTable(&host,error,size);
}
#endif
#endif
