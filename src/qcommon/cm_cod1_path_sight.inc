/* Brush-only native sight; model surfaces require their separate original
 * sight kernels. Never use contact-biased trace or legacy geometry fallback. */
#include "cod1_path_sight.h"
qboolean CM_Cod1PathBrushSight(const vec3_t start,const vec3_t end,
    const vec3_t mins,const vec3_t maxs,const vec3_t origin,int model,
    unsigned mask,int*out){
    C1PathSight t;int first,count,surfaces,b,j,k;float enter,leave;
    v59_brush_info info;v59_brush_side side;
    if(!codWorld||!out||!start||!end||!mins||!maxs||!origin||model<=0||model>=BOX_MODEL_HANDLE-1||
       v59_model_brush_range(codWorld,model,&first,&count,&surfaces)||surfaces)return qfalse;
    if(mins[0]!=-15||mins[1]!=-15||mins[2]!=18||maxs[0]!=15||maxs[1]!=15||maxs[2]!=72)return qfalse;
    for(k=0;k<3;k++)if(!(fabs(start[k])<=1000000&&fabs(end[k])<=1000000&&fabs(origin[k])<=1000000))return qfalse;
    C1PS_Prepare(&t,start,end,mins,maxs,origin);
    for(b=0;b<count;b++){
        if(v59_get_brush(codWorld,first+b,&info)||info.sides<6)return qfalse;
        for(k=0;k<3;k++)if(!(fabs(info.mins[k])<=1000000&&fabs(info.maxs[k])<=1000000&&info.mins[k]<=info.maxs[k]))return qfalse;
        if(!(info.contents&mask)||!C1PS_Axes(&t,info.mins,info.maxs,&enter,&leave))continue;
        for(j=6;j<info.sides;j++){
            if(v59_get_brush_side(codWorld,first+b,j,&side))return qfalse;
            for(k=0;k<3;k++)if(!(fabs(side.normal[k])<=2))return qfalse;
            if(!(fabs(side.distance)<=1000000))return qfalse;
            if(!C1PS_Plane(&t,side.normal,side.distance,&enter,&leave))break;
        }
        if(j==info.sides){*out=-1;return qtrue;}
    }
    *out=0;return qtrue;
}
