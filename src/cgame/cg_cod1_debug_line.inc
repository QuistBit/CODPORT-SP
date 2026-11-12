/* R119 paired script line consumer. Fixed one-world-unit full width, 100 ms
 * server-clock lifetime and the bounded pool are selected port policy.
 * Existing white GUI shader supplies world-positioned overlay ribbons;
 * no retail sink, depth-test or duration equivalence is asserted. The native
 * optional integer is transported, but only zero has an admitted port policy. */
#include "../game/bg_cod1_debug_line.h"

typedef struct {
    int active;
    cod1DebugLine_t event;
} cod1DebugLineClientEntry_t;
typedef struct {
    uint64_t incarnation,epoch,operation_id;
    int time;
} cod1DebugLineClientLease_t;
static struct {
    uint64_t epoch,floor,operation_id,incarnation;
    int last_time,have_time;
    cod1DebugLineClientEntry_t entry[COD1_DEBUG_LINE_POOL];
} cgCod1DebugLine;

static void CCDL_Advance(uint64_t *value) { if(!++*value)++*value; }

void CG_Cod1DebugLineReset(qboolean full) {
    CCDL_Advance(&cgCod1DebugLine.incarnation);
    memset(cgCod1DebugLine.entry,0,sizeof(cgCod1DebugLine.entry));
    cgCod1DebugLine.last_time=0;cgCod1DebugLine.have_time=0;
    if(full) {
        cgCod1DebugLine.epoch=0;cgCod1DebugLine.floor=0;
        cgCod1DebugLine.operation_id=0;
    }
}
static cod1DebugLineClientLease_t CCDL_Lease(void) {
    cod1DebugLineClientLease_t lease;
    lease.incarnation=cgCod1DebugLine.incarnation;
    lease.epoch=cgCod1DebugLine.epoch;
    lease.operation_id=cgCod1DebugLine.operation_id;lease.time=cg.time;
    return lease;
}
static int CCDL_Current(const cod1DebugLineClientLease_t *lease) {
    return lease && lease->incarnation==cgCod1DebugLine.incarnation &&
        lease->epoch==cgCod1DebugLine.epoch &&
        lease->operation_id==cgCod1DebugLine.operation_id && lease->time==cg.time;
}
static int CCDL_ObserveTime(void) {
    if(cg.time<0 || (cgCod1DebugLine.have_time && cg.time<cgCod1DebugLine.last_time)) {
        /* A rewind retires visible events without losing replay history. */
        CG_Cod1DebugLineReset(qfalse);return 0;
    }
    cgCod1DebugLine.last_time=cg.time;cgCod1DebugLine.have_time=1;return 1;
}
static void CCDL_Expire(void) {
    int i;
    for(i=0;i<COD1_DEBUG_LINE_POOL;++i)
        if(cgCod1DebugLine.entry[i].active &&
           (int64_t)cg.time-(int64_t)cgCod1DebugLine.entry[i].event.server_time>=
               COD1_DEBUG_LINE_LIFETIME_MS)
            cgCod1DebugLine.entry[i].active=0;
}

void CG_Cod1DebugLineConfigChanged(int number) {
    cod1DebugLineClientLease_t lease;
    uint64_t epoch,floor;
    const char *text;
    if(number!=CS_COD1_DEBUG_LINE_METADATA)return;
    lease=CCDL_Lease();
    if(!CCDL_Current(&lease))return;
    text=CG_ConfigString(number);
    if(!CCDL_Current(&lease) || !text)return;
    if(!text[0]) {
        floor=cgCod1DebugLine.floor;
        CG_Cod1DebugLineReset(qtrue);cgCod1DebugLine.floor=floor;return;
    }
    if(!CDL_DecodeMetadata(text,&epoch) || epoch==cgCod1DebugLine.epoch ||
       epoch<=cgCod1DebugLine.floor)return;
    CG_Cod1DebugLineReset(qtrue);
    cgCod1DebugLine.epoch=epoch;cgCod1DebugLine.floor=epoch;
}

void CG_Cod1DebugLineCommand(void) {
    char copied[COD1_DEBUG_LINE_ARGC][24];
    const char *arguments[COD1_DEBUG_LINE_ARGC];
    cod1DebugLineClientLease_t lease;
    cod1DebugLine_t event;
    int i,count,slot=-1;
    int64_t age;
    if(!cgCod1DebugLine.epoch)return;
    lease=CCDL_Lease();
    if(!CCDL_Current(&lease))return;
    count=trap_Argc();
    if(!CCDL_Current(&lease) || count!=COD1_DEBUG_LINE_ARGC)return;
    for(i=0;i<count;++i) {
        memset(copied[i],0,sizeof(copied[i]));
        if(!CCDL_Current(&lease))return;
        trap_Argv(i,copied[i],sizeof(copied[i]));
        if(!CCDL_Current(&lease))return;
        arguments[i]=copied[i];
    }
    if(!CDL_DecodeCommand(count,arguments,&event) || event.optional_integer ||
       !CDL_Follows(&event,lease.epoch,lease.operation_id) || !CCDL_Current(&lease))return;
    if(!CCDL_ObserveTime())return;
    /* Unsupported nonzero integers were refused above, before consumption.
     * Valid zero events consume their token even if delayed or degenerate. */
    cgCod1DebugLine.operation_id=event.operation_id;
    CCDL_Expire();age=(int64_t)cg.time-(int64_t)event.server_time;
    if(age>=COD1_DEBUG_LINE_LIFETIME_MS || age < -1000)return;
    for(i=0;i<COD1_DEBUG_LINE_POOL;++i) {
        if(!cgCod1DebugLine.entry[i].active) {slot=i;break;}
        if(slot<0 || cgCod1DebugLine.entry[i].event.operation_id<
            cgCod1DebugLine.entry[slot].event.operation_id)slot=i;
    }
    cgCod1DebugLine.entry[slot].event=event;cgCod1DebugLine.entry[slot].active=1;
}

static int CCDL_Unit(const double input[3],double out[3]) {
    double largest=0.0,squared=0.0,divisor,scaled[3];
    int i;
    for(i=0;i<3;++i) {
        double magnitude;
        if(!(input[i]>=-DBL_MAX && input[i]<=DBL_MAX))return 0;
        magnitude=fabs(input[i]);if(magnitude>largest)largest=magnitude;
    }
    if(!(largest>0.0))return 0;
    for(i=0;i<3;++i) {scaled[i]=input[i]/largest;squared+=scaled[i]*scaled[i];}
    divisor=sqrt(squared);
    for(i=0;i<3;++i)out[i]=scaled[i]/divisor;
    return 1;
}
static void CCDL_Cross(const double a[3],const double b[3],double out[3]) {
    out[0]=a[1]*b[2]-a[2]*b[1];
    out[1]=a[2]*b[0]-a[0]*b[2];
    out[2]=a[0]*b[1]-a[1]*b[0];
}
static int CCDL_FloatGeometry(double value,float *out) {
    if(!(value>=-(double)FLT_MAX && value<=(double)FLT_MAX))return 0;
    *out=(float)value;return CQ_Finite(*out);
}
static int CCDL_Quad(const cod1DebugLine_t *event,const double camera[3],
    const double up[3],const double right[3],polyVert_t vertices[4]) {
    double segment[3],direction[3],towards[3],view[3],cross[3],side[3];
    double half=(double)COD1_DEBUG_LINE_WIDTH*0.5;
    unsigned char color[4];
    int i,j,have_side=0;
    if(!CDL_Validate(event) || event->optional_integer)return 0;
    for(i=0;i<3;++i) {
        segment[i]=(double)event->end[i]-(double)event->start[i];
        towards[i]=camera[i]-((double)event->start[i]+(double)event->end[i])*0.5;
    }
    if(!CCDL_Unit(segment,direction))return 0;
    if(CCDL_Unit(towards,view)) {
        CCDL_Cross(direction,view,cross);have_side=CCDL_Unit(cross,side);
    }
    /* Parallel sight lines and a camera at the midpoint use a finite view
     * axis without inventing length for a zero-length segment. */
    if(!have_side && CCDL_Unit(up,view)) {
        CCDL_Cross(direction,view,cross);have_side=CCDL_Unit(cross,side);
    }
    if(!have_side && CCDL_Unit(right,view)) {
        CCDL_Cross(direction,view,cross);have_side=CCDL_Unit(cross,side);
    }
    if(!have_side)return 0;
    for(i=0;i<4;++i) {
        double component=event->color[i];
        if(component<0.0)component=0.0;else if(component>1.0)component=1.0;
        color[i]=(unsigned char)(component*255.0);
    }
    memset(vertices,0,sizeof(polyVert_t)*4);
    /* start+side -> end+side -> end-side -> start-side. Its normal points
     * towards the camera whenever the sight line is not parallel. */
    for(i=0;i<4;++i) {
        const float *point=(i==1 || i==2)?event->end:event->start;
        double sign=i<2?half:-half;
        for(j=0;j<3;++j)
            if(!CCDL_FloatGeometry((double)point[j]+side[j]*sign,&vertices[i].xyz[j]))return 0;
        memcpy(vertices[i].modulate,color,sizeof(color));
        vertices[i].st[0]=(i==1 || i==2)?1.0f:0.0f;
        vertices[i].st[1]=i<2?0.0f:1.0f;
    }
    return 1;
}

void CG_Cod1DebugLineAdd(void) {
    cod1DebugLineClientLease_t lease;
    double camera[3],up[3],right[3];
    qhandle_t shader;
    int i,j;
    if(!CCDL_ObserveTime() || !cgCod1DebugLine.epoch)return;
    CCDL_Expire();lease=CCDL_Lease();shader=cgs.media.whiteShader;
    if(shader<=0)return;
    for(i=0;i<3;++i) {
        if(!CQ_Finite(cg.refdef.vieworg[i]))return;
        for(j=0;j<3;++j)if(!CQ_Finite(cg.refdef.viewaxis[j][i]))return;
        camera[i]=(double)cg.refdef.vieworg[i];
        up[i]=(double)cg.refdef.viewaxis[2][i];
        right[i]=-(double)cg.refdef.viewaxis[1][i];
    }
    for(i=0;i<COD1_DEBUG_LINE_POOL;++i) {
        cod1DebugLine_t event;polyVert_t vertices[4];
        if(!CCDL_Current(&lease))return;
        if(!cgCod1DebugLine.entry[i].active ||
           cg.time<cgCod1DebugLine.entry[i].event.server_time)continue;
        event=cgCod1DebugLine.entry[i].event;
        if(!CCDL_Quad(&event,camera,up,right,vertices))continue;
        if(!CCDL_Current(&lease))return;
        trap_R_AddPolyToScene(shader,4,vertices);
        if(!CCDL_Current(&lease))return;
    }
}
