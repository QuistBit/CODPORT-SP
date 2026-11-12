#ifndef CG_COD1_SCOPE_H
#define CG_COD1_SCOPE_H
#include "cg_cod1_scope_reticle.h"
/* Retail cgame 0x3000f6b0: overlay appears during the final zoom fraction.
 * 0x3000f7d4..0x3000fa1b draws four mirrored quarter images plus border fills.
 * Width/height are HALF extents, not full image dimensions. */
static float CG_Cod1ScopeAlpha(const playerState_t *ps) {
    const cwDef_t *d=CW_PSDef(ps);float a=CW_ADS(ps),frac;
    /* Retail 0x3000f6c6..d9 activates the transition for a mask shader OR
     * a nonzero ADS reticle kind; reticle-only sights fade the hip art too. */
    if(!CW_Active(ps) || a<=0 ||
       (!*CW_String(d,"adsOverlayShader") &&
        CG_Cod1ReticleKind(CW_String(d,"adsOverlayReticle"))==COD1_RETICLE_NONE))return 0;
    frac=(float)CW_Number(d,(ps->stats[STAT_COD1_EDGES]&CW_ADS_RISING)?"adsZoomInFrac":"adsZoomOutFrac");
    a=frac>0?(a-(1-frac))/frac:(a>=1?1:0);
    if(a<.01f)return 0;return a>1?1:a;
}
#endif
