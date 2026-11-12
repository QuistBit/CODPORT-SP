/* R95 private path identity policy. This is ASCII VFS canonicalization for
 * the opted-in loader, not a claim about original CoD1 script symbol rules.
 * Function identifiers remain case-sensitive. Old modes never call it. */
static int gsc_module_identity_canonical(const char *name,char *out,size_t cap) {
    size_t n=0;int component=0;
    if(!name||!out||cap<2)return 0;
    while(name[n]) {
        unsigned char c=(unsigned char)name[n];
        if(n+1>=cap)return 0;
        if(c=='\\'||c=='/') {
            if(!component)return 0;
            out[n]='/';component=0;
        } else {
            if(c>='A'&&c<='Z')c=(unsigned char)(c+'a'-'A');
            if(!((c>='a'&&c<='z')||c=='_'||
                 (component&&c>='0'&&c<='9')))return 0;
            out[n]=(char)c;component=1;
        }
        ++n;
    }
    if(!n||!component)return 0;
    out[n]=0;return 1;
}
#ifndef GSC_MODULE_IDENTITY_CANONICAL_ONLY
static int gsc_module_identity_equal(const char *left,const char *right) {
    size_t n=0;
    if(!left||!right)return 0;
    while(left[n]&&right[n]) {
        unsigned char a=(unsigned char)left[n],b=(unsigned char)right[n];
        if(a=='\\')a='/';
        if(b=='\\')b='/';
        if(a>='A'&&a<='Z')a=(unsigned char)(a+'a'-'A');
        if(b>='A'&&b<='Z')b=(unsigned char)(b+'a'-'A');
        if(a!=b)return 0;
        ++n;
    }
    return left[n]==right[n];
}

#endif
