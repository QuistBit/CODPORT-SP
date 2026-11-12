/* R240 ordinary-byte subtitle glyph/settled-group restoration. The original
 * eight-line historical message window, scrolling and fades remain separate.
 * Voice playback, exact stop and stream-drain ownership are not changed here.
 * CoD DAT metrics stay floats; no global HUD/fontInfo/renderer policy changes. */
typedef struct {float top,left,advance,s,t,s2,t2;int width,height;qhandle_t shader;} CS240Glyph;
typedef struct {int ready,language,size;float scale;CS240Glyph glyph[256];} CS240Font;
typedef struct {uint64_t generation,sequence;int caption,time,width,height;CGVTrack track;} CS240Scope;
static CS240Font cs240Fonts[4];
static uint64_t cs240Generation=1;
static int cs240Busy,cs240Cancelled,cs240CvarsReady,cs240Exhausted;
static vmCvar_t cs240Cvars[6];
static const char*const cs240Names[6]={"cg_subtitles","cg_subtitleWidth","cg_subtitleMinTime","ui_smallFont","ui_bigFont","ui_extraBigFont"};
static const char*const cs240Defaults[6]={"1","80","3","0.25","0.4","0.55"};
static const char*const cs240Languages[14]={"english","french","german","italian","spanish","british","russian","polish","korean","taiwanese","japanese","chinese","thai","leet"};
void CG_Cod1SubtitleReset(void){
 if(cs240Generation==UINT64_MAX)cs240Exhausted=1;else ++cs240Generation;
 memset(cs240Fonts,0,sizeof(cs240Fonts));memset(cs240Cvars,0,sizeof(cs240Cvars));cs240CvarsReady=0;
 /* An active cold callback retains the busy reservation until its unwind. */
 if(cs240Busy)cs240Cancelled=1;
}
static int CS240_Current(const CS240Scope*c){
 return c&&!cs240Exhausted&&!cs240Cancelled&&cs240Busy&&c->generation==cs240Generation&&
  c->caption==cgvCaption&&c->caption>=0&&c->caption<1024&&c->time==cg.time&&c->sequence==cgvSequence&&
  c->width==cgs.glconfig.vidWidth&&c->height==cgs.glconfig.vidHeight&&
  CGV_Current(c->track.request.packet.sound.epoch,c->track.request.incarnation)&&
  !memcmp(cgvTracks+c->caption,&c->track,sizeof(c->track));
}
static int CS240_Configuration(const CS240Scope*c,int*size,int*language,int*columns,float*alpha){
 unsigned i;char value[32];float scale;long lang;char*end;
 if(!cs240CvarsReady){
  for(i=0;i<6;i++){trap_Cvar_Register(cs240Cvars+i,cs240Names[i],cs240Defaults[i],CVAR_ARCHIVE);if(!CS240_Current(c))return 0;}
  cs240CvarsReady=1;
 }
 for(i=0;i<6;i++){trap_Cvar_Update(cs240Cvars+i);if(!CS240_Current(c))return 0;}
 if(!cs240Cvars[0].integer||cgs.glconfig.vidWidth<=0||cgs.glconfig.vidHeight<=0||
    !isfinite(cs240Cvars[3].value)||!isfinite(cs240Cvars[4].value)||!isfinite(cs240Cvars[5].value)||
    !isfinite(cg_hudAlpha.value))return 0;
 scale=(float)((float)cgs.glconfig.vidHeight/480.0f)*.25f;
 if(!isfinite(scale))return 0;
 *size=scale<=cs240Cvars[3].value?12:scale>=cs240Cvars[5].value?32:scale>=cs240Cvars[4].value?24:16;
 *columns=(cgs.glconfig.vidWidth>640?cgs.glconfig.vidWidth:640)/8-2;
 if(*columns<1)*columns=78;
 if(cs240Cvars[1].integer>0&&cs240Cvars[1].integer<*columns)*columns=cs240Cvars[1].integer;
 /* Input is at most1023 bytes, so a wider column has identical bounded effect. */
 if(*columns>1023)*columns=1023;
 *alpha=cg_hudAlpha.value;if(*alpha<0)*alpha=0;if(*alpha>1)*alpha=1;
 memset(value,0,sizeof(value));trap_Cvar_VariableStringBuffer("loc_language",value,sizeof(value));if(!CS240_Current(c)||!memchr(value,0,sizeof(value)))return 0;
 lang=strtol(value,&end,10);if(end==value||*end||lang<0||lang>=14)lang=0;*language=(int)lang;
 return CS240_Current(c);
}
/* Existence probes use the real VFS and close their own handle. Empty files
 * count as existing, matching native FS_ReadFile(path,NULL)>=0. */
static int CS240_Exists(const CS240Scope*c,const char*path,int*exists){fileHandle_t f=0;int n;
 if(!CS240_Current(c))return 0;n=trap_FS_FOpenFile(path,&f,FS_READ);
 if(f)trap_FS_FCloseFile(f);if(!CS240_Current(c))return 0;*exists=n>=0;return 1;
}
static int CS240_Path(const char*path){size_t i,n;if(!path||!(n=strlen(path))||n>=MAX_QPATH||path[0]=='/'||strstr(path,".."))return 0;
 for(i=0;i<n;i++)if((unsigned char)path[i]<33||(unsigned char)path[i]>126||path[i]=='\\'||path[i]==':')return 0;return 1;
}
static int CS240_Localized(const CS240Scope*c,const char*base,int language,int shader,char out[MAX_QPATH]){
 const char*slash,*dot;char candidate[MAX_QPATH];size_t dir,stem;int exists,n;
 if(!CS240_Path(base))return 0;Q_strncpyz(out,base,MAX_QPATH);if(!language)return 1;
 slash=strrchr(base,'/');dir=slash?(size_t)(slash-base)+1:0;dot=shader?strrchr(base,'.'):NULL;
 if(dot&&slash&&dot<slash)dot=NULL;stem=dot?(size_t)(dot-base)-dir:strlen(base)-dir;
 n=snprintf(candidate,sizeof(candidate),"%.*s%s/%.*s%s",(int)dir,base,cs240Languages[language],(int)stem,base+dir,shader?".tga":"");
 if(n<0||n>=MAX_QPATH)return 0;if(!CS240_Exists(c,candidate,&exists))return 0;
 if(exists){Q_strncpyz(out,candidate,MAX_QPATH);return 1;}
 if(shader){n=snprintf(candidate,sizeof(candidate),"%.*s%s/%.*s.dds",(int)dir,base,cs240Languages[language],(int)stem,base+dir);
  if(n<0||n>=MAX_QPATH||!CS240_Exists(c,candidate,&exists))return 0;if(exists)Q_strncpyz(out,candidate,MAX_QPATH);}
 return CS240_Current(c);
}
static uint32_t CS240_U32(const unsigned char*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float CS240_F32(const unsigned char*p){uint32_t u=CS240_U32(p);float f;memcpy(&f,&u,4);return f;}
static int CS240_Decode(const unsigned char*data,CS240Font*f){unsigned i;
 f->scale=CS240_F32(data+20480);if(!isfinite(f->scale)||f->scale<=0||f->scale>64)return 0;
 for(i=0;i<256;i++){const unsigned char*p=data+i*80;CS240Glyph*g=f->glyph+i;
  g->top=CS240_F32(p+8);g->left=CS240_F32(p+12);g->advance=CS240_F32(p+16);g->width=(int32_t)CS240_U32(p+20);g->height=(int32_t)CS240_U32(p+24);
  g->s=CS240_F32(p+28);g->t=CS240_F32(p+32);g->s2=CS240_F32(p+36);g->t2=CS240_F32(p+40);
  if(!isfinite(g->top)||!isfinite(g->left)||!isfinite(g->advance)||fabsf(g->top)>4096||fabsf(g->left)>4096||fabsf(g->advance)>4096||
     g->width<0||g->height<0||g->width>4096||g->height>4096||!isfinite(g->s)||!isfinite(g->t)||!isfinite(g->s2)||!isfinite(g->t2)||
     g->s<0||g->s>1||g->t<0||g->t>1||g->s2<0||g->s2>1||g->t2<0||g->t2>1||!memchr(p+48,0,32))return 0;
 }return 1;
}
static const CS240Font*CS240_Load(const CS240Scope*c,int size,int language){
 unsigned char*data=NULL;CS240Font*stage=NULL,*slot;char base[MAX_QPATH],path[MAX_QPATH],shader[MAX_QPATH];fileHandle_t f=0;int n,index;unsigned i;int ok=0;
 index=size==12?0:size==16?1:size==24?2:size==32?3:-1;if(index<0)return NULL;slot=cs240Fonts+index;
 if(slot->ready&&slot->size==size&&slot->language==language)return slot;
 stage=calloc(1,sizeof(*stage));data=malloc(20552);if(!stage||!data)goto done;
 snprintf(base,sizeof(base),"fonts/fontImage_%d.dat",size);if(!CS240_Localized(c,base,language,0,path))goto done;
 n=trap_FS_FOpenFile(path,&f,FS_READ);if(!CS240_Current(c)||n!=20552||!f)goto done;
 trap_FS_Read(data,20552,f);if(!CS240_Current(c))goto done;trap_FS_FCloseFile(f);f=0;
 if(!CS240_Current(c)||!CS240_Decode(data,stage))goto done;
 for(i=0;i<255;i++){
  const char*name=(const char*)data+i*80+48;CS240Glyph*g=stage->glyph+i;
  if(!*name){if(g->width&&g->height)goto done;continue;}
  if(!CS240_Localized(c,name,language,1,shader)||!CS240_Current(c))goto done;
  g->shader=trap_R_RegisterShaderNoMip(shader);if(!CS240_Current(c)||g->shader<=0)goto done;
 }
 if(!CS240_Current(c))goto done;stage->ready=1;stage->size=size;stage->language=language;*slot=*stage;ok=1;
done:if(f)trap_FS_FCloseFile(f);free(data);free(stage);return ok&&CS240_Current(c)?slot:NULL;
}
typedef struct {uint16_t cell[8][1024];unsigned length[8],count;} CS240Lines;
static void CS240_Line(CS240Lines*l,const uint16_t*c,unsigned n){unsigned at=l->count%8;memcpy(l->cell[at],c,n*sizeof(*c));l->length[at]=n;++l->count;}
static int CS240_Wrap(const char*text,unsigned limit,CS240Lines*l){
 uint16_t cells[1024];unsigned at=0,col=0,color=7,soft=0,word,n;unsigned char ch;
 n=(unsigned)strlen(text);if(n>=1024||!limit||limit>1023)return 0;memset(l,0,sizeof(*l));
 while(at<n){ch=(unsigned char)text[at];if(ch>=128||(ch<32&&ch!='\r'&&ch!='\n'))return 0;
  if(ch=='^'&&at+1<n&&text[at+1]>='0'&&text[at+1]<'8'){color=(unsigned)(text[at+1]&7);at+=2;continue;}
  word=0;while(word<limit&&at+word<n&&(unsigned char)text[at+word]>=0x21)++word;
  if(word!=limit&&word+col>limit){CS240_Line(l,cells,col);col=0;soft=1;}
  ++at;if(ch=='\n'){CS240_Line(l,cells,col);col=0;}
  else if(ch=='\r')col=0;
  else if(col||ch!=' '||!soft){cells[col++]=(uint16_t)(ch|(color<<8));if(col>=limit){CS240_Line(l,cells,col);col=0;soft=1;}}
 }
 if(col)CS240_Line(l,cells,col);return 1;
}
static int CS240_Quad(const CS240Scope*c,const CS240Glyph*g,float x,float y,float scale,const float color[4]){
 float sx=(float)cgs.glconfig.vidWidth/640.0f,sy=(float)cgs.glconfig.vidHeight/480.0f;
 if(!CS240_Current(c)||g->shader<=0)return 0;trap_R_SetColor(color);if(!CS240_Current(c))return 0;
 trap_R_DrawStretchPic(x*sx,y*sy,g->width*scale*sx,g->height*scale*sy,g->s,g->t,g->s2,g->t2,g->shader);
 return CS240_Current(c);
}
static void CS240_SubtitleDraw(const char*text){
 CS240Scope c;CS240Lines*lines=NULL;const CS240Font*f;char copied[1024];size_t textBytes;int size,lang,cols;float alpha,use,pen,baseline,front[4],black[4]={0,0,0,0};unsigned row,j,start,count,cell;int colored=0;
 if(cs240Busy){cs240Cancelled=1;return;}if(!text||cs240Exhausted||cgvCaption<0||cgvCaption>=1024)return;
 textBytes=strlen(text);if(textBytes>=sizeof(copied))return;memcpy(copied,text,textBytes+1);
 memset(&c,0,sizeof(c));c.generation=cs240Generation;c.sequence=cgvSequence;c.caption=cgvCaption;c.time=cg.time;c.width=cgs.glconfig.vidWidth;c.height=cgs.glconfig.vidHeight;c.track=cgvTracks[cgvCaption];cs240Busy=1;cs240Cancelled=0;
 if(!CS240_Current(&c)||!CS240_Configuration(&c,&size,&lang,&cols,&alpha)||!alpha)goto done;
 /* This restoration deliberately holds DBCS/high-byte encodings rather than
  * pretending an ordinary-byte font is a Unicode text engine. */
 lines=calloc(1,sizeof(*lines));if(!lines||!CS240_Wrap(copied,(unsigned)cols,lines)||!(f=CS240_Load(&c,size,lang)))goto done;
 use=.25f*f->scale;count=lines->count<8?lines->count:8;start=lines->count-count;
 for(row=0;row<count;row++){unsigned r=(start+row)%8;pen=135;baseline=425.0f-(float)(count-1-row)*12.0f;
  for(j=0;j<lines->length[r];j++){const CS240Glyph*g;cell=lines->cell[r][j];g=f->glyph+(cell&255);
   memcpy(front,g_color_table[(cell>>8)&7],sizeof(front));front[3]=alpha;black[3]=alpha;
   {float x=pen+g->left*use,y=baseline-g->top*use;
    colored=1;if(!CS240_Quad(&c,g,x+1,y+1,use,black)||!CS240_Quad(&c,g,x,y,use,front))goto done;
   }pen+=g->advance*use;
  }
 }
done:free(lines);if(colored)trap_R_SetColor(NULL);cs240Busy=0;cs240Cancelled=0;
}
