#ifndef G_COD1_TRAINING_OPENING_SOURCE_H
#define G_COD1_TRAINING_OPENING_SOURCE_H
/* Only complete, hash-authenticated original functions. This initial selector
 * does not admit go_gate/obstacle_master before their dependencies work. */
static int G_Cod1TrainingOpeningSourceSelect(const char *path,char *data,size_t size){
 size_t i;static const unsigned char hash[32]={0xbb,0x86,0xa2,0xa2,0x1f,0x3f,0xa3,0x9c,0x8e,0x53,0xec,0xc5,0x9d,0x47,0xc0,0x99,0x09,0x5a,0x71,0x9a,0xa1,0x1e,0x59,0xc5,0x6d,0x7f,0x87,0xeb,0x24,0x17,0x03,0x15};
 if(strcmp(path,"maps/training.gsc"))return 0;
 if(!data||size!=61690u||!C1TGS_Matches((const unsigned char*)data,size,hash))return -1;
 for(i=0;i<size;i++)if(!((i>=2350u&&i<3400u)||(i>=52029u&&i<52405u))&&data[i]!=13&&data[i]!=10)data[i]=32;
 return 1;
}
#endif
