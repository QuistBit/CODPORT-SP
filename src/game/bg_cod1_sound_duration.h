#ifndef BG_COD1_SOUND_DURATION_H
#define BG_COD1_SOUND_DURATION_H
#include <math.h>
/* Loaded sfx length is in mixer-rate mono samples, including resampled
 * compressed buffers. Match the alias mixer's effective pitch clamp. */
static int C1N_SampleDuration(int samples,int rate,float pitch){
 double milliseconds;
 if(samples<=0||rate<=0||!isfinite(pitch)||pitch<=0)return -1;
 if(pitch<0.25f)pitch=0.25f;if(pitch>4.0f)pitch=4.0f;
 milliseconds=ceil(1000.0*(double)samples/((double)rate*pitch));
 if(milliseconds<1||milliseconds>120000)return -1;return (int)milliseconds;
}
#endif
