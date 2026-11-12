/*
 * CoD1 fixed-function dynamic point projection, native 0x4f4bd0/0x4f30a0.
 * Pure C89: no renderer state and no changes to the floating-point environment.
 * Arithmetic support: binary32 floats, binary64 double operations, native PC53;
 * the caller must supply the host's current rounding mode and execute this
 * code under that same mode. See research/perlight_runtime_r62.
 */
#ifndef COD1_PERLIGHT_PROJECTION_H
#define COD1_PERLIGHT_PROJECTION_H

#if defined(__FAST_MATH__)
#error CoD1 perLight projection requires strict FP; disable fast-math and enable rounding-math
#endif

#include <float.h>
#include <limits.h>
#include <math.h>

typedef enum {
	COD1_PL_ROUND_NEAREST = 0,
	COD1_PL_ROUND_DOWN = 1,
	COD1_PL_ROUND_UP = 2,
	COD1_PL_ROUND_ZERO = 3
} cod1PLRoundMode_t;

typedef struct {
	float radius;
	float origin[3];
	float color[3];
} cod1PLProjectionLight_t;

typedef struct {
	float st[2];
	unsigned char rgba[4];
	unsigned char clip;
} cod1PLProjectionVertex_t;

#define COD1_PL_MAX_VERTICES 8192
#define COD1_PL_MAX_INDICES 49152

/* Native arithmetic runs at 53-bit precision after the CRT reset. Explicit
 * binary64 stores prevent an x87 C compiler retaining wider intermediates. */
static double Cod1PL_Add(double a, double b) {
	volatile double v = a + b;
	return v;
}
static double Cod1PL_Sub(double a, double b) {
	volatile double v = a - b;
	return v;
}
static double Cod1PL_Mul(double a, double b) {
	volatile double v = a * b;
	return v;
}
static double Cod1PL_Div(double a, double b) {
	volatile double v = a / b;
	return v;
}

static int Cod1PL_FiniteFloat(float v) {
	return v == v && v <= FLT_MAX && v >= -FLT_MAX;
}

static int Cod1PL_Spill(double v, float *out) {
	volatile float f;
	if (v != v || v > FLT_MAX || v < -FLT_MAX) return 0;
	f = (float)v;
	if (!Cod1PL_FiniteFloat(f)) return 0;
	*out = f;
	return 1;
}

/* FISTP is not a clamping conversion: retain the native low byte, including
 * negative/greater-than-one RGB. Values outside signed 32-bit conversion are
 * unsupported inputs rather than the native indefinite-integer recovery. */
static int Cod1PL_QuantizeByte(float value, cod1PLRoundMode_t mode,
	unsigned char *out) {
	double base, part, rounded;
	int integer;
	if (!out || !Cod1PL_FiniteFloat(value) || mode < COD1_PL_ROUND_NEAREST ||
		mode > COD1_PL_ROUND_ZERO || INT_MAX < 2147483647 ||
		INT_MIN > (-2147483647 - 1)) return 0;
	base = floor((double)value);
	part = Cod1PL_Sub((double)value, base);
	rounded = base;
	if (mode == COD1_PL_ROUND_UP) {
		if (part != 0.0) rounded = Cod1PL_Add(base, 1.0);
	} else if (mode == COD1_PL_ROUND_ZERO) {
		if (value < 0.0f && part != 0.0) rounded = Cod1PL_Add(base, 1.0);
	} else if (mode == COD1_PL_ROUND_NEAREST) {
		if (part > 0.5) rounded = Cod1PL_Add(base, 1.0);
		else if (part == 0.5) {
			/* Avoid converting base until its bounds have been checked. */
			if (base < -2147483648.0 || base > 2147483647.0) return 0;
			if (((unsigned int)(int)base & 1U) != 0)
				rounded = Cod1PL_Add(base, 1.0);
		}
	}
	if (rounded < -2147483648.0 || rounded > 2147483647.0) return 0;
	integer = (int)rounded;
	*out = (unsigned char)((unsigned int)integer & 255U);
	return 1;
}

/* Generated-basis subset. Native cached/authored TBN is a separate producer;
 * these equations must not replace an existing native authored basis. The
 * finite near-unit restriction is this port's explicit eligibility bound,
 * not a native normalization operation. */
static int Cod1PL_Basis(const float normal[3], float s[3], float t[3]) {
	double x, y, z, length2, product;
	float h, zz;
	int i;
	if (!normal || !s || !t) return 0;
	for (i = 0; i < 3; ++i) if (!Cod1PL_FiniteFloat(normal[i])) return 0;
	x = normal[0]; y = normal[1]; z = normal[2];
	length2 = Cod1PL_Add(Cod1PL_Add(Cod1PL_Mul(x, x),
		Cod1PL_Mul(y, y)), Cod1PL_Mul(z, z));
	if (length2 < 0.9999 || length2 > 1.0001) return 0;
	if (x > -1.0) {
		if (!Cod1PL_Spill(Cod1PL_Div(1.0, Cod1PL_Add(x, 1.0)), &h)) return 0;
		s[0] = normal[1]; t[0] = normal[2];
		if (!Cod1PL_Spill(Cod1PL_Sub(Cod1PL_Mul(Cod1PL_Mul(y, y), h), 1.0), &s[1])) return 0;
		product = Cod1PL_Mul(Cod1PL_Mul(z, y), h);
		if (!Cod1PL_Spill(product, &s[2]) || !Cod1PL_Spill(product, &t[1])) return 0;
		if (!Cod1PL_Spill(Cod1PL_Mul(Cod1PL_Mul(z, z), h), &zz)) return 0;
		if (!Cod1PL_Spill(Cod1PL_Sub(zz, 1.0), &t[2])) return 0;
	} else {
		if (z == -1.0 || !Cod1PL_Spill(Cod1PL_Div(1.0, Cod1PL_Add(z, 1.0)), &h)) return 0;
		product = Cod1PL_Mul(Cod1PL_Mul(x, y), h);
		if (!Cod1PL_Spill(-product, &s[0]) || !Cod1PL_Spill(product, &t[1])) return 0;
		if (!Cod1PL_Spill(Cod1PL_Sub(1.0, Cod1PL_Mul(Cod1PL_Mul(y, y), h)), &s[1])) return 0;
		s[2] = -normal[1]; t[2] = normal[0];
		if (!Cod1PL_Spill(Cod1PL_Sub(Cod1PL_Mul(Cod1PL_Mul(x, x), h), 1.0), &t[0])) return 0;
	}
	return 1;
}

/* Rejection leaves *out unchanged. Origin and vertex must already share the
 * same local space. Plain mode does not read normal. */
static int Cod1PL_ProjectVertex(const cod1PLProjectionLight_t *light,
	const float position[3], const float normal[3], int useBasis,
	cod1PLRoundMode_t mode, cod1PLProjectionVertex_t *out) {
	cod1PLProjectionVertex_t candidate;
	float reciprocal, scaled[3], z, attenuation, value;
	float dx, dy, dz, sb[3], tb[3];
	double raw[3], sd, td, retainedT, distance, term;
	int i;
	if (!light || !position || !out || mode < COD1_PL_ROUND_NEAREST ||
		mode > COD1_PL_ROUND_ZERO || FLT_RADIX != 2 || FLT_MANT_DIG != 24 ||
		DBL_MANT_DIG != 53 || !Cod1PL_FiniteFloat(light->radius) ||
		light->radius <= 0.0f) return 0;
	if (!Cod1PL_Spill(Cod1PL_Div(1.0, light->radius), &reciprocal)) return 0;
	for (i = 0; i < 3; ++i) {
		if (!Cod1PL_FiniteFloat(light->origin[i]) ||
			!Cod1PL_FiniteFloat(light->color[i]) ||
			!Cod1PL_FiniteFloat(position[i])) return 0;
		raw[i] = Cod1PL_Sub(light->origin[i], position[i]);
		if (!Cod1PL_Spill(Cod1PL_Mul(light->color[i], 255.0), &scaled[i])) return 0;
	}
	if (useBasis) {
		if (!Cod1PL_Basis(normal, sb, tb) || !Cod1PL_Spill(raw[0], &dx) ||
			!Cod1PL_Spill(raw[1], &dy) || !Cod1PL_Spill(raw[2], &dz)) return 0;
		sd = Cod1PL_Add(Cod1PL_Add(Cod1PL_Mul(raw[2], sb[2]),
			Cod1PL_Mul(dx, sb[0])), Cod1PL_Mul(dy, sb[1]));
		td = Cod1PL_Add(Cod1PL_Add(Cod1PL_Mul(dz, tb[2]),
			Cod1PL_Mul(dy, tb[1])), Cod1PL_Mul(dx, tb[0]));
		if (!Cod1PL_Spill(Cod1PL_Add(Cod1PL_Add(Cod1PL_Mul(dz, normal[2]),
			Cod1PL_Mul(dy, normal[1])), Cod1PL_Mul(dx, normal[0])), &z)) return 0;
	} else {
		sd = raw[0]; td = raw[1];
		if (!Cod1PL_Spill(raw[2], &z)) return 0;
	}
	if (!Cod1PL_Spill(Cod1PL_Add(Cod1PL_Mul(sd, reciprocal), 0.5), &candidate.st[0])) return 0;
	retainedT = Cod1PL_Add(Cod1PL_Mul(td, reciprocal), 0.5);
	if (!Cod1PL_Spill(retainedT, &candidate.st[1])) return 0;
	candidate.clip = 0;
	if (candidate.st[0] < 0.0f) candidate.clip |= 1;
	if (candidate.st[0] > 1.0f) candidate.clip |= 2;
	if (retainedT < 0.0) candidate.clip |= 4;
	if (retainedT > 1.0) candidate.clip |= 8;
	if (z > light->radius) candidate.clip |= 16;
	if (z < -light->radius) candidate.clip |= 32;
	distance = z < 0.0f ? -(double)z : (double)z;
	if ((candidate.clip & 48) != 0) attenuation = 0.0f;
	else if (Cod1PL_Mul(light->radius, 0.5) > distance) attenuation = 1.0f;
	else {
		term = Cod1PL_Mul(Cod1PL_Sub(light->radius, distance), reciprocal);
		if (!Cod1PL_Spill(Cod1PL_Add(term, term), &attenuation)) return 0;
	}
	for (i = 0; i < 3; ++i) {
		if (!Cod1PL_Spill(Cod1PL_Mul(attenuation, scaled[i]), &value) ||
			!Cod1PL_QuantizeByte(value, mode, &candidate.rgba[i])) return 0;
	}
	candidate.rgba[3] = 255;
	*out = candidate;
	return 1;
}

/* Native uses ushort values; the port stores them in unsigned-int glIndex_t.
 * Two passes preserve output atomically on invalid indices/capacity. */
static int Cod1PL_FilterTriangles(const unsigned int *indices, int count,
	const unsigned char *clips, int numVertices, unsigned int *out, int capacity) {
	int i, kept, write;
	unsigned int a, b, c;
	if (count < 0 || count > COD1_PL_MAX_INDICES || count % 3 != 0 ||
		numVertices < 0 || numVertices > COD1_PL_MAX_VERTICES || capacity < 0) return -1;
	if (count == 0) return 0;
	if (!indices || !clips || !out) return -1;
	kept = 0;
	for (i = 0; i < count; i += 3) {
		a = indices[i]; b = indices[i+1]; c = indices[i+2];
		if (a >= (unsigned int)numVertices || b >= (unsigned int)numVertices ||
			c >= (unsigned int)numVertices || a > 65535U || b > 65535U || c > 65535U) return -1;
		if ((clips[a] & clips[b] & clips[c]) == 0) kept += 3;
	}
	if (kept > capacity) return -1;
	write = 0;
	for (i = 0; i < count; i += 3) {
		a = indices[i]; b = indices[i+1]; c = indices[i+2];
		if ((clips[a] & clips[b] & clips[c]) != 0) continue;
		out[write++] = a; out[write++] = b; out[write++] = c;
	}
	return write;
}

#endif
