/* Native setters write exactly one permission bit. No inventory dependency,
 * forced teleport, delayed script job or restoration when a thread ends. */
static int PlayerStancePermissionInvoke(const GscPlayerNativeRequest *input,
 GscPlayerNativeResult *output,char *error,size_t size,int (*current)(void *),void *lease){
 GscPlayerNativeRequest q;GscPlayerNativeResult result={0};
 playerState_t before;gclient_t *client;cod1PlayerLifetime_t life;int time,bit,ok=0;
 static const char emptyWeapon[128]={0};uint32_t number;
 if(cod1PlayerNativeBusy){cod1PlayerNativeCancelled=1;return 0;}
 if(!input||!output||!current||PlayerNormalHealthBorrowed(input,sizeof(*input))||
    PlayerNormalHealthBorrowed(output,sizeof(*output))||PlayerNormalHealthBorrowed(error,size)||
    G_Cod1PlayerLifetimePrivate(input,sizeof(*input))||G_Cod1PlayerLifetimePrivate(output,sizeof(*output))||
    G_Cod1PlayerLifetimePrivate(error,size)||PlayerNativeOverlap(input,sizeof(*input),output,sizeof(*output))||
    PlayerNativeOverlap(input,sizeof(*input),error,size)||PlayerNativeOverlap(output,sizeof(*output),error,size))return 0;
 q=*input;memcpy(&number,&q.number,4);
 if(q.kind<GSC_PLAYER_ALLOW_STAND||q.kind>GSC_PLAYER_ALLOW_PRONE||q.now_ms<0||
    memcmp(q.weapon,emptyWeapon,sizeof(q.weapon))||number||q.angles_bits[0]||q.angles_bits[1]||q.angles_bits[2]||
    !PlayerNativePlayer(&q.player)||!G_Cod1PlayerLifetimeRead(&q.player,&life))goto refuse;
 client=g_entities[q.player.slot].client;before=client->ps;time=level.time;
 bit=COD1_STANCE_PERMISSION_BIT(q.kind-GSC_PLAYER_ALLOW_STAND);
 cod1PlayerNativeBusy=1;cod1PlayerNativeCancelled=0;
 if(!current(lease)||!cod1PlayerNativeBusy||cod1PlayerNativeCancelled||level.time!=time||
    !G_Cod1PlayerLifetimeSameCompleted(&life)||
    !PlayerNativePlayer(&q.player)||client!=g_entities[q.player.slot].client||
    memcmp(input,&q,sizeof(q))||memcmp(&client->ps,&before,sizeof(before)))goto done;
 client->ps.stats[STAT_COD1_MODE]=q.value?(before.stats[STAT_COD1_MODE]&~bit):(before.stats[STAT_COD1_MODE]|bit);
 G_Cod1PlayerLifetimeActivity(g_entities+q.player.slot);*output=result;ok=1;
done:
 cod1PlayerNativeBusy=0;
refuse:
 if(!ok)EntityError(error,size,"stance permission requires current connected player and integer value");
 return ok;
}
