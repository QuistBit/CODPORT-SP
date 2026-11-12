/* Bounded semantic declaration text, without native compiler WORD identities. */
static int BspDeclarationToken(const char *bytes,size_t length,size_t *at,char token[128]) {
    size_t n=0;int quoted;
    for(;;) {
        while(*at<length&&(unsigned char)bytes[*at]<=32)++*at;
        if(*at==length)return 0;
        if(length-*at>=2&&bytes[*at]=='/'&&bytes[*at+1]=='/') {
            *at+=2;while(*at<length&&bytes[*at]!='\n')++*at;
        } else if(length-*at>=2&&bytes[*at]=='/'&&bytes[*at+1]=='*') {
            *at+=2;
            while(length-*at>=2&&(bytes[*at]!='*'||bytes[*at+1]!='/'))++*at;
            if(length-*at<2)return -1;
            *at+=2;
        } else break;
    }
    quoted=bytes[*at]=='"';if(quoted)++*at;
    while(*at<length) {
        unsigned char c=(unsigned char)bytes[*at];
        if(quoted&&c=='"'){++*at;token[n]=0;return 1;}
        if(!quoted&&c<=32){token[n]=0;return 1;}
        if(quoted&&c=='\\'&&length-*at>=2&&bytes[*at+1]=='"'){c='"';++*at;}
        if(n>=127)return -1;
        token[n++]=(char)c;++*at;
    }
    if(quoted)return -1;
    token[n]=0;return 1;
}
int G_Cod1ParseBspFieldDeclarations(const char *bytes,size_t length,
    cod1BspFieldDeclarationTable_t *out,char *error,size_t size) {
    cod1BspFieldDeclarationTable_t staged;size_t at=0,n;uint32_t type,i;int token;
    char text[128];
    if(ActorOverlap(bytes,length,out,out?sizeof(*out):0)||
       !ActorArguments(bytes,length,out,out?sizeof(*out):0,error,size))return 0;
    if(!out||(!bytes&&length)||length>COD1_BSP_DECLARATION_TEXT_MAX) {
        EntityError(error,size,"BSP declaration text storage or length is invalid");return 0;
    }
    if(length&&memchr(bytes,0,length)) {
        EntityError(error,size,"BSP declaration text contains an embedded NUL");return 0;
    }
    memset(&staged,0,sizeof(staged));
    while((token=BspDeclarationToken(bytes,length,&at,text))>0) {
        if(!strcmp(text,"string"))type=1;else if(!strcmp(text,"float"))type=4;
        else if(!strcmp(text,"int"))type=5;else goto malformed;
        if(staged.count==COD1_BSP_FIELD_DECLARATIONS_MAX||BspDeclarationToken(bytes,length,&at,text)!=1||!text[0])goto malformed;
        for(n=0;text[n];++n) {
            unsigned char c=(unsigned char)text[n];if(c>='A'&&c<='Z')c=(unsigned char)(c+'a'-'A');
            if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'))goto malformed;
            text[n]=(char)c;
        }
        for(i=0;i<staged.count;++i)if(!strcmp(text,staged.fields[i].name))goto malformed;
        strcpy(staged.fields[staged.count].name,text);staged.fields[staged.count++].type=type;
    }
    if(token<0)goto malformed;
    *out=staged;return 1;
malformed:
    EntityError(error,size,"BSP declaration text is malformed or exceeds the bounded schema");return 0;
}
static int BspOwnedDeclarationCurrent(uint64_t receipt) {
    return receipt&&ActorMapAvailable()&&
        ((receipt==cod1BspTextPending&&cod1BspTextPendingEpoch==cod1EntityEpoch)||
         (receipt==cod1BspTextReceipt&&cod1BspTextEpoch==cod1EntityEpoch&&
          cod1BspDeclarationEpoch==cod1EntityEpoch&&cod1BspDeclarations.receipt==receipt));
}
int G_Cod1EntitySelectBspFieldDeclarationText(const char *bytes,size_t length,char *error,size_t size) {
    /* Full-table staging stays off the QVM stack. The parser frame returns
     * before the old selector creates its independent callback-safe copy. */
    static cod1BspFieldDeclarationTable_t parsed;
    static cod1BspFieldDeclarationHost_t host;
    uint64_t epoch,receipt;int accepted;
    if(!ActorArguments(bytes,length,NULL,0,error,size)||
       ActorOverlap(&parsed,sizeof(parsed),error,size)||ActorOverlap(&host,sizeof(host),error,size))return 0;
    if(cod1PlacedBusy||cod1BspTextBusy||cod1BspDeclarationEpoch||!ActorMapAvailable()) {
        EntityError(error,size,"BSP declaration text requires an unbound current map");return 0;
    }
    if(cod1BspTextSerial==UINT64_MAX) {
        EntityError(error,size,"BSP declaration text receipt capacity is exhausted");return 0;
    }
    cod1BspTextBusy=1;
    if(!G_Cod1ParseBspFieldDeclarations(bytes,length,&parsed,error,size)){cod1BspTextBusy=0;return 0;}
    memset(&host,0,sizeof(host));host.count=parsed.count;
    memcpy(host.fields,parsed.fields,sizeof(host.fields));
    epoch=cod1EntityEpoch;receipt=++cod1BspTextSerial;
    host.receipt=receipt;host.current=BspOwnedDeclarationCurrent;
    cod1BspTextPending=receipt;cod1BspTextPendingEpoch=epoch;
    accepted=G_Cod1EntitySelectBspFieldDeclarations(&host,error,size);
    if(accepted){cod1BspTextReceipt=receipt;cod1BspTextEpoch=epoch;}
    cod1BspTextPending=cod1BspTextPendingEpoch=0;cod1BspTextBusy=0;
    return accepted;
}
