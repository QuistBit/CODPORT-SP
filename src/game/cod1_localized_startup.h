#ifndef COD1_LOCALIZED_STARTUP_H
#define COD1_LOCALIZED_STARTUP_H
#include <stdint.h>
/* Paired native port ABI. This is a one-shot script-start capability, not a
 * general permission to register localized strings during gameplay. */
enum { COD1_LOCALIZED_STARTUP_VERSION=1, COD1_LOCALIZED_STARTUP_ARM=1,
 COD1_LOCALIZED_STARTUP_BEGIN=2, COD1_LOCALIZED_STARTUP_END=3,
 COD1_LOCALIZED_STARTUP_CANCEL=4 };
typedef struct {
 uint32_t version,bytes,operation,reserved;
 uint64_t session_id,map_epoch;
 uint32_t player_slot,player_generation;
 char owner[128];
} cod1LocalizedStartup_t;
#endif
