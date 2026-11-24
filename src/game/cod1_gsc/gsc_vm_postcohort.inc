typedef struct {GscRuntime*r;PlacedCohortWitness*w;const void*authority;uint64_t owner;uint32_t rows;} PlacedLateScope;
static int placed_late_bound(const PlacedLateScope*c){return c&&c->r&&c->r->placed_cohort==c->w&&
 c->w->phase==PLACED_COHORT_LATE&&c->w->storage;}
static int placed_late_grant(const PlacedLateScope*c){return gsc_session_postcohort_scope(c->authority,c->r,c->owner,c->rows);}
static int placed_late_current(void*ctx){PlacedLateScope*c=ctx;
 return placed_late_bound(c)&&placed_cohort_same(c->r,c->w,0,1)&&placed_late_grant(c);}
/* native_begin pairs its callback with gsc_placed_postcohort_current.
 * The grant binds this runtime and owner to the active Session. Both checks
 * read the same live witness, with no foreign call between them. */
static int placed_late_session_current(void*ctx){PlacedLateScope*c=ctx;
 return placed_late_bound(c)&&placed_late_grant(c);}
int gsc_placed_postcohort_current(GscRuntime*r,uint64_t owner){PlacedCohortWitness*w=r?r->placed_cohort:NULL;
 return w&&w->owner==owner&&w->phase==PLACED_COHORT_LATE&&w->storage&&placed_cohort_same(r,w,0,1);
}
int gsc_placed_postcohort_call_scope(GscRuntime*r,uint64_t owner,int step){PlacedCohortWitness*w=r?r->placed_cohort:NULL;
 return w&&w->owner==owner&&w->phase==PLACED_COHORT_LATE&&w->storage&&w->late_call==step&&step>=1&&step<=3&&
  r->entity_certificate_busy==1&&(step==3||placed_cohort_same(r,w,0,1));
}
int gsc_execute_placed_postcohort(GscRuntime*r,const void*authority,uint64_t owner,uint32_t rows,char*error,size_t cap){
 PlacedCohortWitness*w=r?r->placed_cohort:NULL;PlacedLateScope scope;int ok=0,begun=0,finished;
 if(!r||!gsc_runtime_storage_separate(r,error,cap))return 0;
 if(!w||!rows||w->owner!=owner||w->row!=rows-1||
  w->phase!=PLACED_COHORT_READY||w->storage||!placed_cohort_quiet(r)||!placed_cohort_same(r,w,0,1))return placed_cohort_fail(r,error,cap,"post-cohort requires the complete current original INIT witness");
 if(!gsc_session_postcohort_admit(authority,r,owner,rows))return placed_cohort_fail(r,error,cap,"post-cohort has no genuine one-shot Session grant");
 w->phase=PLACED_COHORT_LATE;
 w->storage=cohort_storage_capture(r);
 if(!w->storage||!placed_cohort_liveness(r,w,0,1))return placed_cohort_fail(r,error,cap,"post-cohort storage capture or retained owner failed");
 scope.r=r;scope.w=w;scope.authority=authority;scope.owner=owner;scope.rows=rows;
 /* Keep all mutating/entry/free APIs fenced while GAME/Session validates its
  * own providers. Pure node cache and frozen-witness queries remain legal. */
 r->entity_certificate_busy=1;begun=1;w->late_call=1;
 ok=gsc_session_postcohort_native_begin(authority,r,owner,rows,placed_late_session_current,&scope,error,cap)==1&&r->entity_certificate_busy==1&&placed_late_current(&scope);
 w->late_call=0;r->entity_certificate_busy=0;
 if(ok)ok=placed_cohort_liveness(r,w,0,1)&&placed_late_current(&scope);
 r->entity_certificate_busy=1;
 /* Native check ends with its fresh world fence. No foreign call follows
  * check and precedes the fixed pure finish/consumption of GAME ownership. */
 if(ok){w->late_call=2;ok=gsc_session_postcohort_native_check(authority,r,owner,rows)==1&&r->entity_certificate_busy==1&&placed_late_current(&scope);w->late_call=0;}
 w->late_call=3;finished=begun?gsc_session_postcohort_native_finish(authority,r,owner,rows,ok):0;w->late_call=0;
 ok=ok&&finished==1&&r->entity_certificate_busy==1&&placed_cohort_same(r,w,0,1);r->entity_certificate_busy=0;
 if(!ok)return placed_cohort_fail(r,error,cap,"post-cohort native owner phase failed or changed retained state");
 w->phase=PLACED_COHORT_LATE_DONE;return 1;
}

int gsc_start_configured_main_after_cohort(GscRuntime *r,const GscMapExecutionCertificate *input,
 uint64_t owner,uint32_t rows){
 PlacedCohortWitness *w=r?r->placed_cohort:NULL;GscMapExecutionCertificate map;
 if(!w||!rows||!input||!gsc_runtime_storage_separate(r,input,sizeof(*input))||
    w->owner!=owner||w->row!=rows-1||w->phase!=PLACED_COHORT_LATE_DONE||
    r->configured_main_attempted||!r->bootstrap_started||!r->bootstrap_returned||
    r->now_ms||!r->r237_frame_buckets_enabled||!r->r243_thread_start_enabled||
    !placed_cohort_quiet(r)||memcmp(input,&w->map,sizeof(*input))||
    !runtime_start_capabilities(r)||!placed_cohort_liveness(r,w,0,1))
    return rfail(r,0,"cached map entry requires the completed placed cohort and retained tasks");
 r->expand_map_tasks=1;
 map=w->map;
 /* Consume the exact continuation once. A child belongs to the runtime.
  * Only the separately allocated witness copies are released here. */
 r->placed_cohort=NULL;placed_cohort_free(r,w);
 return runtime_start_configured_main(r,&map,1);
}
