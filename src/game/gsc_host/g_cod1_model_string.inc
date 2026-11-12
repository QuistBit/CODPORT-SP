/* R165 uses the same finite C-locale formatter as the actual VM getter. */
#include "../cod1_gsc/gsc_string_cast.inc"
int G_Cod1ActorConvertModelValue(const cod1ActorModelValue_t *input,
    cod1ActorModelString_t *out,char *error,size_t size) {
    return GscStringCast(input,out,error,size);
}
