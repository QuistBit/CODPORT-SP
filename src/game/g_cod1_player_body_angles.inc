#include "../qcommon/cod1_player_body_angles14.h"
/* Called from the ordinary ClientThink post-Pmove publication, before
 * client events/linking/triggers. No VM callback or getter substitution. */
static void G_Cod1PlayerBodyAngles(gentity_t *ent){
 if(G_Cod1MapEntities()&&ent==g_entities&&ent->client&&ent->client==level.clients&&
    ent->client->pers.connected==CON_CONNECTED)
  C1PBA14_Angles(ent->client->ps.viewangles,ent->r.currentAngles);
}
