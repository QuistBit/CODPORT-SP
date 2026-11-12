/* A native pending frame can only be armed by the actual GAME cohort loop.
 * It is borrowed until the facade call returns; it never grants map-main. */
static int Cod1ActorPrefixAccounting(uint64_t);
static int Cod1ActorPostCohortRowsCurrent(void){uint32_t i;
 if(!cod1ActorLate.armed||cod1ActorLate.cancelled||!cod1ActorPrefixBusy||cod1ActorLate.epoch!=cod1ActorPrefix.epoch||
  cod1ActorLate.prefix!=cod1ActorPrefix.receipt||!Cod1ActorPrefixCurrent(cod1ActorLate.prefix)||
  !Cod1ActorPrefixAccounting(cod1ActorLate.epoch)||!cod1ActorPrefix.placedRows||
  cod1ActorPrefix.returnedRows!=cod1ActorPrefix.placedRows||cod1ActorPrefix.accountedRows!=cod1ActorPrefix.placedRows||
  cod1ActorPrefix.runComplete||cod1AiDomain.complete)return 0;
 for(i=0;i<cod1ActorPrefix.placedRows;i++){const cod1ActorBootstrapRow_t*r=cod1ActorPrefix.rows+i;
  if(!r->accounted||r->retired||r->stage!=COD1_ACTOR_ENTRY_ENTERED||r->entryStatus!=GSC_ENTITY_ENTRY_RETURNED||
   !r->placedInitAttempted||!r->placedInitAccepted||r->placedInit.program_identity!=cod1ActorLate.program||
   !G_Cod1ActorPlacedInitCohortCurrent(&r->lease,&r->placedInit))return 0;
 }return 1;
}
int G_Cod1ActorPostCohortNative(GscSession*s,uint64_t id,uint64_t receipt,const void*grant,char*error,size_t cap){
 if(!gsc_session_postcohort_native_scope(s,id,receipt,grant,1)||!cod1ActorLate.armed||cod1ActorLate.service!=grant||Cod1ActorPrefixPrivate(error,cap))return 0;
 if(cod1ActorLate.invoked||cod1ActorLate.finished){cod1ActorLate.cancelled=1;return 0;}
 cod1ActorLate.invoked=1;cod1ActorLate.session=s;cod1ActorLate.id=id;cod1ActorLate.graph=receipt;
 if(!Cod1ActorPostCohortRowsCurrent())return Cod1ActorPrefixFail(error,cap,"post-cohort native rows or currentstate stores retired");
 cod1ActorLate.body=G_Cod1NodeGraphPostCohortInitialOwners(s,id,receipt,grant);
 return cod1ActorLate.body;
}
int G_Cod1ActorPostCohortNativeCheck(GscSession*s,uint64_t id,uint64_t receipt,const void*grant){int ok;
 if(!gsc_session_postcohort_native_scope(s,id,receipt,grant,2)||!cod1ActorLate.armed||cod1ActorLate.service!=grant)return 0;
 if(cod1ActorLate.checking){cod1ActorLate.cancelled=1;return 0;}cod1ActorLate.checking=1;
 ok=!cod1ActorLate.cancelled&&
  cod1ActorLate.invoked&&cod1ActorLate.body&&!cod1ActorLate.finished&&cod1ActorLate.session==s&&cod1ActorLate.id==id&&cod1ActorLate.graph==receipt&&
  Cod1ActorPostCohortRowsCurrent()&&G_Cod1NodeGraphPostCohortCurrent(s,id,receipt,grant)&&!cod1ActorLate.cancelled;
 cod1ActorLate.checking=0;return ok;
}
int G_Cod1ActorPostCohortNativeFinish(GscSession*s,uint64_t id,uint64_t receipt,const void*grant,int keep){int ok,ended=0;
 if(!gsc_session_postcohort_native_scope(s,id,receipt,grant,3)||!cod1ActorLate.armed||cod1ActorLate.service!=grant||cod1ActorLate.finished)return 0;
 ok=keep==1&&!cod1ActorLate.cancelled&&!cod1ActorLate.checking&&cod1ActorLate.invoked&&cod1ActorLate.body&&cod1ActorLate.session==s&&cod1ActorLate.id==id&&cod1ActorLate.graph==receipt&&
  cod1ActorPrefixBusy&&cod1ActorLate.epoch==cod1ActorPrefix.epoch&&cod1ActorLate.prefix==cod1ActorPrefix.receipt&&
  !cod1ActorPrefix.closed&&!cod1ActorPrefix.invalid&&cod1ActorPrefix.returnedRows==cod1ActorPrefix.placedRows&&
  !cod1ActorPrefix.runComplete&&!cod1AiDomain.complete;
 if(cod1ActorLate.invoked&&cod1ActorLate.session==s&&cod1ActorLate.id==id&&cod1ActorLate.graph==receipt)
  ended=G_Cod1NodeGraphPostCohortEnd(s,id,receipt,grant,ok);
 cod1ActorLate.finished=1;cod1ActorLate.completed=ok&&ended==1;return cod1ActorLate.completed;
}
