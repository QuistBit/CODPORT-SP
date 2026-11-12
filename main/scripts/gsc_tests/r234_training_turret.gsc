#using_animtree ("generic_human");
// Placed actor and map initialization for the original Training mission tasks.
// This entry itself is an authored adapter, not the original training main.
main()
{
    precacheString(&"INTROSCREEN_TRAINING_PLACE");
    precacheString(&"INTROSCREEN_TRAINING_DATE");
    precacheString(&"INTROSCREEN_TRAINING_TIME");
    precacheShader("hudStopwatch");
    precacheShader("hudStopwatchNeedle");
    precacheModel("xmodel/explosivepack");
    character\Foley::precache();
    character\Elder::precache();
    character\Moody_training::precache();
    level.explo_fx = loadfx("fx/explosions/metal_b.efx");
    precacheItem("m1carbine");
    precacheItem("fraggrenade");
    precacheItem("thompson");
    precacheItem("springfield");
    wait .05;
    level.script = "training";
    level.current_spawn_num = 0;
    level.mg42_firing = false;
    turrets = getentarray("misc_mg42", "classname");
    maps\_utility::array_thread(turrets, maps\_spawner::mg42_think);
    thread avery_original_route();
    println("[R234] ORIGINAL_TURRET_BOOT_STARTED_BEFORE_PLAYER");
    players = getentarray("player", "classname");
    while (players.size == 0)
    {
        wait .05;
        players = getentarray("player", "classname");
    }
    if (players.size != 1)
        return;
    level.player = players[0];
    level.script = "training";
    level.flags = [];
    level.flags["signs"] = false;
    level.flags["compass"] = false;
    level.guns = [];
    level.flags["got1carbine"] = false;
    level.flags["got2carbine"] = false;
    level.flags["carbine1"] = false;
    level.flags["carbine2"] = false;
    level.starting_sniper_rifles = getentarray("weapon_springfield", "classname");
    level.starting_smgs = getentarray("weapon_thompson", "classname");
    level.flags["nearsniper"] = false;
    level.flags["gotsniper"] = false;
    level.flags["sniper1"] = false;
    level.flags["sniper2"] = false;
    level.flags["nearsmg"] = false;
    level.flags["gotsmg"] = false;
    level.flags["smg1"] = false;
    level.flags["smg_ADShint"] = false;
    level.flags["switchedweapon"] = false;
    level.flags["smg2"] = false;
    level.starting_grenades = getentarray("weapon_fraggrenade", "classname");
    level.flags["neargrenades"] = false;
    level.flags["gotgrenade"] = false;
    level.flags["threwgrenades"] = false;
    level.flags["athealth"] = false;
    level.flags["atbomb"] = false;
    level.flags["bomb_pickup"] = false;
    level.flags["bombplanted"] = false;
    level.flags["bombexploded"] = false;
    level.failing = false;
    thread maps\training::end_training();
    level.starting_carbines = getentarray("weapon_m1carbine", "classname");
    level.moody = getent("moody", "targetname");
    level.player takeAllWeapons();
    level.scr_anim = [];
if (!isdefined (level.scr_special_notetrack))
		level.scr_special_notetrack = [];
	if (!isdefined (level.scr_notetrack))
		level.scr_notetrack = [];
	if (!isdefined (level.scr_face))
		level.scr_face = [];
	if (!isdefined (level.scrsound))
		level.scrsound = [];
    level.scr_animtree = [];
    maps\training_anim::main();
    level.ambient_track ["exterior"] = "ambient_training";
    thread maps\_utility::set_ambient("exterior");
    maps\training::elder();
    level.course_runners = getentarray("runner", "targetname");
    for (i = 0; i < level.course_runners.size; i++)
        level.course_runners[i] animscripts\init::main();
    instructor=getent("foley","targetname");
    instructor animscripts\init::main();
    instructor=getent("moody","targetname");
    instructor animscripts\init::main();
    thread maps\training::foley();
    thread maps\training::moody();
    while (!isdefined(level.moody.standanim))
        wait .05;
    level.moody thread tower_standing();
    level.mg42_firing = false;
    level.current_spawn_num = 0;

    setCvar("cg_deadscreen_levelname", "levelshots/level_name_text/hud@Camp_Toccoa.tga");
    setCvar("cg_deadscreen_backdrop", "levelshots/deadscreen/defeat_american.tga");
    thread maps\training::start();

    thread maps\training::start_dialog();
    thread maps\training::go_gate();
    thread maps\training::obstacle_master();
    thread moody_arrival();
    thread maps\training::moody_dialog();
    thread maps\training::carbine_master();
    thread maps\training::sniper_rifle_master();
    thread maps\training::smg_master();
    thread maps\training::grenade_master();
    thread maps\training::explosives_master();
    thread breach_collision();
    thread maps\training::give_ammo();
    thread maps\training::jumpers();
    wait .05;
    while (getCvar("cg_norender") != "0")
        wait .05;
    musicplay("training_datestamp");
    maps\_introscreen::introscreen(&"INTROSCREEN_TRAINING_PLACE", &"INTROSCREEN_TRAINING_DATE", &"INTROSCREEN_TRAINING_TIME");
    println("[R230] TRAINING_VOICE_TASKS_RUNNING");
    println("[R222] ORIGINAL_CARBINE_MASTER_RUNNING");
    while (1)
        wait 1;
}

// Observer only; real original reach_goal_notify performs the actual wait.
moody_arrival()
{
    while (1)
    {
        level.moody maps\_anim::reach_goal_notify(level);
        println("[R224] ORIGINAL_MOODY_GOAL_REACHED");
    }
}

// Retained port collision completion from the accepted breach scene. The
// original bomb_think remains whole; its visibility swap is observed first.
breach_collision()
{
    level waittill ("bomb_exploded");
    wait .35;
    whole = getent("wall_whole", "targetname");
    whole notsolid();
    println("[R225] ORIGINAL_FOLLOWUP_BREACH_READY");
}

// Native actor movement consumes this original route's actual setgoalnode;
// only collision-tested arrival produces goal. No direct USE or fake goal.
avery_original_route()
{
    actors = getentarray("actor_ally_airborne_garand", "classname");
    for (i = 0; i < actors.size; i++)
    {
        if (actors[i].target == "auto433")
        {
            actors[i] animscripts\init::main();
            spawnStand = actors[i] isStanceAllowed("stand");
            spawnCrouch = actors[i] isStanceAllowed("crouch");
            spawnProne = actors[i] isStanceAllowed("prone");
            println("[R243] FRESH_SPAWN_STANCES_CAPTURED");
            actors[i] maps\_spawner::go_to_node(getnode("auto433", "targetname"));
            println("[R232] ORIGINAL_AVERY_TURRET_ROUTE_RETURNED");
            return;
        }
    }
}

// The noncombat, explicit-standanim branch of animscripts/stop::StandStill.
// Selection follows the actual controller and goal. Native notetracks drive
// DoNoteTracks; the map's own interruption cancels that gesture waiter.
tower_standing()
{
    self endon("death");
    for (;;)
    {
        if (self.cod1_tower_idle_needed && isdefined(self.standanim) && !self animscripts\utility::IsInCombat())
        {
            self animscripts\utility::initialize("stop");
            self setFlaggedAnimKnobAllRestart("idleanim", self.standanim, %body, 1, .1, 1);
            self thread tower_standing_notes();
        }
        wait .05;
    }
}
tower_standing_notes()
{
    self endon("death");
    self endon("stop doing _anim notetracks");
    self endon("killanimscript");
    for (;;)
    {
        self animscripts\shared::DoNoteTracks("idleanim");
        if (!self.cod1_tower_idle_current || !isdefined(self.standanim) || self animscripts\utility::IsInCombat())
            return;
        self setFlaggedAnimKnobAllRestart("idleanim", self.standanim, %body, 1, .1, 1);
    }
}
