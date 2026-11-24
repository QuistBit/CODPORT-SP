static unsigned turret_precache_issues(const GscProgram*g,const int*args,int argc){
 (void)g;(void)args;return argc>=0&&argc<=MAX_PARAMS?0:GSC_LINK_ISSUE_BUILTIN_ARITY;
}
