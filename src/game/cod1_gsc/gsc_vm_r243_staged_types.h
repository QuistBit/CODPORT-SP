/* Private inline Program/Runtime dependency. Requires the staged protocol
 * and complete Func. No live selector or native INIT is supplied here. */
#ifndef GSC_VM_R243_STAGED_TYPES_H
#define GSC_VM_R243_STAGED_TYPES_H
typedef struct R243SInitCall R243SInitCall;
typedef struct R243SInitOwner R243SInitOwner;
typedef struct {
 const GscProgram *program;
 uint64_t program_token,admission_token,init_key,scripted_key;
 int init_module,init_fn,scripted_module,scripted_fn;
 Func init_record,scripted_record;
 const Func *funcs;const Instr *code;const Expr *expr;
 const void *modules;int func_count,code_count,expr_count,module_count;
 /* Actual producer-backed declaration, not parsed basename/AI category. */
 char registration[9];
} R243SProgramDescriptor;
typedef struct {
 int enabled,failed; /* selected contract/fault, NOT gameplay ready */
 uint64_t profile_identity,next_dispatch;
 const R243SProgramDescriptor *descriptor;
 GscStagedScriptedControllerHostV2 host;
 R243SInitCall *active_init;R243SInitOwner *active_owner;
} R243SProfile;
/* Proposed inline fields, no new separately allocated lifetime owner:
 * GscProgram: int r243s_requested;R243SProgramDescriptor r243s_descriptor;
 * GscRuntime: R243SProfile r243s_profile;
 * Program.r243s_requested only via fresh precompile profile selection.
 * Descriptor installed after actual successful whole-function link, before
 * Runtime creation/callbacks. Runtime profile bound after ALL real host
 * dependencies, BEFORE actual start; old fixed selection mutually exclusive.
 */
#endif
