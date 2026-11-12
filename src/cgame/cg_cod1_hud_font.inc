/* Training default-font HUD lane. Native30017F0C + renderer4D5A40/4D5D30/
 * 4CDCC0. DAT decoding/path loading reused from R240, with independent HUD
 * ownership. Ordinary single-line ASCII is the proven input domain. */
typedef struct {float top,left,advance,s,t,s2,t2;int width,height;qhandle_t shader;} CHFGlyph;
typedef struct {int ready,language,size;float scale,height;CHFGlyph glyph[256];} CHFFont;
typedef struct {uint64_t incarnation,epoch,sequence;int time,width,height;} CHFScope;
static CHFFont chfFonts[4];
static int chfBusy,chfCancelled,chfCvarsReady;
static vmCvar_t chfCvars[3];
static const char*const chfNames[3]={"ui_smallFont","ui_bigFont","ui_extraBigFont"};
static const char*const chfDefaults[3]={"0.25","0.4","0.55"};
static const char*const chfLanguages[14]={"english","french","german","italian","spanish","british","russian","polish","korean","taiwanese","japanese","chinese","thai","leet"};
static void CHF_Reset(void){memset(chfFonts,0,sizeof(chfFonts));memset(chfCvars,0,sizeof(chfCvars));chfCvarsReady=0;if(chfBusy)chfCancelled=1;}
static int CHF_Current(const CHFScope*c){return c&&chfBusy&&!chfCancelled&&CHC_Current(c->incarnation,c->epoch,c->time)&&c->sequence==cgCod1HudElements.sequence&&c->width==cgs.glconfig.vidWidth&&c->height==cgs.glconfig.vidHeight;}
static int CHF_Configuration(const CHFScope*c,float drawScale,int*size,int*lang){
 int i;char value[32],*end;long n;float selector;
 if(!chfCvarsReady){for(i=0;i<3;i++){trap_Cvar_Register(chfCvars+i,chfNames[i],chfDefaults[i],CVAR_ARCHIVE);if(!CHF_Current(c))return 0;}chfCvarsReady=1;}
 for(i=0;i<3;i++){trap_Cvar_Update(chfCvars+i);if(!CHF_Current(c)||!isfinite(chfCvars[i].value))return 0;}
 if(c->width<=0||c->height<=0||!isfinite(drawScale)||drawScale<=0||drawScale>4)return 0;
 /* Engine40E430 -> UI vmMain13 passes trunc(scale*100), reconstructed
  * by UI as float*.01, then height/480 in font selector40007D80. */
 selector=(float)((int)(drawScale*100.0f))*.01f;selector*=((float)c->height/480.0f);
 *size=selector<=chfCvars[0].value?12:selector>=chfCvars[2].value?32:selector>=chfCvars[1].value?24:16;
 memset(value,0,sizeof(value));trap_Cvar_VariableStringBuffer("loc_language",value,sizeof(value));if(!CHF_Current(c)||!memchr(value,0,sizeof(value)))return 0;
 n=strtol(value,&end,10);if(end==value||*end||n<0||n>=14)n=0;*lang=(int)n;
 /* DBCS/Thai font selection and multibyte glyph decoding are separate. */
 if(*lang>=8&&*lang<=12)return 0;
 return CHF_Current(c);
}
static int CHF_Exists(const CHFScope*c,const char*path,int*exists){fileHandle_t f=0;int n;
 if(!CHF_Current(c))return 0;n=trap_FS_FOpenFile(path,&f,FS_READ);
 if(f)trap_FS_FCloseFile(f);if(!CHF_Current(c))return 0;*exists=n>=0;return 1;
}
static int CHF_Path(const char*path){size_t i,n;if(!path||!(n=strlen(path))||n>=MAX_QPATH||path[0]=='/'||strstr(path,".."))return 0;
 for(i=0;i<n;i++)if((unsigned char)path[i]<33||(unsigned char)path[i]>126||path[i]=='\\'||path[i]==':')return 0;return 1;
}
static int CHF_Localized(const CHFScope*c,const char*base,int language,int shader,char out[MAX_QPATH]){
 const char*slash,*dot;char candidate[MAX_QPATH];size_t dir,stem;int exists,n;
 if(!CHF_Path(base))return 0;Q_strncpyz(out,base,MAX_QPATH);if(!language)return 1;
 slash=strrchr(base,'/');dir=slash?(size_t)(slash-base)+1:0;dot=shader?strrchr(base,'.'):NULL;
 if(dot&&slash&&dot<slash)dot=NULL;stem=dot?(size_t)(dot-base)-dir:strlen(base)-dir;
 n=snprintf(candidate,sizeof(candidate),"%.*s%s/%.*s%s",(int)dir,base,chfLanguages[language],(int)stem,base+dir,shader?".tga":"");
 if(n<0||n>=MAX_QPATH)return 0;if(!CHF_Exists(c,candidate,&exists))return 0;
 if(exists){Q_strncpyz(out,candidate,MAX_QPATH);return 1;}
 if(shader){n=snprintf(candidate,sizeof(candidate),"%.*s%s/%.*s.dds",(int)dir,base,chfLanguages[language],(int)stem,base+dir);
  if(n<0||n>=MAX_QPATH||!CHF_Exists(c,candidate,&exists))return 0;if(exists)Q_strncpyz(out,candidate,MAX_QPATH);}
 return CHF_Current(c);
}
static uint32_t CHF_U32(const unsigned char*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float CHF_F32(const unsigned char*p){uint32_t u=CHF_U32(p);float f;memcpy(&f,&u,4);return f;}
static int CHF_Decode(const unsigned char*data,CHFFont*f){unsigned i;
 f->height=CHF_F32(data+20484);if(!isfinite(f->height)||f->height<=0||f->height>4096)return 0;
 f->scale=CHF_F32(data+20480);if(!isfinite(f->scale)||f->scale<=0||f->scale>64)return 0;
 for(i=0;i<256;i++){const unsigned char*p=data+i*80;CHFGlyph*g=f->glyph+i;
  g->top=CHF_F32(p+8);g->left=CHF_F32(p+12);g->advance=CHF_F32(p+16);g->width=(int32_t)CHF_U32(p+20);g->height=(int32_t)CHF_U32(p+24);
  g->s=CHF_F32(p+28);g->t=CHF_F32(p+32);g->s2=CHF_F32(p+36);g->t2=CHF_F32(p+40);
  if(!isfinite(g->top)||!isfinite(g->left)||!isfinite(g->advance)||fabsf(g->top)>4096||fabsf(g->left)>4096||fabsf(g->advance)>4096||
     g->width<0||g->height<0||g->width>4096||g->height>4096||!isfinite(g->s)||!isfinite(g->t)||!isfinite(g->s2)||!isfinite(g->t2)||
     g->s<0||g->s>1||g->t<0||g->t>1||g->s2<0||g->s2>1||g->t2<0||g->t2>1||!memchr(p+48,0,32))return 0;
 }return 1;
}
static const CHFFont*CHF_Load(const CHFScope*c,int size,int language){
 unsigned char*data=NULL;CHFFont*stage=NULL,*slot;char base[MAX_QPATH],path[MAX_QPATH],shader[MAX_QPATH];fileHandle_t f=0;int n,index;unsigned i;int ok=0;
 index=size==12?0:size==16?1:size==24?2:size==32?3:-1;if(index<0)return NULL;slot=chfFonts+index;
 if(slot->ready&&slot->size==size&&slot->language==language)return slot;
 stage=calloc(1,sizeof(*stage));data=malloc(20552);if(!stage||!data)goto done;
 snprintf(base,sizeof(base),"fonts/fontImage_%d.dat",size);if(!CHF_Localized(c,base,language,0,path))goto done;
 n=trap_FS_FOpenFile(path,&f,FS_READ);if(!CHF_Current(c)||n!=20552||!f)goto done;
 trap_FS_Read(data,20552,f);if(!CHF_Current(c))goto done;trap_FS_FCloseFile(f);f=0;
 if(!CHF_Current(c)||!CHF_Decode(data,stage))goto done;
 for(i=0;i<255;i++){
  const char*name=(const char*)data+i*80+48;CHFGlyph*g=stage->glyph+i;
  if(!*name){if(g->width&&g->height)goto done;continue;}
  if(!CHF_Localized(c,name,language,1,shader)||!CHF_Current(c))goto done;
  g->shader=trap_R_RegisterShaderNoMip(shader);if(!CHF_Current(c)||g->shader<=0)goto done;
 }
 if(!CHF_Current(c))goto done;stage->ready=1;stage->size=size;stage->language=language;*slot=*stage;ok=1;
done:if(f)trap_FS_FCloseFile(f);free(data);free(stage);return ok&&CHF_Current(c)?slot:NULL;
}
static int CHF_Measure(const CHFFont*f,const char*text,float scale,int*width,int*height){
 double sum=0;float max=1,use=scale*f->scale;size_t i,n=strlen(text);
 if(n>=1024||!isfinite(use)||use<=0)return 0;
 for(i=0;i<n;i++){
  unsigned ch=(unsigned char)text[i];float advance;
  if(ch<32||ch>=127)return 0;
  if(ch=='^'&&text[i+1]>='0'&&text[i+1]<='7'){i++;continue;}
  advance=f->glyph[ch].advance;if(advance==0)advance=f->glyph[(unsigned)'.'].advance;
  sum+=advance;if(sum>max)max=(float)sum;
 }
 if(!isfinite(max*use)||max*use>1048576||!isfinite(scale*f->height))return 0;
 *width=(int)(max*use);*height=(int)(scale*f->height);return 1;
}
static int CHF_Quad(const CHFScope*c,const CHFGlyph*g,float x,float y,float use,const float color[4]){
 float sx=(float)c->width/640.0f,sy=(float)c->height/480.0f;
 if(!CHF_Current(c)||g->shader<=0)return 0;trap_R_SetColor(color);if(!CHF_Current(c))return 0;
 trap_R_DrawStretchPic(x*sx,y*sy,g->width*use*sx,g->height*use*sy,g->s,g->t,g->s2,g->t2,g->shader);return CHF_Current(c);
}
static int CHF_DrawText(const cod1ScriptHud_t*s,uint64_t incarnation,uint64_t epoch,int time,uint64_t sequence){
 CHFScope c;const CHFFont*f;const char*localized;char text[1024];float scale=s->font_scale*.25f,use,pen,baseline,front[4]={1,1,1,1},black[4]={0,0,0,1};int size,lang,width,height,ok=0,colored=0;size_t i,n;
 if(chfBusy){chfCancelled=1;return 0;}
 c.incarnation=incarnation;c.epoch=epoch;c.sequence=sequence;c.time=time;c.width=cgs.glconfig.vidWidth;c.height=cgs.glconfig.vidHeight;chfBusy=1;chfCancelled=0;
 if(!CHF_Current(&c))goto done;
 localized=CG_Cod1MessageLocalize(s->text);if(!CHF_Current(&c)||!localized)goto done;
 n=strlen(localized);if(n>=sizeof(text))goto done;memcpy(text,localized,n+1);
 if(!CHF_Configuration(&c,scale,&size,&lang)||!(f=CHF_Load(&c,size,lang))||!CHF_Measure(f,text,scale,&width,&height))goto done;
 use=scale*f->scale;pen=s->x-width*(s->align_x*.5f);baseline=s->y+height*(1.0f-s->align_y*.5f);
 front[3]=black[3]=CH_Alpha(s,time);
 for(i=0;i<n;i++){
  unsigned ch=(unsigned char)text[i];const CHFGlyph*g;
  if(ch=='^'&&text[i+1]>='0'&&text[i+1]<='7'){float alpha=front[3];memcpy(front,g_color_table[(unsigned)(text[++i]-'0')],sizeof(front));front[3]=alpha;continue;}
  g=f->glyph+ch;if(g->advance==0)g=f->glyph+(unsigned)'.';
  colored=1;
  if(!CHF_Quad(&c,g,pen+g->left*use+1,baseline-g->top*use+1,use,black)||!CHF_Quad(&c,g,pen+g->left*use,baseline-g->top*use,use,front))goto done;
  pen+=g->advance*use;
 }
 ok=CHF_Current(&c);
done:if(colored)trap_R_SetColor(NULL);chfBusy=0;chfCancelled=0;return ok;
}
