#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { E_NUMBER,E_VAR,E_FIELD,E_INDEX,E_METHOD_CALL,E_THREAD_CALL,E_LOCALIZED_STRING,E_RAW_LOCALIZED_STRING,E_RAW_STRING,E_VECTOR,E_ANIM_REF,E_DYNAMIC_CALL,E_EMPTY_ARRAY,E_SCOPED_CALL,E_CAST,E_FUNC_REF,E_ANIMTREE_VALUE,E_BINARY,E_NATIVE_CALL,E_ISDEFINED,E_COUNT };
enum { I_EMPTY,I_GAME_SET,I_METHOD,I_METHOD_THREAD,I_FOR,I_WHILE,I_RETURN,I_BREAK,I_CONTINUE,I_DO_WHILE,I_DYNAMIC_CALL,I_DYNAMIC_THREAD,I_MUTATE,I_SWITCH,I_COUNT };
enum { GSC_THREAD_EXPR_COMPUTED=1,GSC_EXPR_TARGET_LOCAL_SCRIPT=1,FOR_INCREMENT=1,GSC_CASE_RAW_STRING=1,GSC_DEV_FILE=1,GSC_DEV_STATEMENT=2 };
typedef struct {int kind,b,op,call_binding;char name[16];} Expr;
typedef struct {int kind,dest;} Instr;
typedef struct {int start,end,expr_start,expr_end,module,admission,argc;} Func;
typedef struct {int scope,func_start,func_end,module,code_start,code_end,expr_start,expr_end;} DevBlock;
typedef struct {int step_kind;} Loop;
typedef struct {int first_case;} Switch;
typedef struct {int kind,next_case;} Case;
typedef struct {int func_count,code_count,expr_count,loop_count,switch_count,case_count,dev_count,developer_selected;
 Func *funcs;Expr *expr;Instr *code;Loop *loops;Switch *switches;Case *cases;DevBlock *dev_blocks;
 int requires_game_values,requires_receiver_method,requires_literal_semantics,requires_loop_semantics,requires_localized_string,requires_flow_semantics,requires_vector_semantics,requires_anim_ref_semantics,requires_dynamic_call,requires_empty_array_semantics,requires_scoped_expr_call,requires_mutation_semantics,requires_cast_semantics,requires_func_ref_semantics,requires_animtree_value_semantics,requires_integer_op_semantics,requires_switch_semantics,requires_raw_string_semantics,requires_thread_expr_semantics,requires_do_while_semantics,requires_local_expr_call,requires_dev_block;
} GscProgram;
static int literal_name(const char *s){return !strcmp(s,"undefined")||!strcmp(s,"true");}
#define function_admission_requirements original_requirements
#include "old.inc"
#undef function_admission_requirements
#include "new.inc"
static Func funcs[32];static Expr expr[320];static Instr code[320];static Loop loops[3];static Switch switches[3];static Case cases[3];static DevBlock dev[8];
static unsigned seed=0x51b65432u,checks,dense_checks,fallback_checks;
static unsigned rnd(void){seed=1664525u*seed+1013904223u;return seed>>8;}
static void check(GscProgram *v,int only,int masked){GscProgram a=*v,b=*v;original_requirements(&a,only);if(masked)function_admission_requirements_range51(&b,only,1);else function_admission_requirements(&b,only);if(memcmp(&a,&b,sizeof(a))){fprintf(stderr,"mismatch %u only=%d masked=%d\n",checks,only,masked);abort();}++checks;if(masked&&only>=0&&only<v->func_count&&admission_bodies_dense51(v))++dense_checks;else ++fallback_checks;}
static void hide(GscProgram *v,int only){for(int f=0;f<v->func_count;f++)if(f!=only){Func *fn=v->funcs+f;for(int n=fn->start;n<fn->end;n++){memset(v->code+n,0,sizeof(*v->code));v->code[n].kind=I_EMPTY;}for(int n=fn->expr_start;n<fn->expr_end;n++){memset(v->expr+n,0,sizeof(*v->expr));v->expr[n].kind=E_NUMBER;}}}
static void init(GscProgram *v,int layout){memset(v,0,sizeof(*v));memset(funcs,0,sizeof(funcs));v->funcs=funcs;v->code=code;v->expr=expr;v->loops=loops;v->switches=switches;v->cases=cases;v->dev_blocks=dev;v->func_count=1+(int)(rnd()%24);v->loop_count=v->switch_count=v->case_count=3;v->dev_count=8;v->developer_selected=rnd()%2;
 for(int f=0;f<v->func_count;f++){Func *fn=funcs+f;fn->start=v->code_count;fn->expr_start=v->expr_count;if(layout==1&&f%3==0){fn->start++;fn->expr_start++;}if(layout==2&&f&&fn->start&&fn->expr_start){fn->start--;fn->expr_start--;}fn->end=fn->start+(int)(rnd()%9);fn->expr_end=fn->expr_start+(int)(rnd()%9);v->code_count=fn->end;v->expr_count=fn->expr_end;fn->module=rnd()%3;fn->admission=rnd()%2;}
 if(layout==3){v->code_count++;v->expr_count++;}
 const char *names[]={"level","self","undefined","true","other"};
 for(int i=0;i<v->code_count;i++)code[i]=(Instr){(int)(rnd()%I_COUNT),(int)(rnd()%5)-1};
 for(int i=0;i<v->expr_count;i++){expr[i]=(Expr){(int)(rnd()%E_COUNT),(int)(rnd()%3)-1,(int)(rnd()%2), (int)(rnd()%2),{0}};if(rnd()%2)expr[i].op='%';strcpy(expr[i].name,names[rnd()%5]);}
 for(int i=0;i<3;i++){loops[i].step_kind=rnd()%3;switches[i].first_case=rnd()%3;cases[i]=(Case){(int)(rnd()%2),i==2?-1:i+1};}
 for(int i=0;i<8;i++){int j=rnd()%v->func_count;Func *fn=funcs+j;dev[i]=(DevBlock){i%2?GSC_DEV_FILE:GSC_DEV_STATEMENT,j,j+1,fn->module,fn->start,fn->end,fn->expr_start,fn->expr_end};}
}
int main(void){GscProgram v;for(int k=0;k<4000;k++){int layout=k%4;init(&v,layout);check(&v,-1,0);check(&v,(int)(rnd()%v.func_count),0);int only=rnd()%v.func_count;hide(&v,only);check(&v,only,1);check(&v,v.func_count,1);}
 /* Nonempty leading, internal, trailing gaps and overlapping records must fall back. */
 memset(&v,0,sizeof(v));v.funcs=funcs;v.code=code;v.expr=expr;v.func_count=2;v.code_count=v.expr_count=3;memset(code,0,sizeof(code));memset(expr,0,sizeof(expr));
 funcs[0]=(Func){0,1,0,1,0,1,0};funcs[1]=(Func){2,3,2,3,0,1,0};code[1].kind=I_MUTATE;expr[1].kind=E_VECTOR;hide(&v,0);assert(!admission_bodies_dense51(&v));check(&v,0,1);
 printf("%u comparisons PASS (%u selected-body, %u full-scan)\n",checks,dense_checks,fallback_checks);return 0;}
