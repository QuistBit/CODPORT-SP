#include "gsc_native_imports.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GSC_NATIVE_REALLOC
#define GSC_NATIVE_REALLOC realloc
#endif
#ifndef GSC_NATIVE_CALLOC
#define GSC_NATIVE_CALLOC calloc
#endif

typedef struct {
    GscNativeContract contract;
    GscNativeGlobalCallback global;
    GscNativeMethodCallback method;
    void *user;
} NativeEntry;
struct GscNativeRegistry {
    NativeEntry *entries;
    int count, capacity, sealed, active;
};
static void clear_error(char *error,size_t size) {if(error&&size)error[0]=0;}
static int nfail(char *error,size_t size,const char *text) {
    if(error&&size)snprintf(error,size,"%s",text);
    return 0;
}
static int valid_space(int s) {return s==GSC_NATIVE_GLOBAL||s==GSC_NATIVE_METHOD;}
static int bounded_text(const char *s,size_t n) {
    size_t i;
    if(!s)return 0;
    for(i=0;i<n;i++)if(!s[i])return 1;
    return 0;
}
static int identifier(const char *name) {
    size_t n;
    if(!bounded_text(name,GSC_NATIVE_NAME_CAPACITY)||!name[0])return 0;
    for(n=0;name[n];n++) {
        unsigned char c=(unsigned char)name[n];
        if(c=='_'||(c>='a'&&c<='z')||(c>='A'&&c<='Z'))continue;
        if(n&&c>='0'&&c<='9')continue;
        return 0;
    }
    return 1;
}
static int complete(const GscNativeContract *c) {
    unsigned want=c->name_space==GSC_NATIVE_GLOBAL?
        GSC_NATIVE_KNOWN_GLOBAL:GSC_NATIVE_KNOWN_METHOD;
    return c->known==want;
}
static int valid_contract(const GscNativeContract *c) {
    int i;
    if(!c||!valid_space(c->name_space)||!identifier(c->name)||
       !bounded_text(c->provenance,sizeof(c->provenance))||!c->provenance[0]||
       (c->known&~GSC_NATIVE_KNOWN_METHOD)||
       (c->contexts&~(GSC_NATIVE_STATEMENT|GSC_NATIVE_EXPRESSION))||
       (c->result_types&~GSC_NATIVE_TYPE_ALL_WITH_STRUCTS)||
       (c->receiver_types&~GSC_NATIVE_TYPE_ALL_WITH_STRUCTS))return 0;
    if(c->known&GSC_NATIVE_KNOWN_CONTEXT) {if(!c->contexts)return 0;}
    else if(c->contexts)return 0;
    if(c->known&GSC_NATIVE_KNOWN_ARITY) {
        if(c->min_args<0||c->max_args<c->min_args||
           c->max_args>GSC_NATIVE_MAX_ARGS)return 0;
    } else if(c->min_args!=-1||c->max_args!=-1)return 0;
    if((c->known&GSC_NATIVE_KNOWN_ARGUMENTS)&&!(c->known&GSC_NATIVE_KNOWN_ARITY))return 0;
    for(i=0;i<GSC_NATIVE_MAX_ARGS;i++) {
        unsigned mask=c->argument_types[i];
        if(mask&~GSC_NATIVE_TYPE_ALL_WITH_STRUCTS)return 0;
        if((c->known&GSC_NATIVE_KNOWN_ARGUMENTS)&&i<c->max_args) {
            if(!mask)return 0;
        } else if(mask)return 0;
    }
    if(!(c->known&GSC_NATIVE_KNOWN_RESULT)&&c->result_types)return 0;
    if((c->known&GSC_NATIVE_KNOWN_RESULT)&&!c->result_types&&
       (c->contexts&GSC_NATIVE_EXPRESSION))return 0;
    if(c->name_space==GSC_NATIVE_GLOBAL) {
        if((c->known&GSC_NATIVE_KNOWN_RECEIVER)||c->receiver_types)return 0;
    } else if(c->known&GSC_NATIVE_KNOWN_RECEIVER) {
        if(!c->receiver_types)return 0;
    } else if(c->receiver_types)return 0;
    return 1;
}
static int entry_implemented(const NativeEntry *e) {
    return e->contract.name_space==GSC_NATIVE_GLOBAL?e->global!=NULL:e->method!=NULL;
}
GscNativeRegistry *gsc_native_registry_new(void) {
    return GSC_NATIVE_CALLOC(1,sizeof(GscNativeRegistry));
}
void gsc_native_registry_free(GscNativeRegistry *r) {
    if(!r)return;
    free(r->entries);free(r);
}
int gsc_native_registry_find(const GscNativeRegistry *r,int space,const char *name) {
    int i;
    if(!r||!valid_space(space)||!identifier(name))return -1;
    for(i=0;i<r->count;i++)if(r->entries[i].contract.name_space==space&&
       !strcmp(r->entries[i].contract.name,name))return i;
    return -1;
}
int gsc_native_registry_add(GscNativeRegistry *r,const GscNativeContract *c,
    int *out_index,char *error,size_t size) {
    NativeEntry fresh,*entries;int cap;
    clear_error(error,size);
    if(!r||r->sealed||r->active||!valid_contract(c))
        return nfail(error,size,"invalid native declaration or registry state");
    if(gsc_native_registry_find(r,c->name_space,c->name)>=0)
        return nfail(error,size,"duplicate native namespace/name");
    if(r->count>=GSC_NATIVE_MAX_IMPORTS)return nfail(error,size,"native declaration limit");
    memset(&fresh,0,sizeof(fresh));fresh.contract=*c;
    if(r->count==r->capacity) {
        cap=r->capacity?r->capacity*2:8;
        if(cap>GSC_NATIVE_MAX_IMPORTS)cap=GSC_NATIVE_MAX_IMPORTS;
        if((size_t)cap>SIZE_MAX/sizeof(*entries))return nfail(error,size,"native allocation overflow");
        entries=GSC_NATIVE_REALLOC(r->entries,(size_t)cap*sizeof(*entries));
        if(!entries)return nfail(error,size,"native declaration allocation failed");
        r->entries=entries;r->capacity=cap;
    }
    r->entries[r->count]=fresh;
    if(out_index)*out_index=r->count;
    r->count++;return 1;
}
static int can_bind(GscNativeRegistry *r,int index,int space,char *error,size_t size) {
    if(!r||r->sealed||r->active||index<0||index>=r->count||
       r->entries[index].contract.name_space!=space||
       !complete(&r->entries[index].contract)||entry_implemented(&r->entries[index]))
        return nfail(error,size,"native binding requires an unbound complete contract in the correct namespace");
    return 1;
}
int gsc_native_registry_bind_global(GscNativeRegistry *r,int index,
    GscNativeGlobalCallback cb,void *user,char *error,size_t size) {
    clear_error(error,size);
    if(!cb)return nfail(error,size,"missing global native callback");
    if(!can_bind(r,index,GSC_NATIVE_GLOBAL,error,size))return 0;
    r->entries[index].global=cb;r->entries[index].user=user;return 1;
}
int gsc_native_registry_bind_method(GscNativeRegistry *r,int index,
    GscNativeMethodCallback cb,void *user,char *error,size_t size) {
    clear_error(error,size);
    if(!cb)return nfail(error,size,"missing method native callback");
    if(!can_bind(r,index,GSC_NATIVE_METHOD,error,size))return 0;
    r->entries[index].method=cb;r->entries[index].user=user;return 1;
}
int gsc_native_registry_seal(GscNativeRegistry *r,char *error,size_t size) {
    clear_error(error,size);
    if(!r||r->sealed||r->active)return nfail(error,size,"invalid native registry seal state");
    r->sealed=1;return 1;
}
int gsc_native_registry_count(const GscNativeRegistry *r) {return r?r->count:0;}
int gsc_native_registry_get(const GscNativeRegistry *r,int index,GscNativeEntryInfo *out) {
    GscNativeEntryInfo info;
    if(!r||!out||index<0||index>=r->count)return 0;
    memset(&info,0,sizeof(info));info.index=index;info.contract=r->entries[index].contract;
    info.implemented=entry_implemented(&r->entries[index]);info.complete=complete(&info.contract);
    *out=info;return 1;
}
static unsigned valid_value_type(const GscNativeValue *v) {
    if(!v)return 0;
    switch(v->kind) {
      case GSC_NATIVE_UNDEFINED:return GSC_NATIVE_TYPE_UNDEFINED;
      case GSC_NATIVE_NUMBER:
        if(!isfinite(v->number)||(v->integer!=0&&v->integer!=1)||
           (v->integer&&truncf(v->number)!=v->number))return 0;
        return GSC_NATIVE_TYPE_NUMBER;
      case GSC_NATIVE_STRING:
        return bounded_text(v->string,sizeof(v->string))?GSC_NATIVE_TYPE_STRING:0u;
      case GSC_NATIVE_VECTOR:
        return isfinite(v->vector[0])&&isfinite(v->vector[1])&&isfinite(v->vector[2])?
            GSC_NATIVE_TYPE_VECTOR:0u;
      case GSC_NATIVE_ENTITY:
        return v->entity.epoch&&v->entity.generation?GSC_NATIVE_TYPE_ENTITY:0u;
      case GSC_NATIVE_ARRAY:
        return v->array.runtime_token&&v->array.handle?GSC_NATIVE_TYPE_ARRAY:0u;
      case GSC_NATIVE_STRUCT:
        return v->object.runtime_token&&v->object.handle?GSC_NATIVE_TYPE_STRUCT:0u;
      default:return 0;
    }
}
static int invoke(GscNativeRegistry *r,int index,int space,unsigned context,
    const GscNativeValue *receiver,const GscNativeValue *args,int argc,
    GscNativeResult *out,char *error,size_t size) {
    const NativeEntry *e;const GscNativeContract *c;
    GscNativeValue values[GSC_NATIVE_MAX_ARGS],recv,result;
    GscNativeResult committed;int i,ok;char callback_error[256];unsigned type;
    clear_error(error,size);
    if(!r||!r->sealed||r->active||index<0||index>=r->count||!out||
       (context!=GSC_NATIVE_STATEMENT&&context!=GSC_NATIVE_EXPRESSION))
        return nfail(error,size,"invalid native invocation state/context");
    e=&r->entries[index];c=&e->contract;
    if(c->name_space!=space)return nfail(error,size,"native namespace mismatch");
    if(!complete(c))return nfail(error,size,"native contract is incomplete");
    if(!entry_implemented(e))return nfail(error,size,"native implementation is missing");
    if(!(c->contexts&context))return nfail(error,size,"native context is not permitted");
    if(argc<0||argc>GSC_NATIVE_MAX_ARGS||argc<c->min_args||argc>c->max_args||
       (argc&&!args))return nfail(error,size,"native argument count is not permitted");
    if(space==GSC_NATIVE_METHOD) {
        type=valid_value_type(receiver);
        if(!type||!(type&c->receiver_types))return nfail(error,size,"native receiver type/value is not permitted");
        recv=*receiver;
    } else memset(&recv,0,sizeof(recv));
    for(i=0;i<argc;i++) {
        type=valid_value_type(&args[i]);
        if(!type||!(type&c->argument_types[i]))return nfail(error,size,"native argument type/value is not permitted");
        values[i]=args[i];
    }
    memset(&result,0,sizeof(result));result.kind=-1;
    memset(callback_error,0,sizeof(callback_error));r->active=1;
    if(space==GSC_NATIVE_GLOBAL)
        ok=e->global(e->user,argc?values:NULL,argc,c->result_types?&result:NULL,callback_error,sizeof(callback_error));
    else ok=e->method(e->user,&recv,argc?values:NULL,argc,c->result_types?&result:NULL,callback_error,sizeof(callback_error));
    r->active=0;callback_error[sizeof(callback_error)-1]=0;
    if(!ok)return nfail(error,size,callback_error[0]?callback_error:"native callback failed");
    memset(&committed,0,sizeof(committed));
    if(c->result_types) {
        type=valid_value_type(&result);
        if(!type||!(type&c->result_types))return nfail(error,size,"native callback result violates its contract");
        committed.has_value=1;committed.value=result;
    }
    *out=committed;return 1;
}
int gsc_native_invoke_global(GscNativeRegistry *r,int index,unsigned context,
    const GscNativeValue *args,int argc,GscNativeResult *out,char *error,size_t size) {
    return invoke(r,index,GSC_NATIVE_GLOBAL,context,NULL,args,argc,out,error,size);
}
int gsc_native_invoke_method(GscNativeRegistry *r,int index,unsigned context,
    const GscNativeValue *receiver,const GscNativeValue *args,int argc,
    GscNativeResult *out,char *error,size_t size) {
    return invoke(r,index,GSC_NATIVE_METHOD,context,receiver,args,argc,out,error,size);
}

typedef struct {
    const GscProgram *program;const GscNativeRegistry *registry;
    GscNativeImportCallback callback;void *user;
    GscNativeImportSummary summary;int invalid;
} ImportWalk;
static unsigned literal_type(const GscProgram *g,int index,int *invalid) {
    GscSyntaxExprInfo e;
    if(!gsc_program_syntax_expr_get(g,index,&e)) {*invalid=1;return 0;}
    if(e.kind==GSC_SYNTAX_NUMBER)return GSC_NATIVE_TYPE_NUMBER;
    if(e.kind==GSC_SYNTAX_STRING)return GSC_NATIVE_TYPE_STRING;
    if(e.kind==GSC_SYNTAX_VECTOR)return GSC_NATIVE_TYPE_VECTOR;
    return 0;
}
static int import_site(void *user,const GscLinkAuditSite *s) {
    ImportWalk *w=user;GscNativeImportSite out;
    const NativeEntry *entry=NULL;const GscNativeContract *c=NULL;
    int i,script,builtin;
    w->summary.visited++;
    if(s->entity_event_index>=0||s->requested_module[0]||!(s->site_kind==GSC_LINK_STATEMENT_CALL||
       s->site_kind==GSC_LINK_EXPRESSION_CALL||s->site_kind==GSC_LINK_RECEIVER_CALL||
       s->site_kind==GSC_LINK_RECEIVER_EXPRESSION)) {
        w->summary.excluded++;return 1;
    }
    memset(&out,0,sizeof(out));out.source=*s;
    out.name_space=s->site_kind==GSC_LINK_RECEIVER_CALL||
        s->site_kind==GSC_LINK_RECEIVER_EXPRESSION?GSC_NATIVE_METHOD:GSC_NATIVE_GLOBAL;
    out.context=s->site_kind==GSC_LINK_EXPRESSION_CALL||
        s->site_kind==GSC_LINK_RECEIVER_EXPRESSION?GSC_NATIVE_EXPRESSION:GSC_NATIVE_STATEMENT;
    out.registry_index=gsc_native_registry_find(w->registry,out.name_space,s->name);
    if(s->argc<0||s->argc>GSC_NATIVE_MAX_ARGS) {w->invalid=1;return 0;}
    for(i=0;i<s->argc;i++) {
        out.literal_argument_types[i]=literal_type(w->program,s->args[i],&w->invalid);
        if(!out.literal_argument_types[i])out.unknown_argument_mask|=1u<<i;
    }
    if(out.name_space==GSC_NATIVE_METHOD)
        out.literal_receiver_type=literal_type(w->program,s->receiver_expr,&w->invalid);
    if(w->invalid)return 0;
    script=s->binding==GSC_LINK_SCRIPT_LOCAL;
    builtin=s->binding==GSC_LINK_FIXTURE_BUILTIN||s->binding==GSC_LINK_ENTITY_NATIVE;
    if(out.registry_index>=0) {entry=&w->registry->entries[out.registry_index];c=&entry->contract;}
    if((script||builtin)&&c) {
        out.route=GSC_NATIVE_ROUTE_COLLISION;
        out.issues=script?GSC_NATIVE_ISSUE_SCRIPT_COLLISION:GSC_NATIVE_ISSUE_FIXTURE_COLLISION;
    } else if(script)out.route=GSC_NATIVE_ROUTE_SCRIPT;
    else if(builtin)out.route=GSC_NATIVE_ROUTE_FIXTURE;
    else if(!c) {out.route=GSC_NATIVE_ROUTE_UNDECLARED;out.issues=GSC_NATIVE_ISSUE_UNDECLARED;}
    else {
        if(!complete(c))out.issues|=GSC_NATIVE_ISSUE_INCOMPLETE;
        if(!entry_implemented(entry))out.issues|=GSC_NATIVE_ISSUE_IMPLEMENTATION;
        if((c->known&GSC_NATIVE_KNOWN_CONTEXT)&&!(c->contexts&out.context))out.issues|=GSC_NATIVE_ISSUE_CONTEXT;
        if((c->known&GSC_NATIVE_KNOWN_ARITY)&&(s->argc<c->min_args||s->argc>c->max_args))out.issues|=GSC_NATIVE_ISSUE_ARITY;
        if(c->known&GSC_NATIVE_KNOWN_ARGUMENTS)for(i=0;i<s->argc&&i<c->max_args;i++) {
            if(out.literal_argument_types[i]&&!(out.literal_argument_types[i]&c->argument_types[i]))
                out.issues|=GSC_NATIVE_ISSUE_ARGUMENT_TYPE;
        }
        if((c->known&GSC_NATIVE_KNOWN_RECEIVER)&&out.literal_receiver_type&&
           !(c->receiver_types&out.literal_receiver_type))out.issues|=GSC_NATIVE_ISSUE_RECEIVER_TYPE;
        if(out.issues&(GSC_NATIVE_ISSUE_CONTEXT|GSC_NATIVE_ISSUE_ARITY|
           GSC_NATIVE_ISSUE_ARGUMENT_TYPE|GSC_NATIVE_ISSUE_RECEIVER_TYPE))out.route=GSC_NATIVE_ROUTE_REJECTED;
        else if(out.issues&GSC_NATIVE_ISSUE_INCOMPLETE)out.route=GSC_NATIVE_ROUTE_INCOMPLETE;
        else if(out.issues&GSC_NATIVE_ISSUE_IMPLEMENTATION)out.route=GSC_NATIVE_ROUTE_UNIMPLEMENTED;
        else out.route=GSC_NATIVE_ROUTE_READY;
    }
    w->summary.selected++;
    if(out.name_space==GSC_NATIVE_GLOBAL)w->summary.global_sites++;
    else w->summary.method_sites++;
    switch(out.route) {
      case GSC_NATIVE_ROUTE_SCRIPT:w->summary.script_sites++;break;
      case GSC_NATIVE_ROUTE_FIXTURE:w->summary.fixture_sites++;break;
      case GSC_NATIVE_ROUTE_UNDECLARED:w->summary.undeclared++;break;
      case GSC_NATIVE_ROUTE_INCOMPLETE:w->summary.incomplete++;break;
      case GSC_NATIVE_ROUTE_UNIMPLEMENTED:w->summary.unimplemented++;break;
      case GSC_NATIVE_ROUTE_READY:w->summary.ready++;break;
      case GSC_NATIVE_ROUTE_COLLISION:w->summary.collisions++;break;
      case GSC_NATIVE_ROUTE_REJECTED:w->summary.rejected++;break;
      default:w->invalid=1;return 0;
    }
    return !w->callback||w->callback(w->user,&out);
}
int gsc_program_plan_native_imports(const GscProgram *g,const char *entry,
    const GscNativeRegistry *r,GscNativeImportCallback callback,void *user,
    GscNativeImportSummary *out,char *error,size_t size) {
    ImportWalk walk;GscLinkAuditSummary audit;int ok;
    clear_error(error,size);if(out)memset(out,0,sizeof(*out));
    if(!r||!r->sealed||!out)return nfail(error,size,"native import plan requires a sealed registry and output");
    memset(&walk,0,sizeof(walk));walk.program=g;walk.registry=r;
    walk.callback=callback;walk.user=user;
    ok=gsc_program_audit_links(g,entry,import_site,&walk,&audit,error,size);
    if(walk.invalid)return nfail(error,size,"invalid IR in native import plan");
    if(!ok)return 0;
    if(audit.sites!=walk.summary.visited||walk.summary.visited!=walk.summary.selected+walk.summary.excluded)
        return nfail(error,size,"native import plan count mismatch");
    *out=walk.summary;return 1;
}

/* Pure additive private-bank query used only by the selected controller API. */
int gsc_native_registry_storage_separate(const GscNativeRegistry*r,const void*p,size_t n){
 uintptr_t a=(uintptr_t)p,b;size_t bytes;if(!r||!p||!n)return 1;
 b=(uintptr_t)r;if(a<=b?b-a<n:a-b<sizeof(*r))return 0;
 b=(uintptr_t)r->entries;bytes=(size_t)r->capacity*sizeof(*r->entries);
 if(r->entries&&bytes&&(a<=b?b-a<n:a-b<bytes))return 0;return 1;
}
