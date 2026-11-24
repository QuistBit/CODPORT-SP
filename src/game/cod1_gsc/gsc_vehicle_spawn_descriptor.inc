static unsigned vehicle_spawn_issues(const GscProgram*g,const int*args,int argc){
 int n,k;if(argc<5||argc>MAX_PARAMS)return GSC_LINK_ISSUE_BUILTIN_ARITY;
 for(n=0;args&&n<5;n++){k=known_value_kind(g,args[n]);if(k>=0&&k!=(n<3?V_STRING:V_VECTOR))return GSC_LINK_ISSUE_BUILTIN_LITERAL;}
 return 0;
}
