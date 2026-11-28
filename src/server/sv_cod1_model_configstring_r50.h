#ifndef SV_COD1_MODEL_CONFIGSTRING_R50_H
#define SV_COD1_MODEL_CONFIGSTRING_R50_H
#include <string.h>
#include "../qcommon/cod1_model_configstring_r50.h"
/* Exact first match in the occupied prefix, excluding reserved slot zero.
 * Server configstrings are owned NUL-terminated strings. With a name shorter
 * than MAX_STRING_CHARS, truncating a configstring cannot turn a nonmatch
 * into a match. No callbacks, resource loading, writes or retained pointers. */
static int C1R50_FindModelConfigstring(char *const *models,int count,const char *name){
    int i,k;
    if(!models||count<2||!name||!name[0])return 0;
    for(k=1;k<COD1_R50_MODEL_NAME_BYTES&&name[k];++k){}
    if(k==COD1_R50_MODEL_NAME_BYTES)return 0;
    for(i=1;i<count;++i){
        const char *value=models[i];
        if(!value||!value[0])return 0;
        if(!strcmp(value,name))return i;
    }
    return 0;
}
#endif
