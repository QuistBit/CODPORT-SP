/* R229 original command spellings; strict bounded Training fade parameters. */
static int CG_Cod1MissionUnsigned(const char *s,int limit,int *out){
 int n=0,d;if(!s||!*s||!out)return 0;
 while(*s){d=(unsigned char)*s++-'0';if(d<0||d>9||(n>limit/10||(n==limit/10&&d>limit%10)))return 0;n=n*10+d;}
 *out=n;return 1;
}
static int CG_Cod1MissionServerCommand(const char *cmd){
 if(!strcmp(cmd,"scr_fade")){
  int alpha,start,duration;
  if(trap_Argc()!=4)return 1;
  if(!CG_Cod1MissionUnsigned(CG_Argv(1),1,&alpha)||
     !CG_Cod1MissionUnsigned(CG_Argv(2),2147483647,&start)||
     !CG_Cod1MissionUnsigned(CG_Argv(3),60000,&duration))return 1;
  CG_Fade(0,0,0,alpha*255,start,duration);return 1;
 }
 if(!strcmp(cmd,"opendeadscreen")){
  if(trap_Argc()==1)trap_UI_Popup("deadscreen");return 1;
 }
 if(!strcmp(cmd,"openvictoryscreen")){
  if(trap_Argc()==1)trap_UI_Popup("victoryscreen");return 1;
 }
 return 0;
}
