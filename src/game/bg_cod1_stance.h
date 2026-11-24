#ifndef BG_COD1_STANCE_H
#define BG_COD1_STANCE_H
/* PATCH21: RTCW SP bridge, unused bits of its existing wire layout.
 * Original CoD1 defaults: eye heights 60 / 40 / 11 from feet.
 * Collision heights and crawl scale below are bootstrap compatibility values,
 * not a completed port of original prone body/yaw/terrain checks. */
#define PMF_COD1_PRONE 128
#define WBUTTON_COD1_STANCE 64
#define WBUTTON_COD1_PRONE 128
#define COD1_CRAWL_SCALE 0.15f
/* Existing stats[STAT_COD1_MODE] has five weapon-mode bits. These three
 * unused, networked bits carry original ps+0xC stance permissions; original
 * 0x20000/0x40000/0x80000 cannot fit RTCW's 16-bit pm_flags wire field.
 * This is transport mapping, not a change to original permission priority. */
#define COD1_STANCE_PERMISSION_SHIFT 8
#define COD1_STANCE_PERMISSION_MASK (7 << COD1_STANCE_PERMISSION_SHIFT)
#define COD1_STANCE_PERMISSION_BIT(stance) (1 << (COD1_STANCE_PERMISSION_SHIFT+(stance)))
/* Order: stand=0, crouch=1, prone=2. Native ordinary arbitration
 * GAME 20005C61..20005D42. -1 is the native all-stances-disallowed error.
 * Linked-player and other special movement branches have separate contracts. */
static int BG_Cod1PermittedStance(unsigned blocked,int old,int want){
 blocked&=7u;
 if(blocked==7u)return -1;
 if(!(blocked&(1u<<want)))return want;
 if(want==2){
  if(old==0&&!(blocked&1u))return 0;
  return !(blocked&2u)?1:0;
 }
 if(want==1){
  if(old==0&&!(blocked&4u))return 2;
  return !(blocked&1u)?0:2;
 }
 if(old==2&&!(blocked&4u))return 2;
 return !(blocked&2u)?1:2;
}
#endif
