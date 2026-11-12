/* R68 observer only: explicit candidate flags do not change lookup policy. */
static unsigned char name_ascii_lower(unsigned char c) {
    return c>='A'&&c<='Z'?(unsigned char)(c+('a'-'A')):c;
}
static int name_equal(const char *a,const char *b,int fold) {
    const unsigned char *x=(const unsigned char *)a,*y=(const unsigned char *)b;
    while(*x&&*y) {
        if((fold?name_ascii_lower(*x):*x)!=(fold?name_ascii_lower(*y):*y))return 0;
        x++;y++;
    }
    return *x==*y;
}
/* Bounded ASCII validation, deliberately not ctype/locale-dependent. */
static int name_query_valid(const char *s,int path) {
    size_t n;int component=0;
    if(!s||!s[0])return 0;
    for(n=0;n<128;n++) {
        unsigned char c=(unsigned char)s[n];
        if(!c)return component;
        if(path&&c=='/') {if(!component)return 0;component=0;continue;}
        if((c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='_')component=1;
        else if(component&&c>='0'&&c<='9')continue;
        else return 0;
    }
    return 0;
}
int gsc_program_link_module_count(const GscProgram *g) {
    return g&&!g->invalid?g->module_count:0;
}
int gsc_program_link_module_get(const GscProgram *g,int index,GscLinkModuleInfo *out) {
    GscLinkModuleInfo info;
    if(!g||g->invalid||!out||index<0||index>=g->module_count)return 0;
    memset(&info,0,sizeof(info));info.index=index;
    str_copy(info.path,sizeof(info.path),g->modules[index]);*out=info;return 1;
}
int gsc_program_name_candidates(const GscProgram *g,int owner_module,
                                const char *requested_module,const char *name,
                                unsigned flags,GscNameCandidateCallback callback,
                                void *user,GscNameCandidateSummary *out,
                                char *error,size_t error_size) {
    GscNameCandidateSummary s;int m,n,local;const char *path;
    if(error&&error_size)error[0]=0;
    if(out)memset(out,0,sizeof(*out));
    local=!requested_module||!requested_module[0];
    if(!g||g->invalid||!out||owner_module<0||owner_module>=g->module_count||
       (flags&~(GSC_NAME_MODULE_ASCII|GSC_NAME_FUNCTION_ASCII))||
       !name_query_valid(name,0)||(!local&&!name_query_valid(requested_module,1))) {
        if(error&&error_size)snprintf(error,error_size,"invalid script-name query or program state");
        return 0;
    }
    path=local?g->modules[owner_module]:requested_module;
    memset(&s,0,sizeof(s));s.match_flags=flags;s.local_scope=local;
    s.exact_module=s.exact_function=s.unique_module=s.unique_function=-1;
    for(m=0;m<g->module_count;m++) {
        int matched=local?m==owner_module:name_equal(g->modules[m],path,!!(flags&GSC_NAME_MODULE_ASCII));
        if(!matched)continue;
        s.module_matches++;s.unique_module=m;
        if(!strcmp(g->modules[m],path))s.exact_module=m;
    }
    if(s.module_matches!=1)s.unique_module=-1;
    for(n=0;n<g->func_count;n++) {
        const Func *f=&g->funcs[n];GscNameCandidate c;
        if(f->module<0||f->module>=g->module_count) {
            if(error&&error_size)snprintf(error,error_size,"invalid script-name declaration owner");
            return 0;
        }
        m=f->module;
        if(local?m!=owner_module:!name_equal(g->modules[m],path,!!(flags&GSC_NAME_MODULE_ASCII)))continue;
        if(!name_equal(f->name,name,!!(flags&GSC_NAME_FUNCTION_ASCII)))continue;
        s.function_matches++;s.unique_function=n;
        if(m==s.exact_module&&!strcmp(f->name,name))s.exact_function=n;
        if(!callback)continue;
        memset(&c,0,sizeof(c));c.module_index=m;c.function_index=n;c.argc=f->argc;
        c.module_case_changed=strcmp(g->modules[m],path)!=0;c.function_case_changed=strcmp(f->name,name)!=0;
        str_copy(c.module,sizeof(c.module),g->modules[m]);str_copy(c.name,sizeof(c.name),f->name);
        if(!callback(user,&c)) {
            if(error&&error_size)snprintf(error,error_size,"script-name query cancelled after %d candidates",s.function_matches);
            return 0;
        }
    }
    if(s.module_matches!=1||s.function_matches!=1)s.unique_function=-1;
    if(!s.module_matches)s.status=GSC_NAME_NO_MODULE;
    else if(s.module_matches>1)s.status=GSC_NAME_AMBIGUOUS_MODULE;
    else if(!s.function_matches)s.status=GSC_NAME_NO_FUNCTION;
    else if(s.function_matches>1)s.status=GSC_NAME_AMBIGUOUS_FUNCTION;
    else s.status=GSC_NAME_UNIQUE;
    *out=s;return 1;
}
