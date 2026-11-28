#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>
#include <fenv.h>
enum { V_UNDEFINED, V_NUMBER, V_STRING, V_VECTOR, V_ENTITY, V_LEVEL, V_ARRAY, V_FUNCTION, V_LOCALIZED_STRING, V_STRUCT, V_ANIM, V_ANIMATION, V_NODE, V_HUDELEM, V_ANIMATION_TREE, V_NAMESPACED_ANIMATION, V_VEHICLE_NODE };
enum { E_BINARY=1,E_UNARY,T_EQ=256,T_NE,T_AND,T_OR,T_LE,T_GE,GSC_CAST_INT,GSC_CAST_FLOAT };
typedef struct {uint64_t epoch;uint32_t slot,generation;} Ref;
typedef struct {int kind,integer;float number;int integer_exact;int32_t integer_value;char str[128];float vector[3];Ref entity,node,vehicle_node,hud;uint32_t array;unsigned char extra[40];} Value;
typedef struct {int kind,op,float_arithmetic;} Expr;
typedef struct {int scalar_switch_enabled,integer_bitwise_enabled,event_payloads_enabled,vector_arithmetic_enabled,string_int_concat_enabled,r239_mixed_scalar_equality_enabled;unsigned calls,errors;char error[128];} GscRuntime;
static int rfail(GscRuntime*r,int line,const char*fmt,...){va_list v;(void)line;++r->errors;va_start(v,fmt);vsnprintf(r->error,sizeof(r->error),fmt,v);va_end(v);return 0;}
static int truth(Value v){return v.kind==V_STRING?v.str[0]!=0:v.number!=0.0f;}
static int animation_namespace_kind(int kind){return kind==V_ANIMATION_TREE||kind==V_NAMESPACED_ANIMATION;}
static int entity_ref_equal(const Ref*a,const Ref*b){return a->epoch==b->epoch&&a->slot==b->slot&&a->generation==b->generation;}
/* Foreign or noninteger adapters are modeled, not reimplemented. They must
 * never be entered by the selected integer fast path. */
#define STUB(name,id) static int name(GscRuntime*r,int op,const Value*a,const Value*b,int line,Value*out){(void)op;(void)a;(void)b;(void)line;r->calls=r->calls*31u+id;memset(out,0,sizeof(*out));out->kind=V_NUMBER;out->number=(float)id;return 1;}
STUB(animation_namespace_compare,1) STUB(animation_compare,2) STUB(hud_compare,3) STUB(vehicle_node_compare,4) STUB(node_compare,5) STUB(r232_source_float_value,6) STUB(float_arithmetic_binary,7) STUB(vector_arithmetic_binary,8) STUB(r239_mixed_scalar_compare,9)
static int string_int_concat_pair(const GscRuntime*r,const Value*a,const Value*b){return r->string_int_concat_enabled&&((a->kind==V_STRING&&b->kind==V_NUMBER)||(a->kind==V_NUMBER&&b->kind==V_STRING));}
static int string_vector_concat_pair(const GscRuntime*r,const Value*a,const Value*b){return r->string_int_concat_enabled&&r->vector_arithmetic_enabled&&((a->kind==V_STRING&&b->kind==V_VECTOR)||(a->kind==V_VECTOR&&b->kind==V_STRING));}
static int r239_mixed_scalar_pair(const GscRuntime*r,int op,const Value*a,const Value*b){return r->r239_mixed_scalar_equality_enabled&&(op==T_EQ||op==T_NE)&&((a->kind==V_NUMBER&&b->kind==V_STRING)||(a->kind==V_STRING&&b->kind==V_NUMBER));}
static int string_int_concat(GscRuntime*r,const Value*a,const Value*b,int line,Value*out){return vector_arithmetic_binary(r,0,a,b,line,out);}
static int string_vector_concat(GscRuntime*r,const Value*a,const Value*b,int line,Value*out){return vector_arithmetic_binary(r,0,a,b,line,out);}
static int vector_arithmetic_unary(GscRuntime*r,int op,const Value*a,int line,Value*out){return vector_arithmetic_binary(r,op,a,a,line,out);}
#include "scalar.inc"
#include "bitwise.inc"
#include "operator.inc"
#include "gsc_vm_results_binary56.inc"
static uint64_t rng=UINT64_C(0xb19397a1f2c5bd6e);
static unsigned rnd(void){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)rng;}
static Value input(void){Value v;memset(&v,0,sizeof(v));v.kind=V_NUMBER;v.integer=1;v.integer_exact=1;v.integer_value=scalar_signed(rnd());v.number=(float)v.integer_value;switch(rnd()%10){case 0:v.integer_value=INT32_MIN;v.number=(float)v.integer_value;break;case 1:v.integer_value=-1;v.number=-1;break;case 2:v.integer_value=0;v.number=0;break;case 3:v.integer_exact=0;v.number=(float)(rnd()%4096);break;case 4:v.integer_exact=0;v.number=1.5f;break;case 5:v.integer_exact=0;v.number=INFINITY;break;case 6:v.integer_exact=0;v.number=2147483648.0f;break;case 7:v.integer_exact=0;v.number=NAN;break;default:break;}return v;}
int main(void){unsigned i,fast=0,ok=0,failed=0,slow=0;const int ops[]={'+','-','*','/','%','&','|','^',T_EQ,T_NE,'<','>',T_LE,T_GE,T_AND,T_OR,'?','!','~'};
 for(i=0;i<200000;i++){
  GscRuntime a={0},b;Expr e={E_BINARY,ops[rnd()%19],(int)(rnd()%3)};Value x=input(),y=input(),u,v,oldx=x,oldy=y;int ra,rb;
  a.scalar_switch_enabled=i%9!=0;a.integer_bitwise_enabled=rnd()%2;a.event_payloads_enabled=rnd()%2;a.vector_arithmetic_enabled=rnd()%2;a.string_int_concat_enabled=rnd()%2;a.r239_mixed_scalar_equality_enabled=rnd()%2;
  if(i%11==0)x.kind=(int)(rnd()%17);if(i%13==0)y.kind=(int)(rnd()%17);if(i%17==0)x.integer=0;if(i%19==0)y.integer=2;if(i%23==0)e.kind=E_UNARY;
  oldx=x;oldy=y;b=a;memset(&u,0xa5,sizeof(u));memset(&v,0xa5,sizeof(v));
  if(i%1000==0)assert(!fesetround((i/1000)%2?FE_UPWARD:FE_DOWNWARD));else if(i%1000==1)assert(!fesetround(FE_TONEAREST));
  ra=results_operator(&a,&e,x,y,23,&u);rb=results_binary56(&b,&e,&x,&y,23,&v);
  assert(ra==rb&&a.calls==b.calls&&a.errors==b.errors&&!strcmp(a.error,b.error));assert(!memcmp(&u,&v,sizeof(u)));assert(!memcmp(&x,&oldx,sizeof(x))&&!memcmp(&y,&oldy,sizeof(y)));
  if(e.kind==E_BINARY&&a.scalar_switch_enabled&&x.kind==V_NUMBER&&y.kind==V_NUMBER&&x.integer==1&&y.integer==1){++fast;assert(!b.calls);}else ++slow;
  if(ra)++ok;else ++failed;
 }
 assert(!fesetround(FE_TONEAREST));assert(fast&&slow&&ok&&failed);printf("R56B1 PASS: 200000 cases fast=%u fallback=%u accepted=%u refused=%u\n",fast,slow,ok,failed);return 0;}
