/* The first (name) cell was already decoded and did not match. Validate
 * every remaining cell and the same column/decoded-length limits. Plain
 * cells need no materialization; quotes use the authoritative decoder. */
static int Cod1SS_UnselectedRow56(const char *p,const char *end){
 int col=1,bad=0;char cell[256];
 while(p<end){
  const char *stop;
  if(col++>=64)return 0;
  stop=Cod1SS_Delimiter(p,end,',','"');
  if(stop==end||*stop==','){
   if((size_t)(stop-p)>=sizeof(cell))return 0;
   p=stop<end?stop+1:stop;
  }else{
   p=Cod1SS_Cell(p,end,cell,sizeof(cell),&bad);
   if(bad)return 0;
  }
 }
 return 1;
}
