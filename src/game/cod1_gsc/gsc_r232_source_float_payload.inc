/* Explicit PC53/nearest scalar instruction boundary used by the selected
 * original init/look/mg42/common modules. Each operator materializes one binary32, as
 * ENGINE4660DC/4661CF/46629D/46632D. No ambient-control-word inference. */
static int r232_source_float_module(const char *module) {
    return module&&(!strcmp(module,"animscripts/init")||!strcmp(module,"animscripts/look")||!strcmp(module,"animscripts/mg42/common"));
}
static int r232_source_float_bits(uint32_t bits) {
    return (bits&UINT32_C(0x7fffffff))<=UINT32_C(0x4b800000);
}
/* Exact finite binary32 truncation toward zero in the selected +/-2^24
 * domain. Integer operations do not observe/change the caller FP state. */
static int r232_source_float_int(uint32_t bits,int32_t *out) {
    unsigned exponent,magnitude;
    if(!out||!r232_source_float_bits(bits))return 0;
    exponent=(bits>>23)&255u;
    if(exponent<127){*out=0;return 1;}
    exponent-=127;magnitude=(bits&0x7fffffu)|0x800000u;
    magnitude=exponent>=23?magnitude<<(exponent-23):magnitude>>(23-exponent);
    *out=(bits>>31)?-(int32_t)magnitude:(int32_t)magnitude;return 1;
}
static int r232_source_float_binary(int op,uint32_t ab,uint32_t bb,uint32_t *out) {
    float a,b,value;uint32_t bits;
    if(!out||!r232_source_float_bits(ab)||!r232_source_float_bits(bb)||
       (op!='+'&&op!='-'&&op!='*'&&op!='/')||(op=='/'&&!(bb&UINT32_C(0x7fffffff))))return 0;
#if defined(_MSC_VER) && defined(_M_IX86)
    {unsigned char saved[108];const unsigned short cw=0x027f;
    memcpy(&a,&ab,4);memcpy(&b,&bb,4);
    __asm fnsave saved
    __asm fldcw cw
    if(op=='+') {__asm fld a
        __asm fadd b
        __asm fstp value}
    else if(op=='-') {__asm fld a
        __asm fsub b
        __asm fstp value}
    else if(op=='*') {__asm fld a
        __asm fmul b
        __asm fstp value}
    else {__asm fld a
        __asm fdiv b
        __asm fstp value}
    __asm frstor saved
    memcpy(&bits,&value,4);if(!r232_source_float_bits(bits))return 0;*out=bits;return 1;}
#else
    (void)a;(void)b;(void)value;(void)bits;return 0;
#endif
}
