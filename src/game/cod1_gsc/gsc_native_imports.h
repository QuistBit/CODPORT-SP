#ifndef GSC_NATIVE_IMPORTS_H
#define GSC_NATIVE_IMPORTS_H

#include "gsc_fixture.h"
#include <stddef.h>

/* R72 standalone host/native contract boundary. NOT a CoD1 native ABI or a
 * second linker. No API in this file edits GscProgram, binds a VM instruction,
 * evaluates an AST or registers names automatically. R74 routes the existing fixture isdefined and fog builtins through a PRIVATE
 * VM-owned registry. R84 adds explicitly enabled entity host imports and value
 * transport. External registrations are still not VM/linker bindings.
 * Explicit host invocations and the private VM bridge are separately tested.
 * All contract data/provenance are supplied by the caller: completeness is not
 * evidence of original-engine correctness. Observed argument counts must never
 * be supplied as a proven signature merely because they occur in a corpus.
 */
#define GSC_NATIVE_MAX_ARGS 16
#define GSC_NATIVE_MAX_IMPORTS 1024
#define GSC_NATIVE_NAME_CAPACITY 128
#define GSC_NATIVE_PROVENANCE_CAPACITY 256
#define GSC_NATIVE_STRING_CAPACITY 128

enum { GSC_NATIVE_GLOBAL=1, GSC_NATIVE_METHOD };
enum { GSC_NATIVE_STATEMENT=1u, GSC_NATIVE_EXPRESSION=2u };
enum { GSC_NATIVE_UNDEFINED=0, GSC_NATIVE_NUMBER, GSC_NATIVE_STRING,
       GSC_NATIVE_VECTOR, GSC_NATIVE_ENTITY, GSC_NATIVE_ARRAY, GSC_NATIVE_STRUCT };
#define GSC_NATIVE_TYPE_UNDEFINED 1u
#define GSC_NATIVE_TYPE_NUMBER 2u
#define GSC_NATIVE_TYPE_STRING 4u
#define GSC_NATIVE_TYPE_VECTOR 8u
#define GSC_NATIVE_TYPE_ENTITY 16u
#define GSC_NATIVE_TYPE_SCALAR 7u
#define GSC_NATIVE_TYPE_ALL 31u /* frozen legacy mask */
#define GSC_NATIVE_TYPE_ARRAY 32u
#define GSC_NATIVE_TYPE_ALL_WITH_ARRAYS 63u /* frozen */
#define GSC_NATIVE_TYPE_STRUCT 64u
#define GSC_NATIVE_TYPE_ALL_WITH_STRUCTS 127u
/* Values are independent bounded fixture copies, not original Value tags or
 * pointers into the parser/engine. Integer numbers must be
 * finite integral floats. Strings must have a NUL within their fixed array.
 * Vectors contain three finite floats. Entity refs contain a nonzero epoch
 * and generation; this registry checks structure only. The R84 host validates
 * liveness/slot/map immediately before game use. Level values are unsupported. */
typedef struct {
    int kind, integer;
    float number;
    char string[GSC_NATIVE_STRING_CAPACITY];
    float vector[3];
    GscEntityRef entity;
    /* R93 distinct private opaque transport, never an entity or engine pointer.
     * Registry checks nonzero payload only; the owning VM checks provenance. */
    struct { uint64_t runtime_token; uint32_t handle; } array;
    struct { uint64_t runtime_token; uint32_t handle; } object;
} GscNativeValue;

enum { GSC_NATIVE_KNOWN_CONTEXT=1u, GSC_NATIVE_KNOWN_ARITY=2u,
       GSC_NATIVE_KNOWN_ARGUMENTS=4u, GSC_NATIVE_KNOWN_RESULT=8u,
       GSC_NATIVE_KNOWN_RECEIVER=16u };
#define GSC_NATIVE_KNOWN_GLOBAL 15u
#define GSC_NATIVE_KNOWN_METHOD 31u
/* Every unknown field must use the canonical empty representation: zero
 * contexts/masks; min_args=max_args=-1. Declared arity is an inclusive range
 * within the fixture's 16-argument syntax capacity, NOT CoD1's maximum.
 * KNOWN_ARGUMENTS requires KNOWN_ARITY; each permitted argument position has
 * a nonempty type mask, unused slots are zero. Optional argument positions are
 * validated if present, never fabricated. result_types=0 with KNOWN_RESULT
 * means VOID (no value), not undefined. VOID cannot promise expression use.
 * A GLOBAL contract must not carry a receiver type or KNOWN_RECEIVER.
 * provenance is a required nonempty description/reference; not a trust flag.
 */
typedef struct {
    int name_space;
    char name[GSC_NATIVE_NAME_CAPACITY];
    char provenance[GSC_NATIVE_PROVENANCE_CAPACITY];
    unsigned known, contexts;
    int min_args, max_args;
    unsigned argument_types[GSC_NATIVE_MAX_ARGS];
    unsigned result_types, receiver_types;
} GscNativeContract;

typedef struct GscNativeRegistry GscNativeRegistry;
/* Callbacks receive checked copies of inputs. result is NULL only for VOID.
 * A non-VOID callback must explicitly write a valid result. Return nonzero on
 * success. On failure it may fill the bounded error buffer. This API cannot
 * roll back effects the callback performed before failing. The dispatcher
 * validates result kinds/values before committing the caller's output.
 * Callbacks must not free/mutate the registry/program or retain input pointers.
 * Recursive invocation on the same registry is refused before another callback;
 * use is single-threaded, not a locking or sandbox boundary for hostile C code.
 */
typedef int (*GscNativeGlobalCallback)(void *user, const GscNativeValue *args,
    int argc, GscNativeValue *result, char *error, size_t error_size);
typedef int (*GscNativeMethodCallback)(void *user, const GscNativeValue *receiver,
    const GscNativeValue *args, int argc, GscNativeValue *result,
    char *error, size_t error_size);
typedef struct {
    int index, implemented, complete;
    GscNativeContract contract;
} GscNativeEntryInfo;
typedef struct {
    int has_value;
    GscNativeValue value;
} GscNativeResult;

/* Registry owns copies of descriptors, not callback user contexts. IDs are
 * local registration-order indexes, never original native/script/opcode IDs.
 * Exact ASCII identifiers and separate GLOBAL/METHOD keys: same spelling in
 * both spaces is allowed; duplicate same-space registration is refused.
 * Add and bind are legal only before seal. Missing or incomplete declarations
 * may be sealed for diagnostics, but are NEVER invokable. Bind requires a
 * complete valid contract and the correct callback type. No replacement.
 * A failed operation leaves prior declarations/bindings/output index intact.
 */
GscNativeRegistry *gsc_native_registry_new(void);
void gsc_native_registry_free(GscNativeRegistry *registry);
int gsc_native_registry_add(GscNativeRegistry *registry,
    const GscNativeContract *contract, int *out_index,
    char *error, size_t error_size);
int gsc_native_registry_bind_global(GscNativeRegistry *registry, int index,
    GscNativeGlobalCallback callback, void *user, char *error, size_t error_size);
int gsc_native_registry_bind_method(GscNativeRegistry *registry, int index,
    GscNativeMethodCallback callback, void *user, char *error, size_t error_size);
int gsc_native_registry_seal(GscNativeRegistry *registry,
    char *error, size_t error_size);
int gsc_native_registry_count(const GscNativeRegistry *registry);
int gsc_native_registry_find(const GscNativeRegistry *registry,
    int name_space, const char *name);
int gsc_native_registry_get(const GscNativeRegistry *registry, int index,
    GscNativeEntryInfo *out);
/* Explicit host calls. The R74 private builtin VM bridge uses the global
 * entrypoint; arbitrary AST calls are NOT automatically routed here. Require a sealed registry and a
 * single statement/expression context. Validate ALL input values/types before
 * invoking anything. Error leaves out untouched. Namespace mismatch cannot
 * fall through to the other callback kind. Arguments may be NULL only at 0.
 */
int gsc_native_invoke_global(GscNativeRegistry *registry, int index,
    unsigned context, const GscNativeValue *args, int argc, GscNativeResult *out,
    char *error, size_t error_size);
int gsc_native_invoke_method(GscNativeRegistry *registry, int index,
    unsigned context, const GscNativeValue *receiver, const GscNativeValue *args,
    int argc, GscNativeResult *out, char *error, size_t error_size);

/* Non-mutating import plan over ALL retained ordinary unqualified calls.
 * Threads, references, qualified targets, computed targets and fixture
 * primitives are excluded rather than mislabelled as native imports.
 * Existing exact local script/builtin candidates without a declaration stay
 * SCRIPT/FIXTURE. A matching native declaration plus a script/builtin target
 * is a COLLISION, never an assumed native/script precedence rule.
 * Without a declaration, an unknown name is UNDECLARED (native candidate,
 * not a confirmed native). COMPLETE/READY mean only the supplied host contract
 * and callback fit this STATIC call shape. Dynamic argument types still need
 * validation at invocation; no runtime value, VM binding or result is created.
 * Only NUMBER/STRING syntax roots are checked as known literals; expressions,
 * casts, references and raw/localized strings stay unknown. No constant-folding.
 * The registry must be sealed. Plan itself makes no allocations. Callback
 * receives copies, must not mutate/free either input; zero cancels. NULL
 * callback counts only. Failure/cancellation zeroes required summary. Success
 * means the walk completed, NOT that link or any import execution succeeded.
 */
enum { GSC_NATIVE_ROUTE_SCRIPT=1, GSC_NATIVE_ROUTE_FIXTURE,
       GSC_NATIVE_ROUTE_UNDECLARED, GSC_NATIVE_ROUTE_INCOMPLETE,
       GSC_NATIVE_ROUTE_UNIMPLEMENTED, GSC_NATIVE_ROUTE_READY,
       GSC_NATIVE_ROUTE_COLLISION, GSC_NATIVE_ROUTE_REJECTED };
enum { GSC_NATIVE_ISSUE_UNDECLARED=1u, GSC_NATIVE_ISSUE_INCOMPLETE=2u,
       GSC_NATIVE_ISSUE_IMPLEMENTATION=4u, GSC_NATIVE_ISSUE_CONTEXT=8u,
       GSC_NATIVE_ISSUE_ARITY=16u, GSC_NATIVE_ISSUE_ARGUMENT_TYPE=32u,
       GSC_NATIVE_ISSUE_SCRIPT_COLLISION=64u,
       GSC_NATIVE_ISSUE_FIXTURE_COLLISION=128u,
       GSC_NATIVE_ISSUE_RECEIVER_TYPE=256u };
typedef struct {
    GscLinkAuditSite source;
    int name_space, registry_index, route;
    unsigned context, issues, unknown_argument_mask;
    unsigned literal_argument_types[GSC_NATIVE_MAX_ARGS], literal_receiver_type;
} GscNativeImportSite;
typedef struct {
    int visited, selected, excluded, global_sites, method_sites;
    int script_sites, fixture_sites, undeclared, incomplete, unimplemented;
    int ready, collisions, rejected;
} GscNativeImportSummary;
typedef int (*GscNativeImportCallback)(void *user,
    const GscNativeImportSite *site);
int gsc_program_plan_native_imports(const GscProgram *program,
    const char *entry_module, const GscNativeRegistry *registry,
    GscNativeImportCallback callback, void *user, GscNativeImportSummary *out,
    char *error, size_t error_size);

int gsc_native_registry_storage_separate(const GscNativeRegistry*,const void*,size_t);

#endif
