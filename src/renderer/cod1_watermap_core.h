/* CoD1 procedural waterMap CPU reconstruction, research R49 (not wired to GL).
 * Evidence: EXE af73842d..., VA 0x4fc410..0x4fcc1e; see README.md.
 * No names of maps/materials, FS, GL, engine globals, or implicit RNG seed.
 * Caller owns all buffers. Inputs/outputs must not overlap except the documented
 * in-place FFT field. A failed generation/FFT must not be published by caller.
 * Uses long double to model x87 intermediates, float stores at native boundaries.
 * Transcendentals and non-x87 platforms are NOT claimed bit-exact to hardware.
 */
#ifndef COD1_WATERMAP_CORE_R49_H
#define COD1_WATERMAP_CORE_R49_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct { float re, im; } c1w_complex;
typedef struct {
    int width, height;
    float world_width, world_height, gravity, wind_velocity;
    float wind_x, wind_y, amplitude; /* wind direction already normalized */
} c1w_desc;
typedef struct { c1w_complex twiddle[256]; float phase_sine[1024]; } c1w_tables;
typedef int (*c1w_rand15)(void *user); /* explicit source, [0,32767] */
typedef enum {
    C1W_OK=0, C1W_ARGUMENT, C1W_CAPACITY, C1W_RANGE, C1W_RANDOM, C1W_ROUNDING
} c1w_status;

/* Native parser admits powers of 2 in [4,256]. The native normal converter
 * forms (width*height)^2 with signed 32-bit imul. For 256x256 it wraps to 0;
 * this research API rejects that unverified/degenerate case, rather than UB. */
c1w_status C1W_Count(const c1w_desc *d, size_t *count);
void C1W_Tables(c1w_tables *t);
c1w_status C1W_Spectrum(const c1w_desc *d, c1w_rand15 rng, void *user,
                       c1w_complex *initial, float *omega, size_t capacity);
c1w_status C1W_AtTime(const c1w_desc *d, const c1w_tables *t, float seconds,
                     const c1w_complex *initial, const float *omega,
                     c1w_complex *field, size_t capacity);
c1w_status C1W_FFT2(const c1w_desc *d, const c1w_tables *t,
                   c1w_complex *field, size_t capacity);
/* Native converts the complex field to magnitudes in-place. Here a separate
 * caller-owned magnitude buffer preserves the field for differential testing.
 * Periodic backward differences; encoded normal RGB around 127, alpha=255. */
c1w_status C1W_Normals(const c1w_desc *d, const c1w_complex *field,
                      float *magnitude, size_t capacity,
                      uint8_t *rgba, size_t rgba_capacity);
#ifdef __cplusplus
}
#endif
#endif
