/* Encoded high-water bound: zero means unknown (full reference scan).
 * All committed StateField publications use this helper. Holes are legal;
 * neither publication nor collection may move an existing field index. */
static int state_fields_scan_end(const GscRuntime*r){
 int encoded=r->state_field_scan_end_plus1;
 return encoded>0&&encoded-1<=r->state_field_capacity?encoded-1:r->state_field_capacity;
}
static void state_field_publish(GscRuntime*r,int index,const StateField*field){
 r->state_fields[index]=*field;
 if(r->state_field_scan_end_plus1>0&&index>=r->state_field_scan_end_plus1-1)
  r->state_field_scan_end_plus1=index+2;
}
