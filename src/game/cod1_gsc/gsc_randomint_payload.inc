/* R133 typed draw transport. VM owns GetInt/result projection; GAME owns the
 * one shared RNG transition. This is a bounded port capability, not retail ABI. */
static int randomint_request_valid(const GscRandomIntRequest *request) {
    return request && request->operation_id && request->bound>0 &&
        request->draw15==UINT32_MAX;
}
static int randomint_reply_valid(const GscRandomIntRequest *before,
    const GscRandomIntRequest *after) {
    return randomint_request_valid(before) && after &&
        after->operation_id==before->operation_id && after->bound==before->bound &&
        after->draw15<=UINT32_C(32767);
}
