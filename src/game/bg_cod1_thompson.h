#ifndef BG_COD1_THOMPSON_H
#define BG_COD1_THOMPSON_H
/* CoD1 weapons/sp/thompson fireTime=0.0857, fireDelay=0.
 * RTCW simulation stores milliseconds: nearest representable interval is 86.
 * Shared by game and prediction; does not change other weapon definitions. */
#define COD1_THOMPSON_SHOT_MSEC 86
#define COD1_THOMPSON_FIRE_FRAMES 4
#define COD1_THOMPSON_FIRE_FPS 30
#define COD1_THOMPSON_MODEL_FRAMES 5 /* idle + four original fire frames */
static void BG_Cod1ThompsonFrame(int elapsed,int *oldframe,int *frame,float *backlerp) {
    float sample;int first;
    *oldframe=*frame=0;*backlerp=0;
    if(elapsed<0 || elapsed>=100)return;
    sample=elapsed*0.03f;first=(int)sample;
    *oldframe=1+first;*frame=*oldframe+1;if(*frame>4)*frame=4;
    *backlerp=1-(sample-first);
}
#endif
