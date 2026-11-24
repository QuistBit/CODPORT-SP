/* GAME159E0 ->142F0 non-prone player setter. Exact integer delta calculation;
 * currentAngles and ps.viewangles are the port's two corresponding fields.
 * Prone body-yaw/pitch constraints require separately owned native fields. */
#include "../../qcommon/cod1_player_view_angles14.h"
static int PlayerViewAnglesInvoke(const GscPlayerNativeRequest *input,
 GscPlayerNativeResult *output,char *error,size_t size,int(*current)(void*),void *context){
 GscPlayerNativeRequest q;GscPlayerNativeResult result={0};gentity_t beforeEntity,*ent;gclient_t beforeClient,*client;
 cod1PlayerLifetime_t life;int32_t delta[3];int now,ok=0,i;uint32_t number;
 static const char emptyWeapon[128]={0};
 if(cod1PlayerNativeBusy){cod1PlayerNativeCancelled=1;return 0;}
 if(!input||!output||!current||
    PlayerNormalHealthBorrowed(input,sizeof(*input))||PlayerNormalHealthBorrowed(output,sizeof(*output))||
    PlayerNormalHealthBorrowed(error,size)||G_Cod1PlayerLifetimePrivate(input,sizeof(*input))||
    G_Cod1PlayerLifetimePrivate(output,sizeof(*output))||G_Cod1PlayerLifetimePrivate(error,size)||
    PlayerNativeOverlap(input,sizeof(*input),output,sizeof(*output))||PlayerNativeOverlap(input,sizeof(*input),error,size)||
    PlayerNativeOverlap(output,sizeof(*output),error,size))return 0;
 q=*input;memcpy(&number,&q.number,4);
 if(q.kind!=GSC_PLAYER_SET_ANGLES||q.now_ms<0||q.value||number||memcmp(q.weapon,emptyWeapon,sizeof(q.weapon))||
    !PlayerNativePlayer(&q.player)||!G_Cod1PlayerLifetimeRead(&q.player,&life))return 0;
 ent=g_entities+q.player.slot;client=ent->client;beforeEntity=*ent;beforeClient=*client;now=level.time;
 if(client->ps.pm_flags&PMF_COD1_PRONE){EntityError(error,size,"setPlayerAngles active prone body constraints not yet qualified");return 0;}
 if(!C1PVA14_Delta(q.angles_bits,client->pers.cmd.angles,delta)){
  EntityError(error,size,"setPlayerAngles supports finite normal/zero coordinates within one million degrees");return 0;
 }
 cod1PlayerNativeBusy=1;cod1PlayerNativeCancelled=0;
 for(i=0;i<2;i++){
  if(!current(context)||!cod1PlayerNativeBusy||cod1PlayerNativeCancelled||!PlayerNativePlayer(&q.player)||level.time!=now||
     !G_Cod1PlayerLifetimeSameCompleted(&life)||memcmp(input,&q,sizeof(q))||
     memcmp(ent,&beforeEntity,sizeof(*ent))||memcmp(client,&beforeClient,sizeof(*client)))goto done;
 }
 /* No external callback between final lifetime/snapshot fence and stores. */
 memcpy(client->ps.delta_angles,delta,sizeof(delta));
 memcpy(ent->r.currentAngles,q.angles_bits,sizeof(q.angles_bits));
 memcpy(client->ps.viewangles,q.angles_bits,sizeof(q.angles_bits));
 G_Cod1PlayerLifetimeActivity(ent);*output=result;ok=1;
 if(g_debugMove.integer>1)G_Printf("[408 player angles] owner=%u tick=%d xyz=%.9g,%.9g,%.9g delta=%d,%d,%d\n",q.player.slot,now,
  client->ps.viewangles[0],client->ps.viewangles[1],client->ps.viewangles[2],delta[0],delta[1],delta[2]);
done:cod1PlayerNativeBusy=0;return ok;
}
