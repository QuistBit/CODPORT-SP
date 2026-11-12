/* Original engine 44DA80 -> 41D3B0 -> 41CD60 -> 41C8D0/41B3E0.
 * Zero-size, nonrotated point only. 41C8D0 exchanges the capsule with a
 * zero-size brush; 41B8F0 clips its six axial bounds using capsule support.
 * This is the native plane test, including its endcap behavior, not an
 * analytic rounded-capsule replacement. Swept boxes and stationary position
 * tests remain unadmitted; the latter have a separate native caller pretest. */
#ifndef COD1_POINT_CAPSULE_H
#define COD1_POINT_CAPSULE_H
static void C1PC_MovingTrace(trace_t *out,const float start[3],const float end[3],
    const float mins[3],const float maxs[3],const float origin[3],int contents) {
    float a[3],b[3],center[3],support[3],radius,half,enter=0,leave=1;
    int i,side,axis=-1,sign=0,getout=0;
    memset(out,0,sizeof(*out));out->fraction=1;out->entityNum=ENTITYNUM_NONE;
    for(i=0;i<3;i++) {
        center[i]=(mins[i]+maxs[i])*0.5f;
        a[i]=(start[i]-origin[i])-center[i];
        b[i]=(end[i]-origin[i])-center[i];
    }
    radius=maxs[0]-center[0];half=maxs[2]-center[2];
    if(half<radius)radius=half;
    support[0]=support[1]=radius;support[2]=half;
    for(side=0;side<2;side++)for(i=0;i<3;i++) {
        float direction=side?1.0f:-1.0f;
        float d1=a[i]*direction-support[i],d2=b[i]*direction-support[i];
        float delta=d1-d2;
        if(d1<=0) {
            if(d2>0) {
                getout=1;
                if(leave*delta<d1) {
                    leave=d1/delta;
                    if(leave<=enter)goto finished;
                }
            }
        } else {
            if(d2>0) {
                if(delta<=0 || d2>=0.125f)goto finished;
                getout=1;
            }
            if(d1-0.125f>delta*enter) {
                enter=(d1-0.125f)/delta;
                if(leave<=enter)goto finished;
            } else if(axis>=0)continue;
            axis=i;sign=side?1:-1;
        }
    }
    out->contents=contents;
    if(axis<0) {
        out->startsolid=qtrue;
        if(!getout){out->allsolid=qtrue;out->fraction=0;}
    } else {
        out->fraction=enter;
        out->plane.normal[axis]=(float)sign;
        out->plane.type=(byte)axis;
        out->plane.signbits=(byte)(sign<0?(1<<axis):0);
        out->plane.dist=(origin[axis]+center[axis]+sign*support[axis])*sign;
    }
finished:
    for(i=0;i<3;i++)out->endpos[i]=start[i]+out->fraction*(end[i]-start[i]);
}
#endif
