/* Session lifetime fence around read-only, typed native clip metadata. */
int gsc_session_select_r233_namespace_animation_length(GscSession*s,const GscAnimationLengthHost*input,char*error,size_t cap){GscAnimationLengthHost h;
 if((cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,error,cap)||session_animation_namespace_overlap(input,sizeof(*input),error,cap))return 0;
 if(!s||!input)return session_reject(error,cap,"namespace length requires Session host");h=*input;if(!session_enter(s,error,cap))return 0;
 if(s->view.state!=GSC_SESSION_EMPTY||s->program||s->runtime||!s->native_animation_roots_enabled||s->namespace_length_enabled||!h.query)return session_leave_rejected(s,"namespace length requires exclusive idle native-root namespace",error,cap);
 if(!session_animation_resources_current(s)||memcmp(input,&h,sizeof(h)))return session_leave_rejected(s,"namespace length resources expired",error,cap);
 s->namespace_length=h;s->namespace_length_enabled=1;s->busy=0;return 1;
}
typedef struct {SessionTurretAnimationLease base;GscAnimationLengthHost host;GscAnimationLengthRequest *input,copy;int checking,cancelled;} SessionNamespaceLengthLease;
static int session_namespace_length_scope(const SessionNamespaceLengthLease*l){return !l->cancelled&&session_turret_animation_scope(&l->base)&&l->base.session->namespace_length_enabled&&!memcmp(&l->host,&l->base.session->namespace_length,sizeof(l->host))&&!memcmp(l->input,&l->copy,sizeof(l->copy));}
static int session_namespace_length_current(void*opaque){SessionNamespaceLengthLease*l=opaque;int ok;
 if(!session_namespace_length_scope(l))return 0;if(l->checking){l->cancelled=1;return 0;}l->checking=1;
 ok=session_animation_resources_current(l->base.session)&&session_namespace_length_scope(l)&&session_turret_animation_current(&l->base)&&session_namespace_length_scope(l);l->checking=0;return ok;
}
static int session_namespace_length_query(void*user,const GscAnimationLengthRequest*input,GscAnimationLengthReply*out,char*error,size_t cap,int(*current)(void*),void*context){
 GscSession*s=user;SessionNamespaceLengthLease l;GscAnimationLengthRequest q;GscAnimationLengthReply reply;int ok;
 if(!s||!input||!out||!current||(cap&&!error)||session_animation_namespace_private(s,input,sizeof(*input))||session_animation_namespace_private(s,out,sizeof(*out))||session_animation_namespace_private(s,error,cap)||
    session_animation_namespace_overlap(input,sizeof(*input),out,sizeof(*out))||session_animation_namespace_overlap(input,sizeof(*input),error,cap)||session_animation_namespace_overlap(out,sizeof(*out),error,cap))return 0;
 memset(&l,0,sizeof(l));q=*input;l.input=&q;l.copy=q;session_turret_animation_lease(s,&l.base,current,context);l.host=s->namespace_length;
 if(q.runtime_identity!=l.base.instance||!session_namespace_length_current(&l))return 0;
 memset(&reply,0xff,sizeof(reply));ok=l.host.query(l.host.user,&q,&reply,error,cap,session_namespace_length_current,&l);
 if(ok!=1||!session_namespace_length_current(&l)||memcmp(input,&l.copy,sizeof(l.copy)))return 0;*out=reply;return 1;
}
