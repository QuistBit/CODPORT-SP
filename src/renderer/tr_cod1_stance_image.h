/* The retail prone DDS has a broad, dark fringe outside its triangular frame.
 * Preserve its pose and the native transition flash, but remove only the
 * outside fringe and bring the common translucent interior closer to stand.
 * Operates on decoded pixels so no edited copy of the original art is needed.
 */
static void R_Cod1NormalizeStancePixels( const char *name, byte *rgba,
                                        int width, int height ) {
	int prone, y, x;
	if ( width != 64 || height != 64 || !rgba ) {
		return;
	}
	if ( !Q_stricmp( name, "gfx/hud/stance_prone.dds" ) ) {
		prone = 1;
	} else if ( !Q_stricmp( name, "gfx/hud/stance_crouch.dds" ) ) {
		prone = 0;
	} else {
		return; /* standing icon and yellow transition flash remain untouched */
	}

	for ( y = 0; y < height; ++y ) {
		int left = width, right = -1;
		for ( x = 0; x < width; ++x ) {
			const byte *p = rgba + 4 * ( y * width + x );
			if ( p[0] + p[1] + p[2] >= 390 && p[3] >= 150 ) {
				if ( x < left ) left = x;
				right = x;
			}
		}
		for ( x = 0; x < width; ++x ) {
			byte *p = rgba + 4 * ( y * width + x );
			int lightness = ( p[0] + p[1] + p[2] ) / 3;
			/* The prone source has near-black alpha~95 pixels up to four
			 * texels outside the visible frame, including a full strip at
			 * y=60..63.  Leave the frame's antialiased one-pixel edge. */
			if ( prone && lightness < 115 && p[3] <= 160 &&
			     ( right < 0 || x < left - 1 || x > right + 1 ) ) {
				p[3] = 0;
				continue;
			}
			/* Both low stances use a slightly darker translucent interior
			 * (approximately RGB82/A95 vs standing RGB90/A82). */
			if ( lightness >= 75 && lightness <= 105 &&
			     p[3] >= 85 && p[3] <= 106 ) {
				int c;
				for ( c = 0; c < 3; ++c ) {
					int value = ( p[c] * 90 + 41 ) / 82;
					p[c] = (byte)( value > 255 ? 255 : value );
				}
				p[3] = (byte)( ( p[3] * 82 + 47 ) / 95 );
			} else if ( lightness >= 150 && p[3] >= 180 ) {
				/* The baked crouch/prone frame and figure are darker than
				 * standing on screen.  Cap the lift at 16 RGB levels. */
				int c;
				for ( c = 0; c < 3; ++c ) {
					int lift = ( p[c] * 8 + 50 ) / 100;
					int value;
					if ( lift > 16 ) lift = 16;
					value = p[c] + lift;
					p[c] = (byte)( value > 255 ? 255 : value );
				}
			}
		}
	}
}
