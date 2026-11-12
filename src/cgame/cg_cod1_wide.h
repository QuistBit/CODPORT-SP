/* Preserve the 4:3 vertical FOV and reveal more on wide displays. */
static float CG_Cod1WideFov(float fov, int width, int height) {
    double aspect;
    if (width <= 0 || height <= 0) return fov;
    aspect = (double)width / height;
    if (aspect <= 4.0 / 3.0) return fov;
    return (float)(atan(tan(fov * M_PI / 360.0) *
                        (aspect / (4.0 / 3.0))) * 360.0 / M_PI);
}
