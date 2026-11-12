/* Selected player-only native path. Actor/non-client health remains unbound.
 * Retail writes entity.health; ClientEndFrame publishes STAT_HEALTH. */
#include "../cod1_gsc/cod1_normal_health14.h"
static int PlayerNormalHealthBorrowed(const void *p,size_t n) {
    return PlayerHealthFieldStoragePrivate(p,n)||PlayerNativeOverlap(p,n,g_entities,sizeof(g_entities))||
        (level.clients&&level.maxclients>0&&level.maxclients<=MAX_CLIENTS&&
         PlayerNativeOverlap(p,n,level.clients,sizeof(gclient_t)*(size_t)level.maxclients))||
        PlayerNativeOverlap(p,n,&level,sizeof(level))||
        PlayerNativeOverlap(p,n,cod1PlayerFreezeOwners,sizeof(cod1PlayerFreezeOwners));
}
static int PlayerNormalHealthInvoke(const GscPlayerNativeRequest *input,
    GscPlayerNativeResult *output,char *error,size_t size,
    int (*current)(void *),void *lease) {
    GscPlayerNativeRequest request;GscPlayerNativeResult result;
    gentity_t *ent;gclient_t *client;int health,maximum,now;
    int32_t after;
    if(!input||!output||!current||PlayerNormalHealthBorrowed(input,sizeof(*input))||
       PlayerNormalHealthBorrowed(output,sizeof(*output))||PlayerNormalHealthBorrowed(error,size))return 0;
    request=*input;memset(&result,0,sizeof(result));
    if(!PlayerNativePlayer(&request.player)||request.now_ms<0||request.value||
       request.weapon[0]||!isfinite(request.number)||
       (request.kind==GSC_PLAYER_GET_NORMAL_HEALTH&&request.number!=0.0f))goto refused;
    ent=&g_entities[request.player.slot];client=ent->client;
    health=ent->health;maximum=client->ps.stats[STAT_MAX_HEALTH];now=level.time;
    if(request.kind==GSC_PLAYER_SET_NORMAL_HEALTH) {
        if(!Cod1NormalHealth14Set(maximum,request.number,&after))goto refused;
    } else if(request.kind==GSC_PLAYER_GET_NORMAL_HEALTH) {
        result.kind=GSC_PLAYER_RESULT_FLOAT;
        if(!Cod1NormalHealth14Get(health,maximum,&result.number))goto refused;
    } else goto refused;
    cod1PlayerNativeBusy=1;cod1PlayerNativeCancelled=0;
    if(!current(lease)||cod1PlayerNativeCancelled||!PlayerNativePlayer(&request.player)||
       ent->client!=client||level.time!=now||ent->health!=health||client->ps.stats[STAT_MAX_HEALTH]!=maximum||
       memcmp(input,&request,sizeof(request))) {
        cod1PlayerNativeBusy=0;goto refused;
    }
    /* No external callback between currentness/unchanged checks and commit. */
    if(request.kind==GSC_PLAYER_SET_NORMAL_HEALTH)ent->health=after;
    *output=result;cod1PlayerNativeBusy=0;return 1;
refused:
    EntityError(error,size,"normal health requires a current player, positive maxHealth and finite positive setter value");
    return 0;
}
