/* SPDX-License-Identifier: GPL-3.0-or-later
 * CoD1 SHOOTING_TEST_R1: separate ADS reticle geometry.
 * cgamex86.dll sha256 6e51c61f3780654cbaecdef149810ccf3ab665aaeafa5ae75a13f714abc0f29b
 * Preferred VA 0x3000fa1f..0x3000fd77, WeaponDef +0x228.
 * w/h and x/y below are already SCREEN pixels. The original soft lines use
 * 3-pixel thickness and a 1-pixel offset, not 3 virtual HUD units.
 * Pure geometry: no renderer, player state or asset registration here.
 */
#ifndef CG_COD1_SCOPE_RETICLE_H
#define CG_COD1_SCOPE_RETICLE_H

enum { COD1_RETICLE_NONE, COD1_RETICLE_CROSSHAIR, COD1_RETICLE_FG42,
       COD1_RETICLE_SPRINGFIELD, COD1_RETICLE_GEWEHR43 };
enum { COD1_SCOPE_LINE_V, COD1_SCOPE_LINE_H, COD1_SCOPE_CENTER };
typedef struct {
    float x, y, w, h;
    int material;
} cod1ScopeStroke_t;

static int CG_Cod1ReticleKind(const char *name) {
    if (!name || !*name) return COD1_RETICLE_NONE;
    if (!Q_stricmp(name, "crosshair")) return COD1_RETICLE_CROSSHAIR;
    if (!Q_stricmp(name, "FG42")) return COD1_RETICLE_FG42;
    if (!Q_stricmp(name, "Springfield")) return COD1_RETICLE_SPRINGFIELD;
    if (!Q_stricmp(name, "Gewehr43")) return COD1_RETICLE_GEWEHR43;
    /* Unknown strings are NOT coerced to a different weapon's reticle. */
    return COD1_RETICLE_NONE;
}

static void CG_Cod1ScopeStroke(cod1ScopeStroke_t *r, float x, float y,
                              float w, float h, int material) {
    r->x=x; r->y=y; r->w=w; r->h=h; r->material=material;
}

/* Output capacity must be at least three strokes. centerW/H is used only by
 * the generic crosshair type, from reticleCenterSize scaled per screen axis. */
static int CG_Cod1ReticleStrokes(int kind, float x, float y, float w, float h,
                               float centerW, float centerH,
                               cod1ScopeStroke_t out[3]) {
    if (!out || !(w>0) || !(h>0)) return 0;
    switch (kind) {
    case COD1_RETICLE_CROSSHAIR:
        if (!(centerW>0) || !(centerH>0)) return 0;
        CG_Cod1ScopeStroke(&out[0],x-centerW*.5f,y-centerH*.5f,
                          centerW,centerH,COD1_SCOPE_CENTER);
        return 1;
    case COD1_RETICLE_FG42:
    case COD1_RETICLE_GEWEHR43:
        CG_Cod1ScopeStroke(&out[0],x-1,y,3,h*.9f,COD1_SCOPE_LINE_V);
        CG_Cod1ScopeStroke(&out[1],x-w*.9f,y-1,w*.75f,3,COD1_SCOPE_LINE_H);
        CG_Cod1ScopeStroke(&out[2],x+w*.15f,y-1,w*.75f,3,COD1_SCOPE_LINE_H);
        return 3;
    case COD1_RETICLE_SPRINGFIELD:
        CG_Cod1ScopeStroke(&out[0],x-1,y-h*.9f,3,h*1.8f,COD1_SCOPE_LINE_V);
        CG_Cod1ScopeStroke(&out[1],x-w*.9f,y-1,w*1.8f,3,COD1_SCOPE_LINE_H);
        return 2;
    default:
        return 0;
    }
}
#endif
