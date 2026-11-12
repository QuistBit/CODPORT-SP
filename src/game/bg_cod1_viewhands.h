#ifndef BG_COD1_VIEWHANDS_H
#define BG_COD1_VIEWHANDS_H
#include <string.h>
/* Port wire IDs, NOT retail model ordinals. Selected original _loadout models.
 * Zero retains the accepted default. Unknown models are refused, not aliased. */
static const char *Cod1ViewHandsName(int id) {
    switch(id) {
    case 0:return "";
    case 1:return "viewmodel_hands_us";
    case 2:return "viewmodel_hands_british_air";
    case 3:return "viewmodel_hands_russian";
    default:return NULL;
    }
}
static int Cod1ViewHandsId(const char *model,int *out) {
    int i;if(!model||!out)return 0;
    /* Retail zero skips client rebuild; it is not an instruction to restore
     * the port's default hands. Empty native assignment remains unadmitted. */
    if(!*model)return 0;
    if(strncmp(model,"xmodel/",7))return 0;
    for(i=1;i<=3;i++)if(!strcmp(model+7,Cod1ViewHandsName(i))){*out=i;return 1;}
    return 0;
}
#endif
