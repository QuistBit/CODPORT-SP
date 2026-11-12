static unsigned animtime_static_issues(const GscProgram*g,const int*args,int argc){
 if(argc!=1||!args)return GSC_LINK_ISSUE_BUILTIN_ARITY;
 if(args[0]<0||args[0]>=g->expr_count)return GSC_LINK_ISSUE_BUILTIN_LITERAL;
 return 0; /* Typed namespace, same tree and native node-domain checks occur at execution. */
}
