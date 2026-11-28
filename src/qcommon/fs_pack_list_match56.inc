/* Reject unrelated archive names before walking the full path. The original
 * FS_ReturnPath depth rule includes its terminal-separator adjustment, even
 * for a single-character name. No directory/cache state is retained. */
static int FS_PackListMatch56(const char *name,const char *path,int pathLength,
                             int pathDepth,const char *extension,int extensionLength){
 int at,len,last=0,depth=0;
 if(pathLength&&(Q_stricmpn(name,path,pathLength)||name[pathLength]!='/'))return 0;
 len=(int)strlen(name);
 if(len<extensionLength||Q_stricmp(name+len-extensionLength,extension))return 0;
 for(at=0;at<len;++at)if(name[at]=='/'||name[at]=='\\'){last=at;++depth;}
 if(last+1==len)--depth;
 return depth==pathDepth&&pathLength<=last;
}
