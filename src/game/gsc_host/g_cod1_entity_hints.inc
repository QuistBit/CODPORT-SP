/* Map-scoped port hint strings. Allocation identities and monotonically
 * reserved8-bit indices prevent a retired receiver/session from reusing an
 * older snapshot's text. The retail hint-string pool ABI is not asserted. */
typedef struct {GscEntityRef owner;int index;char key[128];} cod1HintOwner_t;
static cod1HintOwner_t cod1HintOwners[MAX_GENTITIES];
static uint64_t cod1HintMap;
static unsigned cod1HintNext=1;
static int cod1HintBusy,cod1HintCancelled;
static int EntityHintStoragePrivate(const void *p,size_t n){
 return ActorOverlap(p,n,&cod1HintBusy,sizeof(cod1HintBusy))||ActorOverlap(p,n,&cod1HintCancelled,sizeof(cod1HintCancelled))||ActorOverlap(p,n,cod1HintOwners,sizeof(cod1HintOwners))||ActorOverlap(p,n,&cod1HintMap,sizeof(cod1HintMap))||ActorOverlap(p,n,&cod1HintNext,sizeof(cod1HintNext));
}
void G_Cod1HintOwnersReset(void){if(cod1HintBusy)cod1HintCancelled=1;memset(cod1HintOwners,0,sizeof(cod1HintOwners));}
static int HintOwnerCurrent(const GscEntityRef *ref){
 return ref&&G_Cod1GscTestFollowupActive()&&EntityValidate(NULL,ref)&&
  G_Cod1TriggerUseEligible(&g_entities[ref->slot])&&G_Cod1EntitySameAllocation(ref)&&G_Cod1GscTestFollowupActive();
}
int G_Cod1HintIndex(const gentity_t *entity){
 GscEntityRef ref;cod1HintOwner_t *row;
 if(cod1HintBusy||!G_Cod1EntityCapture(entity,&ref)||!HintOwnerCurrent(&ref)||cod1HintMap!=ref.epoch)return 0;
 row=&cod1HintOwners[ref.slot];return !memcmp(&ref,&row->owner,sizeof(ref))?row->index:0;
}
int G_Cod1HintRead(const GscEntityRef *ref,GscStringCastValue *out,char *error,size_t size){
 GscStringCastValue value;int index;(void)error;(void)size;
 if(!ref||!out||G_Cod1EntityStorageProtected(out,sizeof(*out))||ActorOverlap(out,sizeof(*out),g_entities,sizeof(g_entities))||!HintOwnerCurrent(ref))return 0;
 memset(&value,0,sizeof(value));index=G_Cod1HintIndex(&g_entities[ref->slot]);
 if(index){value.type=2;strcpy(value.string,cod1HintOwners[ref->slot].key);}*out=value;return 1;
}
int G_Cod1HintWrite(const GscEntityRef *input,const GscStringCastValue *value,char *error,size_t size){
 GscEntityRef ref;cod1HintOwner_t row;char text[132],actual[MAX_STRING_CHARS];int i,index,ok=0;uint64_t epoch;
 if(cod1HintBusy){cod1HintCancelled=1;return 0;}
 if(!input||!value||(value->type!=2&&(value->type!=1||value->string[0]))||!memchr(value->string,0,sizeof(value->string))||strlen(value->string)>=sizeof(row.key)||
    G_Cod1EntityStorageProtected(input,sizeof(*input))||G_Cod1EntityStorageProtected(value,sizeof(*value))||G_Cod1EntityStorageProtected(error,size))return 0;
 cod1HintBusy=1;cod1HintCancelled=0;ref=*input;memset(&row,0,sizeof(row));strcpy(row.key,value->string);
 for(i=0;row.key[i];i++)if(!((row.key[i]>='A'&&row.key[i]<='Z')||(row.key[i]>='a'&&row.key[i]<='z')||(row.key[i]>='0'&&row.key[i]<='9')||row.key[i]=='_'))goto refuse;
 if(!HintOwnerCurrent(&ref)||cod1HintCancelled)goto refuse;epoch=ref.epoch;
 if(cod1HintMap!=epoch){memset(cod1HintOwners,0,sizeof(cod1HintOwners));cod1HintMap=epoch;cod1HintNext=1;}
 if(!row.key[0]){memset(&cod1HintOwners[ref.slot],0,sizeof(row));ok=1;goto done;}
 if(cod1HintNext>=MAX_COD1_SCRIPT_HINTS)goto refuse;
 index=(int)cod1HintNext++;snprintf(text,sizeof(text),"L:%s",row.key);
 trap_GetConfigstring(CS_COD1_SCRIPT_HINTS+index,actual,sizeof(actual));
 if(!HintOwnerCurrent(&ref)||cod1HintCancelled||G_Cod1EntityMapEpoch()!=epoch||actual[0])goto refuse;
 trap_SetConfigstring(CS_COD1_SCRIPT_HINTS+index,text);
 if(!HintOwnerCurrent(&ref)||cod1HintCancelled||G_Cod1EntityMapEpoch()!=epoch)goto refuse;
 trap_GetConfigstring(CS_COD1_SCRIPT_HINTS+index,actual,sizeof(actual));
 if(!HintOwnerCurrent(&ref)||cod1HintCancelled||G_Cod1EntityMapEpoch()!=epoch||strcmp(actual,text))goto refuse;
 row.owner=ref;row.index=index;cod1HintOwners[ref.slot]=row;ok=1;goto done;
refuse:
 if(error&&size)snprintf(error,size,"hint requires current selected use trigger, bounded localized key and available map-owned wire slot");
done:
 cod1HintBusy=0;return ok;
}
