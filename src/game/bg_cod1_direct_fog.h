/* Experimental CoD BSP59 scene fog transport. The original server command is
 * `fog near far density red green blue transitionMs`. C1DFOG1 is only the
 * port's retained latest target for pre-init clients, late joins and saves.
 * No script is interpreted or map-specific value synthesized here.
 */
#ifndef BG_COD1_DIRECT_FOG_H
#define BG_COD1_DIRECT_FOG_H

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define COD1_DIRECT_FOG_TAG "C1DFOG1"
#define COD1_DIRECT_FOG_WIRE_BYTES 128
#define COD1_DIRECT_FOG_FIELD_BYTES 32
#define COD1_DIRECT_FOG_CLEAR_CACHE COD1_DIRECT_FOG_TAG " clear"

typedef enum cod1FogCacheKind_e {
	COD1_FOG_UNOWNED,
	COD1_FOG_SET,
	COD1_FOG_CLEAR,
	COD1_FOG_MALFORMED
} cod1FogCacheKind_t;

typedef struct cod1DirectFogEvent_s {
	float nearDist, farDist, density, color[3];
	int transitionMs;
} cod1DirectFogEvent_t;

/* Read-only target snapshot for native AI visibility. This is not a retained
 * map/Session certificate: users must revalidate their owners and this wire
 * across callbacks. Values describe the fog target, not renderer blending. */
typedef struct cod1DirectFogVisibility_s {
	float distance, squared;
	char retained[COD1_DIRECT_FOG_WIRE_BYTES];
} cod1DirectFogVisibility_t;

static int Cod1DirectFogValid( const cod1DirectFogEvent_t *event ) {
	int i;
	if ( !event || !( event->nearDist >= 0.0f ) ||
		!( event->farDist > event->nearDist ) ||
		/* The renderer interface casts distances to signed int. */
		!( (double)event->farDist < 2147483648.0 ) ||
		!( event->density >= 0.0f && event->density <= FLT_MAX ) ||
		event->transitionMs < 0 ) {
		return 0;
	}
	for ( i = 0; i < 3; ++i ) {
		if ( !( event->color[i] >= 0.0f && event->color[i] <= 1.0f ) ) {
			return 0;
		}
	}
	return 1;
}

/* The entire prefix is reserved, including corrupt suffixes such as
 * C1DFOG1bad. Never feed these strings to RTCW's numeric parser. */
static int Cod1DirectFogTagged( const char *wire ) {
	return wire && !strncmp( wire, COD1_DIRECT_FOG_TAG,
		sizeof( COD1_DIRECT_FOG_TAG ) - 1 );
}

/* Retail GAME4AD70/CRT rounds decimal ties away from zero. UCRT's
 * default standard rounding differs for %g and %.0f. Select the legacy
 * conversion only for this transport, without changing global CRT options.
 * Native parity is verified for the Windows/MSVC build; other CRT targets
 * retain their existing formatter and require their own native parity gate. */
static int Cod1DirectFogFormat( char *wire, size_t capacity,
        const char *format, ... ) {
 int size;
 va_list args;
 va_start(args, format);
#if defined(_MSC_VER) && defined(_CRT_INTERNAL_PRINTF_STANDARD_ROUNDING)
 size = __stdio_common_vsprintf(
  (_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS & ~_CRT_INTERNAL_PRINTF_STANDARD_ROUNDING) |
  _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
  wire, capacity, format, NULL, args);
#else
 size = vsnprintf(wire, capacity, format, args);
#endif
 va_end(args);
 return size;
}

static int Cod1DirectFogEncode( const cod1DirectFogEvent_t *event,
		char *wire, size_t capacity, int retained ) {
	int size;
	if ( !wire || !capacity || !Cod1DirectFogValid( event ) ) {
		return 0;
	}
	size = Cod1DirectFogFormat( wire, capacity, "%s %g %g %g %g %g %g %d",
		retained ? COD1_DIRECT_FOG_TAG : "fog",
		event->nearDist, event->farDist, event->density,
		event->color[0], event->color[1], event->color[2],
		retained ? 0 : event->transitionMs );
	return size > 0 && (size_t)size < capacity;
}

static int Cod1DirectFogEncodeLive( const cod1DirectFogEvent_t *event,
		char *wire, size_t capacity ) {
	return Cod1DirectFogEncode( event, wire, capacity, 0 );
}

static int Cod1DirectFogEncodeCache( const cod1DirectFogEvent_t *event,
		char *wire, size_t capacity ) {
	return Cod1DirectFogEncode( event, wire, capacity, 1 );
}

static int Cod1DirectFogFloat( const char *token, float *result ) {
	char *end;
	double number;
	if ( !token || !token[0] || strlen( token ) >= COD1_DIRECT_FOG_FIELD_BYTES ) {
		return 0;
	}
	errno = 0;
	number = strtod( token, &end );
	if ( errno || *end || !( number >= 0.0 && number <= FLT_MAX ) ) {
		return 0;
	}
	*result = (float)number;
	return 1;
}

static int Cod1DirectFogInt( const char *token, int *result ) {
	char *end;
	long number;
	if ( !token || !token[0] || strlen( token ) >= COD1_DIRECT_FOG_FIELD_BYTES ) {
		return 0;
	}
	errno = 0;
	number = strtol( token, &end, 10 );
	if ( errno || *end || number < 0 || number > INT_MAX ) {
		return 0;
	}
	*result = (int)number;
	return 1;
}

/* args are seven independently stored strings, never repeated CG_Argv()
 * pointers: CG_Argv uses one static return buffer. */
static int Cod1DirectFogDecodeFields( const char *const args[7],
		cod1DirectFogEvent_t *event ) {
	cod1DirectFogEvent_t parsed;
	if ( !event || !Cod1DirectFogFloat( args[0], &parsed.nearDist ) ||
		!Cod1DirectFogFloat( args[1], &parsed.farDist ) ||
		!Cod1DirectFogFloat( args[2], &parsed.density ) ||
		!Cod1DirectFogFloat( args[3], &parsed.color[0] ) ||
		!Cod1DirectFogFloat( args[4], &parsed.color[1] ) ||
		!Cod1DirectFogFloat( args[5], &parsed.color[2] ) ||
		!Cod1DirectFogInt( args[6], &parsed.transitionMs ) ||
		!Cod1DirectFogValid( &parsed ) ) {
		return 0;
	}
	*event = parsed;
	return 1;
}

static int Cod1DirectFogDecodeCache( const char *wire,
		cod1DirectFogEvent_t *event ) {
	const char *at, *start, *args[7];
	char fields[7][COD1_DIRECT_FOG_FIELD_BYTES];
	size_t length;
	int i;
	cod1DirectFogEvent_t parsed;
	if ( !Cod1DirectFogTagged( wire ) ||
		wire[sizeof( COD1_DIRECT_FOG_TAG ) - 1] != ' ' || !event ||
		strlen( wire ) >= COD1_DIRECT_FOG_WIRE_BYTES ) {
		return 0;
	}
	at = wire + sizeof( COD1_DIRECT_FOG_TAG ) - 1;
	for ( i = 0; i < 7; ++i ) {
		while ( *at == ' ' ) ++at;
		start = at;
		while ( *at && *at != ' ' ) ++at;
		length = (size_t)( at - start );
		if ( !length || length >= sizeof( fields[i] ) ) {
			return 0;
		}
		memcpy( fields[i], start, length );
		fields[i][length] = '\0';
		args[i] = fields[i];
	}
	while ( *at == ' ' ) ++at;
	if ( *at || !Cod1DirectFogDecodeFields( args, &parsed ) ||
		parsed.transitionMs != 0 ) {
		return 0;
	}
	*event = parsed;
	return 1;
}

static cod1FogCacheKind_t Cod1FogDecodeRetained( const char *wire,
		cod1DirectFogEvent_t *event ) {
	if ( !Cod1DirectFogTagged( wire ) ) {
		return COD1_FOG_UNOWNED;
	}
	if ( !strcmp( wire, COD1_DIRECT_FOG_CLEAR_CACHE ) ) {
		return COD1_FOG_CLEAR;
	}
	if ( event && Cod1DirectFogDecodeCache( wire, event ) ) {
		return COD1_FOG_SET;
	}
	return COD1_FOG_MALFORMED;
}

/* GAME7ED09 -> 7EBF0/7EA82, float format descriptor at GAME9FC48:
 * {128,-127,24,8,32,127}. 7E955 increments only when the guard bit AND
 * lower discarded bits are set: an exact half rounds toward zero.
 * The denormal branch rounds at half a float ulp, then shifts one more bit.
 * This adapter accepts the six-significant-digit decimal grammar emitted by
 * our actual %g producer. Longer external tokens need the complete original
 * decimal-to-LD12 conversion (7ED4C); never guess their visibility cutoff.
 * Renderer decoding deliberately continues to use Cod1DirectFogFloat. */
static int Cod1DirectFogNativeFloat( const char *token, float *result ) {
 const char *p = token;
 unsigned int bits, digits = 0, seen = 0, nonzero = 0;
 int point = 0;
 float rounded, lower, unit;
 double number, scaled;
 char *end;
 if ( !p || !result || !*p || strlen(p) >= COD1_DIRECT_FOG_FIELD_BYTES ) return 0;
 if ( *p == '+' || *p == '-' ) ++p;
 for ( ; *p && *p != 'e' && *p != 'E'; ++p ) {
  if ( *p == '.' && !point ) { point = 1; continue; }
  if ( *p < '0' || *p > '9' ) return 0;
  seen = 1;
  if ( *p != '0' ) nonzero = 1;
  if ( nonzero && ++digits > 6 ) return 0;
 }
 if ( !seen ) return 0;
 if ( *p ) {
  ++p;
  if ( *p == '+' || *p == '-' ) ++p;
  if ( !*p ) return 0;
  for ( ; *p; ++p ) if ( *p < '0' || *p > '9' ) return 0;
 }
 errno = 0;
 number = strtod(token, &end);
 if ( errno || *end || !(number >= 0.0 && number <= FLT_MAX) ) return 0;
 if ( number == 0.0 ) {
  rounded = (float)number;
 } else if ( number < FLT_MIN ) {
  bits = 1; memcpy(&unit, &bits, sizeof(unit));
  scaled = number / (double)unit;
  bits = (unsigned int)scaled;
  if ( scaled - (double)bits > 0.75 ) ++bits;
  memcpy(&rounded, &bits, sizeof(rounded));
 } else {
  rounded = (float)number;
  if ( (double)rounded > number ) {
   memcpy(&bits, &rounded, sizeof(bits)); --bits;
   memcpy(&lower, &bits, sizeof(lower));
   if ( number == ((double)lower + (double)rounded) * 0.5 ) rounded = lower;
  }
 }
 *result = rounded;
 return 1;
}

/* Input has already passed full retained-cache validation. Keep the native
 * interpretation local to the AI snapshot; no renderer or owner mutation. */
static int Cod1DirectFogNativeTarget( const char *wire, float *farDist, float *density ) {
 const char *p = wire + sizeof(COD1_DIRECT_FOG_TAG) - 1, *start;
 char token[COD1_DIRECT_FOG_FIELD_BYTES];
 size_t n;
 int i;
 for ( i = 0; i < 3; ++i ) {
  while ( *p == ' ' ) ++p;
  start = p;
  while ( *p && *p != ' ' ) ++p;
  n = (size_t)(p - start);
  if ( !n || n >= sizeof(token) ) return 0;
  memcpy(token, start, n); token[n] = '\0';
  if ( i == 1 && !Cod1DirectFogNativeFloat(token, farDist) ) return 0;
  if ( i == 2 && !Cod1DirectFogNativeFloat(token, density) ) return 0;
 }
 return 1;
}

/* GAME16D90: initialize both cells to FLT_MAX; only seven parsed fields
 * with density>=1 update them. x87 retains the unrounded far*0.828 product
 * while multiplying by the stored float32 distance for the squared cell.
 * Decode the actually retained target so formatting is already reflected.
 * Empty fresh-map state and explicit clear mean no fog cutoff. An unrelated
 * RTCW string or malformed tagged value is a refusal, not unlimited sight. */
static int Cod1DirectFogVisibilityFromCache( const char *wire,
		cod1DirectFogVisibility_t *out ) {
	cod1DirectFogVisibility_t next;
	cod1DirectFogEvent_t event;
	cod1FogCacheKind_t kind;
	unsigned int factorBits = 0x3f53f7cfU;
	float factor;
	double product;
	if ( !wire || !out || strlen( wire ) >= sizeof( next.retained ) ) return 0;
	memset( &next, 0, sizeof( next ) );
	next.distance = next.squared = FLT_MAX;
	if ( *wire ) {
		kind = Cod1FogDecodeRetained( wire, &event );
		if ( kind != COD1_FOG_CLEAR && kind != COD1_FOG_SET ) return 0;
		if ( kind == COD1_FOG_SET &&
			!Cod1DirectFogNativeTarget(wire, &event.farDist, &event.density) ) return 0;
		if ( kind == COD1_FOG_SET && event.density >= 1.0f ) {
			memcpy( &factor, &factorBits, sizeof( factor ) );
			product = (double)event.farDist * (double)factor;
			next.distance = (float)product;
			next.squared = (float)( product * (double)next.distance );
		}
	}
	strcpy( next.retained, wire );
	*out = next;
	return 1;
}

/* Explicit port policy for the short native command: exactly one decimal
 * integer in [0, INT_MAX]. Retail takes a float and does not check argc;
 * negative, fractional, extra and nonfinite inputs are unverified there. */
static int Cod1DirectFogClearMs( const char *token, int *result ) {
	const char *p;
	unsigned int value = 0;
	if ( !token || !*token || !result ||
		strlen( token ) >= COD1_DIRECT_FOG_FIELD_BYTES ) {
		return 0;
	}
	for ( p = token; *p; ++p ) {
		unsigned int digit;
		if ( *p < '0' || *p > '9' ) {
			return 0;
		}
		digit = (unsigned int)( *p - '0' );
		if ( value > ( (unsigned int)INT_MAX - digit ) / 10U ) {
			return 0;
		}
		value = 10U * value + digit;
	}
	*result = (int)value;
	return 1;
}

#endif
