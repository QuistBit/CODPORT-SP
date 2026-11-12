/* Original463550 coerces ordinary string/number using45D620(%g) or45D6A0(%i),
 * then4636A0/465E3E compare intern IDs. This copied-value lane reuses exact
 * R164 finite numeric text; it does not reproduce temporary-cell refcounts,
 * native interning, other mixed types, relational operators, or live locale. */
#include "gsc_string_cast.h"
static int GscStringCast(const GscStringCastValue *,GscStringCastResult *,char *,size_t);
int gsc_program_select_r239_mixed_scalar_equality(GscProgram*g){
 if(!g||g->invalid||g->linked||g->r239_mixed_scalar_equality_selected)return 0;
 g->r239_mixed_scalar_equality_selected=1;return 1;
}
int gsc_runtime_select_r239_mixed_scalar_equality(GscRuntime*r){
 if(!r||!r->program||!r->program->r239_mixed_scalar_equality_selected||
    r->r239_mixed_scalar_equality_enabled||r->started||r->advancing||
    r->executing_task||r->task_count||r->error[0]||r->entity_entry_depth||
    r->hud_elements_busy||r->entity_prepare_busy||r->entity_certificate_busy||
    r->entity_sequence_busy||r->entity_inheritance_busy||r->entity_field_busy||
    r->entity_method_busy)return 0;
 r->r239_mixed_scalar_equality_enabled=1;return 1;
}
int gsc_runtime_r239_mixed_scalar_equality_selected(const GscRuntime*r){
 return r&&r->program&&r->program->r239_mixed_scalar_equality_selected&&
        r->r239_mixed_scalar_equality_enabled&&!r->error[0];
}
static int r239_mixed_scalar_pair(const GscRuntime*r,int op,const Value*a,const Value*b){
 return r&&r->r239_mixed_scalar_equality_enabled&&(op==T_EQ||op==T_NE)&&
        ((a->kind==V_NUMBER&&b->kind==V_STRING)||(a->kind==V_STRING&&b->kind==V_NUMBER));
}
static int r239_mixed_scalar_compare(GscRuntime*r,int op,const Value*a,const Value*b,int line,Value*out){
 const Value*number,*string;GscStringCastValue input;GscStringCastResult text;Value result;int equal;
 if(!r||!a||!b||!out||!r239_mixed_scalar_pair(r,op,a,b))return r?rfail(r,line,"R239 mixed equality requires selected number/string EQ/NE"):0;
 number=a->kind==V_NUMBER?a:b;string=a->kind==V_STRING?a:b;
 if(!memchr(string->str,0,sizeof(string->str)))return rfail(r,line,"invalid mixed equality string payload");
 memset(&input,0,sizeof(input));
 if(number->integer==1){input.type=5;if(!scalar_get_integer(r,number,line,&input.integer))return 0;}
 else if(number->integer==0){input.type=4;if(!scalar_float(r,number,line,&input.number))return 0;}
 else return rfail(r,line,"invalid mixed equality scalar tag");
 if(GscStringCast(&input,&text,NULL,0)!=1)return rfail(r,line,"mixed equality numeric text refused");
 equal=!strcmp(text.string,string->str);scalar_integer(&result,op==T_EQ?equal:!equal);
 *out=result;return 1;
}
