
#ifndef BG_COD1_VEHICLE14_H
#define BG_COD1_VEHICLE14_H
/* Native GAME vehicle definition 264h and controller record 218h. This
 * component owns copied values only. World allocation, DObj, frame/path and
 * VM callbacks must be supplied by the GAME owner before exposing a vehicle.
 * Evidence: Campaign-415/vehicle-spawn-native-424.json. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
enum {C1V14_DEFINITIONS=32,C1V14_CAPACITY=32,C1V14_NO_ENTITY=2047};
typedef struct {
 char name[64];int16_t type;uint16_t reserved42;
 int32_t steerWheels,textureScroll,quadBarrel,bulletDamage,grenadeDamage,projectileDamage;
 float textureScrollScale,maxSpeed,accel,rotRate,rotAccel,maxBodyPitch,maxBodyRoll,collisionDamage,collisionSpeed,suspensionTravel;
 char turretWeapon[64];float turretHorizSpanLeft,turretHorizSpanRight,turretVertSpanUp,turretVertSpanDown,turretRotRate;
 char soundNames[6][64];uint8_t soundIndexes[6];uint16_t reserved25e;float engineSoundSpeed;
} C1V14Definition;
typedef struct {
 uint16_t targetname,target;int16_t id;uint16_t reserved6;int32_t rotate;
 float speed,lookahead,origin[3],direction[3],angles[3],distance;int16_t next,previous;
} C1V14PathNode;
typedef struct {
 int16_t nodeIndex;uint16_t flags;float pathFraction,speed,lookahead;
 float pathState;float pathOrigin[3],pathAngles[3],pathLookOrigin[3];
 C1V14PathNode pathNodes[2];float origin[3],oldOrigin[3],angles[3],oldAngles[3];
 float velocity[3],localVelocity[3],angularVelocity[3],wheelA[6],wheelB[6],wheelC[6];
 int32_t entity;int16_t definition,waitNode;float waitSpeed;
 float values160[6];int32_t speedMode;float targetSpeed,speedAccel,stepFraction,unknown188;int32_t targetEntity;float values190[12];int32_t enabled,soundEntities[2];
 float soundBlend[2];uint32_t values1d4[17];
} C1V14Record;
typedef char C1V14DefinitionSize[(sizeof(C1V14Definition)==0x264)?1:-1];
typedef char C1V14PathNodeSize[(sizeof(C1V14PathNode)==64)?1:-1];
typedef char C1V14RecordSize[(sizeof(C1V14Record)==0x218)?1:-1];
static int C1V14_Equal(const char*a,const char*b){unsigned x,y;do{x=(unsigned char)*a++;y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;}while(x);return 1;}
static int C1V14_Text(const char*s,size_t cap,size_t*n){size_t i;if(!s)return 0;for(i=0;i<cap;i++)if(!s[i]){if(n)*n=i;return 1;}return 0;}
static int C1V14_Overlap(const void*a,size_t na,const void*b,size_t nb){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&na&&nb&&(x<y?y-x<na:x-y<nb);}
static int C1V14_Float(const char*text,float*out){
 unsigned char env[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;char*end;double d;float f;int ok;
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 d=strtod(text,&end);ok=end!=text&&!*end&&isfinite(d)&&d<=FLT_MAX&&d>=-FLT_MAX;f=(float)d;ok=ok&&isfinite(f);
 __asm ldmxcsr mx
 __asm frstor env
 if(ok)*out=f;return ok;
}
static int C1V14_Boolean(const char*text,int32_t*out){const unsigned char*p=(const unsigned char*)text;int any=0;if(*p=='+'||*p=='-')p++;if(!*p)return 0;for(;*p;p++){if(*p<'0'||*p>'9')return 0;if(*p!='0')any=1;}*out=any;return 1;}
static int C1V14_Scale(C1V14Definition*d){
 unsigned char env[108];uint32_t mx,nearMx=0x1f80;const unsigned short cw=0x027f;int ok;
 __asm fnsave env
 __asm stmxcsr mx
 __asm fldcw cw
 __asm ldmxcsr nearMx
 d->maxSpeed=(float)((double)d->maxSpeed*17.600000381469727);d->accel=(float)((double)d->accel*17.600000381469727);
 d->collisionSpeed=(float)((double)d->collisionSpeed*17.600000381469727);d->engineSoundSpeed=(float)((double)d->engineSoundSpeed*17.600000381469727);
 ok=isfinite(d->maxSpeed)&&isfinite(d->accel)&&isfinite(d->collisionSpeed)&&isfinite(d->engineSoundSpeed);
 __asm ldmxcsr mx
 __asm frstor env
 return ok;
}
static const struct {const char*name;unsigned offset,kind;} c1v14Fields[]={
 {"type",0x40,8},
 {"steerWheels",0x44,5},
 {"texureScroll",0x48,5},
 {"quadBarrel",0x4c,5},
 {"bulletDamage",0x50,5},
 {"grenadeDamage",0x54,5},
 {"projectileDamage",0x58,5},
 {"texureScrollScale",0x5c,6},
 {"maxSpeed",0x60,6},
 {"accel",0x64,6},
 {"rotRate",0x68,6},
 {"rotAccel",0x6c,6},
 {"collisionDamage",0x78,6},
 {"collisionSpeed",0x7c,6},
 {"suspensionTravel",0x80,6},
 {"maxBodyPitch",0x70,6},
 {"maxBodyRoll",0x74,6},
 {"turretWeapon",0x84,0},
 {"turretHorizSpanLeft",0xc4,6},
 {"turretHorizSpanRight",0xc8,6},
 {"turretVertSpanUp",0xcc,6},
 {"turretVertSpanDown",0xd0,6},
 {"turretRotRate",0xd4,6},
 {"lowIdleSnd",0xd8,0},
 {"highIdleSnd",0x118,0},
 {"lowEngineSnd",0x158,0},
 {"highEngineSnd",0x198,0},
 {"turretSpinSnd",0x1d8,0},
 {"turretStopSnd",0x218,0},
 {"engineSndSpeed",0x260,6},

};
/* Info_ValueForKey chooses the first matching key. Unknown keys remain
 * ignored; malformed/unbounded finite conversion is outside this scope. */
static int C1V14_Parse(const char*name,const char*text,size_t bytes,C1V14Definition*out){
 C1V14Definition d;unsigned seen=0,i;size_t pos=12,keyBegin,valueBegin,n;char key[128],value[1024];
 static const char*types[]={"4 wheel","tank","plane","boat","artillery"};
 if(!out||!C1V14_Text(name,64,&n)||!n||strchr(name,'/')||strchr(name,'\\')||!text||bytes<12||bytes>=8203||
    memcmp(text,"VEHICLEFILE\\",12)||memchr(text,0,bytes)||memchr(text,'"',bytes)||memchr(text,';',bytes)||
    C1V14_Overlap(out,sizeof(*out),name,n+1)||C1V14_Overlap(out,sizeof(*out),text,bytes))return 0;
 memset(&d,0,sizeof(d));memcpy(d.name,name,n+1);
 while(pos<bytes){
  keyBegin=pos;while(pos<bytes&&text[pos]!='\\')pos++;n=pos-keyBegin;if(!n||n>=sizeof(key)||pos==bytes)return 0;
  memcpy(key,text+keyBegin,n);key[n]=0;valueBegin=++pos;while(pos<bytes&&text[pos]!='\\')pos++;n=pos-valueBegin;
  if(n>=sizeof(value))return 0;memcpy(value,text+valueBegin,n);value[n]=0;if(pos<bytes)pos++;
  for(i=0;i<30;i++)if(C1V14_Equal(key,c1v14Fields[i].name))break;
  if(i==30||(seen&(1u<<i)))continue;seen|=1u<<i;if(!n)continue;
  {unsigned char*dst=(unsigned char*)&d+c1v14Fields[i].offset;int32_t integer;float number;
   if(c1v14Fields[i].kind==0){if(n>=64)return 0;memcpy(dst,value,n+1);}
   else if(c1v14Fields[i].kind==5){if(!C1V14_Boolean(value,&integer))return 0;memcpy(dst,&integer,4);}
   else if(c1v14Fields[i].kind==6){if(!C1V14_Float(value,&number))return 0;memcpy(dst,&number,4);}
   else{unsigned t;for(t=0;t<5;t++)if(C1V14_Equal(value,types[t]))break;if(t==5)return 0;d.type=(int16_t)t;}
  }
 }
 if(!C1V14_Scale(&d))return 0;*out=d;return 1;
}
typedef struct {void*user;int(*current)(void*);int(*sound)(void*,const char*,unsigned*);} C1V14SoundHost;
static int C1V14_ResolveSounds(const C1V14Definition*input,C1V14Definition*out,const C1V14SoundHost*host){
 C1V14Definition d,expected;C1V14SoundHost h;unsigned i,value;char name[64],copy[64];
 if(!input||!out||!host||!host->current||!host->sound||C1V14_Overlap(input,sizeof(*input),out,sizeof(*out))||
    C1V14_Overlap(host,sizeof(*host),out,sizeof(*out)))return 0;
 d=expected=*input;h=*host;
 for(i=0;i<6;i++){
  if(!C1V14_Text(d.soundNames[i],64,NULL)||d.soundIndexes[i])return 0;
  if(!h.current(h.user)||memcmp(input,&expected,sizeof(expected))||memcmp(host,&h,sizeof(h)))return 0;
  if(d.soundNames[i][0]){
   memcpy(name,d.soundNames[i],64);memcpy(copy,name,64);value=0;
   if(h.sound(h.user,name,&value)!=1||!h.current(h.user)||memcmp(input,&expected,sizeof(expected))||memcmp(host,&h,sizeof(h))||
      memcmp(name,copy,64)||value>255)return 0;d.soundIndexes[i]=(uint8_t)value;
  }
 }
 if(!h.current(h.user)||memcmp(input,&expected,sizeof(expected))||memcmp(host,&h,sizeof(h)))return 0;
 *out=d;return 1;
}
static void C1V14_NodeInit(C1V14PathNode*n){uint32_t pi=0x40490fdb;unsigned i;memset(n,0,sizeof(*n));n->id=n->next=n->previous=-1;n->speed=n->lookahead=-1;for(i=0;i<3;i++)memcpy(n->angles+i,&pi,4);}
static int C1V14_RecordInit(unsigned entity,unsigned definition,const float*origin,const float*angles,const int*soundEntities,C1V14Record*out){
 C1V14Record r;unsigned i;if(!out||!origin||!angles||!soundEntities||entity>=2046||definition>=32)return 0;
 for(i=0;i<3;i++)if(!isfinite(origin[i])||!isfinite(angles[i]))return 0;
 for(i=0;i<2;i++)if(soundEntities[i]<0||soundEntities[i]>2047||soundEntities[i]==2046||soundEntities[i]==(int)entity)return 0;
 if(soundEntities[0]!=2047&&soundEntities[0]==soundEntities[1])return 0;
 memset(&r,0,sizeof(r));r.nodeIndex=-1;C1V14_NodeInit(r.pathNodes);C1V14_NodeInit(r.pathNodes+1);
 memcpy(r.origin,origin,12);memcpy(r.oldOrigin,origin,12);memcpy(r.angles,angles,12);memcpy(r.oldAngles,angles,12);
 r.entity=(int)entity;r.definition=(int16_t)definition;r.waitNode=-1;r.waitSpeed=-1;r.targetEntity=2047;r.enabled=1;
 r.soundEntities[0]=soundEntities[0];r.soundEntities[1]=soundEntities[1];*out=r;return 1;
}
#endif
