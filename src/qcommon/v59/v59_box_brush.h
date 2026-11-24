#ifndef V59_BOX_BRUSH_H
#define V59_BOX_BRUSH_H
/* ENGINE41B8F0, box half: same axial-first side ordering and PC24
 * materialization as the existing native trace-work preparation. */
static void C1BB14_Sweep(const v59_world*w,const brush*b,trace_work*t){
 int j,k,inside=1;float enter=0,leave=t->fraction;const brush_plane*selected=NULL;
 for(j=0;j<b->n;j++){
  const brush_plane*s=b->planes+j;float d1,d2,dd,num,dist;unsigned signs=0;
  if(s->axis>=0){k=s->axis;d1=fs(fm(fs(t->start[k],s->bound),s->sign),t->size1[k]);d2=fs(fm(fs(t->end[k],s->bound),s->sign),t->size1[k]);}
  else{
   for(k=0;k<3;k++)if(s->n[k]<0)signs|=1u<<k;
   dist=fs(s->d,dp(t->corners[signs],s->n,1,0,2));
   d1=fs(dp(t->start,s->n,0,2,1),dist);d2=fs(dp(t->end,s->n,0,2,1),dist);
  }
  if(d1>0){
   dd=fs(d1,d2);if(d2>0){if(dd<=0||d2>=.125f)return;inside=0;}
   num=fs(d1,.125f);
   if(num>fm(dd,enter)){enter=fd(num,dd);if(enter>=leave)return;selected=s;}
   else if(!selected)selected=s;
  }else if(d2>0){inside=0;dd=fs(d1,d2);if(fm(leave,dd)<d1){leave=fd(d1,dd);if(enter>=leave)return;}}
 }
 t->contents=b->contents;
 if(!selected){t->startsolid=1;if(inside){t->fraction=0;t->allsolid=1;}}
 else{t->fraction=enter;cp(selected->n,t->normal);t->flags=w->materials[selected->material].flags;t->material=72u*(selected->material+1);}
}
#endif
