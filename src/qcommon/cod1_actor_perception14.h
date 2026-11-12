#ifndef COD1_WORK_PERCEPTION14_H
#define COD1_WORK_PERCEPTION14_H
#include <stdint.h>
#include <string.h>
enum { C1PER14_SLOTS=36,C1PER14_WORDS=16,C1PER14_INVALID=-2,C1PER14_NONE=-1 };
typedef struct { uint32_t word[C1PER14_WORDS]; } c1per14Row_t;
typedef struct { c1per14Row_t row[C1PER14_SLOTS]; } c1per14Memory_t;
typedef struct { int32_t activation;uint32_t team; } c1per14ScanCell_t;
typedef struct {
 uint32_t time,targetOrigin[3],sentient54;
 unsigned char sentient58;
 uint32_t observerOrigin[3];
} c1per14Observation_t;
typedef char c1per14RowSize[sizeof(c1per14Row_t)==0x40?1:-1];
typedef char c1per14MemorySize[sizeof(c1per14Memory_t)==0x900?1:-1];

/* Numeric projection of full73D90/73DD0 for an independently authenticated
 * complete pool. start=0 is First; Next must first establish the actual prior
 * member and pass its index+1. NULL is not a synonym for First. The x86 SHL
 * uses only CL's low five bits, including team values outside0..4. */
static int C1PER14_Scan(const c1per14ScanCell_t*pool,unsigned start,uint32_t mask){
 unsigned i;
 if(!pool||start>C1PER14_SLOTS)return C1PER14_INVALID;
 for(i=start;i<C1PER14_SLOTS;i++)
  if(pool[i].activation>=0&&(mask&(UINT32_C(1)<<(pool[i].team&31u))))return (int)i;
 return C1PER14_NONE;
}

/* Full4FD10 stores, in native order; origin payloads are copied as raw bits.
 * The native routine returns a row address, not an AI success flag. This
 * helper only applies that numeric row operation. Membership, actual GAME
 * clock/pose, allocation lifetime and the native caller are external gates. */
static void C1PER14_Observe(c1per14Row_t*row,const c1per14Observation_t*input){
 c1per14Observation_t in=*input;
 row->word[0x18/4]=in.time;
 row->word[0x20/4]=in.targetOrigin[0];
 row->word[0x24/4]=in.targetOrigin[1];
 row->word[0x28/4]=in.targetOrigin[2];
 row->word[0x30/4]=(uint32_t)in.sentient58;
 row->word[0x2c/4]=in.sentient54;
 row->word[0x34/4]=in.observerOrigin[0];
 row->word[0x38/4]=in.observerOrigin[1];
 row->word[0x3c/4]=in.observerOrigin[2];
}

/* 6AF65..6AFAC: signed timestamp comparison, exact source store order.
 * Equality preserves every byte. This is historical peer data, not Observe.
 * Caller separately proves current Actor cohort and winning row provenance. */
static int C1PER14_InheritRow(c1per14Row_t*destination,const c1per14Row_t*peer){
 int32_t a,b;memcpy(&a,destination->word+0x18/4,4);memcpy(&b,peer->word+0x18/4,4);
 if(a>=b)return 0;
 destination->word[0x14/4]=0;destination->word[0x1c/4]=0;
 destination->word[0x18/4]=peer->word[0x18/4];
 destination->word[0x20/4]=peer->word[0x20/4];
 destination->word[0x24/4]=peer->word[0x24/4];
 destination->word[0x28/4]=peer->word[0x28/4];
 destination->word[0x30/4]=peer->word[0x30/4];
 destination->word[0x2c/4]=peer->word[0x2c/4];
 destination->word[0x34/4]=peer->word[0x34/4];
 destination->word[0x38/4]=peer->word[0x38/4];
 destination->word[0x3c/4]=peer->word[0x3c/4];
 return 1; /* Row storage changed, not a native AI return. */
}
#endif
