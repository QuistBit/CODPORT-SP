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
#endif
