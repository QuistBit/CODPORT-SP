#ifndef G_COD1_ANIM_DATA_H
#define G_COD1_ANIM_DATA_H
#include "cod1_gsc/gsc_fixture.h"
/* Explicit one selected VM per ready map incarnation. All its scripts share
 * a distinct fixed anim data owner. VM teardown does not permit same-map
 * rebinding; map reset retires all tasks/aliases. No pointer is retained and
 * no cross-runtime carry, animation tree/pose, Session or SAVE is supplied. */
int G_Cod1SelectAnimData(GscRuntime *,char *,size_t);
#endif
