/* Native GAME2FA30 requires main/precache/spawner before BSP allocation.
 * Port handles are real committed function indexes+1, never native addresses
 * or execution certificates. No writable output can alias program storage. */
uint64_t gsc_program_identity(const GscProgram*g){
 return g&&!g->invalid&&g->linked?g->function_program_token:0;
}
uint32_t gsc_program_script_function_handle(const GscProgram*g,const char*module,const char*function){
 int m,f;size_t n;
 if(!gsc_program_identity(g)||!module||!function)return 0;
 for(n=0;n<128&&module[n];n++){}if(!n||n==128)return 0;
 for(n=0;n<64&&function[n];n++){}if(!n||n==64)return 0;
 m=find_module(g,module);if(m<0||(f=find_func(g,m,function))<0||g->funcs[f].module!=m)return 0;
 return (uint32_t)f+1;
}

/* Resolve several functions of one module without repeating module lookup.
 * Output is caller-owned storage; each result has scalar handle semantics. */
void gsc_program_script_function_handles(const GscProgram*g,const char*module,
 const char*const*functions,uint32_t*handles,unsigned count){
 unsigned k;size_t n;int m=-1,f;
 if(gsc_program_identity(g)&&module&&functions){
  for(n=0;n<128&&module[n];n++){}
  if(n&&n<128)m=find_module(g,module);
 }
 for(k=0;k<count;k++){
  handles[k]=0;
  if(m<0||!functions[k])continue;
  for(n=0;n<64&&functions[k][n];n++){}
  if(!n||n==64)continue;
  f=find_func(g,m,functions[k]);
  if(f>=0&&g->funcs[f].module==m)handles[k]=(uint32_t)f+1;
 }
}
