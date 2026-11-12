/* R196 opt-in scene request once per CGAME initialization. The server owns
 * placement; this does not construct an AI or run the original mission. */
static int cgFoleySceneStarted,cgFoleySceneTime,cgFoleySceneRequested;
void CG_Cod1FoleySceneReset(void) {
 cgFoleySceneStarted=cgFoleySceneTime=cgFoleySceneRequested=0;
}
static void CG_Cod1FoleySceneFrame(void) {
 if(cgFoleySceneRequested||!cg_r196FoleyScene.integer||!cgs.cod1MapEntities||
    cgs.gametype!=GT_SINGLE_PLAYER||!cg.snap||cg.demoPlayback||
    cg.snap->ps.pm_type!=PM_NORMAL||cg.snap->ps.stats[STAT_HEALTH]<=0)return;
 if(!cgFoleySceneStarted){cgFoleySceneStarted=1;cgFoleySceneTime=cg.time;return;}
 if(cg.time<cgFoleySceneTime){cgFoleySceneTime=cg.time;return;}
 if((unsigned)cg.time-(unsigned)cgFoleySceneTime<1500u)return;
 cgFoleySceneRequested=1;
 trap_SendConsoleCommand("cod1_actor_preview 0\n");
 trap_SendClientCommand(cg_r196FoleyScene.integer==2?"cod1_actor_server gesture 140":"cod1_actor_server foley 140");
 CG_Printf("[R197] FOLEY_SCENE_REQUESTED mode=%d; F6 restarts, F7 removes, F8 breathes\n",cg_r196FoleyScene.integer);
}
