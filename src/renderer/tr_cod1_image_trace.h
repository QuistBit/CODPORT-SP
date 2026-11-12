/* R46 read-only CPU pixel diagnostics. Never changes a pixel or GL state.
 * upload-input means the CPU buffer at glTexImage2D, NOT driver readback.
 * Internal mip levels are reported only with r_cod1ImageTrace >= 2. */
static int R_Cod1ImageTraceLevel(void) {
    return ri.Cvar_Get ? ri.Cvar_Get("r_cod1ImageTrace","0",0)->integer : 0;
}
static void R_Cod1ImageTracePixels(const char *phase,const char *name,int level,
                                  const byte *rgba,int width,int height,int format) {
    size_t i,n,zero=0,below=0,opaque=0,black=0;
    int k,lo[4]={255,255,255,255},hi[4]={0,0,0,0};
    unsigned int hash=2166136261u;
    if(!name || !rgba || width<1 || height<1 ||
       (size_t)width>((size_t)-1)/4/(size_t)height)return;
    n=(size_t)width*(size_t)height;
    for(i=0;i<n;i++){
        const byte *p=rgba+i*4;
        for(k=0;k<4;k++){
            if(p[k]<lo[k])lo[k]=p[k];if(p[k]>hi[k])hi[k]=p[k];
            hash=(hash^p[k])*16777619u;
        }
        if(!p[3])zero++;if(p[3]<128)below++;if(p[3]==255)opaque++;
        if(!p[0] && !p[1] && !p[2])black++;
    }
    /* Real image dimensions are int-sized in this renderer. Counts are printed
     * as doubles to avoid size_t printf incompatibilities in older MSVC CRTs. */
    ri.Printf(PRINT_ALL,
        "CoD1 image pixels: phase=%s name=%s level=%d size=%dx%d format=%x rgbaFNV=%08x rgbMin=(%d %d %d) rgbMax=(%d %d %d) alpha=%d..%d pixels=%.0f alphaZero=%.0f alphaLT128=%.0f alpha255=%.0f rgbBlack=%.0f\n",
        phase,name,level,width,height,format,hash,lo[0],lo[1],lo[2],hi[0],hi[1],hi[2],
        lo[3],hi[3],(double)n,(double)zero,(double)below,(double)opaque,(double)black);
}
