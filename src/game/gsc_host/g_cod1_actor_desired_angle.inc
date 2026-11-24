/* Native Actor+148 is storage, not a query of the pending AI queue.
 * C490/D710 birth is zero.54DE0..54E0B copies default+12C or override+140.
 * This read never derives a target or changes the orientation state. */
static uint32_t ActorDesiredAngleSelect(uint32_t mode,uint32_t base,uint32_t override){return mode?override:base;}
static void ActorDesiredAngleCommit(cod1ActorRecord_t*a,int known,uint32_t bits){
 float v;memcpy(&v,&bits,4);known=known&&isfinite(v);
 if(known)a->desiredAngle148=bits;a->desiredAngleUnavailable=!known;
}
static int ActorDesiredAngleRead(const cod1ActorRecord_t*a,GscStringCastValue*out){
 GscStringCastValue v;float angle;
 if(!a||!out||a->desiredAngleUnavailable)return 0;
 memcpy(&angle,&a->desiredAngle148,4);if(!isfinite(angle))return 0;
 memset(&v,0,sizeof(v));v.type=4;v.number=angle;*out=v;return 1;
}
