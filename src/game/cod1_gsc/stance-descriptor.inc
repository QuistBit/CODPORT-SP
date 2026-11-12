static unsigned stance_static_issues(const GscProgram*g,const int*args,int argc){
 const Expr*e;int type;
 if(argc!=1||!args)return GSC_LINK_ISSUE_BUILTIN_ARITY;
 if(args[0]<0||args[0]>=g->expr_count)return GSC_LINK_ISSUE_BUILTIN_LITERAL;
 e=&g->expr[args[0]];type=known_value_kind(g,args[0]);
 if((type>=0&&type!=V_STRING)||e->kind==E_ANIM_REF||e->kind==E_ANIMTREE_VALUE)return GSC_LINK_ISSUE_BUILTIN_LITERAL;
 return 0;
}
