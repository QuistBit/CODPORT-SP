/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cod_step.h"
#include <math.h>
#include <string.h>

/* Preserve the original PC24 arithmetic order. Never build with fast-math/FMA.
 * Volatile stores also prevent x87 excess precision on 32-bit hosts. */
static float add(float a,float b) { volatile float r=a+b; return r; }
static float sub(float a,float b) { volatile float r=a-b; return r; }
static float mul(float a,float b) { volatile float r=a*b; return r; }
static float divf(float a,float b) { volatile float r=a/b; return r; }
static void copy(const float *a,float *b) { memcpy(b,a,3*sizeof(float)); }
static void zero(float *a) { a[0]=a[1]=a[2]=0; }
static float dot(const float *a,const float *b,int i,int j,int k) {
    return add(add(mul(a[i],b[i]),mul(a[j],b[j])),mul(a[k],b[k]));
}
static void normalize(const float *a,float *b) {
    float length=sqrtf(dot(a,a,0,1,2)); int i;
    if(length==0) {copy(a,b);return;}
    length=divf(1,length);
    for(i=0;i<3;i++) b[i]=mul(a[i],length);
}
static void clip(const float *v,const float *n,float *out,int a,int b,int c) {
    float d=dot(v,n,a,b,c); int i;
    d=d<0?mul(d,1.001f):divf(d,1.001f);
    for(i=0;i<3;i++) out[i]=sub(v[i],mul(d,n[i]));
}
static void touch(cod_move *m,int entity) {
    int i;
    if(entity==2046 || m->touch_count>=32) return;
    for(i=0;i<m->touch_count;i++) if(m->touches[i]==entity) return;
    m->touches[m->touch_count++]=entity;
}
static void player_trace(cod_move *m,cod_trace_fn fn,void *ctx,
                         cod_trace *tr,const float *start,const float *end) {
    uint32_t mask=m->trace_mask;
    fn(ctx,tr,start,m->mins,m->maxs,end,m->client,mask);
    if(tr->startsolid && (tr->contents&0xc000)) {
        touch(m,tr->entity); m->trace_mask &= ~0xc000u; mask &= ~0xc000u;
        fn(ctx,tr,start,m->mins,m->maxs,end,m->client,mask);
    }
}
static void event(cod_move *m,int ev,int parm) {
    unsigned slot=(unsigned)m->event_sequence&3;
    m->events[slot]=ev;m->event_params[slot]=parm;m->event_sequence++;
}
static void step_footstep(cod_move *m,int old_bob) {
    unsigned material; int ev;
    if(!(((old_bob+64)^(m->bob_cycle+64))&128)) return;
    /* Called only with a ground entity by cod_step. */
    if(m->water_level==0) {
        material=(m->ground_surface_flags>>20)&31;
        ev=(m->ground_surface_flags&0x2000)||!material ? 0 :
            material + ((m->flags&1)?47:((m->flags&128)?24:1));
        event(m,ev,0);
    } else if(m->water_level==1 || m->water_level==2) {
        event(m,(m->flags&1)?0x43:((m->flags&128)?0x2c:0x15),0);
    }
}
int cod_slide(cod_move *m,int gravity,cod_trace_fn fn,void *ctx) {
    float primal[3],end_velocity[3]={0},planes[8][3],end[3];
    float clipped[3],end_clipped[3],crease[3],time_left=m->frame_time,into,d;
    int count=0,bump,i,j,k,axis; cod_trace tr;
    copy(m->velocity,primal);
    if(gravity) {
        copy(m->velocity,end_velocity);
        end_velocity[2]=sub(end_velocity[2],mul((float)m->gravity,m->frame_time));
        m->velocity[2]=mul(add(end_velocity[2],m->velocity[2]),.5f);
        primal[2]=end_velocity[2];
        if(m->ground_plane) clip(m->velocity,m->ground_normal,m->velocity,1,2,0);
    }
    if(m->ground_plane) copy(m->ground_normal,planes[count++]);
    normalize(m->velocity,planes[count++]);
    for(bump=0;bump<4;bump++) {
        for(axis=0;axis<3;axis++) end[axis]=add(m->origin[axis],mul(time_left,m->velocity[axis]));
        player_trace(m,fn,ctx,&tr,m->origin,end);
        if(tr.allsolid) {m->velocity[2]=0;return 1;}
        if(tr.fraction>0) copy(tr.end,m->origin);
        if(tr.fraction==1) break;
        touch(m,tr.entity);
        time_left=sub(time_left,mul(tr.fraction,time_left));
        if(count>=8) {zero(m->velocity);return 1;}
        for(i=0;i<count;i++) {
            if(dot(tr.normal,planes[i],2,0,1)>.999f) {
                for(axis=0;axis<3;axis++) m->velocity[axis]=add(m->velocity[axis],tr.normal[axis]);
                break;
            }
        }
        if(i<count) continue;
        copy(tr.normal,planes[count++]);
        for(i=0;i<count;i++) {
            into=dot(m->velocity,planes[i],0,2,1);
            if((double)into>=.1) continue;
            if(-into>m->impact_speed) m->impact_speed=-into;
            clip(m->velocity,planes[i],clipped,0,1,2);
            clip(end_velocity,planes[i],end_clipped,0,2,1);
            for(j=0;j<count;j++) {
                if(j==i || (double)dot(clipped,planes[j],1,0,2)>=.1) continue;
                clip(clipped,planes[j],clipped,1,0,2);
                clip(end_clipped,planes[j],end_clipped,1,0,2);
                if(dot(clipped,planes[i],0,2,1)>=0) continue;
                for(axis=0;axis<3;axis++) {
                    int a=(axis+1)%3,b=(axis+2)%3;
                    crease[axis]=sub(mul(planes[i][a],planes[j][b]),mul(planes[i][b],planes[j][a]));
                }
                normalize(crease,crease);
                d=dot(crease,m->velocity,2,1,0);
                for(axis=0;axis<3;axis++) clipped[axis]=mul(d,crease[axis]);
                d=dot(crease,end_velocity,2,1,0);
                for(axis=0;axis<3;axis++) end_clipped[axis]=mul(d,crease[axis]);
                for(k=0;k<count;k++) {
                    if(k!=i && k!=j && (double)dot(clipped,planes[k],2,0,1)<.1) {
                        zero(m->velocity);return 1;
                    }
                }
            }
            copy(clipped,m->velocity);copy(end_clipped,end_velocity);break;
        }
    }
    if(gravity) copy(end_velocity,m->velocity);
    if(m->timer) copy(primal,m->velocity);
    return bump!=0;
}
/* Original SP 20002b60 is a terrain-pose sampler. Its non-debug paths
 * ALWAYS return true; it is not the prone obstruction test its caller suggests.
 * Two radius-6 sphere probes update torso/waist pitch. No acceptance stub. */
static float angle180(float a) {
    int packed=(int)mul(a,182.04444885253906f);
    a=mul((float)((unsigned)packed&65535),.0054931640625f);
    return a>180?sub(a,360):a;
}
static float angle_difference(float a,float b) {
    float d=sub(a,b);
    while(d>180) d=sub(d,360);
    while(d< -180) d=add(d,360);
    return d;
}
static float vector_pitch(const float *v) {
    float horizontal,pitch;
    if(v[0]==0 && v[1]==0) return v[2]>0?270.f:90.f;
    horizontal=sqrtf(add(mul(v[0],v[0]),mul(v[1],v[1])));
    /* atan2 returns the transcendental result before the original PC24 multiply. */
    pitch=(float)(atan2((double)v[2],(double)horizontal)*-180.0);
    pitch=divf(pitch,3.1415927410125732f);
    return pitch<0?add(pitch,360):pitch;
}
static void prone_slope(cod_move *m,cod_trace_fn fn,void *ctx) {
    float direction[3],start[3],end[3],torso[3],waist[3],delta[3];
    const float mins[3]={-6,-6,-6},maxs[3]={6,6,6};
    float radians=mul(sub(m->prone_yaw,180),.01745329238474369f);
    float torso_pitch,waist_pitch,diff;cod_trace tr;int i;
    direction[0]=(float)cos((double)radians);direction[1]=(float)sin((double)radians);direction[2]=-0.f;
    for(i=0;i<3;i++) start[i]=add(m->origin[i],mul(direction[i],24));
    copy(start,end);end[2]=sub(end[2],30);start[2]=add(start[2],30);
    fn(ctx,&tr,start,mins,maxs,end,m->client,0x810011);
    copy(tr.fraction<1?tr.end:end,torso);torso[2]=sub(torso[2],6);
    if(tr.allsolid || tr.startsolid) torso[2]=m->origin[2];
    for(i=0;i<3;i++) start[i]=add(m->origin[i],mul(direction[i],54));
    copy(start,end);end[2]=sub(torso[2],30);start[2]=add(start[2],30);
    fn(ctx,&tr,start,mins,maxs,end,m->client,0x810011);
    copy(tr.fraction<1?tr.end:end,waist);waist[2]=sub(waist[2],6);
    if(tr.allsolid || tr.startsolid) waist[2]=m->origin[2];
    for(i=0;i<3;i++) delta[i]=sub(m->origin[i],torso[i]);
    torso_pitch=angle180(vector_pitch(delta));
    for(i=0;i<3;i++) delta[i]=sub(torso[i],waist[i]);
    waist_pitch=angle180(vector_pitch(delta));
    diff=angle_difference(torso_pitch,waist_pitch);
    if(diff< -50) waist_pitch=angle180(sub(torso_pitch,-50));
    else if(diff>70) waist_pitch=angle180(sub(torso_pitch,70));
    m->prone_height=sub(m->origin[2],m->origin[2]);
    m->prone_torso_pitch=torso_pitch;m->prone_waist_pitch=waist_pitch;
}

int cod_step(cod_move *m,int gravity,cod_trace_fn fn,void *ctx) {
    float start[3],start_v[3],lower[3],lower_v[3],end[3],delta[2];
    float height,used=0,score,lower_score,remaining,dz,scale;
    int ground,jump=0,blocked,axis,n,old_bob,advance;cod_trace tr;
    ground=m->ground_plane && !(m->flags&16);
    copy(m->origin,start);copy(m->velocity,start_v);
    blocked=cod_slide(m,gravity,fn,ctx);
    height=(m->flags&1)?10.f:18.f;
    if(blocked && (m->jump_apex<-.001f || m->jump_apex>.001f)
        && m->ground_entity==2047 && start[2]<m->jump_apex) {
        height=18;
        if(add(start[2],height)>m->jump_apex) {
            height=sub(m->jump_apex,start[2]);
            if(height<1) return 0;
        }
        jump=1;
    } else if(m->ground_entity==2047 && (!(m->flags&16) || m->velocity[2]<=0)) return 0;
    copy(m->origin,lower);copy(m->velocity,lower_v);
    delta[0]=sub(lower[0],start[0]);delta[1]=sub(lower[1],start[1]);
    if(blocked) {
        copy(start,end);end[2]=add(add(height,start[2]),1);
        player_trace(m,fn,ctx,&tr,start,end);
        used=sub(mul(add(height,1),tr.fraction),1);
        if(used<1) used=0;
        else {
            copy(start,m->origin);m->origin[2]=add(start[2],used);
            copy(start_v,m->velocity);cod_slide(m,gravity,fn,ctx);
        }
    }
    if(ground || used!=0) {
        copy(m->origin,end);end[2]=sub(end[2],used);
        if(ground) end[2]=sub(end[2],mul(height,.5f));
        player_trace(m,fn,ctx,&tr,m->origin,end);
        if(tr.entity<4) {copy(lower,m->origin);copy(lower_v,m->velocity);return 0;}
        if(tr.fraction<1) {copy(tr.end,m->origin);clip(m->velocity,tr.normal,m->velocity,1,2,0);}
        else if(used!=0) m->origin[2]=sub(m->origin[2],used);
    }
    score=add(mul(sub(m->origin[0],start[0]),m->velocity[0]),mul(sub(m->origin[1],start[1]),m->velocity[1]));
    lower_score=add(add(mul(delta[0],m->velocity[0]),mul(delta[1],m->velocity[1])),.001f);
    if(!(lower_score<score) || (jump && m->origin[2]>=m->jump_apex)) {
        copy(lower,m->origin);copy(lower_v,m->velocity);
        if(ground) {
            copy(m->origin,end);end[2]=sub(end[2],mul(height,.5f));
            player_trace(m,fn,ctx,&tr,m->origin,end);
            if(tr.fraction<1) {copy(tr.end,m->origin);clip(m->velocity,tr.normal,m->velocity,1,2,0);}
        }
    }
    if(jump && sub(m->origin[2],lower[2])>0) {
        remaining=sub(m->jump_apex,m->origin[2]);
        if(remaining<.1f) m->velocity[2]=0;
        else {
            remaining=sqrtf(mul(add(remaining,remaining),(float)m->gravity));
            if(m->velocity[2]>remaining) m->velocity[2]=remaining;
        }
    }
    if(m->pm_type>=4) return 0;
    if(m->flags&1) prone_slope(m,fn,ctx);
    dz=sub(m->origin[2],lower[2]);
    if(fabsf(dz)<=.5f) return 0;
    /* 2^-30 is the original rounding bias; PC24 rounds the addition first. */
    n=(int)nearbyintf(add(dz,0x1p-30f));
    if(!n) return 0;
    if(n<-16) n=-16; else if(n>24) n=24;
    event(m,0x8f,(n+128)&255);
    scale=add(mul(sub(1,divf(fabsf(sub(m->origin[2],start[2])),height)),.8f),0x1.999998p-3f);
    for(axis=0;axis<3;axis++) m->velocity[axis]=mul(m->velocity[axis],scale);
    if(n<0) n=-n;
    if(n>3 && m->ground_entity!=2047) {
        advance=n/2;if(advance>4) advance=4;old_bob=m->bob_cycle;
        m->bob_cycle=((int)add(add(mul((float)advance,1.25f),7), (float)old_bob))&255;
        step_footstep(m,old_bob);
    }
    return 0;
}
