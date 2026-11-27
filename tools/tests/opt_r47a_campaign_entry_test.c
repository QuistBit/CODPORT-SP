#include <assert.h>
#include <stdio.h>
#include "../../src/game/gsc_host/g_cod1_campaign_entry47.h"
static c1cp47Entry reference(const char *map,const char *path){
 if(map&&path&&!strcmp(map,"us_intro")&&!strcmp(path,"maps/training.gsc"))return C1CP47_US_INTRO;
 if(map&&path&&!strcmp(map,"pathfinder")&&!strcmp(path,"maps/pathfinder.gsc"))return C1CP47_PATHFINDER;
 return C1CP47_NONE;
}
int main(void){
 const char *maps[]={NULL,"","training","us_intro","pathfinder","TRAINING","US_INTRO","Training","maps/training","../training","training.bsp","training/","gamefinished","dam","training2","pathfinder/"};
 const char *paths[]={NULL,"","maps/training.gsc","maps/us_intro.gsc","maps/pathfinder.gsc","maps/Training.gsc","maps/training.GSC","maps/training.gsc/","maps/../training.gsc","scripts/gsc_tests/r232_training_turret.gsc","maps/vehicle_factory_probe.gsc","maps/dam.gsc","maps/pathfinder.gsc.bak"};
 unsigned i,j,cases=0;
 for(i=0;i<sizeof(maps)/sizeof(maps[0]);i++)for(j=0;j<sizeof(paths)/sizeof(paths[0]);j++){
  const c1cp47Route *r=C1CP47_LegacyTransition(maps[i],paths[j]);
  assert((r?r->id:C1CP47_NONE)==reference(maps[i],paths[j]));
  assert(C1CP47_IsRoot(C1CP47_TRAINING,paths[j])==!!(paths[j]&&!strcmp(paths[j],"maps/training.gsc")));
  if(r){assert(!strcmp(r->map,maps[i])&&C1CP47_IsRoot(r->id,r->root));}
  ++cases;
 }
 for(i=0;i<sizeof(c1cp47Routes)/sizeof(c1cp47Routes[0]);i++){
  assert(C1CP47_FindMap(c1cp47Routes[i].map)==c1cp47Routes+i);
  assert(C1CP47_IsRoot(c1cp47Routes[i].id,c1cp47Routes[i].root));
  for(j=0;j<sizeof(c1cp47Routes)/sizeof(c1cp47Routes[0]);j++)assert(C1CP47_IsRoot(c1cp47Routes[i].id,c1cp47Routes[j].root)==(i==j));
 }
 assert(!C1CP47_IsRoot(C1CP47_NONE,"maps/training.gsc"));assert(!C1CP47_FindMap("dam"));
 printf("R47A campaign entry: %u pairs PASS\n",cases);return 0;
}
