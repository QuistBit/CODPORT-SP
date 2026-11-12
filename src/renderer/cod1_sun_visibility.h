/* CoDSP 1.5 SP sun queue/depth subset, original SHA in R63 contract.
 * Pure C89 helpers. The adapter supports a main mono viewport at origin 0.
 * Matrix inputs and outputs are column-major as in OpenGL. No GL or globals.
 */
#ifndef COD1_SUN_VISIBILITY_H
#define COD1_SUN_VISIBILITY_H

#include <float.h>
#include <limits.h>
#include <math.h>

typedef struct cod1SunVisibilityPoint_s {
	int windowX, windowY, radius;
	float depth;
} cod1SunVisibilityPoint_t;

typedef struct cod1SunVisibilityPlan_s {
	int readX, readY, readWidth, readHeight;
	int nominalPixels;
	float fallbackVisibility;
} cod1SunVisibilityPlan_t;

static int Cod1SunVisibility_Finite(double value) {
	return value == value && value <= DBL_MAX && value >= -DBL_MAX;
}

/* FISTP without changing the caller's rounding mode. */
static int Cod1SunVisibility_Integer(double value, int roundMode, int *out) {
	double rounded, lower, fraction;
	if (!out || !Cod1SunVisibility_Finite(value) || roundMode < 0 || roundMode > 3)
		return 0;
	lower = floor(value);
	rounded = lower;
	if (roundMode == 2) rounded = ceil(value);
	else if (roundMode == 3) rounded = value < 0.0 ? ceil(value) : lower;
	else if (roundMode == 0) {
		fraction = value - lower;
		if (fraction > 0.5 || (fraction == 0.5 && fmod(lower, 2.0) != 0.0))
			rounded = lower + 1.0;
	}
	if (rounded < (double)INT_MIN || rounded > (double)INT_MAX) return 0;
	*out = (int)rounded;
	return 1;
}

/* 4cf219..4cf29c. The 2^-30 bias is a real native double constant. */
static int Cod1SunVisibility_Radius(int viewportWidth, float size,
	int roundMode, int *out) {
	volatile float scale;
	int radius;
	if (!out || viewportWidth <= 0 || viewportWidth > 16384 || size < 0.0f ||
		!Cod1SunVisibility_Finite(size) || FLT_RADIX != 2 || FLT_MANT_DIG != 24 ||
		DBL_MANT_DIG != 53) return 0;
	scale = (float)((double)viewportWidth * size * (double)0.0015625000232830644f);
	if (!Cod1SunVisibility_Integer((double)scale + 9.31322574615478515625e-10,
		roundMode, &radius) || radius < 0 || radius > 1073741823) return 0;
	*out = radius;
	return 1;
}

/* 4c9340 native PC53 arithmetic with an explicit float store per row. */
static int Cod1SunVisibility_Multiply(const float matrix[16],
	const float in[4], float out[4]) {
	int row, i;
	volatile double sum;
	float result[4];
	if (!matrix || !in || !out) return 0;
	for (i = 0; i < 16; ++i) if (!Cod1SunVisibility_Finite(matrix[i])) return 0;
	for (i = 0; i < 4; ++i) if (!Cod1SunVisibility_Finite(in[i])) return 0;
	for (row = 0; row < 4; ++row) {
		sum = (double)matrix[row + 8] * in[2] + (double)matrix[row + 4] * in[1];
		if (!row) {
			sum = sum + (double)matrix[row + 12] * in[3];
			sum = sum + (double)matrix[row] * in[0];
		} else {
			sum = sum + (double)matrix[row] * in[0];
			sum = sum + (double)matrix[row + 12] * in[3];
		}
		if (!Cod1SunVisibility_Finite(sum) || sum > FLT_MAX || sum < -FLT_MAX) return 0;
		result[row] = (float)sum;
	}
	for (i = 0; i < 4; ++i) out[i] = result[i];
	return 1;
}

/* Sun record from 4cf210 has w=0 and depth=1. New zero-radius records alone
 * receive the strict three-axis canonical clip check (4ced60..4ced9c).
 */
static int Cod1SunVisibility_ProjectDirection(const float direction[3],
	const float worldModelMatrix[16], const float projectionMatrix[16],
	int width, int height, int originX, int originY, int radius, int existing,
	cod1SunVisibilityPoint_t *out) {
	float source[4], eye[4], clip[4];
	volatile float normalizedX, normalizedY, windowX, windowY;
	double integerX, integerY;
	cod1SunVisibilityPoint_t result;
	int i;
	if (!direction || !out || width <= 0 || width > 16384 || height <= 0 ||
		height > 16384 || originX != 0 || originY != 0 || radius < 0 ||
		radius > 1073741823 || (existing != 0 && existing != 1) ||
		FLT_RADIX != 2 || FLT_MANT_DIG != 24 || DBL_MANT_DIG != 53) return 0;
	for (i = 0; i < 3; ++i) source[i] = direction[i];
	source[3] = 0.0f;
	if (!Cod1SunVisibility_Multiply(worldModelMatrix, source, eye) ||
		!Cod1SunVisibility_Multiply(projectionMatrix, eye, clip) || clip[3] <= 0.0f)
		return 0;
	if (!radius && !existing) {
		for (i = 0; i < 3; ++i)
			if (clip[i] >= clip[3] || clip[i] <= -clip[3]) return 0;
	}
	normalizedX = (float)((double)clip[0] / clip[3]);
	normalizedY = (float)((double)clip[1] / clip[3]);
	windowX = (float)(((double)normalizedX + 1.0) * width * 0.5);
	windowY = (float)(((double)normalizedY + 1.0) * height * 0.5);
	integerX = floor((double)windowX + 0.5);
	integerY = floor((double)windowY + 0.5);
	if (!Cod1SunVisibility_Finite(integerX) || !Cod1SunVisibility_Finite(integerY) ||
		integerX < -1073741824.0 || integerX > 1073741823.0 ||
		integerY < -1073741824.0 || integerY > 1073741823.0) return 0;
	result.windowX = (int)integerX;
	result.windowY = (int)integerY;
	result.radius = radius;
	result.depth = 1.0f;
	*out = result;
	return 1;
}

/* 4cf2d0..4cf41f. The full nominal square remains the denominator when
 * edge clipping shrinks glReadPixels. Completely offscreen sun returns 1.
 */
static int Cod1SunVisibility_PlanDepth(const cod1SunVisibilityPoint_t *point,
	int width, int height, cod1SunVisibilityPlan_t *out) {
	cod1SunVisibilityPlan_t result;
	int side, half, x, y;
	if (!point || !out || width <= 0 || width > 16384 || height <= 0 ||
		height > 16384 || point->radius < 0 || point->radius > 1073741823 ||
		point->windowX < -1073741824 || point->windowX > 1073741823 ||
		point->windowY < -1073741824 || point->windowY > 1073741823 ||
		point->depth != 1.0f) return 0;
	result.readX = result.readY = result.readWidth = result.readHeight = 0;
	result.nominalPixels = 0;
	result.fallbackVisibility = 1.0f;
	if (point->windowX + point->radius < 0 || point->windowX - point->radius >= width ||
		point->windowY + point->radius < 0 || point->windowY - point->radius >= height) {
		*out = result;
		return 1;
	}
	side = point->radius >= 32 ? 65 : point->radius * 2 + 1;
	half = side / 2;
	x = point->windowX - half;
	y = point->windowY - half;
	result.readWidth = result.readHeight = side;
	result.nominalPixels = side * side;
	result.fallbackVisibility = 0.0f;
	if (x < 0) { result.readWidth = x + side; x = 0; }
	else if (side > width - x) result.readWidth = width - x;
	if (y < 0) { result.readHeight = y + side; y = 0; }
	else if (side > height - y) result.readHeight = height - y;
	result.readX = x;
	result.readY = y;
	/* Native emits no read if the capped sample square misses the viewport,
 * even when the larger authored radius passed the initial overlap gate.
 */
	if (result.readWidth <= 0 || result.readHeight <= 0)
		result.readWidth = result.readHeight = 0;
	*out = result;
	return 1;
}

static int Cod1SunVisibility_Ratio(const cod1SunVisibilityPlan_t *plan,
	const float *depths, int count, float *out) {
	int i, visible, expected;
	if (!plan || !out || plan->readWidth < 0 || plan->readWidth > 65 ||
		plan->readHeight < 0 || plan->readHeight > 65 || plan->nominalPixels < 0 ||
		plan->nominalPixels > 4225) return 0;
	expected = plan->readWidth * plan->readHeight;
	if (count != expected || (count && !depths)) return 0;
	if (!plan->nominalPixels) {
		if (expected || plan->fallbackVisibility != 1.0f) return 0;
		*out = 1.0f;
		return 1;
	}
	if (plan->fallbackVisibility != 0.0f || expected > plan->nominalPixels) return 0;
	visible = 0;
	for (i = 0; i < count; ++i) {
		if (!Cod1SunVisibility_Finite(depths[i]) || depths[i] < 0.0f || depths[i] > 1.0f)
			return 0;
		if (depths[i] >= 1.0f) ++visible;
	}
	*out = (float)((double)visible / plan->nominalPixels);
	return 1;
}

#endif
