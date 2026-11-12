#ifndef G_COD1_DYNAMIC_PATH_GRAPH_H
#define G_COD1_DYNAMIC_PATH_GRAPH_H
#include <stdlib.h>
#include "g_cod1_dynamic_path_place.h"
#include "g_cod1_node_keys_data.h"
#include "g_cod1_node_tree.h"
#include "g_cod1_node_visibility.h"
#include "g_cod1_node_v7.h"
#include "g_cod1_node_startup.h"
/* Private, unpublished DATA graph. Input records have already passed spawn
 * parsing/type/flag/Begin-cache preparation; this operation cannot certify that
 * provenance. Real map trace context and ownership are separate production
 * requirements. No GAME native or movement capability is enabled.
 * Placement -> exact native keys/sort/remap -> physical-index v7 -> XY tree.
 * Every failure destroys the entire candidate; caller inputs remain untouched. */
typedef struct {
 unsigned count,linkCount;
 unsigned char *records,*authoredToPhysical;
 C1DPProjection *physical;
 C1DPNode *nodes;
 C1DPLink *links;
 C1NTree *tree;
 C1NVisibility *visibility;
 C1DPRecord *blockRecords;
 C1DPOwner *owners;
 unsigned ownerCount;
 C1NSResult startup;
 C1NAV14_KeysResult keys;
} C1DPGraphCandidate;
typedef struct {unsigned stage,index;C1NAV14_KeysResult keys;C1NSResult startup;C1NSInitialResult initial;int transitionStatus;} C1DPGraphFailure;
static void C1DP_GraphDestroy(C1DPGraphCandidate*g){
 if(!g)return;C1NV_Free(g->visibility);C1NTreeFree(g->tree);free(g->owners);free(g->blockRecords);free(g->records);free(g->authoredToPhysical);free(g->physical);free(g->nodes);free(g->links);free(g);
}
static C1DPGraphCandidate*C1DP_GraphBuild(const unsigned char*prepared,size_t bytes,unsigned count,
 const C1DPV7Row*rows,const C1DPV7Link*data,unsigned linkCount,
 C1DPPlaceQuery trace,void*context,C1DPGraphFailure*failure){
 C1DPGraphCandidate*g;unsigned i,why,k=0;size_t need=(size_t)count*116;C1DPProjection p;
 if(!failure)return NULL;memset(failure,0,sizeof(*failure));failure->stage=1;
 if(!prepared||!trace||!rows||!count||count>2048||bytes<need||(linkCount&&!data)||linkCount>count*32767u)return NULL;
 for(i=0;i<count;i++){
  if(rows[i].first!=k||rows[i].count>32767||rows[i].count>linkCount-k)return NULL;k+=rows[i].count;
 }
 if(k!=linkCount)return NULL;for(i=0;i<linkCount;i++)if(data[i].target>=count)return NULL;
 /* Fresh keys and authored permutation required before invoking any trace. */
 {unsigned char seen[2048]={0};for(i=0;i<count;i++){
  const unsigned char*r=prepared+(size_t)i*116;unsigned id=C1NAV14_SortU16(r+0x28);
  if(id>=count||seen[id]||C1NAV14_SortU16(r+0x50)||C1NAV14_SortU16(r+0x52)||C1NAV14_SortU16(r+0x54))return NULL;seen[id]=1;
 }}
 failure->stage=2;g=(C1DPGraphCandidate*)calloc(1,sizeof(*g));if(!g)return NULL;g->count=count;g->linkCount=linkCount;
 g->records=(unsigned char*)malloc(need);g->authoredToPhysical=(unsigned char*)malloc((size_t)count*2);
 g->physical=(C1DPProjection*)calloc(count,sizeof(*g->physical));g->nodes=(C1DPNode*)calloc(count,sizeof(*g->nodes));
 g->links=linkCount?(C1DPLink*)calloc(linkCount,sizeof(*g->links)):NULL;
 if(!g->records||!g->authoredToPhysical||!g->physical||!g->nodes||(linkCount&&!g->links))goto fail;
 memcpy(g->records,prepared,need);failure->stage=3;
 for(i=0;i<count;i++){
  unsigned char*r=g->records+(size_t)i*116;failure->index=i;memset(&p,0,sizeof(p));
  p.type=C1NAV14_SortU32(r+0x24);memcpy(p.origin,r+0x38,12);
  if(!C1DP_PlaceCandidate(&p,trace,context,&why))goto fail;
  memcpy(r+0x24,&p.type,4);memcpy(r+0x38,p.origin,12);
 }
 failure->stage=4;failure->index=0;
 if(!C1NAV14_NodeKeys(g->records,need,count,g->authoredToPhysical,(size_t)count*2,0,&g->keys)){failure->keys=g->keys;goto fail;}
 for(i=0;i<count;i++){
  const unsigned char*r=g->records+(size_t)i*116;g->physical[i].type=C1NAV14_SortU32(r+0x24);
  g->physical[i].targetname=(uint16_t)C1NAV14_SortU16(r+0x2c);g->physical[i].target=(uint16_t)C1NAV14_SortU16(r+0x30);memcpy(g->physical[i].origin,r+0x38,12);
 }
 failure->stage=5;if(!C1DP_BindV7(rows,count,data,linkCount,g->physical,g->nodes,count,g->links,linkCount))goto fail;
 failure->stage=6;g->tree=C1NTreeBuild(g->records,need,count);if(!g->tree)goto fail;
 /* Native6FA70 initializes the2048-record free ring before6FB20 neighbors.
  * This private pool has no entity owners yet; it grants no runtime authority. */
 failure->stage=9;g->blockRecords=(C1DPRecord*)calloc(C1NS_POOL_COUNT,sizeof(*g->blockRecords));
 if(!g->blockRecords||!C1NS_PoolInit(g->blockRecords,C1NS_POOL_COUNT))goto fail;
 failure->stage=10;
 if(!C1NS_NodeNeighbors(g->records,need,count,&g->startup))goto fail;
 failure->startup=g->startup;if(g->startup.duplicate||g->startup.excess)goto fail;
 memset(failure,0,sizeof(*failure));return g;
fail:C1DP_GraphDestroy(g);return NULL;
}
/* Required-query variant for a complete private graph DATA candidate. Failure
 * of visibility acquisition destroys placement, links and tree together. */
static C1DPGraphCandidate*C1DP_GraphBuildVisible(const unsigned char*prepared,size_t bytes,unsigned count,
 const C1DPV7Row*rows,const C1DPV7Link*data,unsigned linkCount,C1DPPlaceQuery trace,void*traceContext,
 C1NVQuery visible,void*visibilityContext,C1DPGraphFailure*failure){
 C1DPGraphCandidate*g;if(!failure)return NULL;
 if(!visible){memset(failure,0,sizeof(*failure));failure->stage=7;return NULL;}
 g=C1DP_GraphBuild(prepared,bytes,count,rows,data,linkCount,trace,traceContext,failure);if(!g)return NULL;
 g->visibility=C1NV_Build(g->records,(size_t)count*116,g->nodes,count,g->links,linkCount,visible,visibilityContext);
 if(!g->visibility){failure->stage=7;C1DP_GraphDestroy(g);return NULL;}return g;
}
/* Original v7 loader supplies serialized visibility directly; do not rebuild
 * it with unrelated traces. Byte decoding is previously proven DATA code.
 * Count equality alone is not evidence of native physical node identity. */
static C1DPGraphCandidate*C1DP_GraphLoadV7(const unsigned char*prepared,size_t bytes,unsigned count,
 const unsigned char*lump,size_t lumpBytes,C1DPPlaceQuery trace,void*ctx,C1DPGraphFailure*failure){
 C1NAV14_V7Layout layout;C1NAV14_V7NodeData*decodedRows=NULL;C1NAV14_V7LinkData*decodedLinks=NULL;
 C1DPV7Row*rows=NULL;C1DPV7Link*links=NULL;C1NVisibility*v=NULL;C1DPGraphCandidate*g=NULL;unsigned i;
 if(!failure)return NULL;memset(failure,0,sizeof(*failure));failure->stage=8;
 layout=C1NAV14_V7Inspect(lump,lumpBytes,count);if(!layout.valid||!trace||!prepared)return NULL;
 decodedRows=calloc(count,sizeof(*decodedRows));decodedLinks=calloc(layout.links?layout.links:1,sizeof(*decodedLinks));
 rows=calloc(count,sizeof(*rows));links=calloc(layout.links?layout.links:1,sizeof(*links));v=calloc(1,sizeof(*v));
 if(!decodedRows||!decodedLinks||!rows||!links||!v)goto done;
 v->count=count;v->bytes=(unsigned)layout.visibilityBytes;v->bits=calloc(v->bytes?v->bytes:1,1);if(!v->bits)goto done;
 if(!C1NAV14_V7Decode(lump,lumpBytes,count,decodedRows,count,decodedLinks,layout.links,v->bits,v->bytes))goto done;
 for(i=0;i<count;i++){rows[i].first=decodedRows[i].first;rows[i].count=decodedRows[i].count;}
 for(i=0;i<layout.links;i++){links[i].target=decodedLinks[i].target;links[i].distance_bits=decodedLinks[i].distanceBits;}
 g=C1DP_GraphBuild(prepared,bytes,count,rows,links,layout.links,trace,ctx,failure);
 if(g){g->visibility=v;v=NULL;}
done:free(decodedRows);free(decodedLinks);free(rows);free(links);C1NV_Free(v);return g;
}
typedef struct {
 const C1DPOwner *owners;
 const unsigned char *active;
 unsigned count;
 int32_t time;
 C1DPTrace trace;
 void *context;
} C1DPInitialEntities;
/* Optional later6FAD0 startup phase over a supplied immutable DATA snapshot.
 * This is NOT evidence of a complete/current GAME entity table. The future
 * live owner must obtain flags from real spawn registration, supply genuine
 * one-entity traces and validate all allocation/map lifetimes around use.
 * Caller snapshots are not mutated. Query/allocation failure destroys the
 * whole unpublished graph including its partially initialized block pool.
 * Existing heads are refused: they cannot belong to this new free pool. */
static C1DPGraphCandidate*C1DP_GraphLoadV7Entities(const unsigned char*prepared,size_t bytes,unsigned count,
 const unsigned char*lump,size_t lumpBytes,C1DPPlaceQuery trace,void*ctx,
 const C1DPInitialEntities*initial,C1DPGraphFailure*failure){
 C1DPGraphCandidate*g=NULL;C1DPInitialEntities input;C1DPOwner*owners=NULL;unsigned char*active=NULL;C1DPState state;unsigned i;
 if(!failure)return NULL;memset(failure,0,sizeof(*failure));failure->stage=11;
 if(!initial)return NULL;input=*initial;
 if(!input.owners||!input.active||!input.count||input.count>2048||!input.trace)return NULL;
 for(i=0;i<input.count;i++)if(input.owners[i].entity!=i||input.owners[i].head)return NULL;
 owners=(C1DPOwner*)malloc((size_t)input.count*sizeof(*owners));active=(unsigned char*)malloc(input.count);
 if(!owners||!active)goto fail;
 memcpy(owners,input.owners,(size_t)input.count*sizeof(*owners));memcpy(active,input.active,input.count);
 g=C1DP_GraphLoadV7(prepared,bytes,count,lump,lumpBytes,trace,ctx,failure);if(!g)goto fail;
 g->owners=owners;owners=NULL;g->ownerCount=input.count;
 state.nodes=g->nodes;state.links=g->links;state.records=g->blockRecords;state.owners=g->owners;
 state.node_count=g->count;state.link_count=g->linkCount;state.record_count=C1NS_POOL_COUNT;state.owner_count=g->ownerCount;
 failure->stage=12;failure->transitionStatus=C1NS_InitialDisconnect(&state,active,input.count,input.time,input.trace,input.context,&failure->initial);
 if(failure->transitionStatus!=C1DP_OK)goto fail;
 free(active);memset(failure,0,sizeof(*failure));return g;
fail:free(active);free(owners);C1DP_GraphDestroy(g);return NULL;
}
#endif
