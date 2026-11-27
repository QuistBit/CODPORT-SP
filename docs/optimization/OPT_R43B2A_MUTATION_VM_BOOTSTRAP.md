# OPT R43B2A — direct mutation, VM ownership and early campaign admission

Base R43B1: 49df6192194fcced52418de6b7f754ec4b3e5d5a. Comparison from R40
8f92680c078b717f61aa762ef9a5c93e10eb99cb: two forward commits,
zero behind, only OPT_R43A and OPT_R43B1 documents. Read both and R39.
This is a bounded follow-up audit; production executable reference remains
R40. No source edits, native build, test harness, sanitizer or timing run.

## 1. Additional mutation paths and the revision-only counterexample

Paths and line ranges below are relative to src/ at R43B1 (same source as R40).
R43A and B1 already cover the large GenericTransactionCore, FaceFrame,
controller expected-state and generic mounted-commit operations.

| Path / entry | Writes / authority | Implication |
| --- | --- | --- |
| game/gsc_host/g_cod1_actor_generic_host.inc:289–304 ActorDismountTransfer | Read actual mounted 2x25 trees, validate exact lease, no generic busy, revision room; directly overwrite live node rows, detachedMG, mgTick, revision. | Not an ownership swap. Include in dirty tracking; preserve exact node ordinal mapping and turret clock ownership. |
| same:306–314 ActorDismountRestore | Read detached generic state and fill turret actorTree through ActorDismountTrees; runtime/catalog/lease check. | Separate mutable destination, not a generic commit or a new clock tick. |
| game/gsc_host/g_cod1_actor_scripted_prefix.inc:202–245 | Clone owned generic state, retain original pointer identities and exact baseline, stage previous clip cancellation, admit queue push, then memcpy nodes/notify/initialized names into existing live buffers and increment revision; publish prefix/admitted/aux revision. | In-place commit with stable addresses. Cannot detect this via pointer changes; queue admission is an effect boundary. No fabricated finish, rollback or readiness. |
| game/gsc_host/g_cod1_actor_named_notify.inc:47–66 G_Cod1ActorGenericNotifyDrain | Holds generic busy and identity/revision/epoch fence. Delivers packet in ascending ordinal; after callback and packet fence and accepted==1, clears live pending and pendingTick, then checks current again. | **Successful drain mutates live notify state without incrementing generic revision.** A later refusal may follow an accepted delivery. Neither pointer equality nor generic revision is a full mutation certificate. Do not copy FaceFrame poisoning rules into this distinct legacy path without proof. |
| same:5–19,23–42 | CancelSubtree clears flag/token/cursor/name/generation/pending; NamedLeafStage cleans tokenless sidecars, sets name and new generation; NamedAdvance advances node and flag, records pending and tick. | These are staged helpers; not independent live mutation permits. Preserve all inactive and pending fields, including name prefix changes. |
| game/gsc_host/g_cod1_actor_look_host.inc:24–39,115–150,196–211 | A tag query can first advance generic frame to real now; look command uses GenericTransaction; look clock updates separately, including inactive zero-yaw fast path. Orientation and prone frame precede look. | A nominal read may trigger a real writer; cannot give it a blanket read-only capability or schedule all queries after animation. Same-tick no double advance. |
| game/gsc_host/g_cod1_actor_locomotion_select.inc:38–78 | Controller stage consumes state queue; on source change service phase0, publish actor and rebase before, service phase1, final source/selection revision and actor commit. | Intermediate actor publication is visible to callbacks, cannot promise one all-or-nothing transaction across phase0/1. This changes generic clock authority even without writing node bytes. |
| game/gsc_host/g_cod1_actor_scripted_record.inc:90–159,198–251 | Body look/reset/select stage then FaceFrame commit, normal record/actor fields; provider gate and child frame permit on update. | Preserve auxiliary record presence, pointer owner and controller expected-poststate, not merely node values. |
| game/gsc_host/g_cod1_entity_delete_runtime_actor.inc:159–178 | For ordinary lane, after current and actor/entity byte comparison retire generic and turret before motion quiesce; other lanes have explicit deletion/topology paths. | Generic state can retire while actor/entity allocation still exists and before later operation success. Actor lease alone is not generic lifetime proof. |

The counterexample to revision-only fences is a static source proof:
at named_notify.inc:52 revision is captured, at :59 owner and packet
are revalidated, :61 writes two live fields, :62 calls current and checks
the ORIGINAL revision. No revision increment on this route. This is not
a reported corruption or measured performance issue. Do not "fix" by
adding a revision bump alone: the old owner checks expect that revision,
and changing it could change callback admission and acceptance.
R45 must explicitly model effect consumption, including legacy drain,
separately from ordinary transaction revision. Compare exact state
after callback refusal as well as after normal commit.

## 2. Catalog lifetime and relevant reader boundary inventory

generic_host.inc:82–84 declares static per-actor records and catalog
definitions/names. :191–228 ActorGenericBuild allocates staged definitions,
names and default nodes; parses ATR, verifies ordinal coverage, parent
chains/depth and flags, validates tree, rechecks epoch/receipt/cancel,
then copies both arrays and publishes epoch/receipt. Fast path returns
only when same epoch/receipt and animation banks current.
:126–149 private-storage guards cover both arrays; :151–167
ActorGenericRetire releases matching actor only, while ActorGenericReset
first resets locomotion and face, cancels generic operation, frees
records and pool, zeros both catalog arrays and owner markers and retires
look resources. turret_host.inc:39 calls reset as part of broader
turret subsystem teardown. No assumption that an actor retirement
invalidates a shared catalog; no assumption that a non-null static array
pointer implies a live catalog.

Relevant consumer groups inspected at the pinned revision:
- generic_host.inc:243–260,340–601,644–822: ordinal/name, MG mapping,
  mutation kernels and projected generic/normal/legacy reads.
- face_frame.inc:54–77,131,206–230,325–356: exact topology proof cache
  (copy and compare definitions), closures, weight step and event clock.
  Do not simply make the proof cache accept any matching pointer.
- current_root.inc:17–40,45–64,97–149: native-order construction,
  resource lookup, root traversal under snapshot. GeneralRootInterval
  (general_root_interval.inc:19–31) uses its own working/bank/projection.
- locomotion_resources.inc:72–170,173–252: catalog checks and resource
  bank-owned topology/metadata, generic clock and flagged mutation.
  A resource bank can be retired independently of the catalog.
- scripted_frame.inc:12–21,52–82,101–123,209–213:
  source maps, frame/pose samples and scripted mutable helpers.
- scripted_prefix.inc:116–127,206–233 retains raw original addresses
  as identity evidence AND cloned bytes; record.inc:83–121,237
  operates on a FaceFrame stage; controller.inc:712 expected copy.
- face_host.inc:54–79 and unmounted_tags.inc:28 sample state, including
  detached mounted overlays. Their header and body checks differ from
  FaceFrame's full baseline.
- face_tags.inc:99–124,177–197 copies header and dense nodes, then
  checks actual actor/entity/header/node bytes before and after each
  foreign current call and VFS operation; source_tags.inc:4–21
  borrows clip from map bank, builds bone map, checks source revision,
  collects from captured nodes and applies prone packet.
- source_pose.inc:2–27 uses readonly FaceFrameAcquire (baseline copy,
  no writable stage), actual tick/closure, canonical packet resource
  names, jumper resource revision and final callback/Fence; :33–60
  is separate source-pose selection.
- unmounted_publisher.inc:22–27,109–145,177–230 snapshots actor/body
  and generic header. It makes real configstring calls under fences;
  captures publication and samples pose, while world translation remains
  mover-owned. Face tag, snapshot and renderer data are not an alias
  of the server AnimTree. Preserve distinct side effects/ordering.

R44 first prepared topology boundary: keep it a catalog-owned,
callback-free derived representation of validated definition data,
with matching map epoch AND receipt, protected nonoverlapping extents,
publish with catalog, invalidate before reset. Optional acceleration
must not add a failure to a formerly valid catalog. No per-actor
topology copies; no borrowed view across arbitrary current callbacks.
Do not promote this selected-reader inventory into a proof that every
include, alias or callback in the full translation unit was audited.
For the first pure R44 extraction, pass explicit input definitions
instead of global reads, compare original output/failure exactly;
preserve head-insert sibling order/native child blocks and forward-parent
behavior of the separate public generic API. Before publishing a live
prepared cache, audit all storage-protection/caller/reset edges (B2B).
Mutable SoA, active lists and pointer replacement remain later gates.

## 3. VM/Session capabilities are distinct from map and actor lifetime

game/cod1_gsc/gsc_vm_r243_time_root.inc:31–55,97–138:
Registry membership is established by pointer-value lookup, then
physical Runtime/program/incarnation binding, selected lane and
not-closing checks. Required is sticky if registry entry, selected lane,
nonempty root or teardown; never downgrade to inline legacy fields on
invalid membership. Root constructed only after genuine first zero-time
bootstrap preflight, before Tasks/bootstrap_started. :144–197 destruction
refuses active controller/source scopes, marks closing, releases MAIN
retained refs while Task bank exists (membership before dereference),
zeros receipts even on invalid rows, then removes actual registered root
after Task bank free. Never dereference/free only the borrowed root field.

R39 gsc_vm_r237_queue_admission.inc:1–39 already uses a callback-free
scalar view, keeps original final claim and existing-key semantics.
Its differential fixture results are historical; no rerun. Do not
generalize this view across callbacks or task dispatch. Rechecking root
once per pure operation is potentially safe; caching Task membership
or time-root pointer across a wake/VM callback is not.

gsc_vm_r243_controller_driver.inc:54–78,150–166,235–285,288–353:
Controller current is a multi-layered authority check, not pure duplicate
arithmetic. Captures Runtime, catalog, time-root, generation, host table,
input and animation map; rejects reentry, compares retained capture
around animation/resource/entity/map callbacks. Phase6 publishes temporary
source service lease/API/step ranges ONLY during actual host animation_step
then clears them. MAIN replacement phase0 and phase1 have strict sequence;
phase1 issues killanimscript at current bucket before decrementing the
single retained ref. Endon retirement preserves inert DEAD-task queue
metadata; ordinary waiter must become READY/enqueued at same time-root.
The controller prefix admits source selection, obtains actual returned
MAIN task receipt, performs normal update, then publishes frontier.
Failure after an effect does not authorize replay.

gsc_session_r243_controller.inc:25–57,134–164:
Session translates host tick, copies original and forwarded requests
and service tables, checks both, and installs scoped source facade
only around host step. Current checks resource certificates and
turret-animation authority before/after callback; reentry cancels.
Do not conflate logical VM bucket and host GAME time or treat wrapper
copies as unneeded without preserving observable caller-buffer checks.

gsc_vm_r243_source_dispatch.inc:18–69:
Source animation receipt is a one-shot protocol: phase1 offer -> take
phase2 -> exact claim phase3 -> end accepted and still current. Binds
active step, executing task/serial, budget, entity, generation, exact
request pointer/bytes, callback/function context, runtime/catalog.
Animation and look-source dispatch exclude one another; failure cancels.
No generic busy boolean grants this capability. Retain protocol at R47.

R47 measurement: distinguish pure predicate calls/bytes from external
current callbacks (count by layer and actual inclusive/exclusive timing),
queue/root lookup counts, MAIN replacement and accepted event/receipt
counts. Establish which adjacent checks truly share callback-free input
before reducing them. Use R39-style scalar differential tests only on
pure segments; integration tests need reentry, owner retirement, task
reuse, source receipt duplication, wrong time-root, closing and
killanimscript endon vs ordinary waiter. Do not reduce VM correctness
checks to hit rates. Full runtime allocator and all task-directory
mutation/overlap inventory remains a separate R47 prerequisite.

## 4. CFG-free startup: first concrete lifetime dependency (R47)

main/training_player.cfg:11–13 sets vm_game/cgame/ui to 0; :18–43
selects experimental GSC mode 43, explicit maps/training.gsc and
individual capabilities. Module-loader path is distinct from map
admission and needs independent audit, not a mass change of VM defaults.

game/g_spawn.c:1140–1184 actual ordering:
G_Cod1GscCampaignMapProfile -> MapSpawnBegin/SpawnerMapBegin ->
node/vehicle/AI/actor prefix begin -> level.spawning=true ->
ActorPrefixPrepass -> G_Cod1GscTestPrepareBeforeBsp -> worldspawn and
entity replay -> placed currentstate resource preparation ->
level.spawning=false -> registry/prefix finish. The early program
must be selected BEFORE prepass, not merely at first GAME frame.

game/gsc_host/g_cod1_gsc_test.c:1158–1187 registers g_cod1GscTest and
g_cod1GscEntityTest default 0; exits on disabled or nonfresh SP.
:1884–1904 existing campaign profile already runs before prepass,
but requires enabled mode43 and previous path for training -> us_intro;
clears selected capability cvars, sets path and briefing options.
Not a general native campaign selector, does not support clean initial
Training or repeat entry merely by changing default g_cod1GscTest.
:1906–1920 early preparation binds actual actor plan epoch/receipt
after prepass, rejects duplicate attempt and captures session program.
:1925–1952 graph publication before player consumes genuine prepared
Session and exact receipt, failures request cleanup.
game/g_client.c:1776–1784 completed player lifetime and BeginEvent
precede G_Cod1GscTestClientSpawned. Do not collapse into one startup hook.

g_cod1_early_program_handoff.inc:1–45:
There are 24 captured cvar strings, including actual mapname,
savegame_loading and developer_script, not just capability bits.
GscTestEarlyCurrent loops over all 24, rereads via traps, compares strings,
calls owner after each, cancels on reentry/mismatch. This gives a concrete
candidate for R47: replace production feature choices with immutable
typed context owned by real map lifecycle, reducing repeated cvar traffic
by construction. This is not an entitlement to remove current live checks
from the legacy diagnostic mode. :48–71 exact once-only consume and
prepared program identity are separate from configuration. Required
owner current is a callback; not all 24 reads are callback-free.

Design: resolve supported CoD1 SP campaign profile from actual map and
build/module capabilities at map-entry boundary, derive maps/<mapname>.gsc
from original VFS, admit supported feature set rather than auto-enable
every experimental gate. Separate initial declaration from later bound
epoch/actor-plan receipt and Session/program identity. Commit only
after equivalent preflight. Existing explicit test opt-in keeps old
cvar semantics; unsupported maps, RTCW/MP, savegame and restart preserve
their refusal/legacy contracts. Never hide exec or translate every set
to a Cvar_Set sequence. Keep ordinary settings untouched. Audit actual
DLL selection, menu routes and session shutdown/reentry before implementation.
Acceptance: clean configuration, menu and spmap training, same mission
events as R40+CFG, training -> us_intro, repeated entry, invalid/missing
script, unsupported map, explicit diagnostics. No assertion that all
missions/save-load supported. CFG remains reference until acceptance.

## 5. Status / next bounded step

R43B2A completes selected direct mutation and VM ownership/admission
audit with concrete counterexample to revision-only mutation fences and
first CFG-free startup lifetime trace. It does NOT complete R43 all-writer
census, full R47, or any production optimization. Source ranges checked
via GitHub at pinned reference. No performance claim, Hunk unchanged,
no user gameplay test required.

Next R43B2B: finish narrow catalog lifetime proof by tracing all relevant
include callers and central private-range protection/reset/retire
callers (including ActorGenericBuild and TurretAnimationReset, actual
ActorRetireActor, model/entity hooks), validate native-order purity and
resource/map revision ownership, and close R43 migration gate or record
specific unproven edges. Then R44A pure prepared source-order extraction
and actual-source differential harness (strict C compiler and sanitizers
where supported). Client/renderer/backend and DLL/menu details remain
separate R48/R49/R50 and R47 gates. Do not call documentation-only work
an FPS improvement. No intermediate gameplay test requested.
