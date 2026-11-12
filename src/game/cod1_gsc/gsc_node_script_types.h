#ifndef GSC_NODE_SCRIPT_TYPES_H
#define GSC_NODE_SCRIPT_TYPES_H
#include <stdint.h>
#define GSC_NODE_SCRIPT_CAPACITY 32u
typedef struct {uint32_t count;char names[GSC_NODE_SCRIPT_CAPACITY][64];} GscNodeScriptNames;
/* Port-owned opaque DATA reference, not original VM ABI or call admission. */
typedef struct {uint64_t cache,entry;} GscNodeScriptRef;
static int gsc_node_script_name_valid(const char*name){
 unsigned i;if(!name)return 0;
 for(i=0;i<64;i++){unsigned char c=(unsigned char)name[i];if(!c)return i!=0;if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'))return 0;}return 0;
}
#endif
