/* Immutable R50 ordinary-water IR; not part of game/protocol ABI. */
#ifndef COD1_WATER_PROGRAM_TYPES_R50_H
#define COD1_WATER_PROGRAM_TYPES_R50_H
#include "cod1_watermap_core.h"
#define C1WP_STAGES 8
#define C1WP_BUNDLES 2
#define C1WP_MODS 4

enum { C1WP_FILE=1, C1WP_WHITE, C1WP_WATERMAP, C1WP_RENORMALIZE, C1WP_LIGHTMAP };
enum { C1WP_UV=0, C1WP_VECTOR, C1WP_SUN_HALF, C1WP_LIGHTMAP_UV };
enum { C1WP_IDENTITY=1, C1WP_IDENTITY_LIGHTING, C1WP_VERTEX,
       C1WP_EXACT_VERTEX, C1WP_CONST, C1WP_CONST_LIGHTING };
enum { C1WP_ALPHA_IDENTITY=0, C1WP_ALPHA_VERTEX, C1WP_ALPHA_DOT };
enum { C1WP_SCROLL=1, C1WP_SCALE, C1WP_TURB };
enum { C1WP_REPLACE=1, C1WP_MODULATE, C1WP_ADD, C1WP_INTERPOLATE,
       C1WP_DOT3_RGB, C1WP_DOT3_RGBA };
enum { C1WP_PREVIOUS=0, C1WP_TEXTURE, C1WP_CONSTANT };
/* Blend-factor ordinals are local data, NOT OpenGL or RTCW protocol enums. */
enum { C1WP_ZERO=0, C1WP_ONE, C1WP_SRC_ALPHA, C1WP_ONE_MINUS_SRC_ALPHA,
       C1WP_DST_COLOR, C1WP_ONE_MINUS_DST_COLOR, C1WP_DST_ALPHA,
       C1WP_ONE_MINUS_DST_ALPHA, C1WP_SRC_COLOR, C1WP_ONE_MINUS_SRC_COLOR };
typedef struct { int source, alpha, invert; } c1wp_arg;
typedef struct { int op, count; c1wp_arg arg[3]; } c1wp_expr;
typedef struct {
    int enabled, constantPresent, constantLighting;
    float constant[4];
    c1wp_expr rgb,alpha;
} c1wp_combine;
typedef struct { int kind; float value[4]; } c1wp_mod;
typedef struct {
    int imageKind, clamp, tcgen, numMods; /* clamp is C1MAP_* axis bits (R51) */
    unsigned mapLoadFlags; /* C1MAP_NOOPT; retained independently of sampling */
    char image[128];
    c1w_desc water;
    float vectors[2][3];
    c1wp_mod mod[C1WP_MODS];
    c1wp_combine combine;
} c1wp_bundle;
typedef struct {
    int sourceStage,numBundles,rgb,alpha,srcBlend,dstBlend,depthWrite;
    int explicitAlpha, alphaTest; /* 0 off, 1 GT0, 2 LT128, 3 GE128 */
    float color[3];
    c1wp_bundle bundle[C1WP_BUNDLES];
} c1wp_stage;
typedef struct {
    unsigned sourceHash, selectedMask, excludedMask;
    int sourceStages,numStages,sort,cull,polygonOffset,noMip,noPicMip;
    int waterFlags,hasWave,needsSun,needsCube,needsCombine,needsDot3,ignoredTurb3;
    float waveSpread,waveBase,waveAmplitude,wavePhase,waveFrequency,tessSize;
    c1wp_stage stage[C1WP_STAGES];
} c1wp_plan;
typedef struct { int candidate,line; char reason[160]; } c1wp_error;

#endif
