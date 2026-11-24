#ifndef BG_COD1_TURRET_SPAWN_RESOURCES14_H
#define BG_COD1_TURRET_SPAWN_RESOURCES14_H
/* Numeric WeaponDef defaults for the constructor. No precache, sound
 * registration, script loading or world publication is performed here.
 * Stand/bullet/turret definitions are the currently admitted input domain.
 * Native stance table A6374 is stand=0, duck=1, prone=2. */
#include "bg_cod1_weapondef.h"
#include "bg_cod1_turret_constructor14.h"
static int C1TSR14_Defaults(const cwDef_t*d,unsigned weaponId,unsigned loopSound,unsigned stopSound,C1TS14Weapon*out){
 const char *fields[]={"leftArc","rightArc","topArc","bottomArc","convergenceTime","maxRange","accuracy"};
 C1TS14Weapon w;float values[7];unsigned i;int index;double damage,v;int ok=1;
 unsigned char env[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;
 if(!d||!out||!weaponId||weaponId>255||loopSound>255||stopSound>255||C1TS14_Overlap(d,sizeof(*d),out,sizeof(*out))||strcmp(CW_String(d,"weaponClass"),"turret")||strcmp(CW_String(d,"weaponType"),"bullet")||strcmp(CW_String(d,"stance"),"stand"))return 0;
 if((!!*CW_String(d,"loopFireSound"))!=(loopSound!=0)||(!!*CW_String(d,"stopFireSound"))!=(stopSound!=0))return 0;
 memset(&w,0,sizeof(w));
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 for(i=0;i<7;i++){
  index=CW_FieldIndex(fields[i]);v=CW_Number(d,fields[i]);
  if(index<0||!d->value[index]||!*d->value[index]||!isfinite(v)||v< -1000000.0||v>1000000.0){ok=0;break;}
  values[i]=(float)v;
 }
 index=CW_FieldIndex("damage");damage=CW_Number(d,"damage");
 if(index<0||!d->value[index]||!*d->value[index]||!isfinite(damage)||damage< -2147483648.0||damage>2147483647.0||damage!=(int32_t)damage)ok=0;
 if(ok){w.leftArc=values[0];w.rightArc=values[1];w.topArc=values[2];w.bottomArc=values[3];w.convergenceSeconds=values[4];w.maxRange=values[5];w.accuracy=values[6];w.damage=(int32_t)damage;w.stance=0;w.weaponId=(uint8_t)weaponId;w.loopSound=(uint8_t)loopSound;w.stopSound=(uint8_t)stopSound;}
 __asm ldmxcsr mx
 __asm frstor env
 if(ok)*out=w;return ok;
}
#endif
