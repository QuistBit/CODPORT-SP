/* Narrow compare-and-set of the single HUD scene marker. No legacy void
 * reliable broadcast: queue refusal leaves both table and queue unchanged. */
#include "../game/bg_cod1_script_hud.h"
qboolean SV_Cod1TrySetHudMarker(const char *value,const char *expected){
    char next[COD1_SCRIPT_HUD_MARKER_BYTES],before[COD1_SCRIPT_HUD_MARKER_BYTES],command[80];
    const char *actual;char *owned;uint64_t epoch;size_t n,chars=1;int i,human=-1;
    if(!value||!expected||!sv_gametype||sv_gametype->integer!=GT_SINGLE_PLAYER||
       (sv.state!=SS_LOADING&&sv.state!=SS_GAME)||!sv_maxclients||sv_maxclients->integer<1||sv_maxclients->integer>MAX_CLIENTS||!svs.clients)return qfalse;
    for(n=0;n<sizeof(next)&&value[n];n++){}if(n>=sizeof(next))return qfalse;memcpy(next,value,n+1);
    for(n=0;n<sizeof(before)&&expected[n];n++){}if(n>=sizeof(before))return qfalse;memcpy(before,expected,n+1);
    if((*next&&!CH_ReadMarker(next,&epoch))||(*before&&!CH_ReadMarker(before,&epoch)))return qfalse;
    actual=sv.configstrings[CS_COD1_SCRIPT_HUD];if(!actual||strcmp(actual,before))return qfalse;
    if(!strcmp(actual,next))return qtrue;
    for(i=0;i<MAX_CONFIGSTRINGS;i++){
        const char *text=i==CS_COD1_SCRIPT_HUD?next:sv.configstrings[i];size_t length;
        if(!text)return qfalse;if(!*text)continue;length=strlen(text);
        if(length>=BIG_INFO_STRING||length+1>MAX_GAMESTATE_CHARS-chars)return qfalse;chars+=length+1;
    }
    if(sv.state==SS_GAME||sv.restarting)for(i=0;i<sv_maxclients->integer;i++){
        client_t *client=&svs.clients[i];if(client->state<CS_PRIMED)continue;
        if(client->gentity&&(client->gentity->r.svFlags&(SVF_CASTAI|SVF_BOT)))continue;
        if(human>=0||i!=0||client->state!=CS_ACTIVE)return qfalse;human=i;
    }
    owned=CopyString(next); /* All potentially failing allocation precedes queue commit. */
    if(human>=0){
        Com_sprintf(command,sizeof(command),"cs %d \"%s\"",CS_COD1_SCRIPT_HUD,next);
        if(!SV_Cod1TrySendServerCommand(human,command)){Z_Free(owned);return qfalse;}
    }
    Z_Free(sv.configstrings[CS_COD1_SCRIPT_HUD]);sv.configstrings[CS_COD1_SCRIPT_HUD]=owned;return qtrue;
}
