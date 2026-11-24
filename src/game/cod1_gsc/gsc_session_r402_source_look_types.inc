typedef struct {
 GscSession*s;void*controller,*actorLease;
 const GscActorLookRequest*input,*forwarded;GscActorLookRequest q;
 int(*current)(void*);void*context;GscActorLookHost host;
 GscActorSourceLookReceipt receipt;int phase,cancelled;
 GscActorLookRequest gameQ;GscActorLookHost provider;int mapped,hostNow;int64_t origin;
} SessionSourceLookLease;
static int session_source_look_private(const GscSession*,const void*,size_t);
static int session_source_look_begin(GscSession*,const GscActorLookRequest*,int(*)(void*),void*,SessionSourceLookLease*);
static int session_source_look_bind(SessionSourceLookLease*,void*,const GscActorLookRequest*);
static int session_source_look_scope(SessionSourceLookLease*);
static int session_source_look_deny(SessionSourceLookLease*);
static int session_source_look_enter_game(SessionSourceLookLease*);
static int session_source_look_end(SessionSourceLookLease*,int);
static int session_source_look_take_unavailable(void*,const GscActorLookRequest*,int(*)(void*),void*,GscActorSourceLookReceipt*);
static int session_source_look_claim(void*,const GscActorLookRequest*,int(*)(void*),void*,GscActorSourceLookReceipt*);
static int session_source_look_current(void*,const GscActorSourceLookReceipt*,const GscActorLookRequest*,int(*)(void*),void*);
