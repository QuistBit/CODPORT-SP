/* Sparse representation of the same 2078 independent 64-byte name cells.
 * Ordinals remain unchanged. Empty cells read as 64 zero bytes. Storage has
 * full ordinal capacity; only the sorted initialized prefix is semantic data.
 * Each record owns its allocation, and clones copy every defined byte. */
static const char actorGenericEmptyName[64]={0};
static unsigned ActorGenericNameAt(const C1GARecord *r,unsigned ordinal){
 unsigned lo=0,hi=r->notify->nameCount;
 while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(r->names[mid].ordinal<ordinal)lo=mid+1;else hi=mid;}
 return lo;
}
static const char *ActorGenericName(const C1GARecord *r,unsigned ordinal){
 unsigned at;if(!r||!r->notify||!r->names||r->notify->nameCount>C1GA_COUNT||ordinal>=C1GA_COUNT)return actorGenericEmptyName;
 at=ActorGenericNameAt(r,ordinal);return at<r->notify->nameCount&&r->names[at].ordinal==ordinal?r->names[at].text:actorGenericEmptyName;
}
static int ActorGenericNameSet(C1GARecord *r,unsigned ordinal,const char *text,size_t length){
 unsigned at,count;char copy[64]={0};
 if(!r||!r->notify||!r->names||ordinal>=C1GA_COUNT||length>=64||(length&&!text)||r->notify->nameCount>C1GA_COUNT)return 0;
 if(length)memcpy(copy,text,length);
 count=r->notify->nameCount;at=ActorGenericNameAt(r,ordinal);
 if(at<count&&r->names[at].ordinal==ordinal){
  if(length){memcpy(r->names[at].text,copy,64);return 1;}
  memmove(r->names+at,r->names+at+1,(count-at-1)*sizeof(*r->names));r->notify->nameCount=count-1;return 1;
 }
 if(!length)return 1;if(count==C1GA_COUNT)return 0;
 memmove(r->names+at+1,r->names+at,(count-at)*sizeof(*r->names));r->names[at].ordinal=ordinal;memcpy(r->names[at].text,copy,64);r->notify->nameCount=count+1;return 1;
}
static int ActorGenericNamesEqual(const C1GARecord *a,const C1GARecord *b){
 return a&&b&&a->notify&&b->notify&&a->names&&b->names&&a->notify->nameCount<=C1GA_COUNT&&a->notify->nameCount==b->notify->nameCount&&
  R242ExactATRBytesEqual(a->names,b->names,(size_t)a->notify->nameCount*sizeof(*a->names));
}
static int ActorGenericNamesPrivate(const void*p,size_t n,const C1GARecord*r){
 return r&&r->names&&ActorOverlap(p,n,r->names,C1GA_COUNT*sizeof(*r->names));
}
/* Clear tokenless notification cells in ordinal order. The sparse name
 * prefix is checked before using a forward cursor; malformed prefixes keep
 * the old per-ordinal search and its exact failure/partial-write behavior.
 * No callbacks run here. Keep the same memmove for each actual removal,
 * including the otherwise unspecified bytes beyond the initialized prefix. */
static int ActorGenericClearUnnamed(C1GARecord *r){
 unsigned i,at=0,count=r->notify->nameCount;
 int ordered=r->names&&count<=C1GA_COUNT;
 if(ordered)for(i=0;i<count;i++)if(r->names[i].ordinal>=C1GA_COUNT||
    (i&&r->names[i-1].ordinal>=r->names[i].ordinal)){ordered=0;break;}
 for(i=0;i<C1GA_COUNT;i++){
  if(r->notify->flags[i].token){
   if(ordered&&at<count&&r->names[at].ordinal==i)at++;
   continue;
  }
  if(!ordered){if(!ActorGenericNameSet(r,i,NULL,0))return 0;}
  else if(at<count&&r->names[at].ordinal==i){
   memmove(r->names+at,r->names+at+1,(count-at-1)*sizeof(*r->names));
   r->notify->nameCount=--count;
  }
  r->notify->generation[i]=r->notify->pending[i]=0;r->notify->pendingTick[i]=0;
 }
 return 1;
}
