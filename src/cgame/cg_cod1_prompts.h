#ifndef CG_COD1_PROMPTS_H
#define CG_COD1_PROMPTS_H

/* Included from cg_draw.c, after cg_cod1_hud.h.  Retail ui/hud.menu supplies
 * the cursor-hint rect.  Retail gm and obj_* share the normal client channel;
 * gmb uses the bold channel.  Engine message geometry and lifetime are still
 * unknown; the status coordinates and timeouts below are port approximations.
 * RTCW pickups arrive as EV_ITEM_PICKUP.  Coordinates are 640x480 units. */
#include "../ui/keycodes.h"
#include "cg_cod1_keylabels.h"
#include "cg_cod1_locale.h"

#define COD1_PROMPT_STATUS_X        10.0f
#define COD1_PROMPT_BOLD_Y         302.0f
#define COD1_PROMPT_PICKUP_Y       326.0f
#define COD1_PROMPT_HINT_Y         325.0f /* retail ui/hud.menu Cursorhints */
#define COD1_PROMPT_HINT_SIZE       40.0f
#define COD1_PROMPT_HINT_SCALE        .21f
#define COD1_PROMPT_PICKUP_MSEC    3000
#define COD1_PROMPT_BOLD_MSEC      5000
#define COD1_PROMPT_HINT_MSEC       100 /* retail cg_hintFadeTime default */

/* StringEd templates are game resources. Expand only literal %s slots and
 * %%: an asset file must never become a printf format string. */
static void CG_Cod1PromptExpand(char *out,size_t capacity,const char *format,
                                 const char *first,const char *second) {
    size_t used=0;
    int argument=0;
    if (!capacity) return;
    if (!format) format="";
    while (*format && used+1<capacity) {
        if (*format=='%' && format[1]=='s') {
            const char *part=argument++ ? second:first;
            if (!part) part="";
            while (*part && used+1<capacity) out[used++]=*part++;
            format+=2;
        } else if (*format=='%' && format[1]=='%') {
            out[used++]='%';format+=2;
        } else {
            out[used++]=*format++;
        }
    }
    out[used]=0;
}

static const char *CG_Cod1PromptTemplate(const char *key,const char *fallback) {
    const char *translated=CG_Cod1LocaleLookup(key);
    return translated?translated:fallback;
}

/* Also used by the original cp command's server-text localization path. */
const char *CG_Cod1PromptLocalize(const char *key) {
    static char fallback[256];
    char token[128];
    size_t n=0;
    const char *parameter=NULL;
    const char *translated;
    if (!key) return "";
    if (*key == '@') key++;
    /* Retail server messages embed the substitution value after ASCII 20
     * (string) or ASCII 21 (number), e.g. GAME_PICKUP_HEALTH\x15 25.
     * Keep those control bytes away from the rendered font. */
    while (key[n] && (unsigned char)key[n]!=20 && (unsigned char)key[n]!=21 &&
           n<sizeof(token)-1) {token[n]=key[n];n++;}
    token[n]=0;
    if ((unsigned char)key[n]==20 || (unsigned char)key[n]==21)
        parameter=key+n+1;
    translated = CG_Cod1LocaleLookup(token);
    if (!translated) {
        translated = CG_translateString(token);
        if (!translated || !strcmp(translated,token)) translated=NULL;
    }
    /* The engine stores one substitution after ASCII 20 (text) or 21
     * (number); preserve it when the translation came from a .str file. */
    if (translated && (!parameter || !*parameter) && strstr(translated,"%s"))
        translated=NULL;
    if (translated) {
        CG_Cod1PromptExpand(fallback,sizeof(fallback),translated,parameter,NULL);
        return fallback;
    }
    if (!Q_stricmp(token,"GAME_OBJECTIVESUPDATED")) return "Objectives updated.";
    if (!Q_stricmp(token,"GAME_OBJECTIVECOMPLETED")) return "Objective completed.";
    if (!Q_stricmp(token,"GAME_OBJECTIVEFAILED")) return "Objective failed.";
    if (!Q_stricmp(token,"GAME_PICKUP_HEALTH")) {
        if (parameter && *parameter) {
            CG_Cod1PromptExpand(fallback,sizeof(fallback),"Picked up %s health.",parameter,NULL);
            return fallback;
        }
        return "Picked up health.";
    }
    if (!Q_stricmp(token,"GAME_PICKUP_AMMO")) {
        if (parameter && *parameter) {
            CG_Cod1PromptExpand(fallback,sizeof(fallback),"Picked up ammo for the %s",parameter,NULL);
            return fallback;
        }
        return "Picked up ammunition";
    }
    if (!Q_stricmp(token,"GAME_PICKUP_CLIPONLY_AMMO")) {
        if (parameter && *parameter) {
            CG_Cod1PromptExpand(fallback,sizeof(fallback),"Picked up %s ammo",parameter,NULL);
            return fallback;
        }
        return "Picked up ammunition";
    }
    if (!Q_stricmp(token,"GAME_PICKUP_CANTCARRYMOREAMMO")) {
        if (parameter && *parameter) {
            CG_Cod1PromptExpand(fallback,sizeof(fallback),"Can't carry any more %s ammo.",parameter,NULL);
            return fallback;
        }
        return "Can't carry any more ammo.";
    }
    if (!Q_stricmp(token,"GAME_CANT_GET_PRIMARY_WEAP_MESSAGE")) return "Cannot carry another primary weapon";
    if (!Q_stricmp(token,"GAME_CANT_GET_PISTOL_WEAP_MESSAGE")) return "Cannot carry another pistol";
    if (!Q_stricmp(token,"GAME_CANT_GET_GRENADE_WEAP_MESSAGE")) return "Cannot carry another grenade type";
    if (!Q_stricmp(token,"GAME_CANT_GET_SMOKER_WEAP_MESSAGE")) return "Cannot carry another smoke grenade type";
    return token[0] == 'G' && !Q_stricmpn(token,"GAME_",5) ? "Game message" : key;
}

/* Font glyphs come from CoD's normalFont.  Skip Quake colour directives so
 * they never render as literal ^1 or alter the width of centered messages. */
static float CG_Cod1PromptTextWidth(const char *text,float scale) {
    const unsigned char *p=(const unsigned char *)text;
    float width=0,k=scale*codHud.font.glyphScale;
    while (*p) {
        if (Q_IsColorString((const char *)p)) {p+=2;continue;}
        if (*p>=32) width+=codHud.font.glyphs[*p].xSkip*k;
        p++;
    }
    return width;
}
static float CG_Cod1PromptTextHeight(const char *text,float scale) {
    const unsigned char *p=(const unsigned char *)text;
    float height=0,k=scale*codHud.font.glyphScale;
    while (*p) {
        if (Q_IsColorString((const char *)p)) {p+=2;continue;}
        if (*p>=32 && height<codHud.font.glyphs[*p].height)
            height=codHud.font.glyphs[*p].height;
        p++;
    }
    return height*k;
}
static void CG_Cod1PromptText(float x,float y,const char *text,float scale,
                               float alpha,const float *rgb) {
    int pass;
    float s=CodHud_Scale(),k=scale*codHud.font.glyphScale;
    vec4_t color;
    if (alpha<=0 || !text || !*text) return;
    for (pass=0;pass<2;pass++) {
        const unsigned char *p=(const unsigned char *)text;
        float at=x+(pass?0:1),base=y+(pass?0:1);
        color[0]=pass?rgb[0]:0;color[1]=pass?rgb[1]:0;
        color[2]=pass?rgb[2]:0;color[3]=alpha;
        trap_R_SetColor(color);
        while (*p) {
            glyphInfo_t *glyph;
            if (Q_IsColorString((const char *)p)) {p+=2;continue;}
            if (*p<32) {p++;continue;}
            glyph=&codHud.font.glyphs[*p++];
            trap_R_DrawStretchPic(at*s,(base-glyph->top*k)*s,
                glyph->imageWidth*k*s,glyph->imageHeight*k*s,
                glyph->s,glyph->t,glyph->s2,glyph->t2,glyph->glyph);
            at+=glyph->xSkip*k;
        }
    }
    trap_R_SetColor(NULL);
}
static float CG_Cod1PromptFade(int start,int lifetime) {
    int elapsed=cg.time-start;
    int tail=lifetime<400?lifetime:400;
    if (start<=0 || elapsed<0 || elapsed>=lifetime) return 0;
    if (elapsed>lifetime-tail) return (lifetime-elapsed)/(float)tail;
    return 1;
}
/* The original cgame's cursor-hint fade helper uses a fixed 100 ms tail,
 * independently of the player's cg_hintFadeTime setting. */
static float CG_Cod1HintFade(int start,int lifetime) {
    int remaining=lifetime-(cg.time-start);
    if(start<=0 || cg.time<start || lifetime<=0 || remaining<=0)return 0;
    return cg_hudAlpha.value*(remaining<100?remaining*.01f:1.0f);
}

/* Retail cgame 0x3001d294..0x3001d2f8: only modes 1 and 2 grow the icon;
 * the full expansion is halved on each side to keep its center stationary.
 * Mode 2 uses the current hint's refresh time, not elapsed hint lifetime. */
static float CG_Cod1HintIconExpansion(int mode,int now,int start) {
    if (mode==2) return (float)(start%1000)*.01f;
    if (mode==1) return 5.0f*(1.0f+(float)sin((double)now*(double)(1.0f/150.0f)));
    return 0;
}

static const char *CG_Cod1PromptItemName(int index) {
    if (index<=0 || index>=bg_numItems) return "";
    if (cgs.itemPrintNames[index][0]) return cgs.itemPrintNames[index];
    return bg_itemlist[index].pickup_name ? bg_itemlist[index].pickup_name : "";
}

/* Retail weapon hints use the WeaponDef displayName even if the mapped RTCW
 * item has a different pickup_name.  Keep the item text as a fallback for
 * weapons without a loaded CoD definition. */
static const char *CG_Cod1PromptWeaponName(int weapon) {
    const cwDef_t *def=CW_WeaponDef(weapon);
    const char *name=def?CodHud_WeaponPart(CW_String(def,"displayName")):NULL;
    int i;
    if(name && *name)return name;
    for(i=1;i<bg_numItems;i++)
        if(bg_itemlist[i].giType==IT_WEAPON && bg_itemlist[i].giTag==weapon)
            return CG_Cod1PromptItemName(i);
    return "";
}

static qboolean CG_Cod1PromptItemMatchesHint(int type,int itemType) {
    switch (type) {
    case HINT_HEALTH:return itemType==IT_HEALTH;
    case HINT_WEAPON:return itemType==IT_WEAPON;
    /* An already owned weapon emits the native ammo hint for that same
     * weapon item, so the authoritative index can still name an IT_WEAPON. */
    case HINT_AMMO:return itemType==IT_AMMO || itemType==IT_WEAPON;
    case HINT_CLIPBOARD:return itemType==IT_CLIPBOARD;
    case HINT_TREASURE:return itemType==IT_TREASURE;
    case HINT_ARMOR:return itemType==IT_ARMOR;
    case HINT_POWERUP:return itemType==IT_POWERUP;
    case HINT_HOLDABLE:return itemType==IT_HOLDABLE;
    case HINT_INVENTORY:return itemType==IT_KEY;
    default:return qfalse;
    }
}

static qboolean CG_Cod1PromptItemIsTarget(int type,int index) {
    int i;
    if (index<=0 || index>=bg_numItems ||
        !CG_Cod1PromptItemMatchesHint(type,bg_itemlist[index].giType)) return qfalse;
    /* The type/index pair was supplied by the authoritative server selector.
     * Client lerpOrigin cannot reconstruct the item's world bbox center,
     * so an extra view-angle test here could hide a valid 72-unit hint.
     * Bound the entity match to reject unrelated numeric script values. */
    for (i=0;i<cg.snap->numEntities;i++) {
        const entityState_t *state=&cg.snap->entities[i];
        vec3_t dir;
        float distance;
        if (state->number<0 || state->number>=MAX_GENTITIES ||
            state->eType!=ET_ITEM || state->modelindex!=index) continue;
        VectorSubtract(cg_entities[state->number].lerpOrigin,cg.refdef.vieworg,dir);
        distance=VectorLength(dir);
        if (distance<=128) return qtrue;
    }
    return qfalse;
}

/* The port caches the latest normal-channel gm or obj_* text in
 * cod1GameMessage; cod1ObjectiveMessageKind selects the original obj_*
 * green tint.  Old RTCW pickup events remain a fallback.  The cache does
 * not model retail engine message queue semantics, which are unknown. */
static const char *CG_Cod1NormalNoticeLocalize(const char *text) {
    /* Existing obj_* notifications retain their previous GAME_* fallback;
     * a new gm payload uses the composed script-text bridge instead. */
    return cg.cod1ObjectiveMessageKind ? CG_Cod1PromptLocalize(text) :
        CG_Cod1MessageLocalize(text);
}
static void CG_Cod1DrawPickupNotice(void) {
    char line[4096];
    int index=cg.itemPickup;
    float alpha;
    static const float white[3]={.94f,.94f,.94f};
    static const float green[3]={0,1,0}; /* retail obj_* prefixes ^2 */
    const float *color=white;
    alpha=cg.cod1GameMessage[0] ?
        CG_Cod1PromptFade(cg.cod1GameMessageTime,COD1_PROMPT_PICKUP_MSEC) : 0;
    if (alpha>0) {
        Q_strncpyz(line,CG_Cod1NormalNoticeLocalize(cg.cod1GameMessage),sizeof(line));
        if (cg.cod1ObjectiveMessageKind) color=green;
    } else {
        const char *name;
        const gitem_t *item;
        if (index<=0 || index>=bg_numItems) return;
        alpha=CG_Cod1PromptFade(cg.itemPickupTime,COD1_PROMPT_PICKUP_MSEC);
        if (!alpha) return;
        item=&bg_itemlist[index];
        name=CG_Cod1PromptItemName(index);
        if (item->giType==IT_HEALTH)
            Com_sprintf(line,sizeof(line),"Health: %s",name);
        else if (item->giType==IT_AMMO)
            Com_sprintf(line,sizeof(line),"Ammunition: %s",name);
        else
            Com_sprintf(line,sizeof(line),"Picked up: %s",name);
    }
    CG_Cod1PromptText(COD1_PROMPT_STATUS_X,COD1_PROMPT_PICKUP_Y,line,.22f,alpha,color);
}

/* Legacy field/function names: cod1ObjectiveMessage now stores gmb, the
 * separate retail callback-3 channel.  Bold channel font and placement are
 * not established by the supplied DLL; these remain port approximations. */
static void CG_Cod1DrawMissionNotice(void) {
    float alpha=CG_Cod1PromptFade(cg.cod1ObjectiveMessageTime,
                                   COD1_PROMPT_BOLD_MSEC);
    const char *text;
    static const float white[3]={.94f,.94f,.94f};
    if (!alpha || !cg.cod1ObjectiveMessage[0]) return;
    text=CG_Cod1MessageLocalize(cg.cod1ObjectiveMessage);
    CG_Cod1PromptText(COD1_PROMPT_STATUS_X,COD1_PROMPT_BOLD_Y,
                      text,.22f,alpha,white);
}

static const char *CG_Cod1PromptUseKey(void) {
    static char keyName[24];
    return CG_Cod1KeyLabel(trap_Key_GetKey("+activate"),keyName,sizeof(keyName));
}

static void CG_Cod1CustomHintExpand(char *out,size_t capacity,const char *text,const char *key){
 size_t n=0,k;if(!out||!capacity)return;if(!text)text="";if(!key)key="?";
 while(*text&&n+1<capacity){if((text[0]=='&'&&text[1]=='&'&&text[2]=='1')||(text[0]=='%'&&text[1]=='s')){int skip=text[0]=='&'?3:2;for(k=0;key[k]&&n+1<capacity;k++)out[n++]=key[k];text+=skip;}else out[n++]=*text++;}out[n]=0;
}
static void CG_Cod1DrawUsePrompt(void) {
    int type,index;
    const char *key,*name=NULL,*action=NULL,*icon=NULL,*pattern=NULL;
    const char *swapFrom=NULL,*swapTo=NULL;
    char line[256]={0};
    float alpha,width,height,iconExpansion,iconWidth,iconX;
    qboolean wideWeaponIcon=qfalse;
    qhandle_t shader;
    const playerState_t *ps=&cg.snap->ps;
    static const float white[3]={.94f,.94f,.94f};
    if (!cg_cursorHints.integer || cg.renderingThirdPerson || cg.cameraMode || cg_paused.integer ||
        ps->stats[STAT_HEALTH]<=0 || cg_gameType.integer!=GT_SINGLE_PLAYER) return;
    CG_CheckForCursorHints();
    type=cg.cursorHintIcon;
    alpha=CG_Cod1HintFade(cg.cursorHintTime,
                         cg.cursorHintFade>0?cg.cursorHintFade:COD1_PROMPT_HINT_MSEC);
    if (!alpha) return;
    index=cg.cursorHintValue;
    switch (type) {
    case HINT_HEALTH:case HINT_WEAPON:case HINT_AMMO:
    case HINT_CLIPBOARD:case HINT_TREASURE:case HINT_ARMOR:
    case HINT_POWERUP:case HINT_HOLDABLE:case HINT_INVENTORY:
        if (!CG_Cod1PromptItemIsTarget(type,index)) return;
        name=bg_itemlist[index].giType==IT_WEAPON &&
             (type==HINT_WEAPON || type==HINT_AMMO) ?
             CG_Cod1PromptWeaponName(bg_itemlist[index].giTag) :
             CG_Cod1PromptItemName(index);
        if (!*name) return;
        /* The server had already checked this item against pickup limits
         * before setting its hint.  Its index travels in the 8-bit value. */
        action="pick up";
        icon=type==HINT_HEALTH?"hintHealth":"hintActivate";
        if (type==HINT_HEALTH)
            pattern=CG_Cod1PromptTemplate("CGAME_PICKUPHEALTH",
                                             "Press USE [%s] to pickup health");
        else if (type==HINT_AMMO) {
            pattern=CG_Cod1PromptTemplate("CGAME_PICKUPAMMO",
                                             "Press USE [%s] to pickup the %s ammo");
            if(bg_itemlist[index].giType==IT_WEAPON) {
                int weapon=bg_itemlist[index].giTag;
                const cwDef_t *def=CW_WeaponDef(weapon);
                if(!def || !COM_BitCheck(ps->weapons,weapon))return;
                name=CG_Cod1PromptWeaponName(weapon);
                if(*CW_String(def,"ammoIcon"))icon=CW_String(def,"ammoIcon");
            }
        }
        else if (type==HINT_WEAPON) {
            int weapon=bg_itemlist[index].giTag;
            playerState_t slots=*ps;
            const cwDef_t *def=CW_WeaponDef(weapon);
            if(weapon<=WP_NONE || weapon>=WP_NUM_WEAPONS ||
               COM_BitCheck(ps->weapons,weapon))return;
            name=CG_Cod1PromptWeaponName(weapon);
            if(def && *CW_String(def,"hudIcon"))icon=CW_String(def,"hudIcon");
            if(def && CW_Number(def,"wideListIcon"))wideWeaponIcon=qtrue;
            CW_SyncSlots(&slots);
            if (def && !COM_BitCheck(slots.weapons,weapon) &&
                !CW_PickupSlot(&slots,weapon,qfalse) &&
                 CW_PickupSlot(&slots,weapon,qtrue)) {
                int slot=CW_PickupSlot(&slots,weapon,qtrue);
                int previous=slot>0 && slot<(int)(sizeof(slots.cod1Slots)/sizeof(slots.cod1Slots[0]))?
                             slots.cod1Slots[slot]:WP_NONE;
                pattern=CG_Cod1PromptTemplate("CGAME_SWAPWEAPONS",
                                                 "Hold USE [%s] to swap");
                if(previous>WP_NONE && previous<WP_NUM_WEAPONS &&
                   previous!=weapon && COM_BitCheck(slots.weapons,previous)) {
                    swapFrom=CG_Cod1PromptWeaponName(previous);
                    swapTo=name;
                }
            } else pattern=CG_Cod1PromptTemplate("CGAME_PICKUPNEWWEAPON",
                                                  "Hold USE [%s] to pick up the %s");
        }
        break;
    /* RTCW hint enum values are not retail CoD hint IDs.  Map only their
     * equivalent icons; without a retail custom-string payload, generic
     * hints do not have default action text in the native DLL. */
    case HINT_ACTIVATE:
        icon="hintActivate";
        /* R89's checked server selector reserves this value for its isolated
         * generic interaction prompt. This is port policy, not the retail
         * setHintString pool; ordinary RTCW activate hints stay icon-only. */
        if(cgs.cod1MapEntities&&index>0&&index<MAX_COD1_SCRIPT_HINTS){
            const char *raw=CG_ConfigString(CS_COD1_SCRIPT_HINTS+index);
            if (raw[0]=='L' && raw[1]==':') {
                const char *hint=CG_Cod1LocaleLookupObjective(raw+2);
                const char *binding=CG_Cod1PromptUseKey();
                if (!binding) binding=CG_Cod1LocaleLookup("KEY_UNBOUND");
                CG_Cod1CustomHintExpand(line,sizeof(line),hint,binding);
            }
        }
        if (cgs.cod1MapEntities && index==COD1_HINT_USE_VALUE) {
            pattern="Press USE [%s]";
            action="interact";
        }
        break;
    case HINT_BUTTON:
        icon="hintActivate";break;
    case HINT_NOACTIVATE:
        icon="hintNoActivate";break;
    case HINT_DOOR:case HINT_DOOR_ROTATING:
        icon="hintDoor";break;
    case HINT_DOOR_LOCKED:case HINT_DOOR_ROTATING_LOCKED:
        icon="hintNoDoor";break;
    case HINT_MG42:
        if (ps->pm_flags & PMF_DUCKED) return;
        icon="hintMg42";break;
    case HINT_LADDER:icon="hintLadder";break;
    case HINT_EXIT:icon="hintExit";break;
    case HINT_PLYR_FRIEND:icon="hintFriendly";break;
    default:
        return; /* No invented CoD mapping for other RTCW hint kinds. */
    }
    key=(pattern || name)?CG_Cod1PromptUseKey():NULL;
    if ((pattern || name) && key) {
        if (swapFrom && swapTo && *swapFrom && *swapTo) {
            char prefix[128];
            CG_Cod1PromptExpand(prefix,sizeof(prefix),pattern,key,NULL);
            Com_sprintf(line,sizeof(line),"%s %s %s %s",prefix,swapFrom,
                        CG_Cod1PromptTemplate("CGAME_FOR","for"),swapTo);
        } else if (pattern) CG_Cod1PromptExpand(line,sizeof(line),pattern,key,name);
        else if (name) Com_sprintf(line,sizeof(line),"Press USE [%s] to %s %s",key,action,name);
        else Com_sprintf(line,sizeof(line),"Press USE [%s] to %s",key,action);
    } else if (pattern || name) {
        if (name) Com_sprintf(line,sizeof(line),"Bind Use to %s %s",action,name);
        else Com_sprintf(line,sizeof(line),"Bind Use to %s",action);
    }
    /* Retail cgame 0x3001d917 centers text over the cursor-hint rect and
     * places it at rect.y minus font height (textscale .21, shadow style). */
    if (line[0]) {
        width=CG_Cod1PromptTextWidth(line,COD1_PROMPT_HINT_SCALE);
        height=CG_Cod1PromptTextHeight(line,COD1_PROMPT_HINT_SCALE);
        CG_Cod1PromptText((CodHud_Width()-width)*.5f,COD1_PROMPT_HINT_Y-height,
                          line,COD1_PROMPT_HINT_SCALE,alpha,white);
    }
    shader=trap_R_RegisterShaderNoMip(icon);
    /* Some supplied retail PK3s lack aliases that the DLL registers.
     * Use the actual locked-door shader and a usable door icon as fallbacks. */
    if (!shader && (type==HINT_DOOR || type==HINT_DOOR_ROTATING))
        shader=trap_R_RegisterShaderNoMip("hintActivate");
    if (!shader && (type==HINT_DOOR_LOCKED ||
                    type==HINT_DOOR_ROTATING_LOCKED || type==HINT_NOACTIVATE))
        shader=trap_R_RegisterShaderNoMip("hintDoorLocked");
    if (shader) {
        vec4_t tint={1,1,1,alpha};
        iconExpansion=CG_Cod1HintIconExpansion(cg_cursorHints.integer,cg.time,cg.cursorHintTime);
        iconWidth=COD1_PROMPT_HINT_SIZE*(wideWeaponIcon?2.0f:1.0f)+iconExpansion;
        iconX=CodHud_Width()*.5f-iconWidth*.5f;
        trap_R_SetColor(tint);
        CodHud_Pic(iconX,COD1_PROMPT_HINT_Y-iconExpansion*.5f,
                   iconWidth,COD1_PROMPT_HINT_SIZE+iconExpansion,shader);
        trap_R_SetColor(NULL);
    }
}

#endif /* CG_COD1_PROMPTS_H */
