/* PATCH37: client presentation only. Retail CoD1 SP eye-height curves.
 * Physics/collision and authoritative viewheight are unchanged.
 * Reversal continuity and direct stand/prone chaining are adapter policy;
 * not a claim of complete original PMove transition semantics. */
#ifndef CG_COD1_POSTURE_H
#define CG_COD1_POSTURE_H

typedef struct { float percent, height; } cod1EyePoint_t;
typedef struct {
    int valid, lastTime, goal, startTime, duration, curve;
    float height, fromHeight;
} cod1Posture_t;
static const cod1EyePoint_t cod1EyeDown[] = {
    {0, 60.0},
    {1, 59.5},
    {4, 58.5},
    {30, 56.0},
    {80, 44.0},
    {90, 41.5},
    {95, 40.5},
    {100, 40.0}
};
static const cod1EyePoint_t cod1EyeUp[] = {
    {0, 40.0},
    {5, 40.5},
    {10, 41.5},
    {20, 44.0},
    {70, 56.0},
    {96, 58.5},
    {99, 59.5},
    {100, 60.0}
};
static const cod1EyePoint_t cod1EyeProne[] = {
    {0, 40.0},
    {11, 38.0},
    {22, 33.0},
    {34, 25.0},
    {45, 16.0},
    {50, 15.0},
    {55, 16.0},
    {70, 18.0},
    {90, 17.0},
    {100, 11.0}
};
static const cod1EyePoint_t cod1EyeRise[] = {
    {0, 11.0},
    {5, 10.0},
    {30, 21.0},
    {50, 25.0},
    {67, 31.0},
    {83, 34.0},
    {100, 40.0}
};
static float COD1_EyeTarget(int stance) {
    return stance == 2 ? 11.0f : stance == 1 ? 40.0f : 60.0f;
}
static float COD1_EyeCurve(int curve, float fraction) {
    const cod1EyePoint_t *p;
    int count, i;
    float t;
    switch(curve) {
    case 0: p=cod1EyeDown; count=sizeof(cod1EyeDown)/sizeof(*p); break;
    case 1: p=cod1EyeUp; count=sizeof(cod1EyeUp)/sizeof(*p); break;
    case 2: p=cod1EyeProne; count=sizeof(cod1EyeProne)/sizeof(*p); break;
    default:p=cod1EyeRise; count=sizeof(cod1EyeRise)/sizeof(*p); break;
    }
    if(fraction<=0)return p[0].height;
    if(fraction>=1)return p[count-1].height;
    t=fraction*100;
    for(i=1;i<count;i++)if(t<=p[i].percent) {
        float f=(t-p[i-1].percent)/(p[i].percent-p[i-1].percent);
        return p[i-1].height+f*(p[i].height-p[i-1].height);
    }
    return p[count-1].height;
}
static void COD1_EyeStart(cod1Posture_t *s, int now) {
    float start, end, ratio;
    int duration;
    s->startTime=now;s->fromHeight=s->height;
    if(s->height==COD1_EyeTarget(s->goal)){s->duration=0;return;}
    if(s->goal==0)s->curve=s->height<40?3:1;
    else if(s->goal==2)s->curve=s->height>40?0:2;
    else s->curve=s->height>40?0:3;
    start=COD1_EyeCurve(s->curve,0);end=COD1_EyeCurve(s->curve,1);
    duration=s->curve==0?150:s->curve==1?200:400;
    ratio=(end-s->height)/(end-start);
    if(ratio<0)ratio=-ratio;
    if(ratio>1)ratio=1;
    s->duration=(int)(duration*ratio);
    if(s->duration<1)s->duration=1;
}
static void COD1_EyeAdvance(cod1Posture_t *s, int now) {
    int pass;
    for(pass=0;pass<3 && s->duration;pass++) {
        int elapsed=now-s->startTime;
        float f;
        if(elapsed>=s->duration) {
            int next=s->startTime+s->duration;
            s->height=COD1_EyeCurve(s->curve,1);
            COD1_EyeStart(s,next);
            continue;
        }
        f=elapsed>0?(float)elapsed/s->duration:0;
        s->height=COD1_EyeCurve(s->curve,f)+
            (s->fromHeight-COD1_EyeCurve(s->curve,0))*(1-f);
        break;
    }
}
static float COD1_EyeUpdate(cod1Posture_t *s,int now,int goal,int reset) {
    if(goal<0 || goal>2)goal=0;
    if(reset || !s->valid || now<s->lastTime || now-s->lastTime>1000) {
        s->valid=1;s->goal=goal;s->height=COD1_EyeTarget(goal);
        s->duration=0;s->startTime=now;s->fromHeight=s->height;
    } else {
        COD1_EyeAdvance(s,now);
        if(goal!=s->goal){s->goal=goal;COD1_EyeStart(s,now);}
    }
    s->lastTime=now;
    return s->height;
}
#endif
