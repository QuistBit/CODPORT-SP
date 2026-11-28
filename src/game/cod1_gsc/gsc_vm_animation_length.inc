/* Native GetAnimLength returns processed frame-count / binary32 FPS. For the
 * admitted integer metadata, PC24 rounds to binary32 directly. PC53/64 cannot
 * double-round: a non-midpoint rational is at least 1/(2*120) of a binary32
 * significand unit from its midpoint, far beyond their rounding error. */
static int animation_length_ratio(uint32_t frames,uint32_t fps,unsigned rounding,uint32_t *out,int *exact) {
    uint64_t numerator=frames,denominator=fps,remainder,significand,scaled;int exponent=0;
    if(!out||!exact||frames>4095||!fps||fps>120||rounding>GSC_FLOAT_ROUND_ZERO)return 0;
    if(!frames){*out=0;*exact=1;return 1;}
    while(numerator<denominator){numerator*=2;--exponent;}
    while(numerator>=denominator*2){denominator*=2;++exponent;}
    /* After normalization denominator <= 4095 and numerator < 2*denominator.
     * The 23 fractional bits are exactly the quotient of the scaled remainder;
     * its residue is the same one used by the original rounding decisions. */
    scaled=(numerator-denominator)<<23;
    significand=UINT64_C(0x800000)+scaled/denominator;
    remainder=scaled%denominator;
    *exact=remainder==0;
    if((rounding==GSC_FLOAT_ROUND_NEAREST&&(remainder*2>denominator||(remainder*2==denominator&&(significand&1))))||
       (rounding==GSC_FLOAT_ROUND_UP&&remainder))++significand;
    if(significand==UINT64_C(0x1000000)){significand>>=1;++exponent;}
    *out=((uint32_t)(exponent+127)<<23)|((uint32_t)significand&UINT32_C(0x7fffff));return 1;
}
int gsc_program_select_animation_length_query(GscProgram *g) {
    if(!g||g->invalid||g->linked||!g->animation_refs_selected||g->animation_length_selected)return 0;
    g->animation_length_selected=1;return 1;
}
int gsc_runtime_select_animation_length(GscRuntime *r,const GscAnimationLengthHost *input) {
    GscAnimationLengthHost host;
    if(!r||!input||r->started||r->advancing||r->executing_task||r->task_count||r->error[0]||
       !r->program->animation_length_selected||!r->animation_refs_enabled||r->animation_length_enabled||
       r->entity_prepare_busy||r->entity_certificate_busy||r->entity_sequence_busy||r->entity_inheritance_busy||r->entity_field_busy||r->entity_method_busy||
       runtime_private_overlap(r,input,sizeof(*input)))return 0;
    host=*input;if(!host.query||!anim_data_current(r,NULL))return 0;
    r->animation_length=host;r->animation_length_enabled=1;return 1;
}
/* Current/context are borrowed only until query returns. Nested entity entry
 * is blocked by animation_length_busy; public schedulers already hold advancing. */
static int animation_length_context_current(void *user) {
    GscRuntime *r=(GscRuntime *)user;
    return !r->error[0]&&console_context(r)==1;
}
#include "gsc_vm_r233_namespace_length.inc"
static int animation_length_call(GscRuntime *r,const Value *args,int argc,int selected,int line,Value *out) {
    GscAnimationLengthRequest request,original;GscAnimationLengthReply reply;Value value;
    if(r->program->namespace_length_selected)return namespace_length_call(r,args,argc,selected,line,out);
    char error[384];uint32_t bits;unsigned rounding=GSC_FLOAT_ROUND_NEAREST;int n,found=0,accepted,exact;
    if(!r->animation_length_enabled||!out||argc<0||argc>MAX_PARAMS||(argc&&!args))return rfail(r,line,"animation length capability unavailable");
    if(argc<1||!animation_value_valid(r,&args[0]))return rfail(r,line,"getanimlength requires a current animation reference");
    if(r->animation_length_busy||r->entity_field_busy||r->entity_method_busy||r->entity_inheritance_busy||r->entity_prepare_busy||r->entity_certificate_busy)return rfail(r,line,"animation length callback reentered");
    memset(&request,0,sizeof(request));request.runtime_identity=r->array_token;
    for(n=0;n<r->animation_ref_count;++n)if(r->animation_bindings[n].entry==args[0].object){
        const AnimationBinding *binding=&r->animation_bindings[n];strcpy(request.tree,binding->tree);strcpy(request.name,binding->name);request.ordinal=binding->entry;request.flags=binding->flags;found=1;break;
    }
    if(!found)return rfail(r,line,"animation length reference binding unavailable");
    if(request.flags&GSC_ANIMATION_NODE_BRANCH)return rfail(r,line,"getanimlength requires a leaf animation");
    if(selected){if(!r->float_arithmetic_enabled||!float_arithmetic_profile_valid(&r->program->float_arithmetic_profile))return rfail(r,line,"animation length arithmetic profile unavailable");rounding=r->program->float_arithmetic_profile.rounding;}
    original=request;memset(&reply,0xff,sizeof(reply));error[0]=0;
    if(console_context(r)<=0)return 0;
    r->animation_length_busy=1;
    accepted=r->animation_length.query(r->animation_length.user,&request,&reply,error,sizeof(error),animation_length_context_current,r);
    r->animation_length_busy=0;error[sizeof(error)-1]=0;
    if(r->error[0]||console_context(r)<=0)return 0;
    if(memcmp(&request,&original,sizeof(request)))return rfail(r,line,"animation length host changed copied resource intent");
    if(accepted!=1)return rfail(r,line,"animation length host refused: %s",error[0]?error:"unspecified host failure");
    if(!animation_length_ratio(reply.frames,reply.fps,rounding,&bits,&exact))return rfail(r,line,"animation length metadata outside validated clip domain");
    if(!exact&&!selected)return rfail(r,line,"inexact animation length requires explicit module arithmetic profile");
    memset(&value,0,sizeof(value));value.kind=V_NUMBER;memcpy(&value.number,&bits,sizeof(bits));*out=value;return 1;
}
