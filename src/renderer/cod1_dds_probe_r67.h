/*
 * Native CoD1 DDS format disposition, recovered from 0x4e1cd9..0x4e1e2e.
 *
 * This is a format-priority probe, not a payload decoder or upload validator.
 * A nonzero return means the original loader claimed the DDS route; it does
 * not establish valid dimensions, complete payload or multiplyImage support.
 * In particular, native FourCC recognition precedes the header-size check.
 * Only zero authorizes the original DDS -> TGA -> JPEG retry sequence.
 *
 * See research/dds_native_r67/REPORT.md and the independent byte oracle.
 */
#ifndef COD1_DDS_PROBE_R67_H
#define COD1_DDS_PROBE_R67_H

#include <limits.h>

#if UINT_MAX != 0xffffffffU
#error "Native CoD1 DDS format disposition requires 32-bit unsigned int"
#endif

static unsigned int Cod1DDSR67_ReadLE32( const unsigned char *bytes ) {
	return (unsigned int)bytes[0]
		| ( (unsigned int)bytes[1] << 8 )
		| ( (unsigned int)bytes[2] << 16 )
		| ( (unsigned int)bytes[3] << 24 );
}

static unsigned int Cod1DDSR67_ProbeFormat( const unsigned char *file,
		int fileBytes ) {
	unsigned int fourCC, format, width, height, mipCount, maxDimension;

	/* The original full-read loader rejects its signed FS count <= 128. */
	if ( !file || fileBytes <= 128 ) {
		return 0;
	}
	if ( Cod1DDSR67_ReadLE32( file ) != 0x20534444U ) {
		return 0;
	}

	fourCC = Cod1DDSR67_ReadLE32( file + 84 );
	if ( fourCC == 0x31545844U ) {
		format = 0x83f1U;
	} else if ( fourCC == 0x33545844U ) {
		format = 0x83f2U;
	} else if ( fourCC == 0x35545844U ) {
		format = 0x83f3U;
	} else {
		return 0;
	}

	/* Native bad-size return preserves the already published format. */
	if ( Cod1DDSR67_ReadLE32( file + 4 ) != 124U ) {
		return format;
	}

	width = Cod1DDSR67_ReadLE32( file + 16 );
	height = Cod1DDSR67_ReadLE32( file + 12 );
	if ( ( width & ( width - 1U ) ) || ( height & ( height - 1U ) ) ) {
		return 0;
	}

	mipCount = Cod1DDSR67_ReadLE32( file + 28 );
	if ( mipCount != 0U && mipCount != 1U ) {
		maxDimension = width > height ? width : height;
		/* x86 SHL masks CL to five bits; unsigned arithmetic wraps at 32. */
		if ( ( 1U << ( mipCount & 31U ) ) != ( maxDimension << 1 ) ) {
			return 0;
		}
	}
	if ( width > 32768U || height > 32768U ) {
		return 0;
	}
	return format;
}

#endif
