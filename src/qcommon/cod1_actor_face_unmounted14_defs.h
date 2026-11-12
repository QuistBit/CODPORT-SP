#ifndef COD1_ACTOR_FACE_UNMOUNTED14_DEFS_H
#define COD1_ACTOR_FACE_UNMOUNTED14_DEFS_H
/* Exact original generic_human ancestor closure. Source ordinals, not native
 * runtime node IDs. Ordinary sibling evaluation remains descending ordinal. */
#include <stddef.h>
#include "cod1_actor_unmounted_tree14_defs.h"
#define C1AFU14_NODES 56
#define C1AFU14_LEAVES 47
#define C1AFU14_PHASES 7
#define C1AFU14_FACES 5
typedef struct {unsigned ordinal;int parent,leaf,phase;} c1afu14Definition_t;
static const c1afu14Definition_t c1afu14Definitions[C1AFU14_NODES]={
{0,-1,-1,-1},
{4,0,-1,-1},
{5,1,-1,-1},
{75,2,-1,-1},
{113,3,-1,-1},
{114,4,0,0},
{115,4,1,1},
{1609,1,-1,-1},
{1610,7,2,-1},
{1611,7,3,-1},
{1612,7,4,-1},
{1613,7,5,-1},
{2040,0,-1,-1},
{2041,12,6,-1},
{2042,12,7,-1},
{2043,12,8,-1},
{2044,12,9,-1},
{2045,12,10,-1},
{2046,12,11,-1},
{2047,12,12,-1},
{2048,12,13,-1},
{2049,12,14,-1},
{2050,12,15,-1},
{2051,12,16,-1},
{2052,12,17,-1},
{2053,12,18,-1},
{2054,12,19,-1},
{2055,12,20,-1},
{2056,12,21,-1},
{2057,12,22,-1},
{2058,12,23,-1},
{2059,0,-1,-1},
{2060,31,24,-1},
{2061,31,25,-1},
{2062,31,26,-1},
{2063,31,27,-1},
{2064,31,28,-1},
{2065,31,29,-1},
{2066,31,30,-1},
{2067,31,31,-1},
{2068,31,32,-1},
{2069,31,33,-1},
{2070,31,34,-1},
{2071,31,35,-1},
{2072,31,36,-1},
{2073,31,37,-1},
{2074,31,38,-1},
{2075,31,39,-1},
{2076,31,40,-1},
{2077,31,41,-1},
{1614,7,-1,-1},
{1691,50,42,2},
{1692,50,43,3},
{1615,50,44,4},
{1616,50,45,5},
{1617,50,46,6}
};
static const char*const c1afu14FaceNames[C1AFU14_FACES]={"Training_facial_Elder_001_goodtoseeyou","Training_facial_Elder_002_goodluck","facial_alert01","facial_alert02","facial_neutral"};
static const char*C1AFU14_Name(unsigned i){return i<C1AFU14_LEAVES?(i<42?c1aut14Names[i]:c1afu14FaceNames[i-42]):NULL;}
#endif
