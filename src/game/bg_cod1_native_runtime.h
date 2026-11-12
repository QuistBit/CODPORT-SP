/* Native CoD1 viewmodel pose request shared by cgame and the executable.
 * This is an in-process syscall payload, not a network or refEntity_t ABI.
 */
#ifndef BG_COD1_NATIVE_RUNTIME_H
#define BG_COD1_NATIVE_RUNTIME_H

#include "q_shared.h"

#define COD1_NATIVE_LEAVES 21
#define COD1_NATIVE_STATE_VERSION 1
#define COD1_NATIVE_DRAW_VIEW 1

typedef struct {
    int version;
    int size;
    int flags;
    float weight[COD1_NATIVE_LEAVES];
    float time[COD1_NATIVE_LEAVES];
} cod1NativeAnimState_t;

typedef struct {
    orientation_t flash;
    orientation_t brass;
    int flashIndex;
    int brassIndex;
} cod1NativeViewTags_t;

#endif
