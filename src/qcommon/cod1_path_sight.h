/* Original 44EDB0 -> 41EF80/41E960 -> 41D8F0, capsule/brush lane.
 * Sight clips [0,1] WITHOUT the .125 trace contact bias. This is not a
 * trace.fraction conversion. Caller owns contents, geometry and lifetime.
 * Explicit double intermediates model x87 PC53; stores below are native
 * single-precision stores. Only untranslated-axis capsule orientation here.
 */
#ifndef COD1_PATH_SIGHT_H
#define COD1_PATH_SIGHT_H
typedef struct { float start[3],end[3],radius,offset,extents[3]; } C1PathSight;
static float C1PS_Store(double x){volatile float v=(float)x;return v;}
static void C1PS_Prepare(C1PathSight*t,const float*s,const float*e,
    const float*mins,const float*maxs,const float*origin){
    int k;float half[3];
    for(k=0;k<3;k++){
        float center=C1PS_Store(((double)mins[k]+maxs[k])*.5);
        half[k]=C1PS_Store((double)maxs[k]-center);
        t->start[k]=C1PS_Store((double)C1PS_Store((double)s[k]+center)-origin[k]);
        t->end[k]=C1PS_Store((double)C1PS_Store((double)e[k]+center)-origin[k]);
    }
    t->radius=half[0]>half[2]?half[2]:half[0];
    t->offset=C1PS_Store((double)half[2]-t->radius);
    t->extents[0]=t->extents[1]=t->radius;
    t->extents[2]=C1PS_Store((double)t->offset+t->radius);
}
static int C1PS_Clip(float d1,double d2,float*enter,float*leave){
    if(d1>0){
        float delta=C1PS_Store((double)d1-d2);
        if(d2>0)return 0;
        if((double)delta*(*enter)<d1){
            *enter=C1PS_Store((double)d1/delta);
            if(*enter>=*leave)return 0;
        }
    }else if(d2>0){
        double delta=(double)d1-d2;
        if((double)(*leave)*delta<d1){
            *leave=C1PS_Store((double)d1/delta);
            if(*enter>=*leave)return 0;
        }
    }
    return 1;
}
static int C1PS_Axes(const C1PathSight*t,const float*mins,const float*maxs,
    float*enter,float*leave){
    int side,k;*enter=0;*leave=1;
    for(side=0;side<2;side++)for(k=0;k<3;k++){
        double bound=side?maxs[k]:mins[k],sign=side?1.0:-1.0;
        float d1=C1PS_Store(((double)t->start[k]-bound)*sign-t->extents[k]);
        double d2=((double)t->end[k]-bound)*sign-t->extents[k];
        if(!C1PS_Clip(d1,d2,enter,leave))return 0;
    }
    return 1;
}
static int C1PS_Plane(const C1PathSight*t,const float*n,float distance,
    float*enter,float*leave){
    double direction=(double)n[2]*t->offset>0?-1.0:1.0;
    double pz=(double)t->start[2]+direction*t->offset;
    float qz=C1PS_Store((double)t->end[2]+direction*t->offset);
    double dist=(double)distance+t->radius;
    float d1=C1PS_Store(((double)t->start[0]*n[0]+pz*n[2])+(double)t->start[1]*n[1]-dist);
    double d2=((double)t->end[0]*n[0]+(double)qz*n[2])+(double)t->end[1]*n[1]-dist;
    return C1PS_Clip(d1,d2,enter,leave);
}
#endif
