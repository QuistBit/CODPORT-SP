#ifndef BG_COD1_GRENADE_MATH_H
#define BG_COD1_GRENADE_MATH_H

/* CoD1 grenade orientation, reconstructed from the supplied gamex86.dll.
 * Launch: 0x20025210; impact: 0x20023b40; slope pitch: 0x20011860.
 * Binary identity and remaining differences: docs/reverse/GRENADE_ANGLES_R1.md.
 * Requires q_shared.h. RNG samples are explicit so these rules are testable;
 * this does not reproduce the original game's global random-number stream.
 */
static float BG_Cod1GrenadeAngleMod( float angle ) {
    /* Retail's float constants and 16-bit angle wrapping. */
    return ( 360.0f / 65536.0f ) *
        ( (int)( angle * ( 65536.0f / 360.0f ) ) & 65535 );
}

static float BG_Cod1GrenadeGroundPitch( float yaw, const vec3_t normal ) {
    vec3_t yawAngles, forward, projected, angles;
    VectorSet( yawAngles, 0, yaw, 0 );
    AngleVectors( yawAngles, forward, NULL, NULL );
    ProjectPointOnPlane( projected, forward, normal );
    vectoangles( projected, angles );
    /* RTCW vectoangles returns negative pitch; retail vectopitch is [0,360). */
    return angles[PITCH] < 0 ? angles[PITCH] + 360.0f : angles[PITCH];
}

static void BG_Cod1GrenadeInitialAngles( const vec3_t velocity,
    unsigned int pitchRandom15, unsigned int rollRandom15,
    vec3_t angles, vec3_t angularVelocity ) {
    vectoangles( velocity, angles );
    /* Match retail vectoangles' positive representation before subtracting. */
    if ( angles[PITCH] < 0 ) {
        angles[PITCH] += 360.0f;
    }
    angles[PITCH] = BG_Cod1GrenadeAngleMod( angles[PITCH] - 120.0f );
    angularVelocity[PITCH] = 720.0f +
        ( ( pitchRandom15 & 0x7fff ) * 90.0f / 32768.0f - 45.0f );
    angularVelocity[YAW] = 0;
    angularVelocity[ROLL] = 360.0f +
        ( ( rollRandom15 & 0x7fff ) * 90.0f / 32768.0f - 45.0f );
}

static void BG_Cod1GrenadeImpactAngles( vec3_t angles, vec3_t angularVelocity,
    const vec3_t normal, qboolean stopped, unsigned int random15 ) {
    float groundPitch, delta, absDelta, pitch, factor;

    if ( normal[2] <= 0.1f ) {
        if ( !stopped ) {
            angularVelocity[PITCH] = BG_Cod1GrenadeAngleMod(
                angularVelocity[PITCH] + (int)( random15 & 127 ) - 63 );
        }
        return;
    }

    groundPitch = BG_Cod1GrenadeGroundPitch( angles[YAW], normal );
    delta = AngleSubtract( groundPitch, angles[PITCH] );
    absDelta = (float)fabs( delta );
    if ( !stopped ) {
        factor = 0.85f + ( random15 & 0x7fff ) / 32768.0f * 0.3f;
        if ( absDelta < 80.0f ) {
            factor = -factor;
        }
        angularVelocity[PITCH] *= factor;
    }

    pitch = BG_Cod1GrenadeAngleMod( angles[PITCH] );
    if ( pitch > 180.0f ) {
        pitch -= 360.0f;
    }
    angles[PITCH] = pitch;

    /* Thresholds use the pre-normalization slope-relative difference,
     * NOT the absolute model pitch (retail stack slots are reused here). */
    if ( !stopped && absDelta >= 45.0f ) {
        if ( absDelta < 80.0f ) {
            pitch += delta * 0.25f;
        }
        angles[PITCH] = BG_Cod1GrenadeAngleMod( pitch );
        return;
    }

    angles[PITCH] = BG_Cod1GrenadeAngleMod(
        groundPitch + ( fabs( pitch ) > 90.0 ? 180.0f : 0.0f ) );
    /* Retail preserves yaw and roll; stopping the trajectory is the caller's job. */
}

#endif
