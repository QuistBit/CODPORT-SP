#ifndef COD1_ACTOR_LOOK_TREE14_H
#define COD1_ACTOR_LOOK_TREE14_H
#include "cod1_actor_look_tree14_defs.h"
#include "cod1_actor_look_bank14.h"
#include "cod1_actor_sparse_tree14.h"
#include "cod1_actor_dobj_pose14.h"
static int C1ALT14_Nodes(const float current[C1ALT14_NODES],const float mgPhases[2][21],c1ast14Node_t out[C1ALT14_NODES]){c1ast14Node_t staged[C1ALT14_NODES];unsigned i;if(!current||!mgPhases||!out||C1AB14_Overlap(current,C1ALT14_NODES*4,out,sizeof(staged))||C1AB14_Overlap(mgPhases,42*4,out,sizeof(staged))||C1AB14_Overlap(c1alt14Definitions,sizeof(c1alt14Definitions),out,sizeof(staged))||C1AB14_Overlap(c1alt14Leaves,sizeof(c1alt14Leaves),out,sizeof(staged)))return 0;
 for(i=0;i<C1ALT14_NODES;i++){const c1alt14Definition_t*d=c1alt14Definitions+i;if(!(current[i]>=0&&current[i]<=1))return 0;staged[i].ordinal=d->ordinal;staged[i].parent=d->parent;staged[i].leaf=d->leaf;staged[i].currentWeight=current[i];staged[i].phase=0;if(d->leaf>=0){const c1alt14Leaf_t*l=c1alt14Leaves+d->leaf;if(l->mgGroup>=0){float phase=mgPhases[l->mgGroup][l->mgIndex];if(!(phase>=0&&phase<=1))return 0;staged[i].phase=phase;}}}memcpy(out,staged,sizeof(staged));return 1;
}
/* Borrowed immutable clip views. The parent retains both real source banks. */
static int C1ALT14_Bank(const c1ad14Resource_t*r,const c1ab14Clip_t*const mg[2][21],const c1al14Bank_t*look,c1ast14Bank_t*out){c1ast14Bank_t staged;unsigned i,g,j;if(!r||!r->assembly||!r->assembly->count||r->assembly->count>8||!mg||!look||!out||!C1AL14_Ready(look)||C1AB14_Overlap(r,sizeof(*r),out,sizeof(*out))||C1AB14_Overlap(r->assembly,sizeof(*r->assembly),out,sizeof(*out))||C1AB14_Overlap(mg,42*sizeof(*mg[0]),out,sizeof(*out))||C1AB14_Overlap(look,sizeof(*look),out,sizeof(*out)))return 0;
 for(i=0;i<r->assembly->count;i++)if(r->raw[i]&&C1AB14_Overlap(r->raw[i],sizeof(*r->raw[i]),out,sizeof(*out)))return 0;
 for(g=0;g<2;g++)for(j=0;j<21;j++){const c1ab14Clip_t*c=mg[g][j];if(!c||c->magic!=C1AB14_MAGIC||c->flags!=(g?3u:2u)||c->fps!=30||c->intervals!=(g?5u:0u))return 0;}
 memset(&staged,0,sizeof(staged));staged.bones=r->assembly->topology.boneCount;staged.count=C1ALT14_LEAVES;for(i=0;i<C1ALT14_LEAVES;i++){const c1alt14Leaf_t*l=c1alt14Leaves+i;const c1ab14Clip_t*c;unsigned k;staged.clip[i]=c=l->mgGroup>=0?mg[l->mgGroup][l->mgIndex]:look->body[l->lookIndex];if(C1AB14_Overlap(c,sizeof(*c),out,sizeof(*out)))return 0;for(k=0;k<c->bones;k++){const c1ab14Bone_t*t=c->bone+k;if(C1AB14_Overlap(t->qi,t->nq*2,out,sizeof(*out))||C1AB14_Overlap(t->q,t->nq*8,out,sizeof(*out))||C1AB14_Overlap(t->ti,t->nt*2,out,sizeof(*out))||C1AB14_Overlap(t->t,t->nt*12,out,sizeof(*out)))return 0;}if(!C1AD14_Map(r,staged.clip[i],staged.map[i]))return 0;}*out=staged;return 1;
}
#endif
