typedef struct {
 GscSession*s;void*controller,*actorLease;
 const GscActorAnimationRequest*input,*forwarded;GscActorAnimationRequest q;
 int(*current)(void*);void*context;GscActorAnimationHost host;
 GscActorSourceAnimationReceipt receipt;int phase,cancelled;
} SessionSourceAnimationLease;
static int session_source_private(const GscSession*,const void*,size_t);
static int session_source_begin(GscSession*,const GscActorAnimationRequest*,int(*)(void*),void*,SessionSourceAnimationLease*);
static int session_source_bind(SessionSourceAnimationLease*,void*,const GscActorAnimationRequest*);
static int session_source_scope(SessionSourceAnimationLease*);
static int session_source_deny(SessionSourceAnimationLease*);
static int session_source_enter_game(SessionSourceAnimationLease*);
static int session_source_end(SessionSourceAnimationLease*,int);
static int session_source_take_unavailable(void*,const GscActorAnimationRequest*,int(*)(void*),void*,GscActorSourceAnimationReceipt*);
static int session_source_claim(void*,const GscActorAnimationRequest*,int(*)(void*),void*,GscActorSourceAnimationReceipt*);
static int session_source_current(void*,const GscActorSourceAnimationReceipt*,const GscActorAnimationRequest*,int(*)(void*),void*);
