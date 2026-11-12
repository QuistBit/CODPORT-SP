/* Private staged-controller protocol dependency. No public selector or
 * production Session/GAME provider is installed by this header. The INIT
 * owner component can be configured only by a private configured-load path.
 * Old FIXED ABI is unchanged; later native roles remain unimplemented.
 * All envelopes are memset0 before use; unused union/reserved bytes stay0. */
#ifndef GSC_VM_R243_STAGED_PROTOCOL_H
#define GSC_VM_R243_STAGED_PROTOCOL_H

enum {
 GSC_STAGED_CONTROLLER_INIT=1,
 GSC_STAGED_CONTROLLER_PREFIX=2,
 GSC_STAGED_CONTROLLER_BIND=3,
 GSC_STAGED_CONTROLLER_FINALIZE=4,
 GSC_STAGED_CONTROLLER_UPDATE=5
};

/* Static Program/function association, NOT another live Actor current key.
 * Fields are VM minted/resolved at load from the exact selected Program;
 * numeric nonzero alone confers no authority. Private VM binding contains
 * actual Program identity, immutable source/function/map/catalog association.
 * init_source = exact animscripts/init::main; scripted_main_source = exact
 * animscripts/scripted::main. Neither category4, mode4 nor native pointer.
 */
typedef struct {
 uint64_t profile_identity,program_identity;
 uint64_t init_source,scripted_main_source;
} GscStagedControllerProgramBinding;

typedef struct {
 GscActorInitReturnRequest actor;
 /* Exact genuine returning init frame identity is private VM evidence.
  * actor.task_serial is an association, never proof of normal return.
  * No caller-ready/returned/depth flag appears in this request. */
} GscStagedControllerInitInput;

typedef struct {
 GscActorControllerRequest actor;
 uint64_t expected_selection; /* incarnation only;0 for genuine first PREFIX */
 /* No cross-source expected_revision or early request240/260/269 copy.
  * Each native operation samples live revision/cells under its OWN Txn. */
} GscStagedControllerStepInput;

typedef struct {
 uint64_t transition_nonce,dispatch_nonce;
 GscStagedControllerProgramBinding binding;
 uint32_t role,reserved;
 union {
  GscStagedControllerInitInput init; /* INIT ONLY */
  GscStagedControllerStepInput step; /* PREFIX/BIND/FINALIZE/UPDATE ONLY */
 } input;
} GscStagedControllerRequest;

typedef struct {
 uint64_t current_source,requested_source,retained_source;
 uint32_t current_category,requested_category,retained_category;
 uint8_t current_class,requested_class,force,reserved[5];
} GscStagedControllerCells;

typedef struct {
 GscEntityExecutionCertificate certificate;
 uint64_t transition_nonce,dispatch_nonce,selection_serial;
 uint64_t native_revision; /* diagnostic per-call result; not cross-source lock */
 uint32_t role,reserved;
 union {
  struct {GscStagedControllerProgramBinding bound;} init;
  struct {GscStagedControllerCells cells;uint32_t source_changed,reserved;} selection;
  struct {
   /* Actual result of native record update/pop. Existing HELD frontier
    * constants retain EXACT meanings, no invented complete-state9 value. */
   int32_t frontier; /* STATE9_TAIL_HELD or RESUMED4_THINK_HELD */
   uint32_t record_present; /* actual post-operation0/1; not ready proof */
   GscStagedControllerCells cells;
  } update;
 } output;
} GscStagedControllerResult;

typedef struct {
 const GscStagedControllerRequest *request;
 GscStagedControllerResult *result;
 char *error;size_t error_capacity;
} GscStagedControllerCall;

typedef struct {
 uint64_t transition_nonce,dispatch_nonce;
 GscStagedControllerProgramBinding binding;
 uint64_t runtime_identity,catalog_identity,time_root_incarnation;
 uint64_t actor_serial,expected_selection;
 uint64_t frame_generation; /* INIT0 is intentional, not frame authority */
 GscEntityRef entity;
 uint32_t role,reserved;
} GscStagedControllerReceipt;

typedef struct {void *context;
 int(*take)(void*,const GscStagedControllerCall*,int(*)(void*),void*,GscStagedControllerReceipt*);
 int(*claim)(void*,const GscStagedControllerCall*,int(*)(void*),void*,GscStagedControllerReceipt*);
 int(*current)(void*,const GscStagedControllerReceipt*,const GscStagedControllerCall*,int(*)(void*),void*);
} GscStagedControllerBroker;

typedef struct {
 void *user;
 int(*init_return)(void*,const GscStagedControllerCall*,const GscStagedControllerBroker*,int(*)(void*),void*);
 int(*selection_prefix)(void*,const GscStagedControllerCall*,const GscStagedControllerBroker*,int(*)(void*),void*);
 int(*selection_bind)(void*,const GscStagedControllerCall*,const GscStagedControllerBroker*,int(*)(void*),void*);
 int(*selection_finalize)(void*,const GscStagedControllerCall*,const GscStagedControllerBroker*,int(*)(void*),void*);
 int(*normal_update)(void*,const GscStagedControllerCall*,const GscStagedControllerBroker*,int(*)(void*),void*);
 /* AnimationStep retains its separately owned exact phase6/source broker.
  * It does not make source selection/admission/cleanup/INIT decisions. */
 int(*animation_step)(void*,const GscActorControllerRequest*,const GscActorControllerAnimationServices*,char*,size_t,int(*)(void*),void*);
} GscStagedScriptedControllerHostV2;

/* Exact original-call input/return queries, not generic write grants. */
int gsc_runtime_r243_staged_call_input(const GscRuntime*,const GscStagedControllerCall*,
 const GscStagedControllerBroker*,int(*)(void*),void*);
int gsc_runtime_r243_staged_call_return(const GscRuntime*,const GscStagedControllerCall*,
 const GscStagedControllerBroker*,int(*)(void*),void*);
#endif
