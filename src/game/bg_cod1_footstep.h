#ifndef BG_COD1_FOOTSTEP_H
#define BG_COD1_FOOTSTEP_H
/* R195 private eventParm: existing 8-bit field; no state/trace ABI change.
 * Material comes from the existing IMPACT_R1 trace metadata, not RTCW flags.
 * Gait timing remains COD1_Gait. Unknown/default surfaces stay silent. */
#include <stdio.h>
#include <stddef.h>
#define C1F_MATERIALS 23
#define C1F_MODES 3
#define C1F_RUN 0
#define C1F_WALK 1
#define C1F_PRONE 2
#define C1F_TAG 128
static const char *c1fMaterials[C1F_MATERIALS]={"default","bark","brick","carpet","cloth",
 "concrete","dirt","flesh","foliage","glass","grass","gravel","ice","metal","mud",
 "paper","plaster","rock","sand","snow","water","wood","asphault"};
static const char *c1fModes[C1F_MODES]={"run","walk","prone"};
static int C1F_Valid(int parameter) {
 return parameter>=C1F_TAG&&parameter<=255&&((parameter>>5)&3)<C1F_MODES&&
  (parameter&31)>0&&(parameter&31)<C1F_MATERIALS;
}
static int C1F_Parameter(int old_cycle,int cycle,int event_call,int grounded,
 int ladder,int disabled,int nosteps,int known,int material,int water,int prone,int walking) {
 int mode;
 if(!event_call||!grounded||ladder||disabled||nosteps||
    !(((old_cycle+64)^(cycle+64))&128))return 0;
 if(water==1||water==2){known=1;material=20;}
 if(!known||material<=0||material>=C1F_MATERIALS)return 0;
 mode=prone?C1F_PRONE:walking?C1F_WALK:C1F_RUN;
 return C1F_TAG|(mode<<5)|material;
}
static int C1F_Alias(int parameter,char *out,size_t cap) {
 int n;if(!C1F_Valid(parameter)||!out||!cap)return 0;
 n=snprintf(out,cap,"step_%s_%s",c1fModes[(parameter>>5)&3],c1fMaterials[parameter&31]);
 return n>0&&(size_t)n<cap;
}
#endif
