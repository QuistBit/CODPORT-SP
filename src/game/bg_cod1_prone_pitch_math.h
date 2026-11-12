/* Shared native CoD1 SP prone pitch: cgame 0x30005c0b, 0x30006c6c,
 * 0x30006f30 and 0x30031190; game 0x200064c9, 0x200075b7,
 * 0x20007880 and 0x20011860. Verified against both original DLLs.
 * Callers supply the normal-projected ground pitch (native 0x30031190).
 * All movement states live in the shared playerState; render-only calls must
 * not advance the centre or initialise it from stale ground information.
 */
#ifndef BG_COD1_PRONE_PITCH_MATH_H
#define BG_COD1_PRONE_PITCH_MATH_H
#include <math.h>
#include <stdint.h>

static float COD1_PP_Add(float a,float b) { volatile float r=a+b;return r; }
static float COD1_PP_Sub(float a,float b) { volatile float r=a-b;return r; }
static float COD1_PP_Mul(float a,float b) { volatile float r=a*b;return r; }
static float COD1_PP_Div(float a,float b) { volatile float r=a/b;return r; }
static float COD1_PP_GroundPitch(float yaw,const float normal[3]) {
    float radians=COD1_PP_Mul(yaw,.01745329238474369f);
    float forward[3],projected[3],n[3],length_squared,inverse,d,horizontal,pitch;
    int i;
    forward[0]=(float)cos((double)radians);
    forward[1]=(float)sin((double)radians);
    forward[2]=0.f;
    length_squared=COD1_PP_Add(COD1_PP_Add(COD1_PP_Mul(normal[0],normal[0]),
        COD1_PP_Mul(normal[1],normal[1])),COD1_PP_Mul(normal[2],normal[2]));
    inverse=COD1_PP_Div(1.f,length_squared);
    d=COD1_PP_Add(COD1_PP_Add(COD1_PP_Mul(forward[2],normal[2]),
        COD1_PP_Mul(forward[1],normal[1])),COD1_PP_Mul(forward[0],normal[0]));
    d=COD1_PP_Mul(d,inverse);
    for(i=0;i<3;i++) {
        n[i]=COD1_PP_Mul(normal[i],inverse);
        projected[i]=COD1_PP_Sub(forward[i],COD1_PP_Mul(d,n[i]));
    }
    if(projected[0]==0.f && projected[1]==0.f)return projected[2]>0.f?270.f:90.f;
    horizontal=sqrtf(COD1_PP_Add(COD1_PP_Mul(projected[0],projected[0]),
        COD1_PP_Mul(projected[1],projected[1])));
    pitch=(float)(atan2((double)projected[2],(double)horizontal)*-180.0);
    pitch=COD1_PP_Div(pitch,3.1415927410125732f);
    return pitch<0.f?COD1_PP_Add(pitch,360.f):pitch;
}
static float COD1_PP_Normalise(float angle) {
    int packed=(int)COD1_PP_Mul(angle,182.04444885253906f);
    float unpacked=COD1_PP_Mul((float)((unsigned)packed&65535u),.0054931640625f);
    return unpacked>180.f?COD1_PP_Sub(unpacked,360.f):unpacked;
}
static float COD1_PP_Initialise(float view_pitch,float ground_pitch) {
    float difference=COD1_PP_Normalise(COD1_PP_Sub(ground_pitch,view_pitch));
    if(difference < -45.f)return COD1_PP_Sub(view_pitch,45.f);
    if(difference > 45.f)return COD1_PP_Add(view_pitch,45.f);
    /* Original retains the raw 0..360 ground pitch on initialisation. */
    return ground_pitch;
}
static float COD1_PP_Tick(float centre,float ground_pitch,float frame_time) {
    float difference=COD1_PP_Normalise(COD1_PP_Sub(ground_pitch,centre));
    float distance=COD1_PP_Mul(frame_time,70.f);
    if(difference==0.f)return centre;
    if(fabsf(difference)>distance)difference=difference<0.f?-distance:distance;
    return COD1_PP_Normalise(COD1_PP_Add(centre,difference));
}
static int COD1_PP_Clamp(float centre,float *view_pitch,int *delta_pitch) {
    float difference=COD1_PP_Normalise(COD1_PP_Sub(centre,*view_pitch));
    float correction,boundary;
    int packed;
    if(!(COD1_PP_Normalise(difference)>45.f || difference < -45.f))return 0;
    correction=difference>45.f?COD1_PP_Sub(difference,45.f):COD1_PP_Add(difference,45.f);
    packed=(int)COD1_PP_Mul(correction,182.04444885253906f);
    /* Native addition is unsigned 32-bit, with an unsigned 16-bit correction;
     * assigning boundary - cmd discards the original wrapping semantics. */
    *delta_pitch=(int)((uint32_t)*delta_pitch+((uint32_t)packed&65535u));
    boundary=correction>0.f?COD1_PP_Sub(centre,45.f):COD1_PP_Add(centre,45.f);
    *view_pitch=COD1_PP_Normalise(boundary);
    return 1;
}
#endif
