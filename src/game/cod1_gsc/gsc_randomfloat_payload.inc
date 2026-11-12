/* R134 bounded exact randomfloat draw transport. Binary32 payloads are copied
 * as bits; projection uses integer arithmetic only. This is a selected port
 * capability, not the retail GetFloat/heap/FPU ABI. The shared GAME owner is
 * never seeded or rolled back by these helpers. */
static int randomfloat_decode_bound(uint32_t bits,uint32_t *odd,int *exponent) {
    uint32_t magnitude=bits&UINT32_C(0x7fffffff),m,biased;
    int e;
    if(!magnitude) {if(odd)*odd=0;if(exponent)*exponent=0;return 1;}
    biased=magnitude>>23;
    /* Smallest nonzero result (draw1) must remain a normal binary32. */
    if(biased<16 || biased>=255)return 0;
    m=UINT32_C(0x800000)|(magnitude&UINT32_C(0x7fffff));e=(int)biased-150;
    while(!(m&1u)){m>>=1;e++;}
    /* draw<=32767 times odd<=511 is at most 24 significant bits: exact at
     * every original x87 precision/rounding setting, including its final store. */
    if(m>511)return 0;
    if(odd)*odd=m;if(exponent)*exponent=e;return 1;
}
static int randomfloat_integer_bits(int32_t value,uint32_t *bits) {
    uint32_t magnitude=value<0?0u-(uint32_t)value:(uint32_t)value;
    uint32_t scan=magnitude,sign=value<0?UINT32_C(0x80000000):0,sig;
    int top=0;
    if(!bits)return 0;
    if(!magnitude){*bits=0;return 1;}
    while(scan>>1){scan>>=1;top++;}
    if(top<=23)sig=magnitude<<(23-top);
    else {
        if(magnitude&((UINT32_C(1)<<(top-23))-1u))return 0;
        sig=magnitude>>(top-23);
    }
    *bits=sign|((uint32_t)(top+127)<<23)|(sig&UINT32_C(0x7fffff));
    return randomfloat_decode_bound(*bits,NULL,NULL);
}
static int randomfloat_request_valid(const GscRandomFloatRequest *request) {
    return request && request->operation_id && request->draw15==UINT32_MAX &&
        randomfloat_decode_bound(request->bound_bits,NULL,NULL);
}
static int randomfloat_reply_valid(const GscRandomFloatRequest *before,
    const GscRandomFloatRequest *after) {
    return randomfloat_request_valid(before) && after &&
        after->operation_id==before->operation_id &&
        after->bound_bits==before->bound_bits && after->draw15<=UINT32_C(32767);
}
static uint32_t randomfloat_project_bits(uint32_t bound,uint32_t draw) {
    uint32_t odd,product,scan,sig,sign=bound&UINT32_C(0x80000000);
    int exponent,top=0,result_exponent;
    if(!randomfloat_decode_bound(bound,&odd,&exponent) || draw>32767)return 0;
    product=draw*odd;
    if(!product)return sign;
    scan=product;while(scan>>1){scan>>=1;top++;}
    sig=product<<(23-top);result_exponent=top+exponent-15;
    return sign|((uint32_t)(result_exponent+127)<<23)|(sig&UINT32_C(0x7fffff));
}
