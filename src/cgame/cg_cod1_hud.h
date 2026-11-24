#include "../game/bg_cod1_combat.h"
#include "cg_cod1_compass.h"
#include "cg_cod1_damage.h"
#include "cg_cod1_scope.h"
#include "cg_cod1_scope_reticle.h"
#include "../ui/keycodes.h"
#include "cg_cod1_keylabels.h"
/* CoD1 HUD adapter. Stance, weapon info, health and compass geometry come
 * from retail ui/hud.menu; the carried-weapon selector follows measured
 * retail cgame positions. Crosshair and message rects are cgame-side work.
 * Gameplay values come from this port's playerState. */
typedef struct {
    int initialized, enabled;
    qhandle_t stance[3], stanceFlash, healthBack, healthBar, healthCross, ammoBack, nameBack;
    qhandle_t modeIcons[CW_MAX_DEFS], weaponIcons[CW_MAX_DEFS];
    qhandle_t scopeSoftLine, scopeSoftLineH, scopeCenter[CW_MAX_DEFS];
    qhandle_t hipCenter[CW_MAX_DEFS], hipSide[CW_MAX_DEFS];
    qhandle_t compassBack, compassFace, compassHighlight, compassArrow;
    qhandle_t compassFriendly;
    qhandle_t compassObjective, compassObjectiveUp, compassObjectiveDown, compassObjectiveRing;
    qhandle_t friendlyHint, noWeaponIcon, hitDirection;
    fontInfo_t font;
    codCompass_t compassMotion;
    codCompass_t compassPointerMotion;
    float selectionExpand[CW_SLOT_COUNT];
    int selectionBlendTime;
    int stanceLast, stanceTime, stanceInitialized;
    float healthPrevious;
    int healthLastTime, healthLagCountdown, healthInitialized;
    int modeFadeWeapon, modeFadeFrom, modeFadeTo, modeFadeTime, modeFadeActive;
    cod1DamageSlot_t damageSlots[COD1_DAMAGE_SLOTS];
} codHud_t;
static codHud_t codHud;
void CG_Cod1HudReset(void) {memset(&codHud,0,sizeof(codHud));}
void CG_Cod1HudModeReset(void) {codHud.modeFadeActive=0;}
void CG_Cod1DamageReset(void) {
    memset(codHud.damageSlots,0,sizeof(codHud.damageSlots));
}
void CG_Cod1DamageFeedback(int yawByte,int pitchByte,int damage) {
    if(!cg.snap || cgs.gametype!=GT_SINGLE_PLAYER || damage<=0 ||
       (yawByte==255 && pitchByte==255) || !CG_Cod1HudEnabled())return;
    COD1_DamageRecord(codHud.damageSlots,cg.snap->serverTime,
                      cg_hudDamageIconTime.integer,yawByte,pitchByte,
                      (rand()&0x7fff)/32768.0f);
}
/* Retail ownerdraw 83 replaces the icon immediately. Blend the two original
 * modeIcon materials briefly at the user's request, after prediction has
 * confirmed an actual change of mode for the equipped weapon. */
void CG_Cod1HudModeChanged(int weapon,int oldBits,int newBits) {
    int root=CW_WeaponRoot(weapon),bit=CW_ModeBit(weapon);
    const cwDef_t *definition=CW_Get(root);
    int previous,current;
    if(!definition || !bit || definition->altIndex<0 ||
       !CW_Get(definition->altIndex))return;
    previous=(oldBits&bit)?definition->altIndex:root;
    current=(newBits&bit)?definition->altIndex:root;
    if(previous==current)return;
    codHud.modeFadeWeapon=weapon;
    codHud.modeFadeFrom=previous;
    codHud.modeFadeTo=current;
    codHud.modeFadeTime=cg.time;
    codHud.modeFadeActive=1;
}
static float CodHud_Scale(void) {return cgs.glconfig.vidHeight/480.0f;}
static float CodHud_Width(void) {return cgs.glconfig.vidWidth/CodHud_Scale();}
static void CodHud_Pic(float x,float y,float w,float h,qhandle_t shader) {
    float s=CodHud_Scale();
    trap_R_DrawStretchPic(x*s,y*s,w*s,h*s,0,0,1,1,shader);
}
static void CodHud_Rect(float x,float y,float w,float h,const float *color) {
    trap_R_SetColor(color); CodHud_Pic(x,y,w,h,cgs.media.whiteShader);
    trap_R_SetColor(NULL);
}
static qhandle_t CodHud_RegisterMaterial(const cwDef_t *d,const char *key) {
    const char *path=CW_String(d,key);
    return *path?trap_R_RegisterShaderNoMip(path):0;
}
static int CodHud_Init(void) {
    fileHandle_t f;
    int bytes,hudBytes,i;
    if(codHud.initialized) return codHud.enabled;
    codHud.initialized=1;
    bytes=trap_FS_FOpenFile("fonts/fontImage_16.dat",&f,FS_READ);
    if(f) trap_FS_FCloseFile(f);
    hudBytes=trap_FS_FOpenFile("gfx/hud/hud@compassback.tga",&f,FS_READ);
    if(f) trap_FS_FCloseFile(f);
    /* The font size identifies the CoD font converter in the renderer, but
     * should not disable all native HUD drawing when another font is used. */
    if(bytes!=20552 && hudBytes<=0) return 0;
    codHud.enabled=1;
    codHud.scopeSoftLine=trap_R_RegisterShaderNoMip("hudSoftLine");
    codHud.scopeSoftLineH=trap_R_RegisterShaderNoMip("hudSoftLineH");
    for(i=0;i<CW_Count() && i<CW_MAX_DEFS;i++) {
        const cwDef_t *d=CW_Get(i);
        if(CG_Cod1ReticleKind(CW_String(d,"adsOverlayReticle"))==COD1_RETICLE_CROSSHAIR &&
           *CW_String(d,"reticleCenter"))
            codHud.scopeCenter[i]=trap_R_RegisterShaderNoMip(CW_String(d,"reticleCenter"));
    }
    CG_Printf("CoD1 SHOOTING_TEST_R1: native ADS reticles and reload sound fallback\n");
    for(i=0;i<CW_Count() && i<CW_MAX_DEFS;i++) {
        const cwDef_t *d=CW_Get(i);
        codHud.modeIcons[i]=CodHud_RegisterMaterial(d,"modeIcon");
        codHud.weaponIcons[i]=CodHud_RegisterMaterial(d,"hudIcon");
        codHud.hipCenter[i]=CodHud_RegisterMaterial(d,"reticleCenter");
        codHud.hipSide[i]=CodHud_RegisterMaterial(d,"reticleSide");
    }
    codHud.stance[0]=trap_R_RegisterShaderNoMip("hudStanceStand");
    codHud.stance[1]=trap_R_RegisterShaderNoMip("hudStanceCrouch");
    codHud.stance[2]=trap_R_RegisterShaderNoMip("hudStanceProne");
    codHud.stanceFlash=trap_R_RegisterShaderNoMip("hudStanceFlash");
    codHud.healthBack=trap_R_RegisterShaderNoMip("gfx/hud/hud@health_back.tga");
    codHud.healthBar=trap_R_RegisterShaderNoMip("gfx/hud/hud@health_bar.tga");
    codHud.healthCross=trap_R_RegisterShaderNoMip("gfx/hud/hud@health_cross.tga");
    codHud.nameBack=trap_R_RegisterShaderNoMip("gfx/hud/hud@weaponnameback.tga");
    codHud.ammoBack=trap_R_RegisterShaderNoMip("gfx/hud/hud@ammocounterback.tga");
    codHud.compassBack=trap_R_RegisterShaderNoMip("gfx/hud/hud@compassback.tga");
    codHud.compassFace=trap_R_RegisterShaderNoMip("gfx/hud/hud@compassface.tga");
    codHud.compassHighlight=trap_R_RegisterShaderNoMip("gfx/hud/hud@compasshighlight.tga");
    codHud.compassArrow=trap_R_RegisterShaderNoMip("gfx/hud/hud@compass_arrow.tga");
    codHud.compassFriendly=trap_R_RegisterShaderNoMip("gfx/hud/hud@objective_friendly.tga");
    codHud.compassObjective=trap_R_RegisterShaderNoMip("hudObjective");
    codHud.compassObjectiveUp=trap_R_RegisterShaderNoMip("hudObjectiveUp");
    codHud.compassObjectiveDown=trap_R_RegisterShaderNoMip("hudObjectiveDown");
    codHud.compassObjectiveRing=trap_R_RegisterShaderNoMip("hudObjectiveRing");
    codHud.friendlyHint=trap_R_RegisterShaderNoMip("hintFriendly");
    codHud.noWeaponIcon=trap_R_RegisterShaderNoMip("hudNoWeaponIcon");
    codHud.hitDirection=trap_R_RegisterShaderNoMip("hudHitDirection");
    if(bytes==20552 || bytes==(int)sizeof(fontInfo_t))
        trap_R_RegisterFont("fonts/normalFont",16,&codHud.font);
    else codHud.font=cgDC.Assets.textFont;
    CG_Printf("CoD1 HUD: health, ammo, stance and compass enabled\n");
    return 1;
}
/* Server commands can arrive before the first drawn frame. Use the same
 * resource gate for native cp localization as for the CoD HUD itself. */
int CG_Cod1HudEnabled(void) { return CodHud_Init(); }
static float CodHud_TextWidth(const char *text,float scale) {
    float width=0;
    while(*text) width+=codHud.font.glyphs[(unsigned char)*text++].xSkip;
    return width*scale*codHud.font.glyphScale;
}
static void CodHud_TextAlpha(float x,float y,const char *text,float scale,float alpha) {
    int pass;
    float s=CodHud_Scale(), k=scale*codHud.font.glyphScale;
    vec4_t foreground={1,1,1,alpha}, shadow={0,0,0,alpha};
    for(pass=0;pass<2;pass++) {
        const unsigned char *p=(const unsigned char *)text;
        float at=x+(pass?0:1), base=y+(pass?0:1);
        trap_R_SetColor(pass?foreground:shadow);
        while(*p) {
            glyphInfo_t *g=&codHud.font.glyphs[*p++];
            trap_R_DrawStretchPic(at*s,(base-g->top*k)*s,
                g->imageWidth*k*s,g->imageHeight*k*s,g->s,g->t,g->s2,g->t2,g->glyph);
            at+=g->xSkip*k;
        }
    }
    trap_R_SetColor(NULL);
}
static void CodHud_Text(float x,float y,const char *text,float scale) {
    CodHud_TextAlpha(x,y,text,scale,1);
}
/* Both the weapon name and its backing fade for 1800 ms after selection.
 * The last 100 ms follow the retail fade helper at 0x30012960. */
static float CodHud_WeaponNameAlpha(void) {
    int elapsed=cg.time-cg.weaponSelectTime;
    float alpha=cg_hudAlpha.value;
    if(cg.weaponSelectTime<=0 || elapsed<0 || elapsed>=1800)return 0;
    if(elapsed>1700)alpha*=(1800-elapsed)*.01f;
    return alpha;
}
static const char *CodHud_WeaponPart(const char *source) {
    const char *translation;
    if(!source || !*source)return NULL;
    translation=CG_Cod1LocaleLookup(source);
    if(!translation)translation=CG_translateString(source);
    /* The original registration keeps the raw WeaponDef string when
     * StringEd cannot resolve it; an unresolved @TOKEN is still displayed. */
    return translation?translation:source;
}
/* Native compass math in 0x3000d380/0x3001cba0: both ally and objective
 * markers stop at the edge of the disc. The pointer angle is independent
 * of the spring which animates the disc. */
static float CodHud_CompassSize(void) {
    return cg_hudCompassSize.value>0.0f?cg_hudCompassSize.value:0.0f;
}
static float CodHud_CompassRadius(float distance,float size) {
    float min=cg_hudCompassMinRange.value,max=cg_hudCompassMaxRange.value;
    float fraction,inner=cg_hudCompassMinRadius.value;
    if(max<=min)return size*43.75f;
    if(distance<min)distance=min;
    if(distance>max)distance=max;
    fraction=(distance-min)/(max-min);
    return size*43.75f*(inner+(1.0f-inner)*fraction);
}
static float CodHud_CompassMarkerAlpha(float distance) {
    float max=cg_hudCompassMaxRange.value,objectiveMax=cg_hudObjectiveMaxRange.value;
    float fraction,alpha=cg_hudObjectiveMinAlpha.value;
    if(distance<=max)return 1.0f;
    if(objectiveMax<=max)return alpha;
    fraction=(distance-max)/(objectiveMax-max);
    if(fraction>1.0f)fraction=1.0f;
    alpha=1.0f+(alpha-1.0f)*fraction;
    if(alpha<0.0f)alpha=0.0f;
    if(alpha>1.0f)alpha=1.0f;
    return alpha;
}
static void CodHud_CompassPointerScene(refdef_t *rd) {
    memset(rd,0,sizeof(*rd));
    rd->width=cgs.glconfig.vidWidth;
    rd->height=cgs.glconfig.vidHeight;
    rd->fov_x=rd->fov_y=90;
    rd->time=cg.time;
    rd->rdflags=RDF_NOWORLDMODEL;
    AxisClear(rd->viewaxis);
}
/* Retail 0x300109a0 renders this before the regular status HUD. Its local
 * quad is x=-width/2..width/2, y=offset..offset+height; rotation by the
 * camera yaw minus incoming travel yaw puts a front hit above the reticle. */
static void CodHud_DamageIndicators(void) {
    const playerState_t *ps=&cg.predictedPlayerState;
    /* 0x30010a27 supplies TL,TR,BR,BL positions; its separate UV array at
     * 0x30052f3c flips both texture axes. Do not tie UVs to geometry. */
    static const float textureUV[4][2]={{1,1},{0,1},{0,0},{1,0}};
    static const float rectangleUV[4][2]={{0,0},{1,0},{1,1},{0,1}};
    float centerX=CodHud_Width()*.5f,centerY=240.0f;
    float width=cg_hudDamageIconWidth.value,height=cg_hudDamageIconHeight.value;
    float offset=cg_hudDamageIconOffset.value,scale=CodHud_Scale();
    float cameraYaw;
    refdef_t rd;
    int i,count=0;
    if(!cg.snap || !codHud.hitDirection || width<=0 || height<=0 || scale<=0)
        return;
    if(CG_Cod1ScopeAlpha(ps)>0) {
        float x,y;
        if(!cg_hudDamageIconInScope.integer)return;
        /* Retail follows the ADS overlay/reticle center, including
         * reticle-only sights without an overlay shader. */
        CG_Cod1ScopeCenter(&x,&y);
        centerX=x/scale;
        centerY=y/scale;
    }
    cameraYaw=atan2(cg.refdef.viewaxis[0][1],cg.refdef.viewaxis[0][0])*
              (180.0f/(float)M_PI);
    CodHud_CompassPointerScene(&rd);
    trap_R_ClearScene();
    for(i=0;i<COD1_DAMAGE_SLOTS;i++) {
        const cod1DamageSlot_t *slot=&codHud.damageSlots[i];
        float alpha=COD1_DamageAlpha(slot,cg.time);
        float a,co,si;
        polyVert_t vertices[4];
        int j;
        if(alpha<=0)continue;
        a=(cameraYaw-slot->yaw)*(float)(M_PI/180.0);
        co=cos(a);si=sin(a);
        memset(vertices,0,sizeof(vertices));
        for(j=0;j<4;j++) {
            float u=rectangleUV[j][0],v=rectangleUV[j][1];
            float dx,dy,px,py;
            COD1_DamageCorner(width,height,offset,co,si,u,v,&dx,&dy);
            px=(centerX+dx)*scale;
            py=(centerY+dy)*scale;
            vertices[j].xyz[0]=64;
            vertices[j].xyz[1]=64-128*px/cgs.glconfig.vidWidth;
            vertices[j].xyz[2]=64-128*py/cgs.glconfig.vidHeight;
            vertices[j].st[0]=textureUV[j][0];
            vertices[j].st[1]=textureUV[j][1];
            vertices[j].modulate[0]=vertices[j].modulate[1]=
                vertices[j].modulate[2]=255;
            vertices[j].modulate[3]=(byte)(alpha*255.0f);
        }
        trap_R_AddPolyToScene(codHud.hitDirection,4,vertices);
        count++;
    }
    if(count)trap_R_RenderScene(&rd);
}
static void CodHud_CompassPointerQuad(qhandle_t shader,float centerX,float centerY,
                                       float size,float angle,float alpha) {
    static const float corners[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};
    polyVert_t v[4];
    float a=angle*(float)(M_PI/180.0),co=cos(a),si=sin(a);
    float screenScale=CodHud_Scale(),half=size*.5f;
    int i;
    memset(v,0,sizeof(v));
    for(i=0;i<4;i++) {
        float x=corners[i][0],y=corners[i][1];
        float px=(centerX+half*(x*co-y*si))*screenScale;
        float py=(centerY+half*(x*si+y*co))*screenScale;
        v[i].xyz[0]=64;
        v[i].xyz[1]=64-128*px/cgs.glconfig.vidWidth;
        v[i].xyz[2]=64-128*py/cgs.glconfig.vidHeight;
        v[i].st[0]=(x+1)*.5f;
        v[i].st[1]=(y+1)*.5f;
        v[i].modulate[0]=v[i].modulate[1]=v[i].modulate[2]=255;
        v[i].modulate[3]=(byte)(alpha*255.0f);
    }
    trap_R_AddPolyToScene(shader,4,v);
}
/* RTCW snapshot fields give a deliberately limited ally approximation:
 * native CoD1 uses engine cache slots (syscall 0xe0), packed position, and an
 * explicit server-side drawcompassfriendlies switch, whose protocol is not
 * yet available here. Geometric placement and sprite heading are native. */
/* Typed CoD actor bodies are ET_GENERAL. Only their live publication
 * may supply the native friend/compass flags; ordinary map models cannot. */
static int CodHud_ActorAlly(const entityState_t *e){
    return e->eType==ET_GENERAL && e->cod1TurretActorVersion>0 &&
           e->cod1TurretActorGeneration>0 && !(e->eFlags&(EF_NODRAW|EF_DEAD)) &&
           (e->otherEntityNum2 & COD1_ALLY_PRESENT);
}
static void CodHud_Allies(float pointerYaw,float size,float centerX,float centerY) {
    const playerState_t *ps=&cg.predictedPlayerState;
    refdef_t rd;
    int i,count=0;
    if(!cg.snap || cgs.gametype!=GT_SINGLE_PLAYER ||
       !codHud.compassFriendly)return;
    trap_R_ClearScene();
    CodHud_CompassPointerScene(&rd);
    for(i=0;i<cg.snap->numEntities;i++) {
        const entityState_t *e=&cg.snap->entities[i];
        const centity_t *cent;
        float dx,dy,distance,angle,radius;
        if(count==32)break; /* native friend cache has 32 slots */
        if(e->number==ps->clientNum || e->number<0 || e->number>=MAX_GENTITIES ||
           !((CodHud_ActorAlly(e) && (e->otherEntityNum2 & COD1_ALLY_COMPASS)) ||
             (e->eType==ET_PLAYER && ps->teamNum==1 && e->aiChar!=AICHAR_NONE &&
              e->teamNum==ps->teamNum && !(e->eFlags&EF_DEAD))))continue;
        cent=&cg_entities[e->number];
        if(!cent->currentValid)continue;
        dx=cent->lerpOrigin[0]-ps->origin[0];
        dy=cent->lerpOrigin[1]-ps->origin[1];
        distance=(float)sqrt(dx*dx+dy*dy);
        angle=(float)(atan2(dy,dx)-pointerYaw*M_PI/180.0);
        radius=CodHud_CompassRadius(distance,size);
        CodHud_CompassPointerQuad(codHud.compassFriendly,
            centerX-(float)sin(angle)*radius,centerY-(float)cos(angle)*radius,
            size*10.0f,pointerYaw-cent->lerpAngles[YAW],
            CodHud_CompassMarkerAlpha(distance));
        count++;
    }
    if(count)trap_R_RenderScene(&rd);
}
/* Native state 4 with a nonzero world position is the sole source of these
 * glyphs. A state transition into 4 starts one ten-second pulse sequence. */
static void CodHud_ObjectiveMarkers(float pointerYaw,float size,
                                     float centerX,float centerY) {
    const playerState_t *ps=&cg.predictedPlayerState;
    int i;
    for(i=0;i<16;i++) {
        const cod1Objective_t *objective=CG_Cod1ObjectiveAt(i);
        float dx,dy,dz,distance,angle,radius,x,y,alpha,glyphSize;
        qhandle_t shader;
        if(!objective || objective->state!=4 || !objective->hasOrigin ||
           (!objective->origin[0] && !objective->origin[1] &&
            !objective->origin[2]))continue;
        dx=objective->origin[0]-ps->origin[0];
        dy=objective->origin[1]-ps->origin[1];
        dz=objective->origin[2]-ps->origin[2];
        distance=(float)sqrt(dx*dx+dy*dy);
        angle=(float)(atan2(dy,dx)-pointerYaw*M_PI/180.0);
        radius=CodHud_CompassRadius(distance,size);
        x=centerX-(float)sin(angle)*radius;
        y=centerY-(float)cos(angle)*radius;
        alpha=CodHud_CompassMarkerAlpha(distance);
        glyphSize=32*size;
        shader=dz<cg_hudObjectiveMinHeight.value?codHud.compassObjectiveDown:
            (dz>cg_hudObjectiveMaxHeight.value?codHud.compassObjectiveUp:
             codHud.compassObjective);
        if(shader) {
            vec4_t tint={1,1,1,alpha};
            trap_R_SetColor(tint);
            CodHud_Pic(x-glyphSize*.5f,y-glyphSize*.5f,
                       glyphSize,glyphSize,shader);
            trap_R_SetColor(NULL);
        }
        if(codHud.compassObjectiveRing && objective->changeTime>=0 &&
           cg.time>=objective->changeTime &&
           cg_hudObjectiveRingTime.integer>0 &&
           cg_hudObjectiveNumRings.integer>0 &&
           cg.time-objective->changeTime<cg_hudObjectiveRingTime.integer) {
            int num=cg_hudObjectiveNumRings.integer;
            float period=(float)cg_hudObjectiveRingTime.integer/num;
            float elapsed=cg.time-objective->changeTime;
            int index=(int)(elapsed/period);
            float phase=(elapsed-index*period)/period;
            float ringSize=cg_hudObjectiveRingSize.integer*phase*size;
            vec4_t tint={1,1,1,phase<.5f?phase*2.0f:(1.0f-phase)*2.0f};
            if(index>0)ringSize*=.5f;
            trap_R_SetColor(tint);
            CodHud_Pic(x-ringSize*.5f,y-ringSize*.5f,
                       ringSize,ringSize,codHud.compassObjectiveRing);
            trap_R_SetColor(NULL);
        }
    }
}
/* An isolated NOWORLDMODEL scene supplies a rotated textured quad without
 * changing renderer ABI. Same mechanism as RTCW's HUD model previews. */
static void CodHud_Compass(void) {
    refdef_t rd;
    polyVert_t v[4];
    static const float corners[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};
    float s=CodHud_Scale(),size=CodHud_CompassSize();
    float pointerYaw=CodCompass_UpdatePointer(&codHud.compassPointerMotion,
        cg.refdefViewAngles[YAW],cg.time,cg_hudCompassSpringyPointers.integer);
    /* The original subtracts engine configstring 11 as a map-specific north
     * offset. RTCW's configstring 11 is CS_VOTE_NO, so the offset cannot be
     * read until that server-to-client field is explicitly bridged. */
    float yaw=CodCompass_Update(&codHud.compassMotion,cg.refdefViewAngles[YAW],cg.time);
    float a=(yaw-90)*M_PI/180.0f;
    float co=cos(a), si=sin(a);
    float centerX=-25+80*size,centerY=505-80*size;
    int i;
    /* A zero native scale produces no visible layers; avoid a zero viewport. */
    if(size==0.0f)return;
    trap_R_SetColor(NULL);
    memset(&rd,0,sizeof(rd));
    rd.x=(int)(-25*s); rd.y=(int)((505-160*size)*s);
    rd.width=rd.height=(int)(160*size*s);
    rd.fov_x=rd.fov_y=90; rd.time=cg.time;
    rd.rdflags=RDF_NOWORLDMODEL;
    AxisClear(rd.viewaxis);
    memset(v,0,sizeof(v));
    for(i=0;i<4;i++) {
        float u=corners[i][0], t=corners[i][1];
        v[i].xyz[0]=64;
        v[i].xyz[1]=-64*(u*co-t*si);
        v[i].xyz[2]=-64*(u*si+t*co);
        v[i].st[0]=(u+1)*0.5f; v[i].st[1]=(t+1)*0.5f;
        memset(v[i].modulate,255,4);
    }
    /* Retail hud.menu paints both the back and the face with rotating
     * ownerdraw 84, with the fixed highlight between them. */
    trap_R_ClearScene();
    trap_R_AddPolyToScene(codHud.compassBack,4,v);
    trap_R_RenderScene(&rd);
    CodHud_Pic(-25,505-160*size,160*size,160*size,
               codHud.compassHighlight);
    trap_R_ClearScene();
    trap_R_AddPolyToScene(codHud.compassFace,4,v);
    trap_R_RenderScene(&rd);
    CodHud_Pic((35+25)*size-25,(395-345)*size+505-160*size,
               40*size,40*size,codHud.compassArrow);
    CodHud_Allies(pointerYaw,size,centerX,centerY);
    /* Tank markers depend on a native vehicle bit/syscall which RTCW does
     * not transmit. Objective records are delivered by CoD configstrings. */
    CodHud_ObjectiveMarkers(pointerYaw,size,centerX,centerY);
}
/* The scope mask does not contain the crosshair. Native soft-line materials
 * and the selected WeaponDef enum supply the separate second pass. */
static void CodHud_ScopeReticle(const cwDef_t *d, float x, float y,
                                 float w, float h, float alpha) {
    cod1ScopeStroke_t strokes[3];
    int i, count, root;
    int kind=CG_Cod1ReticleKind(CW_String(d,"adsOverlayReticle"));
    float size=(float)CW_Number(d,"reticleCenterSize");
    vec4_t color={0,0,0,1};
    qhandle_t materials[3];
    if(kind==COD1_RETICLE_NONE) return;
    root=CW_Find(d->name);
    materials[COD1_SCOPE_LINE_V]=codHud.scopeSoftLine;
    materials[COD1_SCOPE_LINE_H]=codHud.scopeSoftLineH;
    materials[COD1_SCOPE_CENTER]=(root>=0 && root<CW_MAX_DEFS)?codHud.scopeCenter[root]:0;
    /* The reticle-center material must stay square. The mask and its soft
     * lines still span the physical viewport; only the center art uses the
     * 4:3 height-based scale on wider displays. */
    {
        float sx=cgs.glconfig.vidWidth/640.0f;
        float sy=cgs.glconfig.vidHeight/480.0f;
        if(sx>sy)sx=sy;
        count=CG_Cod1ReticleStrokes(kind,x,y,w,h,size*sx,size*sx,strokes);
    }
    if(kind==COD1_RETICLE_CROSSHAIR) color[0]=color[1]=color[2]=1;
    color[3]=alpha;
    trap_R_SetColor(color);
    for(i=0;i<count;i++) {
        const cod1ScopeStroke_t *r=&strokes[i];
        qhandle_t material=materials[r->material];
        if(material)trap_R_DrawStretchPic(r->x,r->y,r->w,r->h,0,0,1,1,material);
    }
    trap_R_SetColor(NULL);
}
static void CodHud_Scope(void) {
    const playerState_t *ps=&cg.predictedPlayerState;const cwDef_t *d=CW_PSDef(ps);
    float alpha=CG_Cod1ScopeAlpha(ps),x,y,w,h,sx=cgs.glconfig.vidWidth/640.0f,sy=cgs.glconfig.vidHeight/480.0f;
    float sw=cgs.glconfig.vidWidth,sh=cgs.glconfig.vidHeight;vec4_t color={1,1,1,1};
    qhandle_t shader;
    int reticleKind=CG_Cod1ReticleKind(CW_String(d,"adsOverlayReticle"));
    if(alpha<=0 || cg.renderingThirdPerson || ps->stats[STAT_HEALTH]<=0 || (ps->stats[STAT_COD1_EDGES]&CW_LADDER_HIDDEN))return;
    shader=CG_Cod1ScopeShader();
    if(!shader && reticleKind==COD1_RETICLE_NONE)return;
    CG_Cod1ScopeCenter(&x,&y);w=CW_Number(d,"adsOverlayWidth")*sx;h=CW_Number(d,"adsOverlayHeight")*sy;
    if(w<=0 || h<=0)return;
    if(shader) {
        color[3]=alpha;trap_R_SetColor(color);
        trap_R_DrawStretchPic(x-w,y-h,w,h,0,0,1,1,shader);
        trap_R_DrawStretchPic(x,y-h,w,h,1,0,0,1,shader);
        trap_R_DrawStretchPic(x-w,y,w,h,0,1,1,0,shader);
        trap_R_DrawStretchPic(x,y,w,h,1,1,0,0,shader);
        if(x-w>0)trap_R_DrawStretchPic(0,0,x-w,sh,0,0,0,1,shader);
        if(x+w<sw)trap_R_DrawStretchPic(x+w,0,sw-x-w,sh,0,0,0,1,shader);
        if(y-h>0)trap_R_DrawStretchPic(x-w,0,2*w,y-h,0,0,1,0,shader);
        if(y+h<sh)trap_R_DrawStretchPic(x-w,y+h,2*w,sh-y-h,0,0,1,0,shader);
    }
    if(reticleKind!=COD1_RETICLE_NONE)CodHud_ScopeReticle(d,x,y,w,h,alpha);
    trap_R_SetColor(NULL);
}
/* Retail cgame (0x3002b520) visits grenade, pistol, primaryb, primary and
 * an unused zero slot, right to left from (632,10). The zero slot advances
 * x by 34 without drawing, so its omission below is visually identical.
 * Smoke has an input command but is not drawn. */
static void CodHud_WeaponSelect(void) {
    static const char *slotNames[CW_SLOT_COUNT]={
        "none","primary","primaryb","pistol","grenade","smokegrenade"
    };
    playerState_t ps;
    float alpha,x=CodHud_Width()-8,step,dimension,iconWidth,iconX;
    int slot,w,root,baseRoot,selected,elapsed,key,pass,mode,alt,seen[CW_MAX_DEFS];
    int chain[CW_MAX_DEFS],chainCount,chainIndex,letter;
    const cwDef_t *d,*alternate;
    qhandle_t icon;
    char command[48],label[24];
    vec4_t color={1,1,1,1};
    /* Retail 0x3002b523 updates the previous-frame clock even while the
     * selector is invisible. Reopening it must not jump to full expansion. */
    elapsed=cg.time-codHud.selectionBlendTime;
    codHud.selectionBlendTime=cg.time;
    if(!cg.snap || !cg.weaponSelect || !cg.weaponSelectTime ||
       cg.predictedPlayerState.stats[STAT_HEALTH]<=0)return;
    /* RTCW's CG_FadeColor uses a 200 ms tail.  The retail helper uses 100. */
    alpha=CodHud_WeaponNameAlpha();
    if(alpha<=0)return;
    if(elapsed<0)elapsed=0;
    step=elapsed*.01f;
    if(step>1)step=1;
    ps=cg.snap->ps;
    CW_SyncSlots(&ps);
    for(slot=CW_SLOT_GRENADE;slot>=CW_SLOT_PRIMARY;slot--) {
        w=ps.cod1Slots[slot];
        if(w<=WP_NONE || w>=WP_NUM_WEAPONS || !COM_BitCheck(ps.weapons,w) ||
           !CW_SlotCompatible(slot,w))w=WP_NONE;
        /* Stacked grenade types use one occupied place but each may be the
         * currently selected item. Render the selected member of that stack. */
        if(w!=WP_NONE && CW_SlotStackable(w) &&
           cg.weaponSelect>WP_NONE && cg.weaponSelect<WP_NUM_WEAPONS &&
           COM_BitCheck(ps.weapons,cg.weaponSelect) &&
           CW_SlotStackable(cg.weaponSelect) &&
           CW_SlotCompatible(slot,cg.weaponSelect))w=cg.weaponSelect;
        /* Native branch at 0x3002b5d8 skips rendering for an empty place,
         * clears its expansion and still consumes a 32+2 pixel cell. */
        if(w==WP_NONE) {
            codHud.selectionExpand[slot]=0;
            x-=34;
            continue;
        }
        selected=w!=WP_NONE && w==cg.weaponSelect;
        d=w!=WP_NONE?CW_WeaponDef(w):NULL;
        baseRoot=d?CW_WeaponRoot(w):-1;
        if(selected && ps.weapon==w && CW_PSDef(&ps))d=CW_PSDef(&ps);
        root=d?CW_Find(d->name):-1;
        icon=(root>=0 && root<CW_MAX_DEFS)?codHud.weaponIcons[root]:0;
        if(!icon && baseRoot>=0 && baseRoot<CW_MAX_DEFS)icon=codHud.weaponIcons[baseRoot];
        if(selected)codHud.selectionExpand[slot]+=step;
        else codHud.selectionExpand[slot]-=step;
        if(codHud.selectionExpand[slot]<0)codHud.selectionExpand[slot]=0;
        if(codHud.selectionExpand[slot]>1)codHud.selectionExpand[slot]=1;
        /* Expansion affects both dimensions; wideListIcon doubles only width.
         * An occupied slot with a missing icon uses a fixed 32x32 placeholder. */
        dimension=32*(1+codHud.selectionExpand[slot]);
        iconWidth=dimension*((d && CW_Number(d,"wideListIcon"))?2:1);
        if(!icon) {
            icon=codHud.noWeaponIcon;
            dimension=32;iconWidth=32;
            codHud.selectionExpand[slot]=0;
        }
        x-=iconWidth;
        iconX=x;
        color[0]=color[1]=color[2]=selected?1.0f:0.5f;
        color[3]=alpha;
        trap_R_SetColor(color);
        if(icon)CodHud_Pic(iconX,10,iconWidth,dimension,icon);
        trap_R_SetColor(NULL);
        /* An altWeapon chain supplies the small mode rows beneath a selected
         * icon. Keep the current mode on top and guard corrupt/cyclic data. */
        if(selected && root>=0 && root<CW_MAX_DEFS && baseRoot>=0) {
            memset(seen,0,sizeof(seen));chainCount=0;mode=root;
            while(mode>=0 && mode<CW_Count() && mode<CW_MAX_DEFS &&
                  !seen[mode] && CW_FamilyRoot(mode)==baseRoot) {
                seen[mode]=1;
                if(mode!=root)chain[chainCount++]=mode;
                alternate=CW_Get(mode);
                mode=alternate?alternate->altIndex:-1;
            }
            for(chainIndex=0;chainIndex<chainCount;chainIndex++) {
                alt=chain[chainIndex];alternate=CW_Get(alt);
                if(!alternate || alt>=CW_MAX_DEFS || !codHud.weaponIcons[alt])continue;
                color[0]=color[1]=color[2]=0.5f;
                trap_R_SetColor(color);
                CodHud_Pic(iconX+(iconWidth-((CW_Number(alternate,"wideListIcon")?64:32)))*.5f,
                           10+dimension+34*chainIndex,
                           CW_Number(alternate,"wideListIcon")?64:32,32,
                           codHud.weaponIcons[alt]);
                trap_R_SetColor(NULL);
            }
        }
        if(w!=WP_NONE && icon) {
            Com_sprintf(command,sizeof(command),"weaponslot %s",slotNames[slot]);
            key=trap_Key_GetKey(command);
            if(!CG_Cod1KeyLabel(key,label,sizeof(label))) {
                const char *unbound=CG_Cod1LocaleLookup("KEY_UNBOUND");
                Q_strncpyz(label,unbound?unbound:"Unbound",sizeof(label));
            }
            /* Label position follows the original screenshot and the retail
             * 0x3002b3b0 text scale. Exact font/style alignment awaits a
             * matching implementation of the engine's text syscall 0x37. */
            for(pass=0;label[0] && pass<2;pass++) {
                float at=iconX+(pass?0:1),base=15.6f+(pass?0:1);
                float scale=CodHud_Scale(),fontScale=.25f*codHud.font.glyphScale;
                color[0]=color[1]=color[2]=pass?1:0;
                trap_R_SetColor(color);
                for(letter=0;label[letter];letter++) {
                    glyphInfo_t *glyph=&codHud.font.glyphs[(unsigned char)label[letter]];
                    trap_R_DrawStretchPic(at*scale,(base-glyph->top*fontScale)*scale,
                        glyph->imageWidth*fontScale*scale,glyph->imageHeight*fontScale*scale,
                        glyph->s,glyph->t,glyph->s2,glyph->t2,glyph->glyph);
                    at+=glyph->xSkip*fontScale;
                }
                trap_R_SetColor(NULL);
            }
        }
        x-=2;
    }
}
static void CodHud_Crosshair(const playerState_t *ps) {
    const cwDef_t *d=CW_PSDef(ps);
    float x,y,ads,frac,phase=0,adsScale=1,spread,scaleX,scaleY;
    float bloom,minimum,gapX,gapY,centerSize,alpha,sideAlpha;
    const char *stanceMin;
    int definition;
    qhandle_t shader;
    vec4_t color={1,1,1,1};
    if(!cg_drawCrosshair.integer || cg_paused.integer || cg.renderingThirdPerson ||
       ps->weapon==WP_NONE || ps->stats[STAT_COD1_EDGES]&CW_LADDER_HIDDEN ||
       ps->serverCursorHint==HINT_EXIT || !d ||
       cg.refdef.fov_x<=0 || cg.refdef.fov_y<=0)return;
    definition=CW_Find(d->name);
    if(definition<0 || definition>=CW_MAX_DEFS)return;
    /* Retail 0x3001001e: fade hip art during the scope-overlay transition.
     * cg_crosshairAlpha now defaults to retail 1.0; cvar overrides apply. */
    alpha=(1-CG_Cod1ScopeAlpha(ps))*cg_crosshairAlpha.value;
    if(alpha<=.01f)return;
    ads=CW_ADS(ps);
    /* Retail 0x3001006e..8c hides a fully aimed regular reticle while the
     * weapon model is enabled, including iron sights without scope art. */
    if(ads>=1 && cg_drawGun.integer)return;
    frac=(float)CW_Number(d,(ps->stats[STAT_COD1_EDGES]&CW_ADS_RISING)?
                          "adsCrosshairInFrac":"adsCrosshairOutFrac");
    if(frac>0 && ads>1-frac) {
        phase=(ads-(1-frac))/frac;
        if(phase>1)phase=1;
        adsScale=1-.5f*phase;
    }
    scaleX=cgs.glconfig.vidWidth/640.0f;
    scaleY=cgs.glconfig.vidHeight/480.0f;
    /* cg_crosshairDynamic defaults to zero in retail. Its optional projected
     * aim displacement needs an explicit cvar and is not wired in this port.
     * With the default, adsAimPitch is still applied to the center Y. */
    x=cg.refdef.x+cg.refdef.width*.5f;
    y=cg.refdef.y+cg.refdef.height*.5f+
      phase*(float)CW_Number(d,"adsAimPitch")*(480.0f/cg.refdef.fov_y)*scaleY;
    centerSize=(float)CW_Number(d,"reticleCenterSize")*adsScale;
    /* Native cgame 0x30010178..0x30010252: with the model hidden, the ADS
     * transition draws an additional center image before the usual center
     * and sides. It prefers the weapon's reticleCenter material; a weapon
     * without one uses cg_crosshairNoGun. Its size grows from half to the
     * full WeaponDef center size and remains square in physical pixels. */
    if(!cg_drawGun.integer && adsScale<1.0f) {
        float noGunSize=(float)CW_Number(d,"reticleCenterSize")*(1.5f-adsScale);
        shader=codHud.hipCenter[definition];
        if(!shader && cg_crosshairNoGun.string[0])
            shader=trap_R_RegisterShaderNoMip(cg_crosshairNoGun.string);
        if(shader && noGunSize>0) {
            color[3]=alpha;
            trap_R_SetColor(color);
            trap_R_DrawStretchPic(x-noGunSize*.5f,y-noGunSize*.5f,
                                  noGunSize,noGunSize,0,0,1,1,shader);
            trap_R_SetColor(NULL);
        }
    }
    if(!codHud.hipCenter[definition] && !codHud.hipSide[definition])return;
    if(codHud.hipCenter[definition] && centerSize>0) {
        color[3]=alpha;
        trap_R_SetColor(color);
        trap_R_DrawStretchPic(x-centerSize*.5f,y-centerSize*.5f,
                              centerSize,centerSize,0,0,1,1,
                              codHud.hipCenter[definition]);
        trap_R_SetColor(NULL);
    }
    if(!codHud.hipSide[definition])return;
    /* Native 0x30010415..86: spread is an interpolated 0..255 player field,
     * and gap is in degrees via the render FOV, not pixels per degree. */
    bloom=ps->aimSpreadScaleFloat;
    if(bloom<0)bloom=0;
    if(bloom>255)bloom=255;
    stanceMin=(ps->pm_flags&PMF_COD1_PRONE)?"hipSpreadProneMin":
              ((ps->pm_flags&PMF_DUCKED)?"hipSpreadDuckedMin":"hipSpreadStandMin");
    minimum=(float)CW_Number(d,stanceMin);
    spread=(minimum+((float)CW_Number(d,"hipSpreadMax")-minimum)*
            (bloom/255.0f))*adsScale;
    gapX=spread*640.0f/cg.refdef.fov_x;
    gapY=spread*480.0f/cg.refdef.fov_y;
    minimum=(float)CW_Number(d,"reticleMinOfs");
    if(gapX<minimum)gapX=minimum;
    if(gapY<minimum)gapY=minimum;
    /* On wide displays the camera expands horizontal FOV while the retail
     * 640-wide gap also expands by vidWidth/640: the four sides end up too
     * far apart horizontally. Recover the 4:3 FOV from the vertical FOV and
     * scale both gaps by height. Keep the retail arithmetic on 4:3 screens.
     * The center and side materials are already square native-pixel quads. */
    if(cgs.glconfig.vidWidth*3>cgs.glconfig.vidHeight*4) {
        float fov43=atan(tan(cg.refdef.fov_y*M_PI/360.0f)*(4.0f/3.0f))*360.0f/M_PI;
        if(fov43>0) {
            gapX=spread*640.0f/fov43;
            if(gapX<minimum)gapX=minimum;
            scaleX=scaleY;
        }
    }
    gapX*=scaleX;
    gapY*=scaleY;
    /* Retail 0x30010540..10638 draws four rotated side images, not bars.
     * Render the same quads in an isolated NOWORLDMODEL scene, as the compass
     * does. The 90-degree UV rotation uses the engine's positive-angle
     * screen convention; final orientation awaits an in-game comparison. */
    sideAlpha=alpha*(1-bloom/255.0f);
    if(sideAlpha<cg_crosshairAlphaMin.value)
        sideAlpha=cg_crosshairAlphaMin.value; /* native default 0.7 */
    {
        static const float corners[4][2]={{0,0},{1,0},{1,1},{0,1}};
        refdef_t rd;
        polyVert_t vertices[4];
        float size=(float)CW_Number(d,"reticleSideSize")*adsScale;
        float pos=(float)CW_Number(d,"hipReticleSidePos");
        float rx[4],ry[4];
        float vw=(float)cgs.glconfig.vidWidth,vh=(float)cgs.glconfig.vidHeight;
        int i,j,opacity;
        if(size<=0 || vw<=0 || vh<=0)return;
        rx[0]=x-size*.5f;            ry[0]=y-gapY-size*(1-pos)-1;
        rx[1]=x+gapX-size*pos;        ry[1]=y-size*.5f;
        rx[2]=x-size*.5f;            ry[2]=y+gapY-size*pos;
        rx[3]=x-gapX-size*(1-pos)-1; ry[3]=y-size*.5f;
        memset(&rd,0,sizeof(rd));
        rd.width=cgs.glconfig.vidWidth;
        rd.height=cgs.glconfig.vidHeight;
        rd.fov_x=rd.fov_y=90;
        rd.rdflags=RDF_NOWORLDMODEL;
        rd.time=cg.time;
        AxisClear(rd.viewaxis);
        opacity=(int)(sideAlpha*255+.5f);
        if(opacity<0)opacity=0;
        if(opacity>255)opacity=255;
        trap_R_ClearScene();
        for(i=0;i<4;i++) {
            float t0=(float)((i>>1)&1),t1=1-t0;
            for(j=0;j<4;j++) {
                float u=corners[j][0],v=corners[j][1];
                float px=rx[i]+u*size,py=ry[i]+v*size;
                vertices[j].xyz[0]=64;
                vertices[j].xyz[1]=64-128*px/vw;
                vertices[j].xyz[2]=64-128*py/vh;
                vertices[j].st[0]=(i&1)?v:u;
                vertices[j].st[1]=t0+(t1-t0)*((i&1)?1-u:v);
                vertices[j].modulate[0]=vertices[j].modulate[1]=
                    vertices[j].modulate[2]=255;
                vertices[j].modulate[3]=(byte)opacity;
            }
            trap_R_AddPolyToScene(codHud.hipSide[definition],4,vertices);
        }
        trap_R_RenderScene(&rd);
    }
}
static void CodHud_Draw(void) {
    playerState_t *ps=&cg.predictedPlayerState;
    float width=CodHud_Width(), right=width-640, health, nameAlpha;
    vec4_t bar={.7f,.4f,0,1};
    int stance=(ps->pm_flags & PMF_COD1_PRONE)?2:((ps->pm_flags & PMF_DUCKED)?1:0);
    int weapon=ps->weapon, nameWeapon, ammoWeapon, clip, reserve, i;
    const char *weaponName=NULL;
    char nameLine[256];
    CodHud_Scope();
    if(cg_drawStatus.integer) {
        CodHud_Compass();
        /* Retail ID 20 at 0x3001bf60: transition hints last 3000 ms and
         * the overlaid flash lasts 1000 ms. The icon itself is always drawn.
         * Native tank eFlags have no verified RTCW equivalent here. */
        {
            static const char *const commands[3][4][6]={
                { /* standing: jump, stand, crouch, prone */
                    {"+gostand","+moveup"},{NULL},
                    {"gocrouch","togglecrouch","lowerstance","+movedown"},
                    {"goprone","+prone"}
                },
                { /* crouched */
                    {NULL},{"+gostand","raisestance","+moveup"},
                    {NULL},{"goprone","lowerstance","toggleprone","+prone"}
                },
                { /* prone */
                    {NULL},{"+gostand","toggleprone"},
                    {"gocrouch","togglecrouch","raisestance","+movedown","+moveup"},
                    {NULL}
                }
            };
            static const char *const hintKeys[4]={
                "CGAME_STANCEHINT_JUMP","CGAME_STANCEHINT_STAND",
                "CGAME_STANCEHINT_CROUCH","CGAME_STANCEHINT_PRONE"
            };
            static const char *const hintEnglish[4]={
                "Press [%s] to jump","Press [%s] to stand",
                "Press [%s] to crouch","Press [%s] to go prone"
            };
            char hintsCvar[16];
            int hintsEnabled,elapsed;
            trap_Cvar_VariableStringBuffer("cg_hudStanceHintPrints",
                                            hintsCvar,sizeof(hintsCvar));
            hintsEnabled=!hintsCvar[0] || atoi(hintsCvar)!=0;
            if(!hintsEnabled)codHud.stanceTime=-1;
            else if(!codHud.stanceInitialized || cg.time<codHud.stanceTime ||
                    codHud.stanceLast!=stance)codHud.stanceTime=cg.time;
            codHud.stanceLast=stance;
            codHud.stanceInitialized=1;
            CodHud_Pic(100,434.375f,40,40,codHud.stance[stance]);
            elapsed=cg.time-codHud.stanceTime;
            if(hintsEnabled && codHud.stanceTime>=0 && elapsed>=0 && elapsed<3000) {
                char lines[3][192];
                int count=0,action,candidate,key;
                float alpha=elapsed<2000?1:(3000-elapsed)*.001f;
                float lineHeight=codHud.font.glyphs[(unsigned char)'A'].height*
                                 .21f*codHud.font.glyphScale;
                float baseline=452.875f;
                for(action=0;action<4;action++) {
                    const char *pattern,*mark,*command;
                    char label[24]={0};
                    for(candidate=0;candidate<6;candidate++) {
                        command=commands[stance][action][candidate];
                        if(!command)break;
                        key=trap_Key_GetKey(command);
                        if(CG_Cod1KeyLabel(key,label,sizeof(label)))break;
                    }
                    if(!label[0])continue;
                    pattern=CG_Cod1LocaleLookup(hintKeys[action]);
                    if(!pattern || !*pattern)pattern=hintEnglish[action];
                    mark=strstr(pattern,"%s");
                    if(mark) {
                        size_t prefix=(size_t)(mark-pattern);
                        if(prefix>=sizeof(lines[0]))prefix=sizeof(lines[0])-1;
                        memcpy(lines[count],pattern,prefix);
                        lines[count][prefix]=0;
                        Q_strcat(lines[count],sizeof(lines[count]),label);
                        Q_strcat(lines[count],sizeof(lines[count]),mark+2);
                    } else Q_strncpyz(lines[count],pattern,sizeof(lines[count]));
                    if(++count==3)break;
                }
                /* Native x = icon right edge and the line stack is centered
                 * on its 452.875 text baseline, with 1.5 between lines. */
                if(count==1)baseline+=lineHeight*.5f;
                else if(count==3)baseline-=lineHeight*.5f+1.5f;
                for(i=0;i<count;i++) {
                    CodHud_TextAlpha(140,baseline,lines[i],.21f,alpha);
                    baseline+=lineHeight+1.5f;
                }
            }
            if(hintsEnabled && codHud.stanceTime>=0 && elapsed>=0 && elapsed<1000 &&
               codHud.stanceFlash) {
                static const char *const rgbCvars[3]={
                    "cg_hudStanceFlash_r","cg_hudStanceFlash_g","cg_hudStanceFlash_b"
                };
                vec4_t flash={1,1,.3f,(1000-elapsed)*.0008f};
                int channel;
                for(channel=0;channel<3;channel++) {
                    char value[32];
                    trap_Cvar_VariableStringBuffer(rgbCvars[channel],value,sizeof(value));
                    if(value[0]) {
                        flash[channel]=(float)atof(value);
                        if(flash[channel]<0)flash[channel]=0;
                        if(flash[channel]>1)flash[channel]=1;
                    }
                }
                trap_R_SetColor(flash);
                CodHud_Pic(100,434.375f,40,40,codHud.stanceFlash);
                trap_R_SetColor(NULL);
            }
        }
        /* Retail ownerdraw 79 reads the current snapshot, clamps health to
         * [0,1], and uses the item's (.7,.4,0,1) forecolor. */
        {
            const playerState_t *healthPs=cg.snap?&cg.snap->ps:ps;
            int maximum=healthPs->stats[STAT_MAX_HEALTH];
            health=maximum>0?(float)healthPs->stats[STAT_HEALTH]/maximum:0;
        }
        if(health<0) health=0;
        if(health>1) health=1;
        /* 0x3001c877..0x3001c8b9: equality follows the low-half branch. */
        if(health>.5f) {
            bar[0]=2*(1-health)*.7f;
            bar[2]=2*(1-health)*0;
        } else {
            bar[1]=(health+.2f)*.4f+.3f;
        }
        /* 0x3001c8ea..0x3001ca02: one-frame damage hold (retail timer=1ms),
         * then a red segment shrinks at 0.0012 of the bar per millisecond. */
        if(!codHud.healthInitialized || cg.time<codHud.healthLastTime) {
            codHud.healthPrevious=health;
            codHud.healthLagCountdown=1;
            codHud.healthInitialized=1;
        } else {
            int elapsed=cg.time-codHud.healthLastTime;
            if(health>=codHud.healthPrevious) {
                codHud.healthPrevious=health;
                codHud.healthLagCountdown=1;
            } else if(codHud.healthLagCountdown>0) {
                codHud.healthLagCountdown-=elapsed;
                if(codHud.healthLagCountdown<0)codHud.healthLagCountdown=0;
            } else {
                codHud.healthPrevious-=elapsed*.0012f;
                if(codHud.healthPrevious<=health) {
                    codHud.healthPrevious=health;
                    codHud.healthLagCountdown=1;
                }
            }
        }
        codHud.healthLastTime=cg.time;
        CodHud_Pic(right+501,460,130,12,codHud.healthBack);
        trap_R_SetColor(bar);
        if(health>0) {
            float s=CodHud_Scale();
            trap_R_DrawStretchPic((right+502)*s,461*s,128*health*s,10*s,
                                 0,0,health,1,codHud.healthBar);
        }
        if(codHud.healthPrevious>health) {
            vec4_t damage={1,0,0,1};
            float s=CodHud_Scale();
            trap_R_SetColor(damage);
            trap_R_DrawStretchPic((right+502+128*health)*s,461*s,
                128*(codHud.healthPrevious-health)*s,10*s,
                health,0,codHud.healthPrevious,1,codHud.healthBar);
        }
        trap_R_SetColor(NULL);
        CodHud_Pic(right+488,460,12,12,codHud.healthCross);
        nameAlpha=CodHud_WeaponNameAlpha();
        /* Retail name ownerdraw resolves the selected weapon first, even
         * while the old weapon is still equipped during a switch. */
        nameWeapon=cg.weaponSelect;
        if(nameWeapon<=WP_NONE || nameWeapon>=WP_NUM_WEAPONS ||
           !COM_BitCheck(ps->weapons,nameWeapon))nameWeapon=weapon;
        if(ps->eFlags & EF_MG42_ACTIVE) weaponName="MG42";
        else {
            const cwDef_t *definition=nameWeapon==weapon?CW_PSDef(ps):CW_WeaponDef(nameWeapon);
            const char *mode=CodHud_WeaponPart(CW_String(definition,"modeName"));
            weaponName=CodHud_WeaponPart(CW_String(definition,"displayName"));
            if(!weaponName)for(i=1;i<bg_numItems;i++) {
                if(bg_itemlist[i].giType==IT_WEAPON && bg_itemlist[i].giTag==nameWeapon) {
                    weaponName=bg_itemlist[i].pickup_name; break;
                }
            }
            if(weaponName && mode) {
                Com_sprintf(nameLine,sizeof(nameLine),"%s / %s",weaponName,mode);
                weaponName=nameLine;
            }
        }
        if(nameAlpha>0 && weaponName && weaponName[0]) {
            float nameWidth=CodHud_TextWidth(weaponName,.3f);
            vec4_t tint={1,1,1,nameAlpha};
            /* Retail ownerdraw IDs 81/82: width follows the localized name;
             * the name ends 28px before the right edge of the 320px rect. */
            trap_R_SetColor(tint);
            CodHud_Pic(right+562.5f-nameWidth-36,431,nameWidth+36,20,codHud.nameBack);
            trap_R_SetColor(NULL);
            CodHud_TextAlpha(right+534.5f-nameWidth,446,weaponName,.3f,nameAlpha);
        }
        ammoWeapon=nameWeapon; /* ID 5/6 use owned selection before active weapon. */
        if(ammoWeapon>WP_KNIFE && ammoWeapon<WP_NUM_WEAPONS &&
           !(ps->eFlags & EF_MG42_ACTIVE)) {
            const cwDef_t *ammoDef=ammoWeapon==weapon?CW_PSDef(ps):CW_WeaponDef(ammoWeapon);
            int clipIndex=BG_FindClipForWeapon(ammoWeapon);
            int reserveIndex=BG_PlayerAmmoIndex(ps,ammoWeapon);
            int clipValid=clipIndex>0 && (!ammoDef || CW_Number(ammoDef,"clipSize")>0);
            int reserveValid=reserveIndex>0 && (!ammoDef || !CW_Number(ammoDef,"clipOnly"));
            char clipText[24], reserveText[24];
            float textY=420.375f+24.25f, textX=right+570, textWidth=55;
            clip=clipValid?ps->ammoclip[clipIndex]:-1;
            reserve=reserveValid?ps->ammo[reserveIndex]:-1;
            if(clip<0)clipValid=0;
            if(reserve<0)reserveValid=0;
            if(clip>999)clip=999;
            if(reserve>999)reserve=999;
            /* Retail ID 6 draws the full background whenever a weapon is
             * selected; ID 5 chooses both, clip-only, reserve-only or blank.
             * Native WeaponDef+0x2d4 clip suppression remains unmapped. */
            CodHud_Pic(right+557.5f,421.625f,80,40,codHud.ammoBack);
            if(clipValid)Com_sprintf(clipText,sizeof(clipText),"%2i",clip);
            if(reserveValid)Com_sprintf(reserveText,sizeof(reserveText),"%i",reserve);
            if(clipValid && reserveValid) {
                CodHud_Text(textX,textY,clipText,.21f);
                CodHud_Text(textX+textWidth-CodHud_TextWidth(reserveText,.21f),
                            textY,reserveText,.21f);
                CodHud_Text(textX+(textWidth-CodHud_TextWidth("|",.21f))*.5f,
                            textY,"|",.21f);
            } else if(clipValid || reserveValid) {
                const char *remaining=clipValid?clipText:reserveText;
                CodHud_Text(textX+(textWidth-CodHud_TextWidth(remaining,.21f))*.5f,
                            textY,remaining,.21f);
            }
        }
    }
    if(cg_drawStatus.integer && !(ps->eFlags&EF_MG42_ACTIVE)) {
        const cwDef_t *modeDef=nameWeapon==weapon?CW_PSDef(ps):CW_WeaponDef(nameWeapon);
        int mode=modeDef?CW_Find(modeDef->name):-1;
        qhandle_t icon=mode>=0 && mode<CW_MAX_DEFS?codHud.modeIcons[mode]:0;
        int age=cg.time-codHud.modeFadeTime;
        /* Enhancement over native ID 83: a 160 ms crossfade at the same
         * fixed 20x20 rect. Cancel it if the weapon, mode or clock changes. */
        if(codHud.modeFadeActive && nameWeapon==weapon &&
           weapon==codHud.modeFadeWeapon && mode==codHud.modeFadeTo &&
           age>=0 && age<160) {
            qhandle_t previous=codHud.modeFadeFrom>=0 && codHud.modeFadeFrom<CW_MAX_DEFS?
                codHud.modeIcons[codHud.modeFadeFrom]:0;
            float fraction=age/160.0f;
            /* With standard alpha blending, dimming both layers would
             * darken the icon mid-transition.  Keep the outgoing layer
             * opaque beneath the incoming one. */
            vec4_t tint={1,1,1,icon?1.0f:1.0f-fraction};
            if(previous==icon) {
                codHud.modeFadeActive=0;
                if(icon)CodHud_Pic(right+537.5f,430.375f,20,20,icon);
            } else {
                if(previous) {
                    trap_R_SetColor(tint);
                    CodHud_Pic(right+537.5f,430.375f,20,20,previous);
                }
                if(icon) {
                    tint[3]=fraction;
                    trap_R_SetColor(tint);
                    CodHud_Pic(right+537.5f,430.375f,20,20,icon);
                }
                trap_R_SetColor(NULL);
            }
        } else {
            codHud.modeFadeActive=0;
            if(icon)CodHud_Pic(right+537.5f,430.375f,20,20,icon);
        }
    }
    if(cg_drawStatus.integer)CodHud_WeaponSelect();
    CodHud_Crosshair(ps);
}
