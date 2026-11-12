#ifndef G_COD1_ANIMATION_REFS_H
#define G_COD1_ANIMATION_REFS_H
#include "cod1_gsc/gsc_fixture.h"
/* Explicit same-VM/map catalog binding after G_Cod1SelectAnimData, before
 * startup. Copies declared ATR names/semantic identities, not clips/native
 * numeric IDs. No playback, #animtree value, Session or SAVE support. */
int G_Cod1SelectAnimationReferences(GscRuntime *,const char *,const void *,size_t,char *,size_t);
#endif
