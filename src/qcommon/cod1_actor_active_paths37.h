#ifndef COD1_ACTOR_ACTIVE_PATHS37_H
#define COD1_ACTOR_ACTIVE_PATHS37_H
/* Requires a C1AGT14_Valid tree. The mask is private to one clear transaction.
 * A node is active iff it lies on a nonzero-current path from the root to
 * a positive-current leaf. Clearing an inactive node cannot break any such
 * path; clearing an active node does not change its currentWeight. Thus the
 * mask remains exact during the original parent-before-child ClearGoal pass.
 * Do not use this proof for SetGoal, advancement, or across transactions. */
static void C1AGT37_ActivePaths(const c1agt14Definition_t*d,const c1atl14Node_t*n,
 unsigned count,unsigned char*active){
 unsigned i,p;
 memset(active,0,count);
 if(n[0].currentWeight==0)return;
 for(i=0;i<count;i++){
  if(d[i].children||!(n[i].currentWeight>0))continue;
  /* Stop at an already proven ancestor or the root; never mark an
   * incomplete path (a zero-weight ancestor can invalidate the whole leaf). */
  for(p=i;p&&!active[p]&&n[p].currentWeight!=0;p=d[p].parent){}
  if(p&&!active[p])continue;
  for(p=i;!active[p];p=d[p].parent){active[p]=1;if(!p)break;}
 }
}
#endif
