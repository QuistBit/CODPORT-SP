#ifndef COD1_MODEL_CONFIGSTRING_R50_H
#define COD1_MODEL_CONFIGSTRING_R50_H
/* Paired native GAME/EXE extension. Separate from historical g_public.h
 * overlays used by the Windows build. Return 0 for absent, -1 if unavailable. */
#define G_COD1_FIND_MODEL_CONFIGSTRING_R50 1050
#define COD1_R50_MODEL_NAME_BYTES 64
int trap_Cod1FindModelConfigstringR50(const char *name);
#endif
