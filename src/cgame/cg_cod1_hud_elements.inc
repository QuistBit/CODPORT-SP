/* R200 actual local-clock HUD render, included by cg_servercmds.c. Texture
 * names are original script materials. Server packets are bounded mutations,
 * never a per-frame transport. Quads use the existing NOWORLDMODEL HUD path. */
#include "../game/bg_cod1_script_hud.h"
static struct {
    uint64_t epoch,floor,sequence,incarnation;
    int last_snapshot,have_time;
    qhandle_t face,needle,black,material[COD1_SCRIPT_HUD_CAPACITY];
    cod1ScriptHud_t slot[COD1_SCRIPT_HUD_CAPACITY];
} cgCod1HudElements;
static void CHF_Reset(void);
void CG_Cod1HudElementsReset(qboolean full){
    cgIntroMusicPending=0;
    CHF_Reset();
    uint64_t floor=full?0:cgCod1HudElements.floor,incarnation=cgCod1HudElements.incarnation+1;
    memset(&cgCod1HudElements,0,sizeof(cgCod1HudElements));cgCod1HudElements.floor=floor;cgCod1HudElements.incarnation=incarnation?incarnation:1;
}
void CG_Cod1HudElementsConfigChanged(int number){
    const char *text;uint64_t epoch,incarnation=cgCod1HudElements.incarnation;
    if(number!=CS_COD1_SCRIPT_HUD)return;text=CG_ConfigString(number);
    if(incarnation!=cgCod1HudElements.incarnation||!text)return;
    if(!text[0]){CG_Cod1HudElementsReset(qfalse);return;}
    if(!CH_ReadMarker(text,&epoch)){CG_Cod1HudElementsReset(qfalse);return;}
    if(epoch==cgCod1HudElements.epoch||epoch<=cgCod1HudElements.floor)return;
    CG_Cod1HudElementsReset(qfalse);cgCod1HudElements.epoch=cgCod1HudElements.floor=epoch;
}
static int CHC_Current(uint64_t incarnation,uint64_t epoch,int time){
    return incarnation==cgCod1HudElements.incarnation&&epoch==cgCod1HudElements.epoch&&time==cg.time;
}
#include "cg_cod1_hud_font.inc"
void CG_Cod1HudElementsCommand(void){
    char copied[COD1_SCRIPT_HUD_MAX_ARGC][257];const char *arguments[COD1_SCRIPT_HUD_MAX_ARGC];cod1ScriptHud_t state;
    uint64_t incarnation=cgCod1HudElements.incarnation,epoch=cgCod1HudElements.epoch;int time=cg.time,i,count; qhandle_t face,needle;
    if(!epoch||time<0)return;count=trap_Argc();if(!CHC_Current(incarnation,epoch,time)||(count!=COD1_SCRIPT_HUD_ARGC&&count!=COD1_SCRIPT_HUD_INTRO_ARGC))return;
    for(i=0;i<count;i++){memset(copied[i],0,sizeof(copied[i]));trap_Argv(i,copied[i],sizeof(copied[i]));if(!CHC_Current(incarnation,epoch,time))return;arguments[i]=copied[i];}
    if(!CH_Decode(count,arguments,&state)||state.epoch!=epoch||state.sequence<=cgCod1HudElements.sequence)return;
    /* Reliable commands are dispatched during snapshot transition. cg.time may
     * step back after a long render frame. Identity and reliable sequence
     * fence mutations; the snapshot clock below fences a timeline restart. */
    if(cgCod1HudElements.slot[state.index].generation>state.generation||
       (cgCod1HudElements.slot[state.index].owner&&cgCod1HudElements.slot[state.index].owner!=state.owner))return;
    cgCod1HudElements.sequence=state.sequence; /* Consume before resource callbacks. */
    if(state.alive&&state.content==1){
        qhandle_t material=cgCod1HudElements.material[state.index];
        if(!material||strcmp(state.text,cgCod1HudElements.slot[state.index].text))material=trap_R_RegisterShaderNoMip(state.text);
        if(!CHC_Current(incarnation,epoch,time)||cgCod1HudElements.sequence!=state.sequence||!material)return;
        cgCod1HudElements.material[state.index]=material;
        if(!strcmp(state.text,"black"))cgCod1HudElements.black=material;
    }
    if(state.alive&&state.clock){
        face=cgCod1HudElements.face;needle=cgCod1HudElements.needle;
        if(!face)face=trap_R_RegisterShaderNoMip("hudStopwatch");if(!CHC_Current(incarnation,epoch,time)||cgCod1HudElements.sequence!=state.sequence)return;
        if(!needle)needle=trap_R_RegisterShaderNoMip("hudStopwatchNeedle");if(!CHC_Current(incarnation,epoch,time)||cgCod1HudElements.sequence!=state.sequence)return;
        if(!face||!needle){CG_Printf("R200 HUD: original stopwatch shaders unavailable\n");return;}
        cgCod1HudElements.face=face;cgCod1HudElements.needle=needle;
    }
    cgCod1HudElements.slot[state.index]=state;
}
/* Read-only admission from actual received HUD state, never a ready receipt.
 * The current full-screen opaque script black owns the subsequent fade. */
int CG_Cod1HudElementsOwnsPregameFade(void){
    uint64_t incarnation=cgCod1HudElements.incarnation,epoch=cgCod1HudElements.epoch;
    uint64_t sequence=cgCod1HudElements.sequence,markerEpoch;int i,time=cg.time;const char*marker;
    if(cgs.gametype!=GT_SINGLE_PLAYER||!cg.snap||!epoch||!sequence||
       !cgCod1HudElements.black||time<0)return 0;
    marker=CG_ConfigString(CS_COD1_SCRIPT_HUD);
    if(!CHC_Current(incarnation,epoch,time)||sequence!=cgCod1HudElements.sequence||
       !CH_ReadMarker(marker,&markerEpoch)||markerEpoch!=epoch)return 0;
    for(i=0;i<COD1_SCRIPT_HUD_CAPACITY;i++){
        const cod1ScriptHud_t*s=&cgCod1HudElements.slot[i];
        if(CH_Validate(s)&&s->epoch==epoch&&s->sequence<=sequence&&s->alive&&s->content==1&&!strcmp(s->text,"black")&&
           s->x==0&&s->y==0&&!s->align_x&&!s->align_y&&s->width==640&&s->height==480&&
           !s->fade_ms&&s->alpha==255&&s->server_time<=time&&CH_Alpha(s,time)==1.0f)
            return CHC_Current(incarnation,epoch,time)&&sequence==cgCod1HudElements.sequence;
    }
    return 0;
}

static void CHC_Needle(float x,float y,float width,float height,float degrees,qhandle_t shader){
    static const float corners[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};
    refdef_t view;polyVert_t vertices[4];float angle=degrees*0.01745329251994329577f,co=cosf(angle),si=sinf(angle);int i;
    uint64_t incarnation=cgCod1HudElements.incarnation,epoch=cgCod1HudElements.epoch;int time=cg.time;
    CG_AdjustFrom640(&x,&y,&width,&height);if(width<1||height<1)return;
    if(!CHC_Current(incarnation,epoch,time))return;
    memset(&view,0,sizeof(view));view.x=(int)x;view.y=(int)y;view.width=(int)width;view.height=(int)height;
    view.fov_x=view.fov_y=90;view.time=cg.time;view.rdflags=RDF_NOWORLDMODEL;AxisClear(view.viewaxis);
    memset(vertices,0,sizeof(vertices));for(i=0;i<4;i++){
        float u=corners[i][0],v=corners[i][1];vertices[i].xyz[0]=64;vertices[i].xyz[1]=-64*(u*co-v*si);vertices[i].xyz[2]=-64*(u*si+v*co);
        vertices[i].st[0]=(u+1)*.5f;vertices[i].st[1]=(v+1)*.5f;memset(vertices[i].modulate,255,4);
    }
    trap_R_ClearScene();if(!CHC_Current(incarnation,epoch,time))return;
    trap_R_AddPolyToScene(shader,4,vertices);if(!CHC_Current(incarnation,epoch,time))return;trap_R_RenderScene(&view);
}
static void CHC_DrawIntro(uint64_t incarnation,uint64_t epoch,int time){
    cod1ScriptHud_t items[COD1_SCRIPT_HUD_CAPACITY],tmp;int n=0,i,j;uint64_t sequence=cgCod1HudElements.sequence;
    for(i=0;i<COD1_SCRIPT_HUD_CAPACITY;i++)if(cgCod1HudElements.slot[i].alive&&cgCod1HudElements.slot[i].content)items[n++]=cgCod1HudElements.slot[i];
    for(i=1;i<n;i++){tmp=items[i];j=i;while(j>0&&items[j-1].sort>tmp.sort){items[j]=items[j-1];j--;}items[j]=tmp;}
    for(i=0;i<n;i++){
        cod1ScriptHud_t *s=&items[i];vec4_t color={1,1,1,1};float x=s->x,y=s->y;
        if(!CHC_Current(incarnation,epoch,time)||sequence!=cgCod1HudElements.sequence)return;
        color[3]=CH_Alpha(s,time);if(color[3]<=0)continue;
        if(s->content==1){
            if(!cgCod1HudElements.material[s->index])continue;
            x-=s->width*(s->align_x*.5f);y-=s->height*(s->align_y*.5f);
            trap_R_SetColor(color);if(!CHC_Current(incarnation,epoch,time)||sequence!=cgCod1HudElements.sequence)return;
            CG_DrawPic(x,y,(float)s->width,(float)s->height,cgCod1HudElements.material[s->index]);
            if(!strcmp(s->text,"black") && x==0 && y==0 && s->width==640 && s->height==480 && color[3]>=1)
                CG_Cod1IntroMusicFrame();
        }else if(s->content==2&&s->text[0]){
            if(!CHF_DrawText(s,incarnation,epoch,time,sequence))return;
        }
        if(!CHC_Current(incarnation,epoch,time)||sequence!=cgCod1HudElements.sequence)return;trap_R_SetColor(NULL);
    }
}
void CG_Cod1HudElementsDraw(void){
    uint64_t incarnation=cgCod1HudElements.incarnation,epoch=cgCod1HudElements.epoch;int i,time=cg.time;
    if(!epoch||time<0||!cg.snap)return;if(cgCod1HudElements.have_time&&cg.snap->serverTime<cgCod1HudElements.last_snapshot){CG_Cod1HudElementsReset(qfalse);return;}
    cgCod1HudElements.have_time=1;cgCod1HudElements.last_snapshot=cg.snap->serverTime;
    CHC_DrawIntro(incarnation,epoch,time);if(!CHC_Current(incarnation,epoch,time))return;
    for(i=0;i<COD1_SCRIPT_HUD_CAPACITY;i++){
        cod1ScriptHud_t state=cgCod1HudElements.slot[i];float x,y,angle;
        if(!state.alive||!state.clock||!cgCod1HudElements.face||!cgCod1HudElements.needle)continue;
        if(!CHC_Current(incarnation,epoch,time))return;x=state.x-state.width*(state.align_x*.5f);y=state.y-state.height*(state.align_y*.5f);
        trap_R_SetColor(NULL);if(!CHC_Current(incarnation,epoch,time))return;CG_DrawPic(x,y,(float)state.width,(float)state.height,cgCod1HudElements.face);
        if(!CHC_Current(incarnation,epoch,time))return;
        angle=CH_Remaining(&state,time)/(float)state.total_ms*360.0f;
        CHC_Needle(x,y,(float)state.width,(float)state.height,angle,cgCod1HudElements.needle);
        if(!CHC_Current(incarnation,epoch,time))return;trap_R_SetColor(NULL);
    }
}
