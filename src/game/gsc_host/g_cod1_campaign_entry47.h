#ifndef COD1_CAMPAIGN_ENTRY47_H
#define COD1_CAMPAIGN_ENTRY47_H
#include <string.h>
/* Map identity and root identity are different inputs. This table describes
 * known entry paths, not a certificate of completed map support. In R47A
 * no native profile is activated and no cvar is synthesized from this table. */
typedef enum { C1CP47_NONE, C1CP47_TRAINING, C1CP47_US_INTRO, C1CP47_PATHFINDER } c1cp47Entry;
typedef struct {c1cp47Entry id;const char *map,*root;} c1cp47Route;
static const c1cp47Route c1cp47Routes[]={
 {C1CP47_TRAINING,"training","maps/training.gsc"},
 {C1CP47_US_INTRO,"us_intro","maps/us_intro.gsc"},
 {C1CP47_PATHFINDER,"pathfinder","maps/pathfinder.gsc"}
};
static const c1cp47Route *C1CP47_FindMap(const char *map){
 unsigned i;if(!map)return NULL;
 for(i=0;i<sizeof(c1cp47Routes)/sizeof(c1cp47Routes[0]);i++)if(!strcmp(map,c1cp47Routes[i].map))return c1cp47Routes+i;
 return NULL;
}
static int C1CP47_IsRoot(c1cp47Entry id,const char *path){
 unsigned i;if(!path)return 0;
 for(i=0;i<sizeof(c1cp47Routes)/sizeof(c1cp47Routes[0]);i++)if(c1cp47Routes[i].id==id)return !strcmp(path,c1cp47Routes[i].root);
 return 0;
}
/* Exactly the previous explicit-mode transition predicates. The caller still
 * owns fresh SP, Session, save and mode admission. No fallback on bad input. */
static const c1cp47Route *C1CP47_LegacyTransition(const char *map,const char *selectedRoot){
 const c1cp47Route *route=C1CP47_FindMap(map);
 if(!route)return NULL;
 if(route->id==C1CP47_US_INTRO&&C1CP47_IsRoot(C1CP47_TRAINING,selectedRoot))return route;
 if(route->id==C1CP47_PATHFINDER&&C1CP47_IsRoot(C1CP47_PATHFINDER,selectedRoot))return route;
 return NULL;
}
#endif
