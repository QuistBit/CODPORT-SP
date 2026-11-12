#ifndef BG_COD1_CHARACTER_CONTACT_H
#define BG_COD1_CHARACTER_CONTACT_H
/* PM and prediction clip all dynamic bodies separately. If a trusted actor
 * starts inside the player's hull, a separating horizontal move may leave
 * this one body. Other bodies, world geometry and ordinary contacts retain
 * their original traces. Dot >= 0 makes distance nondecreasing along the
 * complete segment, including the coincident-centres case. */
static int BG_Cod1SeparatingContact(const float *start,const float *end,const float *mins,const float *maxs,
                                    const float *origin,const float *bodyMins,const float *bodyMaxs){
    float dx=end[0]-start[0],dy=end[1]-start[1],rx,ry;
    int i;
    if((dx==0 && dy==0) || !mins || !maxs)return 0;
    for(i=0;i<3;i++){
        if(start[i]+maxs[i]<=origin[i]+bodyMins[i] ||
           start[i]+mins[i]>=origin[i]+bodyMaxs[i])return 0;
    }
    rx=start[0]-origin[0]+.5f*(mins[0]+maxs[0]-bodyMins[0]-bodyMaxs[0]);
    ry=start[1]-origin[1]+.5f*(mins[1]+maxs[1]-bodyMins[1]-bodyMaxs[1]);
    return dx*rx+dy*ry>=0;
}
#endif
