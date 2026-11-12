/* CoD1 fixed-function $dlight image, reconstructed from 0x4e53a0.
 * Pure bounded transcription; no RTCW falloff or retail pixel fixture.
 * GPL-3.0-or-later, matching the surrounding renderer. */
#ifndef COD1_DLIGHT_IMAGE_H
#define COD1_DLIGHT_IMAGE_H

#include <float.h>
#include <limits.h>

#define COD1_DLIGHT_IMAGE_SIZE 64
#define COD1_DLIGHT_IMAGE_BYTES (COD1_DLIGHT_IMAGE_SIZE * COD1_DLIGHT_IMAGE_SIZE * 4)

/* Native computes with the x87 stack, spilling only scale to binary32.
 * long double preserves that evaluation order where the compiler supports
 * extended precision. The supported default also has identical final bytes
 * with binary64 intermediates; this is not an arbitrary-input x87 parity claim.
 * Invalid/unrepresentable inputs fail before the output buffer is modified. */
static int Cod1DL_BuildImage( unsigned char *rgba, float radius, float shift ) {
	long double squaredShift, squaredRadius, baseline, denominator;
	long double scaleWide, value, distanceSquared, dx, dy;
	volatile float scale;
	int x, y, pass, b;
	unsigned char *pixel;

	if ( !rgba || !(radius >= -FLT_MAX && radius <= FLT_MAX) ||
		 !(shift >= -FLT_MAX && shift <= FLT_MAX) ) return 0;
	squaredShift = (long double)shift * (long double)shift;
	squaredRadius = (long double)radius * (long double)radius;
	if ( squaredRadius + squaredShift == 0.0L ) return 0;
	baseline = 1.0L / (squaredShift + 1.0L);
	denominator = 1.0L / (squaredRadius + squaredShift) - baseline;
	if ( denominator == 0.0L ) return 0;
	scaleWide = 255.0L / denominator;
	if ( !(scaleWide >= -FLT_MAX && scaleWide <= FLT_MAX) ) return 0;
	scale = (float)scaleWide;
	if ( scale == 0.0f ) return 0;

	/* Pass 0 validates the full conversion domain. Pass 1 writes only after
	 * every native-style truncation is representable as a signed 32-bit int. */
	for ( pass = 0; pass < 2; pass++ ) {
		for ( x = 0; x < COD1_DLIGHT_IMAGE_SIZE; x++ ) {
			dx = 31.5L - (long double)x;
			for ( y = 0; y < COD1_DLIGHT_IMAGE_SIZE; y++ ) {
				dy = 31.5L - (long double)y;
				distanceSquared = (dx * dx + dy * dy) *
					(long double)0.001007810584269464f;
				value = (long double)scale / (distanceSquared + squaredShift) -
					baseline * (long double)scale;
				if ( !(value >= INT_MIN && value <= INT_MAX) ) return 0;
				if ( !pass ) continue;
				b = (int)value;
				if ( b > 255 ) b = 255;
				else if ( b < 0 ) b = 0;
				pixel = rgba + 4 * (y * COD1_DLIGHT_IMAGE_SIZE + x);
				pixel[0] = pixel[1] = pixel[2] = (unsigned char)b;
				pixel[3] = 255;
			}
		}
	}
	return 1;
}

#endif
