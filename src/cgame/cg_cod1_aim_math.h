#ifndef CG_COD1_AIM_MATH_H
#define CG_COD1_AIM_MATH_H
/* PATCH56: native shot-angle producer 0x3002a6e0. The ADS animation root
 * does not participate. Local procedural angles are composed with camera. */
static void COD1_WeaponAimAngles(const float camera[3],const float local[3],
                               float ads,int aimDownSight,float out[2]) {
    vec3_t axis[3],direction,world,angles;int i,j;
    if(!aimDownSight || ads==0) {out[0]=camera[0];out[1]=camera[1];return;}
    AnglesToAxis(camera,axis);AngleVectors(local,direction,NULL,NULL);
    for(i=0;i<3;i++) {world[i]=0;for(j=0;j<3;j++)world[i]+=axis[j][i]*direction[j];}
    vectoangles(world,angles);
    /* Native axis-to-angles normalizes pitch before the integer conversion. */
    if(angles[0]<0)angles[0]+=360;
    for(i=0;i<2;i++)out[i]=SHORT2ANGLE(ANGLE2SHORT(angles[i]));
}
#endif
