#ifndef G_COD1_DYNAMIC_PATH_BIND_H
#define G_COD1_DYNAMIC_PATH_BIND_H
#include "g_cod1_dynamic_path_core.h"
/* DATA binding only, original 6F8F9..6F923 and 6FA10. These projections must
 * eventually come from finalized physical native nodes, not authored BSP order.
 * Copying serialized links and initializing active counts grants no live path
 * or actor authority. Finalized placement/Begin/sort/owner/tree gates remain. */
typedef struct {uint32_t first,count;} C1DPV7Row;
typedef struct {uint16_t target;uint32_t distance_bits;} C1DPV7Link;
typedef struct {uint32_t type;uint16_t targetname,target;float origin[3];} C1DPProjection;
static int C1DP_Overlap(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(!an||!bn)return 0;return x<=y?y-x<an:x-y<bn;}
static int C1DP_BindV7(const C1DPV7Row*rows,uint32_t nn,const C1DPV7Link*data,uint32_t nl,const C1DPProjection*physical,C1DPNode*nodes,size_t nc,C1DPLink*links,size_t lc){
 const void*in[3];size_t sz[3],ns,ls;uint32_t i,j,k=0;
 if(!rows||!physical||!nodes||!nn||nn>2048||nc<nn||lc<nl||(nl&&(!data||!links))||nl>nn*32767u)return 0;
 ns=nn*sizeof(*nodes);ls=nl*sizeof(*links);in[0]=rows;sz[0]=nn*sizeof(*rows);in[1]=data;sz[1]=nl*sizeof(*data);in[2]=physical;sz[2]=nn*sizeof(*physical);
 if(C1DP_Overlap(nodes,ns,links,ls))return 0;
 for(i=0;i<3;i++)if(C1DP_Overlap(nodes,ns,in[i],sz[i])||C1DP_Overlap(links,ls,in[i],sz[i]))return 0;
 for(i=0;i<nn;i++){if(rows[i].first!=k||rows[i].count>32767||rows[i].count>nl-k)return 0;for(j=0;j<rows[i].count;j++)if(data[k+j].target>=nn)return 0;k+=rows[i].count;}
 if(k!=nl)return 0;
 /* All bounded DATA refusal occurs before output stores. No graph owner is
  * inferred from this validation, buffer address or matching static counts. */
 for(i=0;i<nn;i++){nodes[i].first=rows[i].first;nodes[i].active=nodes[i].count=(uint16_t)rows[i].count;memcpy(nodes[i].origin,physical[i].origin,12);
  for(j=0;j<rows[i].count;j++){k=rows[i].first+j;links[k].target=data[k].target;links[k].blockers=0;
   links[k].flags=(uint8_t)(physical[i].type==16&&physical[data[k].target].type==17&&physical[i].target==physical[data[k].target].targetname);
   links[k].payload[0]=0;links[k].payload[1]=data[k].distance_bits;
  }
 }return 1;
}
#endif
