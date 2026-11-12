static int move_delta_descriptor(const GscProgram*g,const char*name){
 return g->move_delta_selected&&callable_name_equal(g,name,"getmovedelta");
}
static unsigned move_delta_static_issues(const GscProgram*g,const int*args,int argc){
 int n,type;const Expr*e;
 if(argc<1||argc>3)return GSC_LINK_ISSUE_BUILTIN_ARITY;
 for(n=0;n<argc;n++){
  if(!args||args[n]<0||args[n]>=g->expr_count)return GSC_LINK_ISSUE_BUILTIN_LITERAL;
  e=&g->expr[args[n]];type=known_value_kind(g,args[n]);
  if(n==0){if(type>=0||e->kind==E_ANIMTREE_VALUE||e->kind==E_CAST)return GSC_LINK_ISSUE_BUILTIN_LITERAL;}
  else if((type>=0&&type!=V_NUMBER)||e->kind==E_ANIM_REF||e->kind==E_ANIMTREE_VALUE)return GSC_LINK_ISSUE_BUILTIN_LITERAL;
 }
 return 0;
}
