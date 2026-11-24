#include "cod1_jobs.h"
#include <windows.h>
#include <process.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifndef C1L_CAPACITY
#define C1L_CAPACITY (1024*1024)
#endif
#ifndef C1L_WRITE
#define C1L_WRITE(f,p,n) fwrite(p,1,n,f)
#endif
#ifndef C1L_FLUSH
#define C1L_FLUSH(f) fflush(f)
#endif
static struct {
 int initialized,active,stop,error;FILE *file;CRITICAL_SECTION lock,producer;
 HANDLE thread,wake,progress;unsigned char *bytes;size_t head,used;
 uint64_t queued,written,requested,flushed,backpressure;
} c1log;
static unsigned __stdcall C1L_Worker(void *unused){
 (void)unused;
 for(;;){size_t count=0,head=0,written=0;uint64_t flushThrough=0;int stop=0,flush=0,error=0;
  EnterCriticalSection(&c1log.lock);
  if(c1log.error){LeaveCriticalSection(&c1log.lock);break;}
  if(c1log.flushed<c1log.requested && c1log.written>=c1log.requested){flush=1;flushThrough=c1log.written;}
  else if(c1log.used){head=c1log.head;count=c1log.used;if(count>C1L_CAPACITY-head)count=C1L_CAPACITY-head;if(count>32768)count=32768;}
  else if(c1log.stop)stop=1;
  LeaveCriticalSection(&c1log.lock);
  if(stop)break;
  if(count){
   written=C1L_WRITE(c1log.file,c1log.bytes+head,count);
   if(!written){written=C1L_WRITE(c1log.file,c1log.bytes+head,count);if(!written)error=errno?errno:EIO;}
   if(written>count){written=0;error=EIO;}
  }else if(flush){if(C1L_FLUSH(c1log.file)!=0)error=errno?errno:EIO;}
  else {WaitForSingleObject(c1log.wake,INFINITE);continue;}
  EnterCriticalSection(&c1log.lock);
  if(written){c1log.head=(c1log.head+written)%C1L_CAPACITY;c1log.used-=written;c1log.written+=written;}
  if(flush&&!error)c1log.flushed=flushThrough;
  if(error)c1log.error=error;
  SetEvent(c1log.progress);LeaveCriticalSection(&c1log.lock);
 }SetEvent(c1log.progress);return 0;
}
static void C1L_Init(void){if(!c1log.initialized){InitializeCriticalSection(&c1log.lock);InitializeCriticalSection(&c1log.producer);c1log.initialized=1;}}
int C1L_Matches(FILE *file){return c1log.active&&file==c1log.file;}
int C1L_Attach(FILE *file){
 if(!file||!C1J_Enabled())return 0;
 if(C1L_Matches(file))return 1;
 if(c1log.active)return 0;
 if(C1L_FLUSH(file)!=0)return 0;
 C1L_Init();c1log.bytes=(unsigned char*)malloc(C1L_CAPACITY);if(!c1log.bytes)return 0;
 c1log.wake=CreateEventA(NULL,FALSE,FALSE,NULL);c1log.progress=CreateEventA(NULL,TRUE,FALSE,NULL);
 if(!c1log.wake||!c1log.progress)goto fail;
 c1log.head=c1log.used=0;c1log.queued=c1log.written=c1log.requested=c1log.flushed=c1log.backpressure=0;
 c1log.file=file;c1log.error=0;c1log.stop=0;
 c1log.thread=(HANDLE)_beginthreadex(NULL,128*1024,C1L_Worker,NULL,0,NULL);if(!c1log.thread)goto fail;
 c1log.active=1;return 1;
 fail:if(c1log.wake)CloseHandle(c1log.wake);if(c1log.progress)CloseHandle(c1log.progress);free(c1log.bytes);c1log.bytes=NULL;c1log.wake=c1log.progress=NULL;c1log.file=NULL;return 0;
}
int C1L_Write(FILE *file,const void *bytes,size_t count){const unsigned char *p=(const unsigned char*)bytes;size_t left=count;
 if(!C1L_Matches(file))return 0;
 EnterCriticalSection(&c1log.producer);
 while(left){size_t room,tail,n;
  EnterCriticalSection(&c1log.lock);
  if(c1log.error){LeaveCriticalSection(&c1log.lock);break;}
  room=C1L_CAPACITY-c1log.used;
  if(!room){c1log.backpressure++;ResetEvent(c1log.progress);LeaveCriticalSection(&c1log.lock);WaitForSingleObject(c1log.progress,INFINITE);continue;}
  tail=(c1log.head+c1log.used)%C1L_CAPACITY;n=left;if(n>room)n=room;if(n>C1L_CAPACITY-tail)n=C1L_CAPACITY-tail;
  memcpy(c1log.bytes+tail,p,n);c1log.used+=n;c1log.queued+=n;left-=n;p+=n;
  LeaveCriticalSection(&c1log.lock);SetEvent(c1log.wake);
 }LeaveCriticalSection(&c1log.producer);return left==0;
}
int C1L_Flush(FILE *file,int wait){uint64_t target;int ok;
 if(!C1L_Matches(file))return 0;
 EnterCriticalSection(&c1log.producer);EnterCriticalSection(&c1log.lock);
 target=c1log.queued;c1log.requested=target;SetEvent(c1log.wake);
 while(wait&&c1log.flushed<target&&!c1log.error){
  ResetEvent(c1log.progress);LeaveCriticalSection(&c1log.lock);WaitForSingleObject(c1log.progress,INFINITE);EnterCriticalSection(&c1log.lock);
 }
 ok=!c1log.error;LeaveCriticalSection(&c1log.lock);LeaveCriticalSection(&c1log.producer);return ok;
}
int C1L_Detach(FILE *file){int ok;
 if(!c1log.active|| (file&&file!=c1log.file))return 1;
 ok=C1L_Flush(c1log.file,1);
 EnterCriticalSection(&c1log.lock);c1log.stop=1;LeaveCriticalSection(&c1log.lock);SetEvent(c1log.wake);
 WaitForSingleObject(c1log.thread,INFINITE);CloseHandle(c1log.thread);CloseHandle(c1log.wake);CloseHandle(c1log.progress);
 c1log.active=0;c1log.file=NULL;free(c1log.bytes);c1log.bytes=NULL;c1log.wake=c1log.progress=c1log.thread=NULL;return ok;
}
void C1L_Status(c1lStatus *out){memset(out,0,sizeof(*out));if(!c1log.initialized)return;
 EnterCriticalSection(&c1log.lock);out->queued=c1log.queued;out->written=c1log.written;out->flushed=c1log.flushed;out->backpressure=c1log.backpressure;out->pending=(unsigned)c1log.used;out->active=c1log.active;out->error=c1log.error;LeaveCriticalSection(&c1log.lock);
}
