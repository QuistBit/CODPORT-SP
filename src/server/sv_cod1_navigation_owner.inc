/* ENGINE-resident observer tied to the exact live GAME VM. No callback
 * address from the DLL is retained. VM_Free/VM_Clear detach before unload,
 * including error paths which bypass normal GAME_SHUTDOWN. */
static vm_t *cod1NavigationGameVM;
static void SV_Cod1NavigationVMFree(vm_t *owner){
 if(owner!=cod1NavigationGameVM)return;
 CM_Cod1NavigationSetObserver(NULL);cod1NavigationGameVM=NULL;
}
static void SV_Cod1NavigationRetire(uint64_t incarnation){
 vm_t *owner=cod1NavigationGameVM;
 if(!incarnation||!owner||owner!=gvm)return;
 VM_Call(owner,GAME_COD1_NAV_RESOURCE_RETIRED,(int)(uint32_t)incarnation,(int)(uint32_t)(incarnation>>32));
}
static void SV_Cod1NavigationBeginGameModule(void){
 cod1NavigationGameVM=gvm;gvm->beforeFree=SV_Cod1NavigationVMFree;
 CM_Cod1NavigationSetObserver(SV_Cod1NavigationRetire);
}
