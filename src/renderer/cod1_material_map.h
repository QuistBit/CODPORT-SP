/* R51: shared authored map operands for .shader, .stype and water IR.
 * CoDSP af73842d...: qualifier loop 0x4e8769..0x4e888c;
 * $lightmap selection 0x4e88b0..0x4e891a. No material-name rules.
 * This file classifies operands only: callers retain bounded tokenization,
 * backend capability checks and image ownership. It is NOT a full map VM.
 */
#ifndef COD1_MATERIAL_MAP_H
#define COD1_MATERIAL_MAP_H
#include <string.h>
#include <ctype.h>
enum { C1MAP_REPEAT=0, C1MAP_CLAMP_S=1, C1MAP_CLAMP_T=2, C1MAP_CLAMP_ST=3 };
/* Native noopt is image-load policy (0x100), NOT nopicmip, nomipmaps,
 * RMSE reduction, compression or alpha conversion. 0x4e8876 sets this bit;
 * 0x4e4078 prevents deferred/optimized image allocation when it is set. */
enum { C1MAP_NOOPT=0x100u };
enum { C1MAP_FILE=0, C1MAP_WHITE, C1MAP_LIGHTMAP, C1MAP_DLIGHT,
       C1MAP_TEMPLATE, C1MAP_UNKNOWN_SYMBOL };
static int C1Map_Equal(const char *a,const char *b) {
    if(!a||!b)return 0;
    while(*a&&*b) {if(tolower((unsigned char)*a++)!=tolower((unsigned char)*b++))return 0;}
    return *a==*b;
}
/* Native flags are ORed, not overwritten; clampx+clampy means both axes.
 * clamps/clampt are original aliases, not invented leniency. */
static int C1Map_Qualifier(const char *word,int *axes) {
    int add;
    if(C1Map_Equal(word,"clamp"))add=C1MAP_CLAMP_ST;
    else if(C1Map_Equal(word,"clampx")||C1Map_Equal(word,"clamps"))add=C1MAP_CLAMP_S;
    else if(C1Map_Equal(word,"clampy")||C1Map_Equal(word,"clampt"))add=C1MAP_CLAMP_T;
    else return 0;
    *axes|=add;return 1;
}
/* Keep the original axis-only helper for callers which validate a narrower
 * grammar; new source-driven map callers preserve policy separately. */
static int C1Map_Option(const char *word,int *axes,unsigned *loadFlags) {
    if(C1Map_Qualifier(word,axes))return 1;
    if(C1Map_Equal(word,"noopt")){*loadFlags|=C1MAP_NOOPT;return 1;}
    return 0;
}
static int C1Map_Source(const char *word) {
    if(C1Map_Equal(word,"$whiteimage")||C1Map_Equal(word,"*white"))return C1MAP_WHITE;
    if(C1Map_Equal(word,"$lightmap"))return C1MAP_LIGHTMAP;
    if(C1Map_Equal(word,"$dlight"))return C1MAP_DLIGHT;
    if(C1Map_Equal(word,"$texturename"))return C1MAP_TEMPLATE;
    return word&&*word=='$'?C1MAP_UNKNOWN_SYMBOL:C1MAP_FILE;
}
static int C1Map_Wrap(int axes,int repeat,int edge,int clampS,int clampT) {
    switch(axes) {
    case C1MAP_REPEAT:return repeat;
    case C1MAP_CLAMP_S:return clampS;
    case C1MAP_CLAMP_T:return clampT;
    default:return edge;
    }
}
#endif
