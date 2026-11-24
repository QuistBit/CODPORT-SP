#ifndef G_COD1_NAVIGATION_WORLD_H
#define G_COD1_NAVIGATION_WORLD_H
/* Synchronous normal query world. Independent from construction no1000 gate. */
int G_Cod1NavigationWorldBegin(unsigned mask,uint64_t*);
int G_Cod1NavigationWorldCurrent(uint64_t);
void G_Cod1NavigationWorldEnd(uint64_t);
int G_Cod1RuntimeScriptMask(const GscEntityRef*,unsigned*);
#endif
