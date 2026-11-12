/* Original 20034BD5..BEB operation order, explicitly PC53/nearest. The
 * shared owner supplies one real draw15; this helper never seeds an owner.
 * Inputs are finite binary32 in [-2^24,2^24]; subnormal results are retained. */
static int r232_range_bounds(uint32_t low,uint32_t high) {
    uint32_t a=low&UINT32_C(0x7fffffff),b=high&UINT32_C(0x7fffffff),ak,bk;
    if(a>UINT32_C(0x4b800000)||b>UINT32_C(0x4b800000))return 0;
    if(!a&&!b)return 0;
    ak=low>>31?~low:low^UINT32_C(0x80000000);bk=high>>31?~high:high^UINT32_C(0x80000000);
    return ak<bk;
}
static int r232_range_project(uint32_t low,uint32_t high,uint32_t draw,uint32_t *out) {
    uint32_t result;float a,b,value;const float divisor=32768;int signedDraw=(int)draw;
    if(!out||draw>32767||!r232_range_bounds(low,high))return 0;
#if defined(_MSC_VER) && defined(_M_IX86)
    {unsigned char saved[108];const unsigned short cw=0x027f;
    memcpy(&a,&low,4);memcpy(&b,&high,4);
    __asm fnsave saved
    __asm fldcw cw
    __asm {
        fld b
        fsub a
        fild signedDraw
        fmulp st(1),st
        fdiv divisor
        fadd a
        fstp value
    }
    __asm frstor saved
    memcpy(&result,&value,4);if((result&UINT32_C(0x7f800000))==UINT32_C(0x7f800000))return 0;
    *out=result;return 1;}
#else
    (void)a;(void)b;(void)value;(void)divisor;(void)signedDraw;(void)result;return 0;
#endif
}
