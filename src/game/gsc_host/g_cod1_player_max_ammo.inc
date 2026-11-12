/* Retail 20015570 -> 2001A940: owned weapon, max-stock > 0 then AddAmmo
 * with fillClip=0. This lane uses existing port WeaponDef/slot mapping.
 * clipOnly, alternate modes and ambiguous shared-pool maxima remain refused. */
static int PlayerMaxAmmoInvoke(const GscPlayerNativeRequest *input,
    GscPlayerNativeResult *output,char *error,size_t size,int(*current)(void*),void *lease) {
    GscPlayerNativeRequest request;GscPlayerNativeResult result;
    playerState_t before,after;gclient_t *client;const cwDef_t *def;
    int weapon,definition,ammo,clip,maximum,maxclip,w,time;unsigned revision;
    if(!input||!output||!current||PlayerNormalHealthBorrowed(input,sizeof(*input))||
       PlayerNormalHealthBorrowed(output,sizeof(*output))||PlayerNormalHealthBorrowed(error,size))return 0;
    request=*input;memset(&result,0,sizeof(result));
    if(request.kind!=GSC_PLAYER_GIVE_MAX_AMMO||request.now_ms<0||request.value||request.number!=0.0f||
       !memchr(request.weapon,0,sizeof(request.weapon))||!PlayerNativePlayer(&request.player)||
       !PlayerNativeWeapon(request.weapon,&definition,&weapon))goto refuse;
    revision=CW_RegistryRevision();time=level.time;if(!revision)goto refuse;
    client=g_entities[request.player.slot].client;before=client->ps;after=before;
    /* The observed unowned-weapon branch is a genuine no-op. */
    if(COM_BitCheck(before.weapons,weapon)) {
        def=CW_Get(definition);if(!def||definition!=CW_WeaponRoot(weapon)||CW_Number(def,"clipOnly"))goto refuse;
        ammo=BG_PlayerAmmoIndex(&before,(weapon_t)weapon);clip=BG_FindClipForWeapon((weapon_t)weapon);
        maximum=CW_MaxAmmo(&before,weapon);maxclip=CW_MaxClip(&before,weapon);
        if(ammo<=0||ammo>=MAX_WEAPONS||clip<=0||clip>=MAX_WEAPONS||maximum<0||maxclip<0||before.ammo[ammo]<0||before.ammoclip[clip]<0)goto refuse;
        for(w=1;w<WP_NUM_WEAPONS;w++)if(CW_WeaponDef(w)&&BG_PlayerAmmoIndex(&before,(weapon_t)w)==ammo&&CW_MaxAmmo(&before,w)!=maximum)goto refuse;
        if(before.ammo[ammo]<maximum) {
            after.ammo[ammo]=maximum;
            /* AddAmmo clamps excess clip but does not fill a partial clip. */
            if(after.ammoclip[clip]>maxclip)after.ammoclip[clip]=maxclip;
        }
    }
    cod1PlayerNativeBusy=1;cod1PlayerNativeCancelled=0;
    if(!current(lease)||cod1PlayerNativeCancelled||!PlayerNativePlayer(&request.player)||
       client!=g_entities[request.player.slot].client||level.time!=time||CW_RegistryRevision()!=revision||
       memcmp(input,&request,sizeof(request))||memcmp(&before,&client->ps,sizeof(before))) {
        cod1PlayerNativeBusy=0;goto refuse;
    }
    client->ps=after;*output=result;cod1PlayerNativeBusy=0;return 1;
refuse:
    EntityError(error,size,"giveMaxAmmo requires current player, loaded base weapon and supported ordinary ammo pool");return 0;
}
