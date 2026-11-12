#ifndef BG_COD1_TURRET_MOUNT_H
#define BG_COD1_TURRET_MOUNT_H
/* Closed goal-transform kernel of native6C7C1..6C8D3 ONLY. Caller must supply
 * current modelspace gun tag_weapon, actual engine-equivalent blended actor
 * root q2/T3, actual base transform and existing empty-ROOT-link relative Z.
 * This does not choose animation blend children, sample clips, own a link,
 * grant CanUseTurret, smooth/trace/move an actor or emit a successful mount.
 * Native root translation Z selects the prior blend; final local actor Z is
 * preserved link-relative Z, not the root's Z and not tag_player. */
#include <math.h>
#include <string.h>
typedef struct {float origin[3],angles[3],axis[3][3];} c1tmGoal_t;
static int C1TM_Finite(float x,float n){return x>=-n&&x<=n;}
static float C1TM_Degrees(double rad){return (float)(rad*57.2957795130823208768);}
static float C1TM_Positive(float a){while(a<0)a+=360;while(a>=360)a-=360;return a;}
static void C1TM_Axes(const float a[3],float axis[3][3]){
 float p=a[0]*.01745329238474369f,y=a[1]*.01745329238474369f,r=a[2]*.01745329238474369f;
 float sp=(float)sin(p),cp=(float)cos(p),sy=(float)sin(y),cy=(float)cos(y),sr=(float)sin(r),cr=(float)cos(r);
 axis[0][0]=cp*cy;axis[0][1]=cp*sy;axis[0][2]=-sp;
 axis[1][0]=sr*sp*cy-cr*sy;axis[1][1]=sr*sp*sy+cr*cy;axis[1][2]=sr*cp;
 axis[2][0]=cr*sp*cy+sr*sy;axis[2][1]=cr*sp*sy-sr*cy;axis[2][2]=cr*cp;
}
/* tagWeapon[0..2] forward, [4..6] left,[8..10] up,[12..14] position.
 * Padding slots ignored, matching original current 64-byte bone matrix. */
static int C1TM_SolveGoal(const float baseOrigin[3],const float baseAngles[3],const float tagWeapon[16],const float rootRotation2[2],const float rootTranslation3[3],float existingRelativeZ,c1tmGoal_t *out){
 c1tmGoal_t g;float base[3][3],local[3][3],localAngles[3]={0,0,0},position[3],yaw,rootYaw,n,root2x,root2y,c,s,rad;int i,j,k;
 if(!baseOrigin||!baseAngles||!tagWeapon||!rootRotation2||!rootTranslation3||!out||!C1TM_Finite(existingRelativeZ,1000000))return 0;
 for(i=0;i<3;i++){if(!C1TM_Finite(baseOrigin[i],1000000)||!C1TM_Finite(baseAngles[i],3600)||!C1TM_Finite(rootTranslation3[i],1000000)||!C1TM_Finite(tagWeapon[12+i],1000000))return 0;for(j=0;j<3;j++)if(!C1TM_Finite(tagWeapon[i*4+j],2))return 0;}
 /* A finite zero/corrupted matrix is not a current posed tag. */
 for(i=0;i<3;i++){double norm=0;for(k=0;k<3;k++)norm+=(double)tagWeapon[i*4+k]*tagWeapon[i*4+k];if(norm<.98||norm>1.02)return 0;for(j=0;j<i;j++){double dot=0;for(k=0;k<3;k++)dot+=(double)tagWeapon[i*4+k]*tagWeapon[j*4+k];if(dot<-.01||dot>.01)return 0;}}
 if(!C1TM_Finite(rootRotation2[0],4)||!C1TM_Finite(rootRotation2[1],4))return 0;
 root2x=rootRotation2[0];root2y=rootRotation2[1];n=root2x*root2x+root2y*root2y;if(!(n>1e-12f&&n<=16))return 0;
 yaw=(tagWeapon[0]==0&&tagWeapon[1]==0)?0:C1TM_Degrees(atan2((double)tagWeapon[1],(double)tagWeapon[0]));
 rootYaw=C1TM_Degrees(atan2(2.0*(double)root2x*root2y/n,1.0-2.0*(double)root2x*root2x/n));
 rad=yaw*.01745329238474369f;c=(float)cos(rad);s=(float)sin(rad);
 position[0]=c*rootTranslation3[0]-s*rootTranslation3[1]+tagWeapon[12];position[1]=s*rootTranslation3[0]+c*rootTranslation3[1]+tagWeapon[13];position[2]=existingRelativeZ;
 localAngles[1]=rootYaw+yaw;C1TM_Axes(baseAngles,base);C1TM_Axes(localAngles,local);
 for(k=0;k<3;k++){
  g.origin[k]=position[0]*base[0][k]+position[1]*base[1][k]+position[2]*base[2][k]+baseOrigin[k];if(!C1TM_Finite(g.origin[k],4000000))return 0;
  for(i=0;i<3;i++){g.axis[i][k]=0;for(j=0;j<3;j++)g.axis[i][k]+=local[i][j]*base[j][k];if(!C1TM_Finite(g.axis[i][k],2))return 0;}
 }
 /* Equivalent finite nongimbal Euler output. Native10B80 uses its own
  * axis->Euler helpers; tests compare axes and wrapped Euler degrees. */
 {double h=sqrt((double)g.axis[0][0]*g.axis[0][0]+(double)g.axis[0][1]*g.axis[0][1]);if(h<1e-5)return 0;
  g.angles[0]=C1TM_Positive(C1TM_Degrees(atan2(-(double)g.axis[0][2],h)));g.angles[1]=C1TM_Positive(C1TM_Degrees(atan2((double)g.axis[0][1],(double)g.axis[0][0])));g.angles[2]=C1TM_Degrees(atan2((double)g.axis[1][2],(double)g.axis[2][2]));}
 *out=g;return 1;
}
#endif
