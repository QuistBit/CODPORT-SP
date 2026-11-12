#ifndef BG_COD1_TURRET_RESOURCES_H
#define BG_COD1_TURRET_RESOURCES_H
/* R231 original turret dependency plan. It copies real WeaponDef metadata
 * and decoded source skeleton indices. This is not a current animated muzzle,
 * a registered entity, an armed controller or a successful GSC method. */
#include "bg_cod1_weapondef.h"
#include "../qcommon/cod1_model_resource.h"
#include <math.h>
#include <string.h>
typedef struct {int damage,fireMs;float arc[4],turnSpeed[2],convergenceSeconds,accuracy,maxRange,animHorRotateIncrement;char script[64],loopAlias[128],stopAlias[128],worldFlash[128];} C1TRWeapon;
typedef struct {unsigned bones;int flash,pivot,aim,animatedAim;int requiresCurrentPreparedPose;} C1TRModel;
static int C1TR_Token(const char *s){size_t i,n;if(!s||(n=strlen(s))==0||n>=128)return 0;for(i=0;i<n;i++)if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')||s[i]=='_'))return 0;return 1;}
static int C1TR_Number(const cwDef_t *d,const char *key,double lo,double hi,float *out){int i=CW_FieldIndex(key);double v;if(i<0||!d->value[i]||!*d->value[i])return 0;v=CW_Number(d,key);if(!(v>=lo&&v<=hi))return 0;*out=(float)v;return 1;}
static int C1TR_Weapon(const cwDef_t *d,C1TRWeapon *out){C1TRWeapon p;float n;const char *arcs[]={"leftArc","rightArc","topArc","bottomArc"};int i;const char *loop,*stop,*flash;
 if(!d||!out||strcmp(d->name,"mg42_bipod_stand")||strcmp(CW_String(d,"weaponClass"),"turret")||strcmp(CW_String(d,"weaponType"),"bullet")||strcmp(CW_String(d,"stance"),"stand")||strcmp(CW_String(d,"script"),"mg42/stand"))return 0;
 memset(&p,0,sizeof(p));if(!C1TR_Number(d,"damage",1,10000,&n)||n!=(int)n)return 0;p.damage=(int)n;if(!C1TR_Number(d,"fireTime",1,10000,&n)||n!=(int)n)return 0;p.fireMs=(int)n;
 for(i=0;i<4;i++)if(!C1TR_Number(d,arcs[i],0,180,&p.arc[i]))return 0;
 if(!C1TR_Number(d,"horTurnSpeed",0.001,3600,&p.turnSpeed[0])||!C1TR_Number(d,"vertTurnSpeed",0.001,3600,&p.turnSpeed[1])||!C1TR_Number(d,"convergenceTime",0.001,60,&p.convergenceSeconds)||!C1TR_Number(d,"accuracy",0,1,&p.accuracy)||!C1TR_Number(d,"maxRange",0,1000000,&p.maxRange))return 0;
 if(!C1TR_Number(d,"animHorRotateInc",0.001,180,&p.animHorRotateIncrement))return 0;
 loop=CW_String(d,"loopFireSound");stop=CW_String(d,"stopFireSound");flash=CW_String(d,"worldFlashEffect");if(!C1TR_Token(loop)||!C1TR_Token(stop)||strncmp(flash,"fx/",3)||strlen(flash)>=sizeof(p.worldFlash)||strstr(flash,"..")||strchr(flash,'\\'))return 0;
 strcpy(p.script,"mg42/stand");strcpy(p.loopAlias,loop);strcpy(p.stopAlias,stop);strcpy(p.worldFlash,flash);*out=p;return 1;
}
static int C1TR_Model(const c1mrResource_t *r,C1TRModel *out){C1TRModel p;unsigned i;if(!r||!out||!r->skeleton.boneCount||r->skeleton.boneCount>=128)return 0;memset(&p,0,sizeof(p));p.flash=p.pivot=p.aim=p.animatedAim=-1;p.bones=r->skeleton.boneCount;
 for(i=0;i<p.bones;i++){const char *n=r->skeleton.boneName[i];if(!strcmp(n,"tag_flash")){if(p.flash>=0)return 0;p.flash=i;}if(!strcmp(n,"tag_pivot")){if(p.pivot>=0)return 0;p.pivot=i;}if(!strcmp(n,"tag_aim")){if(p.aim>=0)return 0;p.aim=i;}if(!strcmp(n,"tag_aim_animated")){if(p.animatedAim>=0)return 0;p.animatedAim=i;}}
 if(p.flash<0||p.pivot<0||p.aim<0||p.animatedAim<0)return 0;p.requiresCurrentPreparedPose=1;*out=p;return 1;
}
/* Native intern comparison from gamex86 200333F0, not donor-mode names. */
static int C1TR_ModeBits(const char *name,unsigned *out){unsigned bits;if(!name||!out)return 0;if(!strcmp(name,"auto_ai"))bits=3;else if(!strcmp(name,"auto_nonai"))bits=2;else if(!strcmp(name,"manual"))bits=0;else if(!strcmp(name,"manual_ai"))bits=1;else return 0;*out=bits;return 1;}
#endif
