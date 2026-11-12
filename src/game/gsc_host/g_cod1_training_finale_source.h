#ifndef G_COD1_TRAINING_FINALE_SOURCE_H
#define G_COD1_TRAINING_FINALE_SOURCE_H
#include "g_cod1_training_followup_source.h"
/* Original complete functions plus an explicitly authored opening controller.
 * go_gate/obstacle_master dialogue/AI bodies are NOT declared supported. */
static int G_Cod1TrainingFinaleSourceSelect(const char *path,char *data,size_t size){
 size_t i;static const unsigned char hash[32]={0xbb,0x86,0xa2,0xa2,0x1f,0x3f,0xa3,0x9c,0x8e,0x53,0xec,0xc5,0x9d,0x47,0xc0,0x99,0x09,0x5a,0x71,0x9a,0xa1,0x1e,0x59,0xc5,0x6d,0x7f,0x87,0xeb,0x24,0x17,0x03,0x15};
 if(strcmp(path,"maps/training.gsc"))return G_Cod1TrainingFollowupSourceSelect(path,data,size);
 if(!data||size!=61690u||!C1TGS_Matches((const unsigned char*)data,size,hash))return -1;
 for(i=0;i<size;i++)if(!((i>=22860u&&i<23192u)||(i>=14619u&&i<16247u)||(i>=16251u&&i<17822u)||(i>=17826u&&i<19354u)||(i>=50312u&&i<50555u)||(i>=50561u&&i<50846u)||(i>=61122u&&i<61686u)||(i>=58969u&&i<60525u)||(i>=19360u&&i<20742u)||(i>=50850u&&i<51005u)||(i>=20748u&&i<22856u)||(i>=60529u&&i<61118u)||(i>=51009u&&i<52023u)||(i>=2350u&&i<3400u)||(i>=52029u&&i<52405u)||(i>=10731u&&i<10887u)||(i>=43806u&&i<44548u)||(i>=44552u&&i<45432u)||(i>=45436u&&i<46423u)||(i>=46427u&&i<47239u)||(i>=47243u&&i<47863u)||(i>=47867u&&i<48495u)||(i>=49723u&&i<50021u))&&data[i]!=13&&data[i]!=10)data[i]=32;
 return 1;
}
#endif
