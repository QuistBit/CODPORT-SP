/* CoD1-style input mapped onto RTCW's shared movement implementation. */
#include "../game/bg_cod1_stance.h"
static int cod1Stance, cod1HeldCrouch, cod1HeldProne;
static int cod1StanceMode = 1; /* CoD profile from the first command. */
static int cod1LastSpawn=-1,cod1LastTime=-1;
static void CL_Cod1StanceReset(void){
 if(!cl.snap.valid)return;
 if(cod1LastSpawn>=0 && (cod1LastSpawn!=cl.snap.ps.persistant[PERS_SPAWN_COUNT] || cl.snap.serverTime<cod1LastTime)){cod1Stance=0;cod1HeldCrouch=cod1HeldProne=0;}
 cod1LastSpawn=cl.snap.ps.persistant[PERS_SPAWN_COUNT];cod1LastTime=cl.snap.serverTime;
}
static void IN_Cod1Prone(void){CL_Cod1StanceReset();cod1StanceMode=1;cod1Stance=2;}
static void IN_Cod1Crouch(void){CL_Cod1StanceReset();cod1StanceMode=1;cod1Stance=1;}
static void IN_Cod1Stand(void){CL_Cod1StanceReset();cod1StanceMode=1;cod1Stance=0;}
static void IN_Cod1ToggleProne(void){CL_Cod1StanceReset();cod1StanceMode=1;cod1Stance=cod1Stance==2?0:2;}
static void IN_Cod1ToggleCrouch(void){CL_Cod1StanceReset();cod1StanceMode=1;cod1Stance=cod1Stance==1?0:1;}
static void IN_Cod1ProneDown(void){CL_Cod1StanceReset();cod1StanceMode=1;cod1HeldProne=1;}
static void IN_Cod1ProneUp(void){cod1HeldProne=0;}
static void IN_Cod1CrouchDown(void){CL_Cod1StanceReset();cod1StanceMode=1;cod1HeldCrouch=1;}
static void IN_Cod1CrouchUp(void){cod1HeldCrouch=0;}
static void IN_Cod1StandDown(void){
 int low=cod1Stance || (cl.snap.ps.pm_flags&(PMF_DUCKED|PMF_COD1_PRONE));
 IN_Cod1Stand();
 /* First press while low requests rising. A new press while standing jumps. */
 if(!low)IN_UpDown();
}
static void IN_Cod1StandUp(void){IN_UpUp();}
static void CL_Cod1StanceMove(usercmd_t*cmd){int target;
 if(!cod1StanceMode)return;
 target=cod1HeldProne?2:(cod1HeldCrouch?1:cod1Stance);
 /* Legacy +moveup/+movedown binds remain usable. */
 if(cmd->upmove>0){cod1Stance=0;target=0;}
 else if(cmd->upmove<0)target=1;
 cmd->wbuttons|=WBUTTON_COD1_STANCE;
 if(target==2){cmd->wbuttons|=WBUTTON_COD1_PRONE;cmd->upmove=0;}
 else if(target==1)cmd->upmove=-127;
}
static void CL_Cod1StanceCommands(void){
 Cmd_AddCommand("goprone",IN_Cod1Prone);Cmd_AddCommand("gocrouch",IN_Cod1Crouch);Cmd_AddCommand("gostand",IN_Cod1Stand);
 Cmd_AddCommand("toggleprone",IN_Cod1ToggleProne);Cmd_AddCommand("togglecrouch",IN_Cod1ToggleCrouch);
 Cmd_AddCommand("+prone",IN_Cod1ProneDown);Cmd_AddCommand("-prone",IN_Cod1ProneUp);
 Cmd_AddCommand("+crouch",IN_Cod1CrouchDown);Cmd_AddCommand("-crouch",IN_Cod1CrouchUp);
 Cmd_AddCommand("+gostand",IN_Cod1StandDown);Cmd_AddCommand("-gostand",IN_Cod1StandUp);
 Cmd_AddCommand("+gocrouch",IN_Cod1CrouchDown);Cmd_AddCommand("-gocrouch",IN_Cod1CrouchUp);
}
