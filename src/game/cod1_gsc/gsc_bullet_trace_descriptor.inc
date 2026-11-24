static unsigned bullet_trace_issues(const GscProgram*g,const int*args,int argc){
 int n,k;if(argc<4||argc>MAX_PARAMS)return GSC_LINK_ISSUE_BUILTIN_ARITY;
 for(n=0;args&&n<4;n++){k=known_value_kind(g,args[n]);if(k>=0&&k!=(n<2?V_VECTOR:n==2?V_NUMBER:V_UNDEFINED))return GSC_LINK_ISSUE_BUILTIN_LITERAL;}return 0;
}
