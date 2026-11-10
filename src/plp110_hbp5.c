#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/audit.h>
#include <linux/capability.h>
#include <linux/filter.h>
#include <linux/input.h>
#include <linux/seccomp.h>
#include <sched.h>
#include <signal.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/system_properties.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static const char *root = "/system/libexec/plp110-hbp5";
static volatile sig_atomic_t interrupted;
static void on_signal(int s) { interrupted = s; }
static void fail(const char *s) { perror(s); _exit(111); }
static void bind_path(const char *source, const char *target, int directory, int ro) {
    char path[512];
    if (snprintf(path,sizeof(path),"%s%s",root,target) >= (int)sizeof(path)) fail("path size");
    if (directory) { if(mkdir(path,0755) && errno!=EEXIST) fail(path); }
    else { int f=open(path,O_WRONLY|O_CREAT|O_CLOEXEC,0600); if(f<0) fail(path); close(f); }
    if(mount(source,path,NULL,MS_BIND,NULL)) fail(source);
    if(ro && mount(NULL,path,NULL,MS_BIND|MS_REMOUNT|MS_RDONLY|MS_NOSUID,NULL)) fail("readonly bind");
}
static void restrict_syscalls(void) {
#define DENY(n) BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,(n),0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM)
    struct sock_filter f[] = {
        BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,arch)),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,AUDIT_ARCH_AARCH64,1,0),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_KILL_PROCESS),
        BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,nr)),
        DENY(__NR_mount), DENY(__NR_umount2), DENY(__NR_pivot_root), DENY(__NR_chroot),
        DENY(__NR_reboot), DENY(__NR_kexec_load), DENY(__NR_ptrace), DENY(__NR_bpf),
        DENY(__NR_process_vm_writev), DENY(__NR_open_by_handle_at), DENY(__NR_mknodat),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)
    };
    struct sock_fprog p = {sizeof(f)/sizeof(f[0]),f};
    struct __user_cap_header_struct h={_LINUX_CAPABILITY_VERSION_3,0};
    struct __user_cap_data_struct c[2]={{0},{0}};
    if(syscall(SYS_capset,&h,c)) fail("drop caps");
    if(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)) fail("no new privs");
    if(prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&p)) fail("seccomp");
}
// Share only Recovery's own ramdisk files, in the child's private mount
// namespace. Keep the small OEM-only library set local to the touch root.
static void share_runtime(void) {
    char config[512], path[256];
    if(snprintf(config,sizeof(config),"%s/runtime-share.list",root)>=(int)sizeof(config))
        fail("runtime config path");
    FILE *f=fopen(config,"re");
    if(!f) { if(errno==ENOENT)return; fail("runtime share list"); }
    int count=0;
    while(fgets(path,sizeof(path),f)) {
        size_t n=strcspn(path,"\r\n");path[n]=0;
        if(!n||path[0]=='#')continue;
        if(strstr(path,"..") ||
           (strncmp(path,"/system/lib64/",14)&&strcmp(path,"/system/bin/linker64")))
            fail("runtime share entry");
        bind_path(path,path,0,1);
        count++;
    }
    if(ferror(f))fail("read runtime share list");
    fclose(f);
    fprintf(stderr,"PLP110 HBP: shared %d Recovery ramdisk runtime files\n",count);
}
static void child(int list) {
    if(setpgid(0,0)) fail("child group");
    if(unshare(CLONE_NEWNS)) fail("private namespace");
    if(mount(NULL,"/",NULL,MS_REC|MS_PRIVATE,NULL)) fail("private propagation");
    share_runtime();
    const char *nodes[]={"null","zero","random","urandom","binder","hwbinder","vndbinder","hbp_core","hbp0","hbp1",NULL};
    for(int i=0;nodes[i];i++) {
        char p[128]; snprintf(p,sizeof(p),"/dev/%s",nodes[i]);
        bind_path(p,p,0,0);
    }
    bind_path("/dev/__properties__","/dev/__properties__",1,1);
    bind_path("/dev/input","/dev/input",1,1);
    bind_path("/proc/bus/input/devices","/proc/bus/input/devices",0,1);
    // sysfs is visible read-only. No /dev/block, full /proc, host /data,
    // property-service socket, or partition mount is exposed inside the chroot.
    bind_path("/sys","/sys",1,1);
    if(chroot(root) || chdir("/")) fail("chroot");
    restrict_syscalls();
    setenv("LD_LIBRARY_PATH","/odm/lib64:/system/lib64",1);
    setenv("LD_PRELOAD","/odm/lib64/libhbp5_log_stderr.so",1);
    setenv("TMPDIR","/tmp",1);
    char *run[]={"/system/bin/linker64","/odm/bin/hw/vendor-oplus-hardware-touch-V2-hbp5-service",NULL};
    char *ldd[]={"/system/bin/linker64","--list","/odm/bin/hw/vendor-oplus-hardware-touch-V2-hbp5-service",NULL};
    execv(run[0],list?ldd:run); fail("exec linker");
}
static void disable_irq(int id) {
    int f=open("/sys/devices/platform/soc/soc:hbp@0/irq_enable",O_WRONLY|O_CLOEXEC);
    if(f<0) { perror("restore irq open"); return; }
    char b[16]; int n=snprintf(b,sizeof(b),"%d,0\n",id);
    if(write(f,b,n)!=n) perror("restore irq write");
    close(f);
}
// Keep the unselected panel quiet without changing controller firmware or
// running either fold sensors or a UI-switch daemon. Only write on state=1.
static void enforce_inactive_irq(int id) {
    int f=open("/sys/devices/platform/soc/soc:hbp@0/irq_enable",O_RDONLY|O_CLOEXEC);
    if(f<0)return;
    char b[256]={0},wanted[80];ssize_t n=read(f,b,sizeof(b)-1);close(f);
    if(n<=0)return;
    snprintf(wanted,sizeof(wanted),"devices id %d, irq_enabled state 1",id);
    if(strstr(b,wanted))disable_irq(id);
}
int main(int argc,char **argv) {
    char mode[PROP_VALUE_MAX]={0}, project[PROP_VALUE_MAX]={0};
    int list=argc==2 && !strcmp(argv[1],"--list");
    int serve=argc==2 && !strcmp(argv[1],"--serve");
    if ((!list&&!serve)||getuid()!=0||
        __system_property_get("ro.bootmode",mode)<=0||strcmp(mode,"recovery")||
        __system_property_get("ro.boot.prjname",project)<=0||strcmp(project,"25001"))return 2;
    const char *required[]={"/dev/hbp_core","/dev/hbp0","/dev/hbp1","/dev/binder","/dev/hwbinder","/dev/vndbinder","/dev/input","/dev/__properties__",NULL};
    int ready=0;
    for(int attempt=0;attempt<300;attempt++) {
        ready=1;
        for(int i=0;required[i];i++)if(access(required[i],F_OK)){ready=0;break;}
        if(ready)break;
        usleep(100000);
    }
    if(!ready){fputs("PLP110 HBP: required device missing; leaving UI/ADB intact\n",stderr);return 3;}
    signal(SIGTERM,on_signal);signal(SIGINT,on_signal);signal(SIGHUP,on_signal);
    pid_t p=fork();if(p<0)fail("fork");if(!p)child(list);setpgid(p,p);
    int status=0,done=0;
    char inner[PROP_VALUE_MAX]={0};
    __system_property_get("vendor.recovery.inner.enable",inner);
    int inactive=strcmp(inner,"1")==0?1:0;
    time_t last_policy=0;
    struct timespec start,now;clock_gettime(CLOCK_MONOTONIC,&start);
    if(serve)__system_property_set("vendor.recovery.touch.bridge","worker-running");
    while(!interrupted) {
        pid_t r=waitpid(p,&status,WNOHANG);
        if(r==p){done=1;break;}
        if(r<0&&errno!=EINTR)break;
        clock_gettime(CLOCK_MONOTONIC,&now);
        if(list&&now.tv_sec-start.tv_sec>=10)break;
        if(serve&&now.tv_sec-start.tv_sec>=4&&now.tv_sec!=last_policy) {
            enforce_inactive_irq(inactive);last_policy=now.tv_sec;
        }
        usleep(100000);
    }
    if(!done){kill(-p,SIGTERM);usleep(200000);kill(-p,SIGKILL);while(waitpid(p,&status,0)<0&&errno==EINTR){}}
    if(serve){disable_irq(0);disable_irq(1);__system_property_set("vendor.recovery.touch.bridge","stopped");}
    if(interrupted)return 0;
    return done&&WIFEXITED(status)?WEXITSTATUS(status):1;
}
