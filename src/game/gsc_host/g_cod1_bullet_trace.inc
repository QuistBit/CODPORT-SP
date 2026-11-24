#include "../bg_cod1_bullet_trace14.h"
#include "../bg_cod1_impact.h"
static int gscBulletTraceBusy,gscBulletTraceCancelled;
static void GscBulletTraceReset(void){if(gscBulletTraceBusy)gscBulletTraceCancelled=1;}
static int GscBulletTracePrivate(const void*p,size_t n){return C1V14_Overlap(p,n,&gscBulletTraceBusy,sizeof(gscBulletTraceBusy))||C1V14_Overlap(p,n,&gscBulletTraceCancelled,sizeof(gscBulletTraceCancelled))||GscMapPosePrivate(p,n);}
static int GscBulletTraceQuery(void*user,const GscBulletTraceRequest*input,GscBulletTraceReply*out,char*error,size_t cap,int(*current)(void*),void*ctx){
 GscBulletTraceRequest q;GscBulletTraceReply result;cod1SpatialTraceQuery_t traceQuery,expected;trace_t trace;GscSession*session;uint64_t epoch;int time,status,bits,ok=0;
 (void)user;if(gscBulletTraceBusy){gscBulletTraceCancelled=1;return 0;}
 if(!input||!out||!current||(cap&&!error)||GscBulletTracePrivate(input,sizeof(*input))||GscBulletTracePrivate(out,sizeof(*out))||GscBulletTracePrivate(error,cap)||C1V14_Overlap(input,sizeof(*input),out,sizeof(*out))||C1V14_Overlap(input,sizeof(*input),error,cap)||C1V14_Overlap(out,sizeof(*out),error,cap))return 0;
 q=*input;if(!q.program||q.now_ms<0||(q.actors!=0&&q.actors!=1)||!C1BT14_Vector(q.start)||!C1BT14_Vector(q.end))return 0;
 epoch=G_Cod1EntityMapEpoch();session=gscTestSession;time=level.time;gscBulletTraceBusy=1;gscBulletTraceCancelled=0;
#define BULLET_TRACE_CURRENT (gscBulletTraceBusy&&!gscBulletTraceCancelled&&session&&session==gscTestSession&&epoch==gscTestMapEpoch&&GscVehicleMapCurrent(epoch)&&level.time==time&&!memcmp(input,&q,sizeof(q)))
 if(!BULLET_TRACE_CURRENT||current(ctx)!=1||!BULLET_TRACE_CURRENT)goto done;
 memset(&traceQuery,0,sizeof(traceQuery));traceQuery.version=COD1_SPATIAL_VERSION;traceQuery.bytes=sizeof(traceQuery);memcpy(traceQuery.start,q.start,12);memcpy(traceQuery.end,q.end,12);traceQuery.passEntity[0]=traceQuery.passEntity[1]=ENTITYNUM_NONE;traceQuery.contentsMask=q.actors?0x0280e031u:0x00802031u;expected=traceQuery;
 memset(&trace,0xff,sizeof(trace));status=trap_Cod1SpatialPointTrace(&traceQuery,&trace);
 if(!BULLET_TRACE_CURRENT||memcmp(&traceQuery,&expected,sizeof(expected))||status!=COD1_SPATIAL_OK||current(ctx)!=1||!BULLET_TRACE_CURRENT)goto done;
 /* Never turn an unsupported live entity result into a world hit. */
 if((trace.entityNum!=ENTITYNUM_NONE&&trace.entityNum!=ENTITYNUM_WORLD)||!isfinite(trace.fraction)||trace.fraction<0||trace.fraction>1||!C1BT14_Vector(trace.endpos))goto done;
 memset(&result,0,sizeof(result));result.fraction=trace.fraction;memcpy(result.position,trace.endpos,12);result.entity_kind=trace.entityNum==ENTITYNUM_WORLD;
 if(trace.fraction<1){bits=CI_TraceBits(&trace);if(!(bits&CI_KNOWN)||!C1BT14_Vector(trace.plane.normal))goto done;memcpy(result.normal,trace.plane.normal,12);result.surface=(bits&31)<23?(bits&31):0;}
 else{if(result.entity_kind||!C1BT14_MissNormal(q.start,q.end,result.normal))goto done;result.surface=23;}
 if(!BULLET_TRACE_CURRENT||current(ctx)!=1||!BULLET_TRACE_CURRENT)goto done;
 *out=result;ok=1;
done:
#undef BULLET_TRACE_CURRENT
 gscBulletTraceBusy=0;if(!ok)GscTestError(error,cap,"bulletTrace world/miss query unavailable or retired");return ok;
}
static const GscBulletTraceHost gscBulletTraceHost={NULL,GscBulletTraceQuery};
