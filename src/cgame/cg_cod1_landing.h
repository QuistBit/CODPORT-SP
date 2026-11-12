/* PATCH57: original cgame 0x3002a961..0x3002a9dd.
 * Additional weapon displacement, beyond the camera's landChange envelope.
 * World vertical, not camera/model up; not attenuated by ADS.
 */
#ifndef CG_COD1_LANDING_H
#define CG_COD1_LANDING_H
static float COD1_LandWeaponOffset(int elapsed,float magnitude) {
    if(elapsed<0 || elapsed>=450)return 0;
    if(elapsed<150)return ((float)elapsed*magnitude)*.25f/150.f;
    return ((float)(450-elapsed)*magnitude)*.25f/300.f;
}
#endif
