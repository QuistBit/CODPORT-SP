/* R105 private exact scalar lane. Original type5 is signed int32, type4 is
 * float32. Cached `number` is only a float view; integer operations never read
 * it when an exact payload exists. Unsigned arithmetic implements native low32
 * wrapping without C signed overflow. Float/int promotion intentionally rounds
 * to float32. Earlier runtime modes never select these helpers. */
static int32_t scalar_signed(uint32_t u) {
    return u<=INT32_MAX?(int32_t)u:-(int32_t)(UINT32_MAX-u)-1;
}
static void scalar_integer(Value *out,int32_t number) {
    memset(out,0,sizeof(*out));out->kind=V_NUMBER;out->integer=1;
    out->integer_exact=1;out->integer_value=number;out->number=(float)number;
}
static int scalar_get_integer(GscRuntime *r,const Value *v,int line,int32_t *out) {
    double number;
    if(v->kind!=V_NUMBER||v->integer!=1)return rfail(r,line,"operator requires int32 operands");
    if(v->integer_exact){*out=v->integer_value;return 1;}
    /* Existing bounded host-produced booleans/counts have no exact payload. */
    number=(double)v->number;
    if(!isfinite(v->number)||number<INT32_MIN||number>INT32_MAX||trunc(number)!=number)
        return rfail(r,line,"invalid integer scalar payload");
    *out=(int32_t)number;return 1;
}
static int scalar_float(GscRuntime *r,const Value *v,int line,float *out) {
    int32_t i;
    if(v->kind!=V_NUMBER)return rfail(r,line,"numeric operator requires numbers");
    if(v->integer) {
        if(!scalar_get_integer(r,v,line,&i))return 0;
        *out=(float)i;return 1;
    }
    if(!isfinite(v->number))return rfail(r,line,"nonfinite scalar payload");
    *out=v->number;return 1;
}
static int scalar_cast(GscRuntime *r,int kind,const Value *v,int line,Value *out) {
    float number;int32_t integer;double truncated;
    if(v->kind!=V_NUMBER)return rfail(r,line,"numeric cast accepts only numbers in this stage");
    if(kind==GSC_CAST_INT) {
        if(v->integer) {
            if(!scalar_get_integer(r,v,line,&integer))return 0;
        } else {
            if(!scalar_float(r,v,line,&number))return 0;
            truncated=trunc((double)number);
            if(truncated<INT32_MIN||truncated>INT32_MAX)
                return rfail(r,line,"float-to-int cast outside supported signed32 range");
            integer=(int32_t)truncated;
        }
        scalar_integer(out,integer);return 1;
    }
    if(kind!=GSC_CAST_FLOAT)return rfail(r,line,"invalid scalar cast");
    if(!scalar_float(r,v,line,&number))return 0;
    memset(out,0,sizeof(*out));out->kind=V_NUMBER;out->number=number;return 1;
}
static int scalar_unary(GscRuntime *r,const Value *v,int line,Value *out) {
    int32_t i;float f;
    if(v->integer) {
        if(!scalar_get_integer(r,v,line,&i))return 0;
        scalar_integer(out,scalar_signed(0u-(uint32_t)i));return 1;
    }
    if(!scalar_float(r,v,line,&f))return 0;
    memset(out,0,sizeof(*out));out->kind=V_NUMBER;out->number=-f;return 1;
}
/* R171 original ordinary-string addition (46adb0), selected with the scalar
 * lane. This admitted domain excludes mixed/coerced and localized operands.
 * The original allocates/interns arbitrary-length text; the port owns inline
 * strings and refuses a result longer than 127 bytes, without truncation.
 * Stage the complete output before assigning, including when out aliases an
 * operand. No native string IDs/refcounts or caller-value mutation are used. */
static int scalar_string_add(GscRuntime *r,const Value *a,const Value *b,int line,Value *out) {
    const char *aend,*bend;size_t alen,blen;Value result;
    aend=(const char *)memchr(a->str,0,sizeof(a->str));
    bend=(const char *)memchr(b->str,0,sizeof(b->str));
    if(!aend||!bend)return rfail(r,line,"invalid string concatenation payload");
    alen=(size_t)(aend-a->str);blen=(size_t)(bend-b->str);
    if(alen+blen>=sizeof(result.str))
        return rfail(r,line,"string concatenation exceeds 127 bytes");
    memset(&result,0,sizeof(result));result.kind=V_STRING;
    memcpy(result.str,a->str,alen);memcpy(result.str+alen,b->str,blen);
    *out=result;return 1;
}
static int scalar_binary_selected(GscRuntime *r,int selected,int op,const Value *a,const Value *b,int line,Value *out) {
    int32_t ai,bi,result;float af,bf,f;int equal;
    if(op=='+'&&string_int_concat_pair(r,a,b))return string_int_concat(r,a,b,line,out);
    if(selected==2&&a->kind==V_NUMBER&&b->kind==V_NUMBER&&(!a->integer||!b->integer)&&(op=='+'||op=='-'||op=='*'||op=='/'))
        return r232_source_float_value(r,op,a,b,line,out);
    if(selected==1&&a->kind==V_NUMBER&&b->kind==V_NUMBER&&(!a->integer||!b->integer))
        return float_arithmetic_binary(r,op,a,b,line,out);
    if(animation_namespace_kind(a->kind)||animation_namespace_kind(b->kind))return animation_namespace_compare(r,op,a,b,line,out);
    if(a->kind==V_ANIMATION||b->kind==V_ANIMATION)return animation_compare(r,op,a,b,line,out);
    if(a->kind==V_HUDELEM||b->kind==V_HUDELEM)return hud_compare(r,op,a,b,line,out);
    if(a->kind==V_VEHICLE_NODE||b->kind==V_VEHICLE_NODE)return vehicle_node_compare(r,op,a,b,line,out);
    if(a->kind==V_NODE||b->kind==V_NODE)return node_compare(r,op,a,b,line,out);
    if(op==T_AND||op==T_OR) {
        scalar_integer(out,op==T_AND?(truth(*a)&&truth(*b)):(truth(*a)||truth(*b)));return 1;
    }
    if(op=='+'&&a->kind==V_STRING&&b->kind==V_STRING)
        return scalar_string_add(r,a,b,line,out);
    if(a->kind==V_STRING&&b->kind==V_STRING&&(op==T_EQ||op==T_NE)) {
        equal=!strcmp(a->str,b->str);scalar_integer(out,op==T_EQ?equal:!equal);return 1;
    }
    if(a->kind!=V_NUMBER||b->kind!=V_NUMBER)return rfail(r,line,"numeric operator requires numbers");
    if(a->integer&&b->integer) {
        if(!scalar_get_integer(r,a,line,&ai)||!scalar_get_integer(r,b,line,&bi))return 0;
        switch(op) {
          case '+':result=scalar_signed((uint32_t)ai+(uint32_t)bi);break;
          case '-':result=scalar_signed((uint32_t)ai-(uint32_t)bi);break;
          case '*':result=scalar_signed((uint32_t)ai*(uint32_t)bi);break;
          case '&':result=scalar_signed((uint32_t)ai&(uint32_t)bi);break;
          case '/':
            if(!bi)return rfail(r,line,"division by zero");
            if(ai==INT32_MIN&&bi==-1)return rfail(r,line,"native division overflow is unsupported");
            result=ai/bi;break;
          case '%':
            if(!bi)return rfail(r,line,"modulo by zero");
            if(ai==INT32_MIN&&bi==-1)return rfail(r,line,"native modulo overflow is unsupported");
            result=ai%bi;break;
          case T_EQ:result=ai==bi;break;case T_NE:result=ai!=bi;break;
          case '<':result=ai<bi;break;case '>':result=ai>bi;break;
          case T_LE:result=ai<=bi;break;case T_GE:result=ai>=bi;break;
          default:goto floating;
        }
        scalar_integer(out,result);return 1;
    }
    if(op=='%'||op=='&')return rfail(r,line,"operator requires int32 operands");
floating:
    if(!scalar_float(r,a,line,&af)||!scalar_float(r,b,line,&bf))return 0;
    switch(op) {
      case T_EQ:scalar_integer(out,fabsf(af-bf)<0.000001f);return 1;
      case T_NE:scalar_integer(out,fabsf(af-bf)>=0.000001f);return 1;
      case '<':scalar_integer(out,af<bf);return 1;case '>':scalar_integer(out,af>bf);return 1;
      case T_LE:scalar_integer(out,af<=bf);return 1;case T_GE:scalar_integer(out,af>=bf);return 1;
      case '+':f=af+bf;break;case '-':f=af-bf;break;case '*':f=af*bf;break;
      case '/':if(bf==0)return rfail(r,line,"division by zero");f=af/bf;break;
      default:return rfail(r,line,"unsupported scalar operator");
    }
    if(!isfinite(f))return rfail(r,line,"nonfinite scalar expression");
    memset(out,0,sizeof(*out));out->kind=V_NUMBER;out->number=f;return 1;
}
static int scalar_binary(GscRuntime *r,int op,const Value *a,const Value *b,int line,Value *out) {
    return scalar_binary_selected(r,0,op,a,b,line,out);
}
static int scalar_mutation(GscRuntime *r,int op,const Value *a,const Value *b,int line,Value *out) {
    return scalar_binary_selected(r,float_arithmetic_active(r),op==GSC_MUT_ADD||op==GSC_MUT_INC?'+':
        op==GSC_MUT_SUB||op==GSC_MUT_DEC?'-':op==GSC_MUT_MUL?'*':
        op==GSC_MUT_DIV?'/':0,a,b,line,out);
}
/* Exact integers cannot be silently exported as integer-tagged float payloads
 * into the old generic native-number ABI. Explicit float casts are available. */
static int scalar_native_exact(GscRuntime *r,const Value *v,int line) {
    int32_t i;
    if(!r->scalar_switch_enabled||v->kind!=V_NUMBER||!v->integer)return 1;
    if(!scalar_get_integer(r,v,line,&i))return 0;
    return (double)(float)i==(double)i?1:rfail(r,line,"integer is not exact in float-only native bridge; cast explicitly");
}
