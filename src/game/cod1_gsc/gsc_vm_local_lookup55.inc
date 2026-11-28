/* Exact, callback-free local lookup. Reads search newest first; writes and
 * event bindings search oldest first. Prefix rejection avoids strcmp on
 * unrelated names without adding persistent metadata to Frame. */
static int frame_local_match55(const char *candidate,const char *name,char first,char second){
    if(candidate[0]!=first)return 0;
    if(!first)return 1;
    if(candidate[1]!=second)return 0;
    return !second||!strcmp(candidate+2,name+2);
}
static int frame_local_find55(const Frame *f,const char *name,int reverse){
    int n;char first=name[0],second=first?name[1]:0;
    if(reverse){
        for(n=f->local_count-1;n>=0;n--)
            if(frame_local_match55(f->locals[n].name,name,first,second))return n;
    }else{
        for(n=0;n<f->local_count;n++)
            if(frame_local_match55(f->locals[n].name,name,first,second))return n;
    }
    return -1;
}
