/* Private prepared model-light snapshots. Public renderer ABI is unchanged. */
#ifndef COD1_MODEL_LIGHT_TYPES_H
#define COD1_MODEL_LIGHT_TYPES_H
typedef struct {
    unsigned int type;
    float normalizedRGB[3]; /* runtime +04..0c: BSP chroma; native scene path stores raw RGB */
    float scalar;          /* runtime +10: luminance, or signed sky priority */
    float ambient[3];      /* runtime +14..1c */
    float diffuse[3];      /* runtime +24..2c; negative synthesized sky is valid */
    float specular[3];     /* runtime +34..3c; zero for supported BSP records */
    float position[3];     /* runtime +44..4c: point, or unnormalized direction */
    float positionW;       /* runtime +50: 0 directional, 1 point */
    float spotDirection[3];/* runtime +54..5c */
    float attenuation[3];  /* runtime +60/+64/+68: constant/linear/quadratic */
    float spotExponent;    /* runtime +6c */
    float spotCutoff;      /* runtime +70: degrees; 180 disables cone */
    float radius;          /* runtime +74: scene candidate radius, separate */
} c1mLight;

typedef struct {
    float ambient[3]; /* native world +d8..e0 */
    float sun[3];     /* native world +e8..f0 */
    float sky[3];     /* native world +f8..100 */
    float skyLuminance; /* native world +108 */
} c1mWorldCoefficients;

#define C1M_PREPARED_LIMIT 8
typedef struct {
    float minIntensity, tailCutoff;
    int maxLights, diffuseSunQuality, hardwareMaxLights, reduceTail;
} c1mPrepareSettings;
typedef struct {
    c1mLight lights[C1M_PREPARED_LIMIT];
    float weights[C1M_PREPARED_LIMIT], scores[C1M_PREPARED_LIMIT];
    int sourceIndices[C1M_PREPARED_LIMIT], count, budget;
    int syntheticSkyCount, aggregatedTailCount;
    float remainingSky;
} c1mPrepared;

typedef struct {
    c1mWorldCoefficients world;
    c1mPrepared prepared;
    float center[3];
    int valid;
} c1mModelLightFrame;
#endif
