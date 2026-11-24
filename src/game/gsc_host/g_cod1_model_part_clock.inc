/* Session elapsed time and GAME map time are different clock domains.
 * Preserve both and verify the existing selected Session clock relation.
 * This translates an owned request only; no GSC timing or result changes. */
static int GscTestTurretModelPart(void*u,const GscModelPartRequest*input,char out[128],char*error,size_t cap,int(*current)(void*),void*ctx){
 GscModelPartRequest source,q;GscTestTurretClock t;GscModelPartHost host=*G_Cod1ModelPartHost();char result[128];int ok;(void)u;
 if(!host.query||!GscTestTurretBuffers(input,sizeof(*input),out,128,error,cap))return 0;
 source=q=*input;GscTestTurretClockSet(&t,input,&source,sizeof(source),source.now_ms,current,ctx);
 if(!GscTestTurretClockCurrent(&t))return 0;q.now_ms=t.actual;memset(result,0xff,sizeof(result));
 ok=host.query(host.user,&q,result,error,cap,GscTestTurretClockCurrent,&t);
 if(ok!=1||!GscTestTurretClockCurrent(&t)||q.now_ms!=t.actual||memcmp(&host,G_Cod1ModelPartHost(),sizeof(host))||!memchr(result,0,128)||!result[0])return 0;
 q.now_ms=source.now_ms;if(memcmp(&q,&source,sizeof(q)))return 0;
 memcpy(out,result,128);return 1;
}
