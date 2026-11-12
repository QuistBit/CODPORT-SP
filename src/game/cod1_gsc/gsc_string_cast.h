#ifndef GSC_STRING_CAST_H
#define GSC_STRING_CAST_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* R165 shared semantic input/result only. Neither layout is a retail Value
 * or a shared EXE/GAME ABI record. type is the raw native Value tag. */
typedef struct { uint32_t type;int32_t integer;float number,vector[3];char string[128]; } GscStringCastValue;
typedef struct { uint32_t converted;char string[128]; } GscStringCastResult;
#endif
