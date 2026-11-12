/* Native CoD1 sun effect math. Offline contract: research/sun_runtime_r63/effect.
 * This header has no renderer state or GL calls. All fallible outputs are atomic.
 * Arithmetic models the native default x87 precision (53 bits) and float spills;
 * libm cosine/sqrt are bounded port implementations, not an x87 emulator. */
#ifndef COD1_SUN_EFFECT_H
#define COD1_SUN_EFFECT_H

#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include "cod1_sun_config.h"

typedef enum {
    C1SUN_ROUND_NEAREST = 0, C1SUN_ROUND_DOWN = 1,
    C1SUN_ROUND_UP = 2, C1SUN_ROUND_ZERO = 3
} cod1SunRoundMode_t;

typedef struct {
    float minCos, maxCos, maxAmount;
    int fadeInMs, fadeOutMs;
} cod1SunCurve_t;

typedef struct {
    float minHalf, maxHalf, flareAlpha;
    float flareMinCos, flareMaxCos;
    int flareInMs, flareOutMs;
    cod1SunCurve_t blind, glare;
} cod1SunEffectCache_t;

typedef struct {
    float blindFraction, glareFraction;
    int lastTime;
} cod1SunEffectState_t;

typedef struct { float xyz[4][3]; } cod1SunQuad_t;

static int Cod1SunEffect_Finite(double x) {
    return x == x && x <= DBL_MAX && x >= -DBL_MAX;
}

static int Cod1SunEffect_Float(double x, float *out) {
    float v;
    if (!out || !Cod1SunEffect_Finite(x) || x > FLT_MAX || x < -FLT_MAX)
        return 0;
    v = (float)x;
    *out = v;
    return 1;
}

static int Cod1SunEffect_RoundSpill(float spill, cod1SunRoundMode_t mode, int *out) {
    double x, lo, f, r;
    if (!out || !Cod1SunEffect_Finite(spill)) return 0;
    x = (double)spill + 9.31322574615478515625e-10;
    lo = floor(x);
    if (mode == C1SUN_ROUND_DOWN) r = lo;
    else if (mode == C1SUN_ROUND_UP) r = ceil(x);
    else if (mode == C1SUN_ROUND_ZERO) r = x < 0.0 ? ceil(x) : lo;
    else if (mode == C1SUN_ROUND_NEAREST) {
        f = x - lo;
        r = lo;
        if (f > 0.5 || (f == 0.5 && fmod(lo, 2.0) != 0.0)) r += 1.0;
    } else return 0;
    if (r < (double)INT_MIN || r > (double)INT_MAX) return 0;
    *out = (int)r;
    return 1;
}

/* 0x4dcdf0: float32 seconds*1000 spill, add 2^-30, FISTP with host RC. */
static int Cod1SunEffect_SecondsToMs(float seconds, cod1SunRoundMode_t mode,
                                    int *out) {
    float spill;
    if (!Cod1SunEffect_Float((double)seconds * 1000.0, &spill)) return 0;
    return Cod1SunEffect_RoundSpill(spill, mode, out);
}

/* x87 uses the two stored float constants, without an intermediate f32 spill. */
static int Cod1SunEffect_CosAngle(float degrees, float *out) {
    volatile double radians;
    if (!Cod1SunEffect_Finite(degrees)) return 0;
    radians = (double)degrees * 3.1415927410125732421875;
    radians = radians * 0.0055555556900799274444580078125;
    /* FCOS leaves its argument unchanged for |argument| >= 2^63. The port
     * rejects that domain rather than turning it into an arbitrary angle. */
    if (radians <= -9223372036854775808.0 || radians >= 9223372036854775808.0)
        return 0;
    return Cod1SunEffect_Float(cos(radians), out);
}

static int Cod1SunEffect_Curve(float dot, float minCos, float maxCos,
                              float *out) {
    float fraction;
    if (!out || !Cod1SunEffect_Finite(dot) || !Cod1SunEffect_Finite(minCos) ||
        !Cod1SunEffect_Finite(maxCos)) return 0;
    if (dot <= minCos) fraction = 0.0f;
    else if (dot >= maxCos) fraction = 1.0f;
    else if (!Cod1SunEffect_Float(((double)dot - minCos) /
                                 ((double)maxCos - minCos), &fraction)) return 0;
    *out = fraction;
    return 1;
}

static int Cod1SunEffect_BuildCurve(const cod1SunParams_t *p, int first,
                                  cod1SunRoundMode_t mode, cod1SunCurve_t *out) {
    cod1SunCurve_t c;
    if (!p || !out || first < 0 || first + 4 >= C1SUN_COUNT ||
        !Cod1SunEffect_CosAngle(p->value[first], &c.minCos) ||
        !Cod1SunEffect_CosAngle(p->value[first + 1], &c.maxCos) ||
        !Cod1SunEffect_Finite(p->value[first + 2]) ||
        !Cod1SunEffect_SecondsToMs(p->value[first + 3], mode, &c.fadeInMs) ||
        !Cod1SunEffect_SecondsToMs(p->value[first + 4], mode, &c.fadeOutMs))
        return 0;
    c.maxAmount = p->value[first + 2];
    *out = c;
    return 1;
}

static int Cod1SunEffect_CacheBuild(const cod1SunParams_t *p,
                                  cod1SunRoundMode_t mode,
                                  cod1SunEffectCache_t *out) {
    cod1SunEffectCache_t c;
    if (!p || !out ||
        !Cod1SunEffect_Float((double)p->value[C1SUN_FLARE_MIN_SIZE] * 0.5, &c.minHalf) ||
        !Cod1SunEffect_Float((double)p->value[C1SUN_FLARE_MAX_SIZE] * 0.5, &c.maxHalf) ||
        !Cod1SunEffect_CosAngle(p->value[C1SUN_FLARE_MIN_ANGLE], &c.flareMinCos) ||
        !Cod1SunEffect_CosAngle(p->value[C1SUN_FLARE_MAX_ANGLE], &c.flareMaxCos) ||
        !Cod1SunEffect_Finite(p->value[C1SUN_FLARE_MAX_ALPHA]) ||
        !Cod1SunEffect_SecondsToMs(p->value[C1SUN_FLARE_FADEIN], mode, &c.flareInMs) ||
        !Cod1SunEffect_SecondsToMs(p->value[C1SUN_FLARE_FADEOUT], mode, &c.flareOutMs) ||
        !Cod1SunEffect_BuildCurve(p, C1SUN_BLIND_MIN_ANGLE, mode, &c.blind) ||
        !Cod1SunEffect_BuildCurve(p, C1SUN_GLARE_MIN_ANGLE, mode, &c.glare)) return 0;
    c.flareAlpha = p->value[C1SUN_FLARE_MAX_ALPHA];
    *out = c;
    return 1;
}

static int Cod1SunEffect_Dot(const float direction[3], const float forward[3],
                            float *out) {
    volatile double v;
    int i;
    if (!direction || !forward || !out) return 0;
    for (i = 0; i < 3; ++i)
        if (!Cod1SunEffect_Finite(direction[i]) || !Cod1SunEffect_Finite(forward[i]))
            return 0;
    v = (double)direction[2] * forward[2] + (double)direction[1] * forward[1];
    v = v + (double)direction[0] * forward[0];
    return Cod1SunEffect_Float(v, out);
}

/* Return 1 for a queued flare, 0 for angular rejection, -1 for bad input.
 * Native size is minHalf + maxHalf*f, not interpolation of the two sizes. */
static int Cod1SunEffect_Flare(const cod1SunEffectCache_t *c, float dot,
                              float *halfSize, float *alpha) {
    float fraction, h, a;
    if (!c || !halfSize || !alpha || !Cod1SunEffect_Curve(dot, c->flareMinCos,
        c->flareMaxCos, &fraction)) return -1;
    if (dot <= c->flareMinCos) return 0;
    if (!Cod1SunEffect_Float((double)c->maxHalf * fraction + c->minHalf, &h) ||
        !Cod1SunEffect_Float((double)c->flareAlpha * fraction, &a)) return -1;
    *halfSize = h;
    *alpha = a;
    return 1;
}

/* 0x4c8ce0. A negative elapsed time is retained, as in the native helper. */
static int Cod1SunEffect_Approach(float current, float target, int dtMs,
                                 int inMs, int outMs, float *out) {
    double v;
    float result;
    if (!out || !Cod1SunEffect_Finite(current) || !Cod1SunEffect_Finite(target))
        return 0;
    if (current < target) {
        if (inMs <= 0) result = target;
        else {
            v = (double)current + (double)dtMs / inMs;
            if (v >= target) result = target;
            else if (!Cod1SunEffect_Float(v, &result)) return 0;
        }
    } else if (current > target) {
        if (outMs <= 0) result = target;
        else {
            v = (double)current - (double)dtMs / outMs;
            if (v <= target) result = target;
            else if (!Cod1SunEffect_Float(v, &result)) return 0;
        }
    } else result = current;
    *out = result;
    return 1;
}

/* LastTime==0 is the native sentinel, including a real first frame at time 0. */
static int Cod1SunEffect_Postprocess(const cod1SunEffectCache_t *c,
                                    cod1SunEffectState_t *state, int now,
                                    float dot, float rawVisibility,
                                    float *blind, float *glare) {
    cod1SunEffectState_t s;
    float b = 0.0f, g = 0.0f, fraction, target;
    double delta;
    int dt;
    if (!c || !state || !blind || !glare || !Cod1SunEffect_Finite(rawVisibility) ||
        !Cod1SunEffect_Finite(dot) || !Cod1SunEffect_Finite(c->blind.maxAmount) ||
        !Cod1SunEffect_Finite(c->glare.maxAmount) ||
        !Cod1SunEffect_Finite(state->blindFraction) ||
        !Cod1SunEffect_Finite(state->glareFraction))
        return 0;
    s = *state;
    delta = s.lastTime ? (double)now - s.lastTime : 10.0;
    if (delta < INT_MIN || delta > INT_MAX) return 0;
    dt = (int)delta;
    s.lastTime = now;
    if (c->blind.maxAmount > 0.0f) {
        if (!Cod1SunEffect_Curve(dot, c->blind.minCos, c->blind.maxCos, &fraction) ||
            !Cod1SunEffect_Float((double)fraction * rawVisibility, &target) ||
            !Cod1SunEffect_Approach(s.blindFraction, target, dt,
                                  c->blind.fadeInMs, c->blind.fadeOutMs,
                                  &s.blindFraction) ||
            !Cod1SunEffect_Float((double)s.blindFraction * c->blind.maxAmount, &b))
            return 0;
    }
    if (c->glare.maxAmount > 0.0f) {
        if (!Cod1SunEffect_Curve(dot, c->glare.minCos, c->glare.maxCos, &fraction) ||
            !Cod1SunEffect_Float((double)fraction * rawVisibility, &target) ||
            !Cod1SunEffect_Approach(s.glareFraction, target, dt,
                                  c->glare.fadeInMs, c->glare.fadeOutMs,
                                  &s.glareFraction) ||
            !Cod1SunEffect_Float((double)s.glareFraction * c->glare.maxAmount, &g))
            return 0;
    }
    *state = s;
    *blind = b;
    *glare = g;
    return 1;
}

static void Cod1SunEffect_Reset(cod1SunEffectState_t *state) {
    if (state) memset(state, 0, sizeof(*state));
}

/* 0x4dcb90, n must come from the native normalized worldspawn producer. */
static int Cod1SunEffect_DiskQuad(const float n[3], float size,
                                 cod1SunQuad_t *out) {
    cod1SunQuad_t q;
    float seed[3], t[3], b[3], plus[3], minus[3], half;
    float lengthSquared, length, inverse;
    int i;
    if (!n || !out || !Cod1SunEffect_Float((double)size *
        0.001311092986725270748138427734375, &half)) return 0;
    for (i = 0; i < 3; ++i) if (!Cod1SunEffect_Finite(n[i])) return 0;
    seed[0] = 0.0f;
    if ((double)n[2] * n[2] > 0.9900000095367431640625) {
        seed[1] = 1.0f; seed[2] = 0.0f;
    } else { seed[1] = n[2]; seed[2] = -n[1]; }
    if (!Cod1SunEffect_Float((double)n[1] * seed[2] - (double)n[2] * seed[1], &t[0]) ||
        !Cod1SunEffect_Float((double)n[2] * seed[0] - (double)n[0] * seed[2], &t[1]) ||
        !Cod1SunEffect_Float((double)n[0] * seed[1] - (double)n[1] * seed[0], &t[2]))
        return 0;
    if (!Cod1SunEffect_Float((double)t[0] * t[0] + (double)t[1] * t[1] +
        (double)t[2] * t[2], &lengthSquared) ||
        !Cod1SunEffect_Float(sqrt((double)lengthSquared), &length) ||
        !(length > 0.0f) || !Cod1SunEffect_Float(1.0 / length, &inverse)) return 0;
    for (i = 0; i < 3; ++i) {
        if (!Cod1SunEffect_Float((double)t[i] * inverse, &t[i]) ||
            !Cod1SunEffect_Float((double)t[i] * half, &t[i])) return 0;
    }
    if (!Cod1SunEffect_Float((double)n[2] * t[1] - (double)n[1] * t[2], &b[0]) ||
        !Cod1SunEffect_Float((double)n[0] * t[2] - (double)n[2] * t[0], &b[1]) ||
        !Cod1SunEffect_Float((double)n[1] * t[0] - (double)n[0] * t[1], &b[2]))
        return 0;
    for (i = 0; i < 3; ++i) {
        if (!Cod1SunEffect_Float((double)t[i] + b[i], &plus[i]) ||
            !Cod1SunEffect_Float((double)t[i] - b[i], &minus[i]) ||
            !Cod1SunEffect_Float((double)n[i] + plus[i], &q.xyz[0][i]) ||
            !Cod1SunEffect_Float((double)n[i] + minus[i], &q.xyz[1][i]) ||
            !Cod1SunEffect_Float((double)n[i] - plus[i], &q.xyz[2][i]) ||
            !Cod1SunEffect_Float((double)n[i] - minus[i], &q.xyz[3][i])) return 0;
    }
    *out = q;
    return 1;
}

/* Drawn half extent scales the queued angular half size by viewport width/640. */
static int Cod1SunEffect_PixelHalfSize(float halfSize, int viewportWidth, float *out) {
    if (viewportWidth <= 0) return 0;
    return Cod1SunEffect_Float((double)halfSize *
        0.001562500023283064365386962890625 * viewportWidth, out);
}

static int Cod1SunEffect_ScreenQuad(int x, int y, float halfSize, float z,
                                   cod1SunQuad_t *out) {
    cod1SunQuad_t q;
    if (!out || !Cod1SunEffect_Finite(z) ||
        !Cod1SunEffect_Float((double)x - halfSize, &q.xyz[0][0]) ||
        !Cod1SunEffect_Float((double)y - halfSize, &q.xyz[0][1]) ||
        !Cod1SunEffect_Float((double)x - halfSize, &q.xyz[1][0]) ||
        !Cod1SunEffect_Float((double)y + halfSize, &q.xyz[1][1]) ||
        !Cod1SunEffect_Float((double)x + halfSize, &q.xyz[2][0]) ||
        !Cod1SunEffect_Float((double)y + halfSize, &q.xyz[2][1]) ||
        !Cod1SunEffect_Float((double)x + halfSize, &q.xyz[3][0]) ||
        !Cod1SunEffect_Float((double)y - halfSize, &q.xyz[3][1])) return 0;
    q.xyz[0][2] = q.xyz[1][2] = q.xyz[2][2] = q.xyz[3][2] = z;
    *out = q;
    return 1;
}

/* 0x4adfe0 and 0x4cf470 use the CRT truncate helper, then retain the low byte.
 * Native bad-int conversion is outside this bounded finite port domain. */
static int Cod1SunEffect_Byte(float value, unsigned char *out) {
    double v, rounded;
    if (!out || !Cod1SunEffect_Finite(value)) return 0;
    v = (double)value * 255.0;
    rounded = v < 0.0 ? ceil(v) : floor(v);
    if (rounded < INT_MIN || rounded > INT_MAX) return 0;
    *out = (unsigned char)(int)rounded;
    return 1;
}

static int Cod1SunEffect_ComposeColor(float blind, float glare,
                                     unsigned char out[4]) {
    unsigned char color[4];
    if (!out || !Cod1SunEffect_Byte(glare, &color[0]) ||
        !Cod1SunEffect_Byte(blind, &color[3])) return 0;
    color[1] = color[2] = color[0];
    memcpy(out, color, sizeof(color));
    return 1;
}

static int Cod1SunEffect_FlareColor(float intensity, float fadedVisibility,
                                   float identityLight, unsigned char out[4]) {
    unsigned char color[4];
    float rgb;
    if (!out || !Cod1SunEffect_Float((double)intensity * identityLight, &rgb) ||
        !Cod1SunEffect_Byte(rgb, &color[0]) ||
        !Cod1SunEffect_Byte(fadedVisibility, &color[3])) return 0;
    color[1] = color[2] = color[0];
    memcpy(out, color, sizeof(color));
    return 1;
}

#endif
