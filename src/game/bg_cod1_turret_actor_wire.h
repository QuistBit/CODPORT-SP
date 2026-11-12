#ifndef BG_COD1_TURRET_ACTOR_WIRE_H
#define BG_COD1_TURRET_ACTOR_WIRE_H
#include <string.h>
#include "../renderer/cod1_turret_actor_state.h"
#define COD1_TURRET_ACTOR_WIRE_VERSION 1
#define COD1_TURRET_ACTOR_WIRE_GENERIC_VERSION 2
#define COD1_TURRET_ACTOR_WIRE_FIELDS 99
#define COD1_TURRET_ACTOR_WIRE_GENERIC_FIELDS 197
#define CS_COD1_TURRET_ACTORS 1750
/* Model combinations are immutable for the map epoch, including retired owners. */
#define MAX_COD1_TURRET_ACTORS 16
#define CS_COD1_JUMPER (CS_COD1_TURRET_ACTORS + MAX_COD1_TURRET_ACTORS)
#define CS_COD1_ALLY_INFO (CS_COD1_JUMPER+1)
#define COD1_ALLY_INFO_COUNT (CS_COD1_SCRIPT_HINTS-CS_COD1_ALLY_INFO)
#define COD1_ALLY_PRESENT 256
#define COD1_ALLY_COMPASS 128
#define COD1_ALLY_INFO_MASK 63
#if COD1_ALLY_INFO_COUNT > COD1_ALLY_INFO_MASK || COD1_ALLY_INFO_COUNT < 1
#error Invalid ally HUD config range
#endif
#if CS_COD1_TURRET_ACTORS + MAX_COD1_TURRET_ACTORS >= CS_COD1_SCRIPT_HINTS
#error CoD1 actor configurations overlap script hints
#endif
/* 7 integers +50 CURRENT node weights +42 explicit current leaf phases.
 * Epoch uses the existing typed turret epoch pair. No interpolated input. */
static int C1TAW_Valid(const entityState_t*s,int snapshotTime){unsigned g,i;int active=0;
 if(!s||(s->cod1TurretActorVersion!=COD1_TURRET_ACTOR_WIRE_VERSION&&s->cod1TurretActorVersion!=COD1_TURRET_ACTOR_WIRE_GENERIC_VERSION)||s->number<0||s->number>=MAX_GENTITIES||s->eType!=ET_GENERAL||s->pos.trType!=TR_STATIONARY||s->apos.trType!=TR_STATIONARY||
    !(s->cod1TurretEpochLo|s->cod1TurretEpochHi)||!s->cod1TurretActorGeneration||s->cod1TurretActorGun<0||s->cod1TurretActorGun>=MAX_GENTITIES||s->cod1TurretActorGun==s->number||!s->cod1TurretActorGunGeneration||
    s->cod1TurretActorTick<0||s->cod1TurretActorTick>snapshotTime||s->cod1TurretActorConfig<CS_COD1_TURRET_ACTORS||s->cod1TurretActorConfig>=CS_COD1_TURRET_ACTORS+MAX_COD1_TURRET_ACTORS||s->cod1TurretActorFlags!=1)return 0;
 for(i=0;i<3;i++)if(!(s->pos.trBase[i]>=-1000000&&s->pos.trBase[i]<=1000000)||!(s->apos.trBase[i]>=-3600&&s->apos.trBase[i]<=3600))return 0;
 for(g=0;g<2;g++){
  for(i=0;i<25;i++)if(!(s->cod1TurretActorCurrent[g][i]>=0&&s->cod1TurretActorCurrent[g][i]<=1))return 0;
  for(i=0;i<21;i++)if(!(s->cod1TurretActorPhases[g][i]>=0&&s->cod1TurretActorPhases[g][i]<=1))return 0;
  if(s->cod1TurretActorCurrent[g][0]>0){unsigned r,c;int path=0;for(r=0;r<3;r++)if(s->cod1TurretActorCurrent[g][1+r]>0)for(c=0;c<7;c++)if(s->cod1TurretActorCurrent[g][4+r*7+c]>0)path=1;if(!path)return 0;active=1;}
 }
 if(s->cod1TurretActorVersion==COD1_TURRET_ACTOR_WIRE_GENERIC_VERSION){
  for(i=0;i<98;i++)if(!(s->cod1TurretActorGenericCurrent[i]>=0&&s->cod1TurretActorGenericCurrent[i]<=1))return 0;
  /* Full sparse hierarchy: exact paths/resource coverage are validated by the
   * typed CG producer against its immutable original catalog closure. */
  if(s->cod1TurretActorGenericCurrent[0]!=1)return 0;
  for(g=0;g<2;g++)for(i=0;i<25;i++)if(s->cod1TurretActorCurrent[g][i]!=0)return 0;
  return 1;
 }
 for(i=0;i<98;i++)if(s->cod1TurretActorGenericCurrent[i]!=0)return 0;
 return active;
}
static int C1TAW_Name(const char*s){size_t i,n;if(!s||!memchr(s,0,64)||!(n=strlen(s))||n>=64||strstr(s,".."))return 0;for(i=0;i<n;i++)if((unsigned char)s[i]<32||strchr("/\\:@|",s[i]))return 0;return 1;}
/* Map-owned canonical composition. Parse atomically; no fixed Avery variant. */
static int C1TAW_Composition(const char*text,cod1TurretActorResourceState_t*out){cod1TurretActorResourceState_t v;size_t at=6,n=0,i=0,j;if(!text||!out||strncmp(text,"c1ta1|",6))return 0;while(n<1024&&text[n])n++;if(n>=1024)return 0;memset(&v,0,sizeof(v));v.version=COD1_TURRET_ACTOR_RESOURCE_VERSION;v.size=sizeof(v);
 while(at<n){if(i>=8)return 0;j=0;while(at<n&&text[at]!='@'){if(text[at]=='|'||j>=63)return 0;v.model[i][j++]=text[at++];}if(at>=n||text[at++]!='@'||!C1TAW_Name(v.model[i]))return 0;j=0;while(at<n&&text[at]!='|'){if(j>=63)return 0;v.tag[i][j++]=text[at++];}if(v.tag[i][0]&&(strncmp(v.tag[i],"tag_",4)||!C1TAW_Name(v.tag[i])))return 0;i++;if(at<n){at++;if(at==n)return 0;}}
 if(!i||v.tag[0][0])return 0;v.partCount=i;*out=v;return 1;
}
#endif
