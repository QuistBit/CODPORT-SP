/* Independent descriptor range; normal receiver binding retains local override. */
static int vehicle_method_kind(int descriptor){return descriptor>=2201&&descriptor<=2206?descriptor-2200:0;}
static int vehicle_method_descriptor(const GscProgram*g,const char*name){
 static const char*names[]={"attachPath","startPath","setWaitNode","setWaitSpeed","setSpeed","resumeSpeed"};int i;
 if(!g->vehicle_methods_selected)return -1;for(i=0;i<6;i++)if(callable_name_equal(g,name,names[i]))return 2201+i;return -1;
}
