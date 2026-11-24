/* Portable serial backend until a platform worker implementation is supplied. */
#include "cod1_jobs.h"
#include <string.h>
void C1J_Configure(int enabled,int requested){(void)enabled;(void)requested;}
void C1J_Shutdown(void){}
int C1J_Enabled(void){return 0;}
int C1J_ParallelFor(unsigned count,unsigned grain,c1jJobFn fn,void *context){(void)grain;return !count?1:fn(context,0,count);}
void C1J_Status(c1jStatus *s){memset(s,0,sizeof(*s));s->logical=s->available=1;}
void C1J_FpGet(c1jFp *s){memset(s,0,sizeof(*s));}
void C1J_FpRestore(const c1jFp*s,unsigned a,unsigned b){(void)s;(void)a;(void)b;}
int C1L_Attach(FILE*f){(void)f;return 0;}
int C1L_Matches(FILE*f){(void)f;return 0;}
int C1L_Write(FILE*f,const void*p,size_t n){return fwrite(p,1,n,f)==n;}
int C1L_Flush(FILE*f,int wait){(void)wait;return fflush(f)==0;}
int C1L_Detach(FILE*f){(void)f;return 1;}
void C1L_Status(c1lStatus*s){memset(s,0,sizeof(*s));}
