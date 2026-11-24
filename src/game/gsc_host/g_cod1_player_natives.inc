#include "../bg_cod1_stance.h"
/* R198: explicitly selected player inventory/input adapter. The name-to-WP
 * mapping is the existing CoD WeaponDef/inventory adapter, never a guessed
 * retail numeric ID or an RTCW weapon-name fallback. Give/switch/ammo behavior
 * is a bounded port policy; it does not claim the original inventory ABI. */
#include "../bg_cod1_inventory.h"

typedef struct {
    GscEntityRef player;
    int present, inheritedIgnoreInput;
} Cod1PlayerFreezeOwner;
static Cod1PlayerFreezeOwner cod1PlayerFreezeOwners[MAX_CLIENTS];
static int cod1PlayerNativeBusy, cod1PlayerNativeCancelled;

static int PlayerNativeOverlap(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return a&&b&&na&&nb&&(x<=y?y-x<na:x-y<nb);
}
static int PlayerNativeRefEqual(const GscEntityRef *a,const GscEntityRef *b) {
    return a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;
}
static int PlayerNativePlayer(const GscEntityRef *ref) {
    const gentity_t *ent;
    if(!ref||!cod1EntityReady||level.spawning||level.restarted||
       !G_Cod1MapEntities()||g_gametype.integer!=GT_SINGLE_PLAYER||
       !EntityValidate(NULL,ref)||!level.clients||level.maxclients<=0||
       level.maxclients>MAX_CLIENTS||ref->slot>=(uint32_t)level.maxclients||
       ref->slot>=MAX_CLIENTS)return 0;
    ent=&g_entities[ref->slot];
    return ent->client==&level.clients[ref->slot]&&
        !(ent->r.svFlags&(SVF_CASTAI|SVF_BOT))&&!ent->aiCharacter&&
        !ent->aiAttributes&&!ent->AIScript_AlertEntity&&!ent->aiInactive&&
        ent->client->pers.connected==CON_CONNECTED&&!ent->client->ps.aiChar&&
        (!ent->aiName||!strcmp(ent->aiName,"player"));
}
static int PlayerNativeWeapon(const char *name,int *definition,int *weapon) {
    int def,root,w;
    def=CW_Find(name);root=CW_FamilyRoot(def);
    if(def<0||root<0||!CW_Get(def)||!CW_Get(root))return 0;
    for(w=1;w<WP_NUM_WEAPONS;++w)if(CW_WeaponRoot(w)==root) {
        *definition=def;*weapon=w;return 1;
    }
    return 0;
}
/* Cleanup must survive restart/spawn/disconnect admission changes while the
 * original entity allocation still exists. No invocation capability is
 * inferred from this identity-only cleanup guard. */
static int PlayerNativeFreezeOwnerCurrent(const GscEntityRef *ref) {
    return ref&&ref->slot<MAX_CLIENTS&&G_Cod1EntitySameAllocation(ref)&&
        level.clients&&level.maxclients>0&&level.maxclients<=MAX_CLIENTS&&
        ref->slot<(uint32_t)level.maxclients&&
        g_entities[ref->slot].client==&level.clients[ref->slot];
}
static void PlayerNativeActionReset(playerState_t *ps,int weapon) {
    ps->weapon=weapon;ps->weaponstate=WEAPON_READY;
    ps->weaponTime=ps->weaponDelay=ps->grenadeTimeLeft=ps->weapAnimTimer=0;
    ps->stats[STAT_COD1_RELOAD]=ps->stats[STAT_COD1_SWITCH_TARGET]=0;
    ps->stats[STAT_COD1_ADS]=ps->stats[STAT_COD1_ADS_REMAINDER]=0;
}
static int PlayerNativeFallback(const playerState_t *ps) {
    int i,w;
    for(i=1;i<CW_SLOT_COUNT;++i) {
        w=ps->cod1Slots[i];
        if(w>WP_NONE&&w<WP_NUM_WEAPONS&&CW_WeaponDef(w)&&COM_BitCheck(ps->weapons,w))return w;
    }
    for(w=1;w<WP_NUM_WEAPONS;++w)
        if(CW_WeaponDef(w)&&COM_BitCheck(ps->weapons,w))return w;
    return WP_NONE;
}
/* Stop/unload restores only a changed, still-current flag. It neither clears
 * a flag inherited from another owner nor writes a replacement allocation.
 * The legacy PMF bit cannot distinguish a different subsystem acquiring that
 * SAME bit after GSC froze controls; only inherited ownership is preserved. */
void G_Cod1PlayerNativeReset(void) {
    int i;
    for(i=0;i<MAX_CLIENTS;++i) {
        Cod1PlayerFreezeOwner *owner=&cod1PlayerFreezeOwners[i];
        if(owner->present&&!owner->inheritedIgnoreInput&&PlayerNativeFreezeOwnerCurrent(&owner->player))
            g_entities[i].client->ps.pm_flags&=~PMF_IGNORE_INPUT;
        memset(owner,0,sizeof(*owner));
    }
}
static int PlayerNativeLease(const GscPlayerNativeRequest *request,int hostTime,
    unsigned revision,int (*current)(void *),void *lease) {
    return !cod1PlayerNativeCancelled&&current&&current(lease)&&
        !cod1PlayerNativeCancelled&&level.time==hostTime&&
        CW_RegistryRevision()==revision&&revision&&PlayerNativePlayer(&request->player);
}
static int PlayerHealthFieldStoragePrivate(const void *,size_t);
#include "g_cod1_player_normal_health.inc"
#include "g_cod1_player_viewmodel.inc"
#include "g_cod1_player_health_fields.inc"
#include "g_cod1_player_max_ammo.inc"
#include "g_cod1_player_slot_weapon.inc"
#include "g_cod1_player_view_angles.inc"
#include "g_cod1_player_stance_permissions.inc"
static int PlayerNativeInvoke(void *user,const GscPlayerNativeRequest *input,
    GscPlayerNativeResult *output,char *error,size_t size,
    int (*current)(void *),void *lease) {
    GscPlayerNativeRequest request;
    GscPlayerNativeResult result;
    playerState_t before,after;
    usercmd_t commandBefore;
    Cod1PlayerFreezeOwner freezeBefore,freezeAfter;
    gclient_t *client;
    const cwDef_t *def;
    unsigned revision;
    int weapon=0,definition=-1,hostTime,ammo=0,clip=0,limit=0,i,needsWeapon,sendWeapon=-1;
    char command[64];
    (void)user;
    /* A nested diagnostic must not overwrite the new native field cell. */
    if(PlayerHealthFieldStoragePrivate(error,size)) {
        if(cod1PlayerNativeBusy)cod1PlayerNativeCancelled=1;
        return 0;
    }
    if(cod1PlayerNativeBusy) {
        cod1PlayerNativeCancelled=1;
        if(!PlayerNormalHealthBorrowed(error,size))EntityError(error,size,"player native callback reentry refused");return 0;
    }
    if(!input||!output||!current||
       PlayerHealthFieldStoragePrivate(input,sizeof(*input))||PlayerHealthFieldStoragePrivate(output,sizeof(*output))||
       PlayerHealthFieldStoragePrivate(error,size)||
       PlayerNativeOverlap(input,sizeof(*input),output,sizeof(*output))||
       PlayerNativeOverlap(input,sizeof(*input),error,size)||
       PlayerNativeOverlap(output,sizeof(*output),error,size))return 0;
    if(input->kind>=GSC_PLAYER_ALLOW_STAND&&input->kind<=GSC_PLAYER_ALLOW_PRONE)return PlayerStancePermissionInvoke(input,output,error,size,current,lease);
    if(input->kind==GSC_PLAYER_SET_ANGLES)return PlayerViewAnglesInvoke(input,output,error,size,current,lease);
    if(input->angles_bits[0]||input->angles_bits[1]||input->angles_bits[2])return 0;
    if(input->kind==GSC_PLAYER_SHELLSHOCK||input->kind==GSC_PLAYER_STOP_SHELLSHOCK){
        if(!PlayerNativePlayer(&input->player))return 0;
        return G_Cod1PlayerShellshock(input,output,error,size,current,lease);
    }
    if(input->kind==GSC_PLAYER_GIVE_MAX_AMMO)return PlayerMaxAmmoInvoke(input,output,error,size,current,lease);
    if(input->kind==GSC_PLAYER_SET_VIEWMODEL)return PlayerViewmodelInvoke(input,output,error,size,current,lease);
    if(input->kind==GSC_PLAYER_SET_NORMAL_HEALTH||input->kind==GSC_PLAYER_GET_NORMAL_HEALTH)
        return PlayerNormalHealthInvoke(input,output,error,size,current,lease);
    request=*input;memset(&result,0,sizeof(result));
    if(request.number!=0.0f)goto refuse;
    if(request.kind<GSC_PLAYER_GET_CURRENT_WEAPON||(request.kind>GSC_PLAYER_FREEZE_CONTROLS&&request.kind!=GSC_PLAYER_GET_STANCE&&request.kind!=GSC_PLAYER_GET_SLOT_WEAPON)||
       request.now_ms<0||!memchr(request.weapon,0,sizeof(request.weapon))||
       !PlayerNativePlayer(&request.player))goto refuse;
    needsWeapon=request.kind>=GSC_PLAYER_HAS_WEAPON&&request.kind<=GSC_PLAYER_SET_AMMO_STOCK&&
        request.kind!=GSC_PLAYER_TAKE_ALL_WEAPONS;
    if((needsWeapon&&!PlayerNativeWeapon(request.weapon,&definition,&weapon))||
       (!needsWeapon&&request.kind!=GSC_PLAYER_GET_SLOT_WEAPON&&request.weapon[0])||
       ((request.kind!=GSC_PLAYER_SET_AMMO_CLIP&&request.kind!=GSC_PLAYER_SET_AMMO_STOCK&&
         request.kind!=GSC_PLAYER_FREEZE_CONTROLS)&&request.value)||
       ((request.kind==GSC_PLAYER_SET_AMMO_CLIP||request.kind==GSC_PLAYER_SET_AMMO_STOCK)&&request.value<0)||
       (request.kind==GSC_PLAYER_FREEZE_CONTROLS&&(request.value<0||request.value>1)))goto refuse;
    revision=CW_RegistryRevision();hostTime=level.time;
    if(!revision)goto refuse;
    client=g_entities[request.player.slot].client;
    before=client->ps;after=before;commandBefore=client->pers.cmd;
    freezeBefore=cod1PlayerFreezeOwners[request.player.slot];freezeAfter=freezeBefore;
    if(freezeAfter.present&&!PlayerNativeRefEqual(&freezeAfter.player,&request.player))
        memset(&freezeAfter,0,sizeof(freezeAfter));
    def=needsWeapon?CW_Get(definition):NULL;
    switch(request.kind) {
    case GSC_PLAYER_GET_SLOT_WEAPON:
        result.kind=GSC_PLAYER_RESULT_STRING;
        if(!PlayerNativeSlotWeapon(request.weapon,&before,result.text,sizeof(result.text)))goto refuse;
        break;
    case GSC_PLAYER_GET_STANCE:
        result.kind=GSC_PLAYER_RESULT_STRING;
        strcpy(result.text,(before.pm_flags&PMF_COD1_PRONE)?"prone":(before.pm_flags&PMF_DUCKED)?"crouch":"stand");
        break;
    case GSC_PLAYER_GET_CURRENT_WEAPON:
        result.kind=GSC_PLAYER_RESULT_STRING;
        if(before.weapon==WP_NONE)strcpy(result.text,"none");
        else {
            if(before.weapon<WP_NONE||before.weapon>=WP_NUM_WEAPONS||
               !COM_BitCheck(before.weapons,before.weapon)||(def=CW_PSDef(&before))==NULL||
               !memchr(def->name,0,sizeof(def->name)))goto refuse;
            strcpy(result.text,def->name);
        }
        break;
    case GSC_PLAYER_HAS_WEAPON:
        result.kind=GSC_PLAYER_RESULT_INT;result.value=COM_BitCheck(before.weapons,weapon)?1:0;break;
    case GSC_PLAYER_GIVE_WEAPON:
        if(!COM_BitCheck(before.weapons,weapon)) {
            ammo=BG_PlayerAmmoIndex(&before,(weapon_t)weapon);clip=BG_FindClipForWeapon((weapon_t)weapon);
            if(ammo<=0||ammo>=MAX_WEAPONS||clip<=0||clip>=MAX_WEAPONS)goto refuse;
            limit=CW_MaxAmmo(&before,weapon);i=(int)CW_Number(def,"startAmmo");
            if(limit<0||CW_MaxClip(&before,weapon)<0||i<0)goto refuse;
            COM_BitSet(after.weapons,weapon);
            after.ammoclip[clip]=CW_MaxClip(&before,weapon);
            after.ammo[ammo]=CW_Number(def,"clipOnly")?0:(i>limit?limit:i);
        }
        CW_SetMode(&after,weapon,definition);CW_SyncSlots(&after);break;
    case GSC_PLAYER_TAKE_WEAPON:
        COM_BitClear(after.weapons,weapon);CW_SyncSlots(&after);
        if(after.weapon==weapon) {
            sendWeapon=PlayerNativeFallback(&after);PlayerNativeActionReset(&after,sendWeapon);
        }
        break;
    case GSC_PLAYER_TAKE_ALL_WEAPONS:
        memset(after.weapons,0,sizeof(after.weapons));memset(after.cod1Slots,0,sizeof(after.cod1Slots));
        memset(after.ammo,0,sizeof(after.ammo));memset(after.ammoclip,0,sizeof(after.ammoclip));
        after.stats[STAT_COD1_MODE]&=COD1_STANCE_PERMISSION_MASK;sendWeapon=WP_NONE;PlayerNativeActionReset(&after,WP_NONE);break;
    case GSC_PLAYER_SWITCH_TO_WEAPON:
        if(!COM_BitCheck(before.weapons,weapon))goto refuse;
        CW_SetMode(&after,weapon,definition);sendWeapon=weapon;PlayerNativeActionReset(&after,weapon);break;
    case GSC_PLAYER_SET_AMMO_CLIP:
    case GSC_PLAYER_SET_AMMO_STOCK:
        if(!COM_BitCheck(before.weapons,weapon))goto refuse;
        if(request.kind==GSC_PLAYER_SET_AMMO_CLIP) {
            clip=BG_FindClipForWeapon((weapon_t)weapon);limit=CW_MaxClip(&before,weapon);
            if(clip<=0||clip>=MAX_WEAPONS||limit<0)goto refuse;
            after.ammoclip[clip]=request.value>limit?limit:request.value;
        } else {
            ammo=BG_PlayerAmmoIndex(&before,(weapon_t)weapon);limit=CW_MaxAmmo(&before,weapon);
            if(ammo<=0||ammo>=MAX_WEAPONS||limit<0)goto refuse;
            after.ammo[ammo]=CW_Number(def,"clipOnly")?0:(request.value>limit?limit:request.value);
        }
        break;
    case GSC_PLAYER_ATTACK_PRESSED:
        result.kind=GSC_PLAYER_RESULT_INT;result.value=!!(commandBefore.buttons&BUTTON_ATTACK);break;
    case GSC_PLAYER_USE_PRESSED:
        result.kind=GSC_PLAYER_RESULT_INT;result.value=!!(commandBefore.buttons&BUTTON_ACTIVATE);break;
    case GSC_PLAYER_RELOAD_PRESSED:
        result.kind=GSC_PLAYER_RESULT_INT;result.value=!!(commandBefore.wbuttons&WBUTTON_RELOAD);break;
    case GSC_PLAYER_MELEE_PRESSED:
        /* The restored CoD melee input uses BUTTON_SPRINT in PM_Weapon,
         * independently of RTCW's legacy wolfkick member. */
        result.kind=GSC_PLAYER_RESULT_INT;result.value=!!(commandBefore.buttons&BUTTON_SPRINT);break;
    case GSC_PLAYER_FREEZE_CONTROLS:
        if(request.value) {
            if(!freezeAfter.present) {
                freezeAfter.player=request.player;freezeAfter.present=1;
                freezeAfter.inheritedIgnoreInput=!!(before.pm_flags&PMF_IGNORE_INPUT);
            }
            after.pm_flags|=PMF_IGNORE_INPUT;
        } else if(freezeAfter.present) {
            if(!freezeAfter.inheritedIgnoreInput)after.pm_flags&=~PMF_IGNORE_INPUT;
            memset(&freezeAfter,0,sizeof(freezeAfter));
        }
        break;
    default:goto refuse;
    }
    cod1PlayerNativeBusy=1;cod1PlayerNativeCancelled=0;
    if(!PlayerNativeLease(&request,hostTime,revision,current,lease)||
       client!=g_entities[request.player.slot].client||
       memcmp(&before,&client->ps,sizeof(before))||
       memcmp(&commandBefore,&client->pers.cmd,sizeof(commandBefore))||
       memcmp(&freezeBefore,&cod1PlayerFreezeOwners[request.player.slot],sizeof(freezeBefore)))goto busyRefuse;
    /* No callbacks occur between the final identity check and this commit. */
    if(request.kind!=GSC_PLAYER_GET_SLOT_WEAPON){client->ps=after;cod1PlayerFreezeOwners[request.player.slot]=freezeAfter;}
    if(sendWeapon>=0) {
        snprintf(command,sizeof(command),"cod1scriptweapon %d",sendWeapon);
        trap_SendServerCommand((int)request.player.slot,command);
        if(!PlayerNativeLease(&request,hostTime,revision,current,lease))goto busyRefuse;
    }
    *output=result;cod1PlayerNativeBusy=0;return 1;
busyRefuse:
    cod1PlayerNativeBusy=0;
refuse:
    EntityError(error,size,"player native requires a current connected CoD SP player and supported loaded inventory/input request");
    return 0;
}
const GscPlayerNativeHost *G_Cod1PlayerNativeHost(void) {
    static const GscPlayerNativeHost host={NULL,PlayerNativeInvoke};
    return &host;
}
