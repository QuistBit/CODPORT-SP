/* CoD1 sunfile configuration: native ri+0x44 callback at 0x431550.
 * Pure bounded C89 transcription of its default space-delimited parse mode.
 * GPL-3.0-or-later, matching the surrounding renderer. */
#ifndef COD1_SUN_CONFIG_H
#define COD1_SUN_CONFIG_H

#include <stddef.h>
#include <string.h>
#include <float.h>

#define COD1_SUN_NAME_CAPACITY 64
#define COD1_SUN_TOKEN_CAPACITY 1024

typedef enum {
	C1SUN_SPRITE_SHADER = 0,
	C1SUN_SPRITE_SIZE,
	C1SUN_FLARE_SHADER,
	C1SUN_FLARE_MIN_SIZE,
	C1SUN_FLARE_MIN_ANGLE,
	C1SUN_FLARE_MAX_SIZE,
	C1SUN_FLARE_MAX_ANGLE,
	C1SUN_FLARE_MAX_ALPHA,
	C1SUN_FLARE_FADEIN,
	C1SUN_FLARE_FADEOUT,
	C1SUN_BLIND_MIN_ANGLE,
	C1SUN_BLIND_MAX_ANGLE,
	C1SUN_BLIND_MAX_DARKEN,
	C1SUN_BLIND_FADEIN,
	C1SUN_BLIND_FADEOUT,
	C1SUN_GLARE_MIN_ANGLE,
	C1SUN_GLARE_MAX_ANGLE,
	C1SUN_GLARE_MAX_LIGHTEN,
	C1SUN_GLARE_FADEIN,
	C1SUN_GLARE_FADEOUT,
	C1SUN_COUNT
} cod1SunKey_t;

static const char * const cod1SunKeys[C1SUN_COUNT] = {
	"r_sunsprite_shader", "r_sunsprite_size", "r_sunflare_shader",
	"r_sunflare_min_size", "r_sunflare_min_angle", "r_sunflare_max_size",
	"r_sunflare_max_angle", "r_sunflare_max_alpha", "r_sunflare_fadein",
	"r_sunflare_fadeout", "r_sunblind_min_angle", "r_sunblind_max_angle",
	"r_sunblind_max_darken", "r_sunblind_fadein", "r_sunblind_fadeout",
	"r_sunglare_min_angle", "r_sunglare_max_angle", "r_sunglare_max_lighten",
	"r_sunglare_fadein", "r_sunglare_fadeout"
};

static const char * const cod1SunDefaults[C1SUN_COUNT] = {
	"sun", "16", "sunFlareShader", "0", "45", "2500", "2", "1", "1", "1",
	"30", "5", ".75", ".5", "3", "30", "5", ".75", ".5", "3"
};

typedef struct {
	char spriteShader[COD1_SUN_NAME_CAPACITY];
	char flareShader[COD1_SUN_NAME_CAPACITY];
	/* Slots 0 and 2 are unused numerically. Other slots retain cvar.value,
	 * including negative values and authored min/max ordering. */
	float value[C1SUN_COUNT];
} cod1SunParams_t;

typedef struct {
	unsigned int seenMask;
	unsigned int assignments;
	unsigned int unknownKeys;
	unsigned int truncatedTokens;
	unsigned int line;
	int complete;
} cod1SunApplyResult_t;

/* Called for every recognized assignment, including duplicates and empty
 * values. The engine adapter must use its force setter (ri.Cvar_Set). The
 * callback is deliberately void: native callback counts presence even when
 * the cvar setter ignores an identical string. */
typedef void (*cod1SunSetCallback_t)( void *context, int key, const char *value );
typedef void (*cod1SunUnknownCallback_t)( void *context, const char *key,
		unsigned int line );

typedef struct {
	const char *text;
	size_t length, cursor;
	unsigned int line, truncatedTokens;
} cod1SunLexer_t;

static int Cod1Sun_Character( const cod1SunLexer_t *lexer, size_t at ) {
	if ( at >= lexer->length ) return 0;
	/* Native whitespace uses signed char. Bytes with the high bit set are
	 * therefore whitespace in its default C/ASCII token mode. */
	return (int)(signed char)lexer->text[at];
}

static int Cod1Sun_EqualKey( const char *a, const char *b ) {
	int ca, cb;
	do {
		ca = (unsigned char)*a++;
		cb = (unsigned char)*b++;
		if ( ca >= 'A' && ca <= 'Z' ) ca += 'a' - 'A';
		if ( cb >= 'A' && cb <= 'Z' ) cb += 'a' - 'A';
		if ( ca != cb ) return 0;
	} while ( ca );
	return 1;
}

/* Native COM_Parse in a new session starts in spaceDelimited=1. It retains
 * quoted empty strings, handles only backslash-quote as a quoted escape, and
 * truncates tokens to 1023 bytes while consuming their full source extent.
 * For a value, crossing whitespace newline yields an empty token and leaves
 * the next nonspace token available for the following key. A newline inside
 * a block comment changes line count but does not set that whitespace flag. */
static void Cod1Sun_Token( cod1SunLexer_t *lexer, int allowLineBreak,
		char token[COD1_SUN_TOKEN_CAPACITY] ) {
	size_t at, count;
	int c, next, crossedLine, truncated, escapedQuote;

	at = lexer->cursor;
	count = 0;
	crossedLine = truncated = 0;
	token[0] = 0;
	for ( ;; ) {
		c = Cod1Sun_Character( lexer, at );
		while ( c && c <= ' ' ) {
			if ( c == '\n' ) { lexer->line++; crossedLine = 1; }
			at++;
			c = Cod1Sun_Character( lexer, at );
		}
		if ( !c ) { lexer->cursor = at; return; }
		if ( crossedLine && !allowLineBreak ) {
			lexer->cursor = at;
			return;
		}
		next = Cod1Sun_Character( lexer, at + 1 );
		if ( c == '/' && next == '/' ) {
			while ( c && c != '\n' ) {
				at++;
				c = Cod1Sun_Character( lexer, at );
			}
			continue;
		}
		if ( c == '/' && next == '*' ) {
			at += 2;
			for ( ;; ) {
				c = Cod1Sun_Character( lexer, at );
				if ( !c ) break;
				if ( c == '*' && Cod1Sun_Character( lexer, at + 1 ) == '/' ) {
					at += 2;
					break;
				}
				if ( c == '\n' ) lexer->line++;
				at++;
			}
			continue;
		}
		break;
	}
	if ( c == '"' ) {
		at++;
		for ( ;; ) {
			c = Cod1Sun_Character( lexer, at );
			if ( !c ) break;
			at++;
			escapedQuote = 0;
			if ( c == '\\' && Cod1Sun_Character( lexer, at ) == '"' ) {
				c = '"';
				at++;
				escapedQuote = 1;
			} else if ( c == '"' ) break;
			/* The original tests the character AFTER the consumed char. */
			if ( !escapedQuote && Cod1Sun_Character( lexer, at ) == '\n' ) lexer->line++;
			if ( count < COD1_SUN_TOKEN_CAPACITY - 1 ) token[count++] = (char)c;
			else truncated = 1;
		}
	} else {
		while ( c > ' ' ) {
			if ( count < COD1_SUN_TOKEN_CAPACITY - 1 ) token[count++] = (char)c;
			else truncated = 1;
			at++;
			c = Cod1Sun_Character( lexer, at );
		}
	}
	token[count] = 0;
	lexer->cursor = at;
	if ( truncated ) lexer->truncatedTokens++;
}

/* Returns native success only if all 20 distinct keys were encountered.
 * Missing keys do not roll back already applied values. Unknown keys skip
 * the entire remaining physical line. Empty key terminates the file. The
 * bounded input ends at the first NUL or supplied length, whichever comes
 * first; an unterminated quote cannot read beyond that bound. */
static int Cod1Sun_Apply( const char *text, size_t length,
		cod1SunSetCallback_t setValue, cod1SunUnknownCallback_t unknown,
		void *context, cod1SunApplyResult_t *out ) {
	cod1SunLexer_t lexer;
	cod1SunApplyResult_t result;
	char token[COD1_SUN_TOKEN_CAPACITY];
	int key, c;
	unsigned int keyLine;

	if ( !text || !setValue || !out ) return 0;
	memset( &lexer, 0, sizeof(lexer) );
	memset( &result, 0, sizeof(result) );
	lexer.text = text;
	lexer.length = length;
	lexer.line = 1;
	for ( ;; ) {
		Cod1Sun_Token( &lexer, 1, token );
		if ( !token[0] ) break;
		keyLine = lexer.line;
		for ( key = 0; key < C1SUN_COUNT; key++ ) {
			if ( Cod1Sun_EqualKey( token, cod1SunKeys[key] ) ) break;
		}
		if ( key == C1SUN_COUNT ) {
			result.unknownKeys++;
			if ( unknown ) unknown( context, token, keyLine );
			c = Cod1Sun_Character( &lexer, lexer.cursor );
			while ( c ) {
				lexer.cursor++;
				if ( c == '\n' ) { lexer.line++; break; }
				c = Cod1Sun_Character( &lexer, lexer.cursor );
			}
			continue;
		}
		Cod1Sun_Token( &lexer, 0, token );
		setValue( context, key, token );
		result.assignments++;
		result.seenMask |= 1u << key;
	}
	result.line = lexer.line;
	result.truncatedTokens = lexer.truncatedTokens;
	result.complete = result.seenMask == ((1u << C1SUN_COUNT) - 1u);
	*out = result;
	return result.complete;
}

/* Uses actual renderer cvar strings/float values rather than guessing how a
 * platform CRT parses arbitrary numeric spellings. Empty shader strings are
 * allowed and disable that component; the nonempty string "0" is retained.
 * Names longer than the renderer's 63-character shader domain and nonfinite
 * numeric values are explicit port-domain failures. Output commits atomically. */
static int Cod1Sun_Snapshot( const char * const strings[C1SUN_COUNT],
		const float values[C1SUN_COUNT], cod1SunParams_t *out ) {
	cod1SunParams_t result;
	size_t spriteLength, flareLength;
	int i;

	if ( !strings || !values || !out || !strings[C1SUN_SPRITE_SHADER] ||
		 !strings[C1SUN_FLARE_SHADER] ) return 0;
	for ( spriteLength = 0; spriteLength < COD1_SUN_NAME_CAPACITY; spriteLength++ )
		if ( !strings[C1SUN_SPRITE_SHADER][spriteLength] ) break;
	for ( flareLength = 0; flareLength < COD1_SUN_NAME_CAPACITY; flareLength++ )
		if ( !strings[C1SUN_FLARE_SHADER][flareLength] ) break;
	if ( spriteLength == COD1_SUN_NAME_CAPACITY ||
		 flareLength == COD1_SUN_NAME_CAPACITY ) return 0;
	memset( &result, 0, sizeof(result) );
	for ( i = 0; i < C1SUN_COUNT; i++ ) {
		if ( i == C1SUN_SPRITE_SHADER || i == C1SUN_FLARE_SHADER ) continue;
		if ( !(values[i] >= -FLT_MAX && values[i] <= FLT_MAX) ) return 0;
		result.value[i] = values[i];
	}
	memcpy( result.spriteShader, strings[C1SUN_SPRITE_SHADER], spriteLength + 1 );
	memcpy( result.flareShader, strings[C1SUN_FLARE_SHADER], flareLength + 1 );
	*out = result;
	return 1;
}

/* World load resets only the name here; its caller must also zero the full
 * cached sun runtime/fade object, matching native memset(0x11ce798,0,0x9c).
 * The 20 global cvars persist. */
static void Cod1Sun_ResetName( char name[COD1_SUN_NAME_CAPACITY] ) {
	if ( name ) name[0] = 0;
}

/* 1: captured; 0: empty/missing warning, old name retained; -1: overflow
 * warning, name cleared. These are the native top-level sunfile semantics. */
static int Cod1Sun_CaptureName( char name[COD1_SUN_NAME_CAPACITY], const char *token ) {
	size_t length;
	if ( !name || !token || !token[0] ) return 0;
	for ( length = 0; length < COD1_SUN_NAME_CAPACITY; length++ )
		if ( !token[length] ) break;
	if ( length == COD1_SUN_NAME_CAPACITY ) { name[0] = 0; return -1; }
	memmove( name, token, length + 1 );
	return 1;
}

#endif
