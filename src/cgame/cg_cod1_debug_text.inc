/* R118 paired print3d consumer. The 100 ms server-clock lifetime, 32-entry
 * pool and billboard sizing are port policy; retail sink details are unknown.
 * Existing GUI glyph shaders are retained: these are world-positioned overlay
 * quads, deliberately visible through walls, not a depth-tested retail claim. */
#include "../game/bg_cod1_debug_text.h"

extern displayContextDef_t cgDC;

typedef struct {
    int active;
    cod1DebugText_t event;
} cod1DebugTextClientEntry_t;
typedef struct {
    uint64_t incarnation,epoch,operation_id;
    int time;
} cod1DebugTextClientLease_t;
static struct {
    uint64_t epoch,floor,operation_id,incarnation;
    int last_time,have_time,font_attempted,font_busy;
    cod1DebugTextClientEntry_t entry[COD1_DEBUG_TEXT_POOL];
    fontInfo_t font;
} cgCod1DebugText;

static void CCT_Advance(uint64_t *value) { if(!++*value)++*value; }

void CG_Cod1DebugTextReset(qboolean full) {
    CCT_Advance(&cgCod1DebugText.incarnation);
    memset(cgCod1DebugText.entry,0,sizeof(cgCod1DebugText.entry));
    cgCod1DebugText.last_time=0;cgCod1DebugText.have_time=0;
    cgCod1DebugText.font_busy=0;
    if(full) {
        cgCod1DebugText.epoch=0;cgCod1DebugText.floor=0;
        cgCod1DebugText.operation_id=0;cgCod1DebugText.font_attempted=0;
        memset(&cgCod1DebugText.font,0,sizeof(cgCod1DebugText.font));
    }
}

static cod1DebugTextClientLease_t CCT_Lease(void) {
    cod1DebugTextClientLease_t lease;
    lease.incarnation=cgCod1DebugText.incarnation;
    lease.epoch=cgCod1DebugText.epoch;
    lease.operation_id=cgCod1DebugText.operation_id;lease.time=cg.time;
    return lease;
}
static int CCT_Current(const cod1DebugTextClientLease_t *lease) {
    return lease && lease->incarnation==cgCod1DebugText.incarnation &&
        lease->epoch==cgCod1DebugText.epoch &&
        lease->operation_id==cgCod1DebugText.operation_id && lease->time==cg.time;
}
static int CCT_ObserveTime(void) {
    if(cg.time<0 || (cgCod1DebugText.have_time && cg.time<cgCod1DebugText.last_time)) {
        /* Clock rewinds retire drawn events but retain same-epoch replay history. */
        CG_Cod1DebugTextReset(qfalse);return 0;
    }
    cgCod1DebugText.last_time=cg.time;cgCod1DebugText.have_time=1;return 1;
}
static void CCT_Expire(void) {
    int i;
    for(i=0;i<COD1_DEBUG_TEXT_POOL;++i)
        if(cgCod1DebugText.entry[i].active &&
           (int64_t)cg.time-(int64_t)cgCod1DebugText.entry[i].event.server_time>=
               COD1_DEBUG_TEXT_LIFETIME_MS)
            cgCod1DebugText.entry[i].active=0;
}

void CG_Cod1DebugTextConfigChanged(int number) {
    cod1DebugTextClientLease_t lease;
    uint64_t epoch,floor;
    const char *text;
    if(number!=CS_COD1_DEBUG_TEXT_METADATA)return;
    lease=CCT_Lease();
    if(!CCT_Current(&lease))return;
    text=CG_ConfigString(number);
    if(!CCT_Current(&lease) || !text)return;
    if(!text[0]) {
        floor=cgCod1DebugText.floor;
        CG_Cod1DebugTextReset(qtrue);cgCod1DebugText.floor=floor;return;
    }
    /* A malformed retained marker never erases a valid scene or history. */
    if(!CTD_DecodeMetadata(text,&epoch) || epoch==cgCod1DebugText.epoch ||
       epoch<=cgCod1DebugText.floor)return;
    CG_Cod1DebugTextReset(qtrue);
    cgCod1DebugText.epoch=epoch;cgCod1DebugText.floor=epoch;
}

void CG_Cod1DebugTextCommand(void) {
    char copied[COD1_DEBUG_TEXT_ARGC-1][24];
    char payload[COD1_DEBUG_TEXT_CAPACITY*2];
    const char *arguments[COD1_DEBUG_TEXT_ARGC];
    cod1DebugTextClientLease_t lease;
    cod1DebugText_t event;
    int i,count,slot=-1;
    int64_t age;
    if(!cgCod1DebugText.epoch)return;
    lease=CCT_Lease();
    if(!CCT_Current(&lease))return;
    count=trap_Argc();
    if(!CCT_Current(&lease) || count!=COD1_DEBUG_TEXT_ARGC)return;
    for(i=0;i<count;++i) {
        char *out=i<count-1?copied[i]:payload;
        int capacity=i<count-1?(int)sizeof(copied[i]):(int)sizeof(payload);
        memset(out,0,capacity);
        if(!CCT_Current(&lease))return;
        trap_Argv(i,out,capacity);
        if(!CCT_Current(&lease))return;
        arguments[i]=out;
    }
    if(!CTD_DecodeCommand(count,arguments,&event) ||
       !CTD_Follows(&event,lease.epoch,lease.operation_id) || !CCT_Current(&lease))return;
    if(!CCT_ObserveTime())return;
    /* Consume valid events before deciding visual usefulness. Delayed packets
     * cannot restart a duration on arrival, nor reappear after map_restart. */
    cgCod1DebugText.operation_id=event.operation_id;
    CCT_Expire();age=(int64_t)cg.time-(int64_t)event.server_time;
    /* Up to one second of early delivery is queued; farther future stamps are
     * discarded rather than tying up the bounded debug pool indefinitely. */
    if(age>=COD1_DEBUG_TEXT_LIFETIME_MS || age < -1000 || !event.length || !event.scale)return;
    for(i=0;i<COD1_DEBUG_TEXT_POOL;++i) {
        if(!cgCod1DebugText.entry[i].active) {slot=i;break;}
        if(slot<0 || cgCod1DebugText.entry[i].event.operation_id<
            cgCod1DebugText.entry[slot].event.operation_id)slot=i;
    }
    cgCod1DebugText.entry[slot].event=event;cgCod1DebugText.entry[slot].active=1;
}

static int CCT_FontReady(const fontInfo_t *font) {
    int i;
    if(!CQ_Finite(font->glyphScale) || !font->glyphScale)return 0;
    for(i=32;i<=126;++i)
        if(font->glyphs[i].glyph>0 && font->glyphs[i].imageWidth>0 &&
           font->glyphs[i].imageHeight>0)return 1;
    return 0;
}
static const fontInfo_t *CCT_Font(const cod1DebugTextClientLease_t *lease) {
    fontInfo_t loaded;
    if(!CCT_Current(lease))return NULL;
    if(CCT_FontReady(&cgDC.Assets.textFont))return &cgDC.Assets.textFont;
    if(CCT_FontReady(&cgCod1DebugText.font))return &cgCod1DebugText.font;
    if(cgCod1DebugText.font_attempted || cgCod1DebugText.font_busy)return NULL;
    cgCod1DebugText.font_busy=1;memset(&loaded,0,sizeof(loaded));
    if(!CCT_Current(lease))return NULL;
    /* Native HUD uses this same renderer-backed CoD font conversion. The
     * local result cannot overwrite a new lifecycle after a reentrant trap. */
    trap_R_RegisterFont("fonts/normalFont",16,&loaded);
    if(!CCT_Current(lease)) {
        if(lease->incarnation==cgCod1DebugText.incarnation &&
           lease->epoch==cgCod1DebugText.epoch)cgCod1DebugText.font_busy=0;
        return NULL;
    }
    cgCod1DebugText.font_busy=0;cgCod1DebugText.font_attempted=1;
    cgCod1DebugText.font=loaded;
    return CCT_FontReady(&cgCod1DebugText.font)?&cgCod1DebugText.font:NULL;
}
static int CCT_FloatGeometry(double value,float *out) {
    if(!(value>=-(double)FLT_MAX && value<=(double)FLT_MAX))return 0;
    *out=(float)value;return CQ_Finite(*out);
}
static int CCT_Quad(const cod1DebugText_t *event,const glyphInfo_t *glyph,
    double cursor,double scale,const double right[3],const double up[3],polyVert_t vertices[4]) {
    double x[4],y[4];
    unsigned char color[4];
    int i,j;
    if(glyph->glyph<=0 || glyph->imageWidth<=0 || glyph->imageHeight<=0 ||
       !CQ_Finite(glyph->s) || !CQ_Finite(glyph->t) ||
       !CQ_Finite(glyph->s2) || !CQ_Finite(glyph->t2))return 0;
    x[0]=x[1]=cursor;x[2]=x[3]=cursor+(double)glyph->imageWidth*scale;
    y[0]=y[3]=(double)glyph->top*scale;
    y[1]=y[2]=y[0]-(double)glyph->imageHeight*scale;
    for(i=0;i<4;++i) {
        double component=event->color[i];
        if(component<0.0)component=0.0;else if(component>1.0)component=1.0;
        color[i]=(unsigned char)(component*255.0);
    }
    memset(vertices,0,sizeof(polyVert_t)*4);
    for(i=0;i<4;++i) {
        for(j=0;j<3;++j)
            if(!CCT_FloatGeometry((double)event->origin[j]+right[j]*x[i]+up[j]*y[i],
                &vertices[i].xyz[j]))return 0;
        memcpy(vertices[i].modulate,color,sizeof(color));
        vertices[i].st[0]=i<2?glyph->s:glyph->s2;
        vertices[i].st[1]=(i==0 || i==3)?glyph->t:glyph->t2;
    }
    return 1;
}

void CG_Cod1DebugTextAdd(void) {
    cod1DebugTextClientLease_t lease;
    const fontInfo_t *font;
    double right[3],up[3];
    int i,j,visible=0;
    if(!CCT_ObserveTime() || !cgCod1DebugText.epoch)return;
    CCT_Expire();
    for(i=0;i<COD1_DEBUG_TEXT_POOL;++i)
        if(cgCod1DebugText.entry[i].active &&
           cg.time>=cgCod1DebugText.entry[i].event.server_time)visible=1;
    if(!visible)return;
    lease=CCT_Lease();
    for(i=0;i<3;++i) {
        if(!CQ_Finite(cg.refdef.viewaxis[1][i]) || !CQ_Finite(cg.refdef.viewaxis[2][i]))return;
        right[i]=-(double)cg.refdef.viewaxis[1][i];up[i]=cg.refdef.viewaxis[2][i];
    }
    font=CCT_Font(&lease);
    if(!font || !CCT_Current(&lease))return;
    for(i=0;i<COD1_DEBUG_TEXT_POOL;++i) {
        cod1DebugText_t event;
        double scale,cursor=0.0;
        if(!CCT_Current(&lease))return;
        if(!cgCod1DebugText.entry[i].active ||
           cg.time<cgCod1DebugText.entry[i].event.server_time)continue;
        event=cgCod1DebugText.entry[i].event;
        scale=(double)event.scale*(double)font->glyphScale;
        for(j=0;j<(int)event.length;++j) {
            glyphInfo_t glyph=font->glyphs[(unsigned char)event.text[j]];
            polyVert_t vertices[4];
            /* Carets are printable glyphs; no Q_IsColorString interpretation. */
            if(CCT_Quad(&event,&glyph,cursor,scale,right,up,vertices)) {
                if(!CCT_Current(&lease))return;
                trap_R_AddPolyToScene(glyph.glyph,4,vertices);
                if(!CCT_Current(&lease))return;
            }
            cursor+=(double)glyph.xSkip*scale;
        }
    }
}
