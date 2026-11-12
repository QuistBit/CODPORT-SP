#include "../bg_cod1_viewhands.h"
/* Player-owned state: normal Session end does not undo a script assignment.
 * Player initialization/map lifecycle clears the playerState with its owner.
 * No renderer handles or guessed retail model index cross this boundary. */
static int PlayerViewmodelInvoke(const GscPlayerNativeRequest *input,
    GscPlayerNativeResult *output,char *error,size_t size,
    int (*current)(void *),void *lease) {
    GscPlayerNativeRequest request;GscPlayerNativeResult result;
    gentity_t *ent;gclient_t *client;int selected,previous,now;
    if(!input||!output||!current||PlayerNormalHealthBorrowed(input,sizeof(*input))||
       PlayerNormalHealthBorrowed(output,sizeof(*output))||PlayerNormalHealthBorrowed(error,size))return 0;
    request=*input;memset(&result,0,sizeof(result));
    if(!PlayerNativePlayer(&request.player)||request.kind!=GSC_PLAYER_SET_VIEWMODEL||
       request.value||request.number!=0.0f||request.now_ms<0||
       !memchr(request.weapon,0,sizeof(request.weapon))||!Cod1ViewHandsId(request.weapon,&selected))goto refused;
    ent=&g_entities[request.player.slot];client=ent->client;
    previous=client->ps.cod1ViewHands;now=level.time;
    cod1PlayerNativeBusy=1;cod1PlayerNativeCancelled=0;
    if(!current(lease)||cod1PlayerNativeCancelled||!PlayerNativePlayer(&request.player)||
       ent->client!=client||level.time!=now||client->ps.cod1ViewHands!=previous||
       memcmp(input,&request,sizeof(request))) {
        cod1PlayerNativeBusy=0;goto refused;
    }
    client->ps.cod1ViewHands=selected;*output=result;cod1PlayerNativeBusy=0;return 1;
refused:
    EntityError(error,size,"setViewmodel requires a current player and an admitted original loadout hands model");return 0;
}
