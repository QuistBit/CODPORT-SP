/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cod_jump_land.h"
#include <math.h>

/* Original x87 PC24 operation order; do not compile with fast-math or FMA. */
static float add(float a,float b) { volatile float r=a+b; return r; }
static float sub(float a,float b) { volatile float r=a-b; return r; }
static float mul(float a,float b) { volatile float r=a*b; return r; }
static float divf(float a,float b) { volatile float r=a/b; return r; }
static float root(float a) { volatile float r=(float)sqrt((double)a); return r; }
static float dot(const float *a,const float *b) {
    return add(add(mul(a[2],b[2]),mul(a[1],b[1])),mul(a[0],b[0]));
}
static void normalize(float *v) {
    float length=root(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2])));
    int i;
    if(length==0) return;
    length=divf(1,length);
    for(i=0;i<3;i++) v[i]=mul(v[i],length);
}
static unsigned material(const cod_jump_land *s) {
    return s->surface_flags&0x2000 ? 0 : (s->surface_flags>>20)&31;
}
static void event(cod_jump_land *s,int ev,int parm) {
    unsigned slot=(unsigned)s->event_sequence&3;
    if(!ev) return;
    s->events[slot]=ev&255;
    s->event_params[slot]=parm&255;
    s->event_sequence++;
}
static int stance(const cod_jump_land *s) {
    if((s->flags&1) || s->view_target==s->prone_view ||
       (s->view_transition_time && s->view_target==s->crouch_view &&
        !s->view_transition_down)) return 1;
    if((s->flags&2) || (s->view_transition_time &&
        s->view_target==s->crouch_view)) return 2;
    return 0;
}
int cod_check_jump(cod_jump_land *s) {
    float f[3], reflected[3], d;
    unsigned mat;
    int i;
    if(s->gravity<=0) return -1;
    if((int32_t)((uint32_t)s->command_time-(uint32_t)s->jump_time)<500 ||
       (s->flags&(0x800|0x2000)) || stance(s) || s->upmove<10) return 0;
    if(s->flags&8) {s->upmove=0;return 0;}
    s->ground_plane=s->walking=0;
    s->flags|=8;
    s->ground_entity=2047;
    s->velocity[2]=root(mul((float)s->gravity,78));
    s->jump_apex=add(s->origin[2],39);
    /* 0x10 is LADDER, not a generic airborne/jumping flag. */
    if(s->flags&16) {
        s->velocity[2]=mul(s->velocity[2],.75f);
        f[0]=s->forward[0];f[1]=s->forward[1];f[2]=0;
        normalize(f);
        if(dot(s->forward,s->ladder_normal)<0) {
            d=mul(dot(f,s->ladder_normal),-2);
            for(i=0;i<3;i++) reflected[i]=add(mul(d,s->ladder_normal[i]),f[i]);
            normalize(reflected);
            s->velocity[0]=mul(128,reflected[0]);
            s->velocity[1]=mul(128,reflected[1]);
        } else {
            s->velocity[0]=mul(128,f[0]);
            s->velocity[1]=mul(128,f[1]);
        }
        s->flags&=~16u;
    }
    mat=material(s);
    event(s,mat?(int)mat+70:0,0);
    s->aim_spread=add(s->aim_spread,64);
    if(s->aim_spread>255) s->aim_spread=255;
    return 1;
}
int cod_crash_land(cod_jump_land *s,float *height) {
    float a,b,c,den,t,speed,h,scale;
    int damage=0,parm,timer,i;
    unsigned mat;
    if(s->water_level==3) return 0;
    if(s->gravity<=0) return -1;
    a=mul(-(float)s->gravity,.5f);
    b=s->previous_velocity[2];
    c=sub(s->previous_origin[2],s->origin[2]);
    den=sub(mul(b,b),mul(mul(c,a),4));
    if(den<0) return 0;
    t=divf(sub(-b,root(den)),add(a,a));
    speed=mul(add(b,mul(t,-(float)s->gravity)),-1);
    h=divf(mul(speed,speed),add((float)s->gravity,(float)s->gravity));
    if(height) *height=h;
    if(s->fall_max>s->fall_min && s->fall_min>0 && h>s->fall_min &&
       !(s->surface_flags&1) && s->pm_type<4) {
        damage=h>=s->fall_max ? 100 :
            (int)mul(divf(sub(h,s->fall_min),sub(s->fall_max,s->fall_min)),100);
        if(damage<0) damage=0;
        if(damage>100) damage=100;
    }
    if(s->water_level==2) damage=(int)mul((float)damage,.5f);
    parm=h<12 ? 0 : (int)mul(add(mul(sub(h,12),1.f/26.f),1),4);
    if(parm>24) parm=24;
    if(s->jump_apex<-.001f || s->jump_apex>.001f) {
        s->timer=200;s->flags|=0x2000;
    }
    mat=material(s);
    if(damage) {
        if(damage<100 && !(s->surface_flags&2)) {
            timer=damage*35+500;
            if(timer>2000) {timer=2000;scale=.2f;}
            else if(timer<=500) scale=.5f;
            else if(timer>=1500) scale=.2f;
            else scale=sub(.5f,mul(mul(sub((float)timer,500),.001f),.3f));
            s->timer=timer;s->flags|=0x100;
        } else scale=.67f;
        for(i=0;i<3;i++) s->velocity[i]=mul(s->velocity[i],scale);
        event(s,(int)mat+116,damage);
    } else if(h>4) {
        if(h<8) event(s,mat?(int)mat+24:0,0);
        else if(h<12) event(s,mat?(int)mat+1:0,0);
        else {
            for(i=0;i<3;i++) s->velocity[i]=mul(s->velocity[i],.67f);
            event(s,(int)mat+93,parm);
        }
    }
    return 1;
}
