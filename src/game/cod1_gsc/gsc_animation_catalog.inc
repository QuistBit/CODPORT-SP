/* Bounded declared-symbol catalog. This is not the native XAnim builder and
 * ordinals below never stand for native tree/node IDs or loaded clip data. */
struct GscAnimationCatalog {uint64_t token;uint32_t count;char tree[64];GscAnimationCatalogEntry entries[GSC_ANIMATION_CATALOG_CAPACITY];};
typedef struct {const unsigned char *bytes;size_t length,at,offset;int token;char word[64];char *error;size_t error_size;GscAnimationCatalog *catalog;} AnimationParser;
static int animation_catalog_error(AnimationParser *p,const char *message){if(p->error&&p->error_size)snprintf(p->error,p->error_size,"animation catalog byte %lu: %s",(unsigned long)p->offset,message);return 0;}
static int animation_catalog_name_equal(const char *a,const char *b){for(;;){unsigned char x=(unsigned char)*a++,y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;if(!x)return 1;}}
static int animation_catalog_id(unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';}
static int animation_catalog_token(AnimationParser *p){size_t n=0;
 for(;;){while(p->at<p->length&&p->bytes[p->at]<=32)++p->at;
  if(p->at+1<p->length&&p->bytes[p->at]=='/'&&p->bytes[p->at+1]=='/'){p->at+=2;while(p->at<p->length&&p->bytes[p->at]!='\n')++p->at;continue;}
  if(p->at+1<p->length&&p->bytes[p->at]=='/'&&p->bytes[p->at+1]=='*'){p->at+=2;while(p->at+1<p->length&&!(p->bytes[p->at]=='*'&&p->bytes[p->at+1]=='/'))++p->at;if(p->at+1>=p->length)return animation_catalog_error(p,"unterminated comment");p->at+=2;continue;}break;}
 p->offset=p->at;memset(p->word,0,sizeof(p->word));if(p->at==p->length){p->token=0;return 1;}
 if(p->bytes[p->at]=='{'||p->bytes[p->at]=='}'||p->bytes[p->at]==':'){p->token=p->bytes[p->at++];return 1;}
 if(!animation_catalog_id(p->bytes[p->at])||(p->bytes[p->at]>='0'&&p->bytes[p->at]<='9'))return animation_catalog_error(p,"unsupported token");
 while(p->at<p->length&&animation_catalog_id(p->bytes[p->at])){if(n>=63)return animation_catalog_error(p,"name exceeds 63 bytes");p->word[n++]=(char)p->bytes[p->at++];}p->token=256;return 1;
}
static int animation_catalog_sequence(AnimationParser *p,uint32_t parent,unsigned depth){
 while(p->token&&p->token!='}'){
  GscAnimationCatalogEntry *entry;uint32_t ordinal;unsigned i;
  if(p->token!=256)return animation_catalog_error(p,"expected declared name");
  if(p->catalog->count>=GSC_ANIMATION_CATALOG_CAPACITY)return animation_catalog_error(p,"declaration capacity exceeded");
  for(i=0;i<p->catalog->count;++i)if(animation_catalog_name_equal(p->catalog->entries[i].name,p->word))return animation_catalog_error(p,"duplicate declared name");
  ordinal=++p->catalog->count;entry=&p->catalog->entries[ordinal-1];entry->ordinal=ordinal;entry->parent=parent;entry->source_offset=p->offset;strcpy(entry->name,p->word);
  if(!animation_catalog_token(p))return 0;
  if(p->token==':'){
   if(!animation_catalog_token(p))return 0;
   while(p->token==256){uint32_t flag=!strcmp(p->word,"complete")?GSC_ANIMATION_NODE_COMPLETE:!strcmp(p->word,"loopsync")?GSC_ANIMATION_NODE_LOOPSYNC:!strcmp(p->word,"nonloopsync")?GSC_ANIMATION_NODE_NONLOOPSYNC:0;
    if(!flag||(entry->flags&flag)||((flag&(GSC_ANIMATION_NODE_LOOPSYNC|GSC_ANIMATION_NODE_NONLOOPSYNC))&&(entry->flags&(GSC_ANIMATION_NODE_LOOPSYNC|GSC_ANIMATION_NODE_NONLOOPSYNC))))return animation_catalog_error(p,"unsupported or repeated flag");
    entry->flags|=flag;if(!animation_catalog_token(p))return 0;}
   if(!entry->flags||p->token!='{')return animation_catalog_error(p,"flags require a branch");
  }
  if(p->token=='{'){
   if(depth>=63){return animation_catalog_error(p,"tree depth exceeded");}
   entry->flags|=GSC_ANIMATION_NODE_BRANCH;
   if(!animation_catalog_token(p)||!animation_catalog_sequence(p,ordinal,depth+1))return 0;
   if(p->token!='}'){return animation_catalog_error(p,"missing closing brace");}
   if(!animation_catalog_token(p))return 0;
  }
 }
 return 1;
}
GscAnimationCatalog *gsc_animation_catalog_parse(const char *tree,const void *bytes,size_t length,char *error,size_t size){
 static uint64_t serial;AnimationParser parser;GscAnimationCatalog *catalog;size_t n,k;
 if(!tree||!bytes||(size&&!error)||length>131072)return NULL;
 for(n=0;n<64&&tree[n];++n){}if(Cod1ModelStringOverlap(tree,n<64?n+1:64,error,size)||Cod1ModelStringOverlap(bytes,length,error,size))return NULL;
 if(error&&size)error[0]=0;
 if(!n||n==64){if(error&&size)str_copy(error,size,"invalid animation tree name");return NULL;}
 for(k=0;k<n;++k)if(!animation_catalog_id((unsigned char)tree[k])){if(error&&size)str_copy(error,size,"invalid animation tree name");return NULL;}
 for(k=0;k<length;++k){unsigned char c=((const unsigned char *)bytes)[k];if(!c||c>126||(c<32&&c!='\t'&&c!='\r'&&c!='\n')){if(error&&size)str_copy(error,size,"unsupported animation catalog byte");return NULL;}}
 if(serial==UINT64_MAX){if(error&&size)str_copy(error,size,"animation catalog identity exhausted");return NULL;}
 catalog=GSC_RUNTIME_CALLOC(1,sizeof(*catalog));if(!catalog){if(error&&size)str_copy(error,size,"animation catalog allocation failed");return NULL;}
 memset(&parser,0,sizeof(parser));parser.bytes=bytes;parser.length=length;parser.error=error;parser.error_size=size;parser.catalog=catalog;
 if(!animation_catalog_token(&parser)||!animation_catalog_sequence(&parser,0,0)||parser.token||!catalog->count){if(error&&size&&!error[0])str_copy(error,size,"invalid animation catalog");GSC_RUNTIME_FREE(catalog);return NULL;}
 strcpy(catalog->tree,tree);catalog->token=++serial;return catalog;
}
void gsc_animation_catalog_free(GscAnimationCatalog *catalog){if(catalog)GSC_RUNTIME_FREE(catalog);}
size_t gsc_animation_catalog_count(const GscAnimationCatalog *catalog){return catalog&&catalog->token?catalog->count:0;}
int gsc_animation_catalog_entry(const GscAnimationCatalog *catalog,size_t index,GscAnimationCatalogEntry *out){if(!catalog||!catalog->token||!out||index>=catalog->count||Cod1ModelStringOverlap(out,sizeof(*out),catalog,sizeof(*catalog)))return 0;*out=catalog->entries[index];return 1;}

int gsc_animation_catalog_storage_overlap(const GscAnimationCatalog*c,const void*p,size_t n){return c&&p&&n&&Cod1ModelStringOverlap(c,sizeof(*c),p,n);}
