#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <android/log.h>

int __android_log_vprint(int p,const char *tag,const char *fmt,va_list ap) {
    fprintf(stderr,"OEMLOG[%d][%s] ",p,tag?tag:"");
    int n=fmt?vfprintf(stderr,fmt,ap):0;fputc('\n',stderr);return n;
}
int __android_log_print(int p,const char *tag,const char *fmt,...) {
    va_list ap;va_start(ap,fmt);int r=__android_log_vprint(p,tag,fmt,ap);va_end(ap);return r;
}
int __android_log_write(int p,const char *tag,const char *s) {
    return __android_log_print(p,tag,"%s",s?s:"");
}
int __android_log_buf_print(int b,int p,const char *tag,const char *fmt,...) {
    (void)b;va_list ap;va_start(ap,fmt);int r=__android_log_vprint(p,tag,fmt,ap);va_end(ap);return r;
}
void __android_log_assert(const char *c,const char *tag,const char *fmt,...) {
    fprintf(stderr,"OEM_ASSERT condition=%s tag=%s\n",c?c:"",tag?tag:"");
    if(fmt) { va_list ap;va_start(ap,fmt);vfprintf(stderr,fmt,ap);va_end(ap);fputc('\n',stderr); }
    abort();
}
void __android_log_write_log_message(struct __android_log_message *m) {
    __android_log_print(m->priority,m->tag,"%s",m->message?m->message:"");
}

// The diagnostic only needs local HBP worker threads, not a new system HAL.
// Do not publish a replacement AIDL service into the running Recovery.
int AServiceManager_addService(void *binder,const char *instance) {
    (void)binder;fprintf(stderr,"DIAGNOSTIC: AIDL publication suppressed: %s\n",instance);return 0;
}
void ABinderProcess_joinThreadPool(void) {
    fprintf(stderr,"DIAGNOSTIC: main thread waiting for bounded supervisor\n");
    for(;;) pause();
}
