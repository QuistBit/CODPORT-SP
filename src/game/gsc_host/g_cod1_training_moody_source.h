#ifndef G_COD1_TRAINING_MOODY_SOURCE_H
#define G_COD1_TRAINING_MOODY_SOURCE_H
/* Whole original wait helpers, preserving offsets and byte identity. */
static int G_Cod1TrainingMoodySourceSelect(const char *path,char *data,size_t size){
    size_t i;static const unsigned char hash[32]={0x47,0xff,0x16,0x4b,0x8e,0x5e,0x3f,0x65,0x10,0x01,0x29,0xfd,0x90,0x40,0x58,0x43,0x4b,0xca,0x13,0xec,0x10,0xea,0xbc,0x2f,0x02,0x16,0xf5,0xde,0x07,0xe8,0x79,0xb7};
    if(strcmp(path,"maps/_anim.gsc"))return 0;
    if(!data||size!=26623u||!C1TGS_Matches((const unsigned char *)data,size,hash))return -1;
    for(i=0;i<size;i++)if(!((i>=21735u&&i<21849u)||(i>=21853u&&i<22091u))&&data[i]!=13&&data[i]!=10)data[i]=32;
    return 1;
}

#endif
