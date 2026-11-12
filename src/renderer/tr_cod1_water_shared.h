/* Included after tr_local.h, only with COD1_WATER_RUNTIME_R50. */
#ifndef TR_COD1_WATER_SHARED_R50_H
#define TR_COD1_WATER_SHARED_R50_H
#include "cod1_water_program_types.h"
typedef struct cod1WaterMaterial_s {
    c1wp_plan plan;
    image_t *image[C1WP_STAGES][C1WP_BUNDLES];
} cod1WaterMaterial_t;
static unsigned long R_Cod1WaterState(const c1wp_stage *s) {
    static const unsigned long src[]={GLS_SRCBLEND_ZERO,GLS_SRCBLEND_ONE,
      GLS_SRCBLEND_SRC_ALPHA,GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA,GLS_SRCBLEND_DST_COLOR,
      GLS_SRCBLEND_ONE_MINUS_DST_COLOR,GLS_SRCBLEND_DST_ALPHA,GLS_SRCBLEND_ONE_MINUS_DST_ALPHA,0,0};
    static const unsigned long dst[]={GLS_DSTBLEND_ZERO,GLS_DSTBLEND_ONE,
      GLS_DSTBLEND_SRC_ALPHA,GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA,0,0,
      GLS_DSTBLEND_DST_ALPHA,GLS_DSTBLEND_ONE_MINUS_DST_ALPHA,GLS_DSTBLEND_SRC_COLOR,GLS_DSTBLEND_ONE_MINUS_SRC_COLOR};
    unsigned long bits=s->depthWrite?GLS_DEPTHMASK_TRUE:0;
    if(!(s->srcBlend==C1WP_ONE&&s->dstBlend==C1WP_ZERO))bits|=src[s->srcBlend]|dst[s->dstBlend];
    if(s->alphaTest==1)bits|=GLS_ATEST_GT_0;
    else if(s->alphaTest==2)bits|=GLS_ATEST_LT_80;
    else if(s->alphaTest==3)bits|=GLS_ATEST_GE_80;
    return bits;
}
#endif
