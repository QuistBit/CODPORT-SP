/* Separately selected pure vector operations. Their private statement target
 * is outside all earlier native ranges; frozen registry/import IDs stay put. */
enum { VECTOR_MATH_DISTANCE=1,VECTOR_MATH_DISTANCE_SQUARED,VECTOR_MATH_LENGTH,
       VECTOR_MATH_LENGTH_SQUARED,VECTOR_MATH_NORMALIZE,VECTOR_MATH_DOT,
       VECTOR_MATH_FORWARD,VECTOR_MATH_RIGHT,VECTOR_MATH_UP,
       VECTOR_MATH_SIN,VECTOR_MATH_COS,VECTOR_MATH_ASIN,VECTOR_MATH_TO_ANGLES };
#define VECTOR_MATH_TARGET_BASE 10000
static int vector_math_descriptor(const GscProgram *g,const char *name) {
    static const char *names[]={"distance","distancesquared","length","lengthsquared",
        "vectornormalize","vectordot","anglestoforward","anglestoright","anglestoup",
        "sin","cos","asin","vectortoangles"};
    int k;if(!g->vector_math_selected)return 0;
    for(k=0;k<(int)(sizeof(names)/sizeof(names[0]));k++)if(callable_name_equal(g,name,names[k]))return k+1;
    return 0;
}
static int vector_math_arity(int kind) {
    return kind==VECTOR_MATH_DISTANCE||kind==VECTOR_MATH_DISTANCE_SQUARED||kind==VECTOR_MATH_DOT?2:
        kind>=VECTOR_MATH_LENGTH&&kind<=VECTOR_MATH_TO_ANGLES?1:0;
}
static unsigned vector_math_static_issues(const GscProgram *g,int kind,const int *args,int argc) {
    unsigned issues=0;int k,type;
    if(!vector_math_arity(kind)||argc!=vector_math_arity(kind))return GSC_LINK_ISSUE_BUILTIN_ARITY;
    for(k=0;k<argc;k++) {
        type=known_value_kind(g,args[k]);
        if(type>=0&&type!=((kind>=VECTOR_MATH_SIN&&kind<=VECTOR_MATH_ASIN)?V_NUMBER:V_VECTOR))
            issues|=GSC_LINK_ISSUE_BUILTIN_LITERAL;
    }
    return issues;
}
