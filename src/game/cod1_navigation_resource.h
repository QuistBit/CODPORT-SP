/* Private port ABI69. This identifies bytes captured from the actual loaded
 * CM BSP, not a path, hash, caller DATA buffer or finalized navigation graph. */
#ifndef COD1_NAVIGATION_RESOURCE_H
#define COD1_NAVIGATION_RESOURCE_H
#include <stdint.h>
#define COD1_NAV_RESOURCE_VERSION 1u
#define COD1_NAV_RESOURCE_LIMIT (16u*1024u*1024u)
enum { COD1_NAV_DESCRIBE=1, COD1_NAV_COPY=2, COD1_NAV_CURRENT=3 };
typedef struct {
 uint32_t version,bytes;
 uint64_t incarnation;
 uint32_t length,operation;
} cod1NavigationRequest_t;
typedef struct {
 uint32_t version,bytes;
 uint64_t incarnation;
 uint32_t length,reserved;
} cod1NavigationResource_t;
typedef char cod1NavigationRequestSize[(sizeof(cod1NavigationRequest_t)==24)?1:-1];
typedef char cod1NavigationResourceSize[(sizeof(cod1NavigationResource_t)==24)?1:-1];
#endif
