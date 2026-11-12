/* Native CoD1 cgame compass integrators: 0x30012b00 (dial) and 0x30012e30
 * (optional pointer spring). Source evidence and constants are in
 * docs/reverse/CoD1_HUD_COMPASS_SPRING_DISASM.md. */
typedef struct { float yaw, velocity; int time, valid; } codCompass_t;

/* Original code quantizes angles through 16-bit short-angle units. */
static float CodCompass_Short360(float angle) {
    unsigned short packed=(unsigned short)((int)(angle*182.04444885253906f));
    return packed*0.0054931640625f;
}
static float CodCompass_Short180(float angle) {
    float wrapped=CodCompass_Short360(angle);
    return wrapped>=180.0f?wrapped-360.0f:wrapped;
}
/* CoD's AngleSubtract chooses +180 when the delta equals -180. */
static float CodCompass_Difference(float a,float b) {
    float d=a-b;
    while(d>=180.0f)d-=360.0f;
    while(d<=-180.0f)d+=360.0f;
    return d;
}
static float CodCompass_Step(float error,float *velocity,float dt,int pointer) {
    const float accel=pointer?1500.0f:1000.0f;
    const float drag=pointer?3.0f:2.0f;
    const float outwardDrag=pointer?5.0f:3.5f;
    const float dryFriction=pointer?2.0f:1.0f;
    const float limit=pointer?2000.0f:30000.0f;
    float v=*velocity;
    error=pointer?CodCompass_Short180(error+v*dt):CodCompass_Short360(error+v*dt);
    if(!pointer && error>180.0f)error-=360.0f;
    if(error>0.0f)v-=accel*dt;
    else if(error<0.0f)v+=accel*dt;
    v-=drag*v*dt;
    if(v>0.0f) {
        if(error>0.0f)v-=outwardDrag*v*dt;
        v-=dryFriction*dt;
        if(v<0.0f)v=0.0f;
    } else {
        if(error<0.0f)v-=outwardDrag*v*dt;
        v+=dryFriction*dt;
        if(v>0.0f)v=0.0f;
    }
    if(v>limit)v=limit;
    if(v< -limit)v= -limit;
    *velocity=v;
    return error;
}
static float CodCompass_Update(codCompass_t *s,float target,int time) {
    int delta,step;
    float error;
    target=CodCompass_Short360(target);
    if(!s->valid || time<s->time || time-s->time>=500) {
        s->yaw=target;s->velocity=0;s->time=time;s->valid=1;
        return s->yaw;
    }
    delta=time-s->time;s->time=time;
    error=CodCompass_Difference(s->yaw,target);
    while(delta>0) {
        if(fabs(error)<.25f && fabs(s->velocity)<1.0f) {
            s->yaw=target;s->velocity=0;
            return s->yaw;
        }
        step=delta>5?5:delta;
        error=CodCompass_Step(error,&s->velocity,step*.001f,0);
        delta-=step;
    }
    s->yaw=CodCompass_Short360(error+target);
    return s->yaw;
}
static float CodCompass_UpdatePointer(codCompass_t *s,float viewYaw,int time,
                                      int springEnabled) {
    int delta,step;
    float error;
    if(!springEnabled) {
        /* Retail does not advance the optional pointer spring while off. */
        s->yaw=viewYaw;
        return viewYaw;
    }
    if(!s->valid || time<s->time || time-s->time>=500) {
        s->yaw=viewYaw;s->velocity=0;s->time=time;s->valid=1;
        return s->yaw;
    }
    delta=time-s->time;
    if(delta==0)return s->yaw;
    s->time=time;
    error=CodCompass_Difference(s->yaw,viewYaw);
    if(error>10.0f)error=10.0f;
    else if(error< -10.0f)error= -10.0f;
    while(delta>0) {
        if(fabs(error)<.5f && fabs(s->velocity)<2.0f) {
            s->yaw=viewYaw;s->velocity=0;
            return s->yaw;
        }
        step=delta>5?5:delta;
        error=CodCompass_Step(error,&s->velocity,step*.001f,1);
        delta-=step;
    }
    s->yaw=CodCompass_Short360(error+viewYaw);
    return s->yaw;
}
