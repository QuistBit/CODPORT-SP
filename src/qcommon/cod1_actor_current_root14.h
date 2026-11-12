#ifndef COD1_ACTOR_CURRENT_ROOT14_H
#define COD1_ACTOR_CURRENT_ROOT14_H
/* Compact ENGINE111 current-phase/GOAL-weight arithmetic. Pure bounded input
 * graph only; not caller authority over a live actor or resource admission. */
#include "cod1_actor_root14.h"
/* ordinal is the actual native DEFINITION child-table ordinal, NOT source
 * ATR parse order. A live mapper must prove its source-to-definition mapping. */
typedef struct {unsigned ordinal,definition_children;int parent,leaf,runtime_present;float goalWeight,currentWeight,phase;} c1arc14Node_t;
typedef struct {float q2[2],translation[3];} c1arc14Result_t;
static int C1ARC14_Finite(float f){uint32_t u;memcpy(&u,&f,4);return(u&0x7f800000u)!=0x7f800000u;}
static int C1ARC14_Overlap(const void*a,size_t n,const void*b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&n&&m&&(x<=y?y-x<n:x-y<m);}
static int C1ARC14_Leaf(const c1ar14Root_t*r,float phase,float weight,float sum[6]) {
    c1ar14Sample_t sample;uint32_t bits=0x38000100u;float scale;double factor;unsigned j;
    if(!(r->flags&2))return 1;
    if(!C1AR14_SampleInner(r,phase,&sample,1))return 0;
    memcpy(&scale,&bits,4);factor=(double)weight*scale;
    for(j=0;j<2;j++)sum[j]=(float)((double)sample.q2[j]*factor+sum[j]);
    sum[2]=(float)((double)sum[2]+weight);
    for(j=0;j<3;j++)sum[j+3]=(float)((double)sample.translation[j]*weight+sum[j+3]);
    return 1;
}
static int C1ARC14_Recurse(const c1ar14Root_t*const*bank,const c1arc14Node_t*nodes,
    unsigned count,unsigned index,float input,int initialize,int normalize,float out[6],unsigned depth) {
    unsigned i,j,active=0,first=0,last=0,chosen;float local[6]={0},*sum;double norm,factor;
    if(depth>32)return 0;
    if(initialize)memset(out,0,24);
    /* Native composites do not require their OWN runtime slot. The parent
     * selects children by child slots; only a leaf reads its own phase slot. */
    if(!nodes[index].definition_children){
        if(nodes[index].runtime_present==0)return 1;
        if(nodes[index].runtime_present<0||nodes[index].leaf<0)return 0;
        return C1ARC14_Leaf(bank[nodes[index].leaf],nodes[index].phase,input,out);
    }
    for(i=0;i<count;i++)if(nodes[i].parent==(int)index&&nodes[i].goalWeight>0){
        if(nodes[i].runtime_present<0)return 0;
        if(nodes[i].runtime_present){active++;first=i;}
    }
    if(!active)return 1;
    if(active==1)return C1ARC14_Recurse(bank,nodes,count,first,input,initialize,normalize,out,depth+1);
    sum=initialize?out:local;
    for(j=0;j<active;j++){
        chosen=~0u;
        for(i=0;i<count;i++)if(nodes[i].parent==(int)index&&nodes[i].runtime_present&&nodes[i].goalWeight>0&&
            (!j||nodes[i].ordinal>last)&&(chosen==~0u||nodes[i].ordinal<nodes[chosen].ordinal))chosen=i;
        if(chosen==~0u)return 0;
        last=nodes[chosen].ordinal;
        if(!C1ARC14_Recurse(bank,nodes,count,chosen,nodes[chosen].goalWeight,j==0,1,sum,depth+1))return 0;
    }
    if(!normalize){
        if(sum[2]>0){factor=1./sum[2];for(i=0;i<3;i++)sum[i+3]=(float)(sum[i+3]*factor);}
        if(!initialize)memcpy(out,sum,24);
        return 1;
    }
    norm=(double)sum[0]*sum[0]+(double)sum[1]*sum[1];
    if(norm>0){factor=(double)input/sqrt(norm);for(i=0;i<2;i++)out[i]=initialize?(float)(sum[i]*factor):(float)(sum[i]*factor+out[i]);}
    if(sum[2]>0){factor=(double)input/sum[2];out[2]=initialize?input:(float)((double)out[2]+input);
        for(i=0;i<3;i++)out[i+3]=initialize?(float)(sum[i+3]*factor):(float)(sum[i+3]*factor+out[i+3]);}
    return 1;
}
static int C1ARC14_Inner(const c1ar14Root_t*const*bank,unsigned leaves,const c1arc14Node_t*nodes,
    unsigned count,unsigned selected,c1arc14Result_t*out) {
    unsigned i,j,walk;float sum[6]={0};c1arc14Result_t result;
    if((leaves&&!bank)||!nodes||!out||leaves>82||!count||count>128||selected>=count||nodes[0].parent!=-1)return 0;
    for(i=0;i<leaves;i++){c1ar14Sample_t probe;
        if(!bank[i]||!C1AR14_SampleInner(bank[i],0,&probe,1))return 0;}
    for(i=0;i<count;i++){
        const c1arc14Node_t*n=nodes+i;
        if(n->ordinal>=65535||n->parent< -1||n->parent>=(int)count||(!i?n->parent!=-1:n->parent<0)||
           n->leaf< -1||n->leaf>=(int)leaves||(n->runtime_present< -1||n->runtime_present>1)||n->definition_children>=65535||
           !C1ARC14_Finite(n->phase)||n->phase<0||n->phase>1||
           !C1ARC14_Finite(n->goalWeight)||n->goalWeight<0||n->goalWeight>1||
           !C1ARC14_Finite(n->currentWeight)||n->currentWeight<0||n->currentWeight>1)return 0;
        for(j=0;j<i;j++)if(n->ordinal==nodes[j].ordinal)return 0;
        walk=i;for(j=0;walk;j++){if(j>=32||nodes[walk].parent<0)return 0;walk=(unsigned)nodes[walk].parent;}
        {unsigned children=0;for(j=0;j<count;j++)if(nodes[j].parent==(int)i)children++;
         if(children>n->definition_children||(!n->definition_children&&children)||
            (n->definition_children&&n->leaf!=-1))return 0;}
        /* Compact cuts omit only branches PROVED inactive (zero goal or a
         * genuinely absent native slot). A live mapper must never infer
         * absence from zeros and may use the narrower zero-goal cut. It
         * must prove each cut from its owned full tree; metadata is not authority
         * over a live actor. Unknown resource/presence is refused if visited. */
    }
    if(!C1ARC14_Recurse(bank,nodes,count,selected,1,1,0,sum,0))return 0;
    result.q2[0]=sum[0];result.q2[1]=sum[1];
    if(result.q2[0]==0&&result.q2[1]==0)result.q2[1]=1;
    for(i=0;i<3;i++)result.translation[i]=sum[i+3];
    for(i=0;i<5;i++)if(!C1ARC14_Finite(((float*)&result)[i]))return 0;
    memcpy(out,&result,sizeof(result));return 1;
}
static int C1ARC14_Current(const c1ar14Root_t*const*bank,unsigned leaves,const c1arc14Node_t*nodes,
    unsigned count,unsigned selected,c1arc14Result_t*out) {
    c1arc14Result_t result;unsigned i;int ok;
    if((leaves&&!bank)||!nodes||!out||leaves>82||!count||count>128||
       C1ARC14_Overlap(out,sizeof(*out),bank,leaves*sizeof(*bank))||
       C1ARC14_Overlap(out,sizeof(*out),nodes,count*sizeof(*nodes)))return 0;
    for(i=0;i<leaves;i++)if(!bank[i]||C1ARC14_Overlap(out,sizeof(*out),bank[i],sizeof(*bank[i])))return 0;
#if defined(_MSC_VER)&&defined(_M_IX86)
    {unsigned char outer[108];uint32_t mx,nmx=0x1f80;unsigned short cw=0x027f;
    __asm fnsave outer
    __asm stmxcsr mx
    __asm fldcw cw
    __asm ldmxcsr nmx
    ok=C1ARC14_Inner(bank,leaves,nodes,count,selected,&result);
    __asm ldmxcsr mx
    __asm frstor outer
    }
#else
    ok=C1ARC14_Inner(bank,leaves,nodes,count,selected,&result);
#endif
    if(ok)memcpy(out,&result,sizeof(result));return ok;
}
#endif
