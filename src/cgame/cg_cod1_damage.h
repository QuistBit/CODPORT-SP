#ifndef CG_COD1_DAMAGE_H
#define CG_COD1_DAMAGE_H

/* Retail SP cgamex86.dll, 0x3001e6e1..0x3001e775 and
 * 0x30010a80..0x30010adc. The snapshot writes one of eight slots,
 * replacing the one with the oldest start time. World damage (255,255)
 * updates the red flash but never creates a directional icon. */
#define COD1_DAMAGE_SLOTS 8
typedef struct {
    int start;
    int duration;
    float yaw;
} cod1DamageSlot_t;

static void COD1_DamageRecord(cod1DamageSlot_t slots[COD1_DAMAGE_SLOTS],
                              int start,int duration,int yawByte,int pitchByte,
                              float randomFraction) {
    int i,oldest=0,shortAngle;
    float angle;
    if(yawByte==255 && pitchByte==255)return;
    for(i=1;i<COD1_DAMAGE_SLOTS;i++)
        if(slots[i].start<slots[oldest].start)oldest=i;
    /* Retail: BYTE2ANGLE uses /255, then adds +/-10 degrees of jitter,
     * quantizes the result through the 16-bit angle representation. */
    angle=yawByte*(360.0f/255.0f)+(randomFraction-.5f)*20.0f;
    shortAngle=((int)(angle*(65536.0f/360.0f)))&0xffff;
    slots[oldest].start=start;
    slots[oldest].duration=duration;
    slots[oldest].yaw=shortAngle*(360.0f/65536.0f);
}

static float COD1_DamageAlpha(const cod1DamageSlot_t *slot,int now) {
    int elapsed=now-slot->start;
    float alpha;
    if(elapsed<=0 || slot->duration<=0 || elapsed>=slot->duration)return 0;
    alpha=2.0f-2.0f*(float)elapsed/(float)slot->duration;
    return alpha<1.0f?alpha:1.0f;
}

static void COD1_DamageCorner(float width,float height,float offset,
                              float cosine,float sine,float u,float v,
                              float *outX,float *outY) {
    float x=width*(u-.5f),y=offset+height*v;
    *outX=x*cosine-y*sine;
    *outY=x*sine+y*cosine;
}

#endif
