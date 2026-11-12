/* SPDX-License-Identifier: GPL-3.0-or-later
 * SHOOTING_TEST_R1: exact native scope mask only. Normalize BEFORE image-cache
 * lookup, and also for direct creation, so preload order cannot preserve an
 * earlier GL_CLAMP variant. Other RTCW/CoD images keep their requested mode.
 * No image contents are modified; both TGA and DDS lookup names are supported.
 */
#ifndef TR_COD1_SCOPE_IMAGE_H
#define TR_COD1_SCOPE_IMAGE_H
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
static int R_Cod1ScopeImageWrap(const char *name, int requested) {
    if (!name || (Q_stricmp(name,"ui/assets/reticle_q.tga") &&
                  Q_stricmp(name,"ui/assets/reticle_q.dds") &&
                  Q_stricmp(name,"ui/assets/reticle_q"))) return requested;
    if (atof(glConfig.version_string)>=1.2 ||
        strstr(glConfig.extensions_string,"GL_EXT_texture_edge_clamp") ||
        strstr(glConfig.extensions_string,"GL_SGIS_texture_edge_clamp"))
        return GL_CLAMP_TO_EDGE;
    /* Preserve compatibility on renderers without edge-clamp support. */
    return requested;
}
#endif
