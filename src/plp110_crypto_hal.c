/* OEM HAL launch adapter. All services use Fox's single platform Binder. */
#define _GNU_SOURCE
#include <android/binder_ibinder.h>
#include <dlfcn.h>
#include <errno.h>
#include <grp.h>
#include <linux/capability.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/system_properties.h>
#include <sys/wait.h>
#include <unistd.h>

#include "n6-family-compat/firmware_props.h"
#include "n6-family-compat/firmware_props_android.inc"

#define ASSETS "/system/libexec/plp110-fbe"
#define RUN "/tmp/plp110-crypto"

static int fail(const char *what) { perror(what); return 1; }

static int private_mounts(void) {
    return unshare(CLONE_NEWNS) || mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL);
}

static int use_firmware_properties(void) {
    return mount(RUN "/properties", "/dev/__properties__", NULL, MS_BIND, NULL);
}

static int wait_service(const char *service) {
    void *lib = dlopen("libbinder_ndk.so", RTLD_NOW);
    AIBinder *(*check)(const char *) = lib ? dlsym(lib, "AServiceManager_checkService") : NULL;
    if (!check) return fail("platform Binder service lookup");
    for (int i = 0; i < 100; ++i) {
        AIBinder *binder = check(service);
        if (binder) { AIBinder_decStrong(binder); dlclose(lib); return 0; }
        usleep(100000);
    }
    dlclose(lib);
    fprintf(stderr, "PLP110 HAL unavailable: %s\n", service);
    return 1;
}

/* The ramdisk and OEM use different ownership for these same kernel devices.
 * Bind aliases only inside the OEM HAL view; leave Recovery's nodes unchanged. */
static int node(const char *path, uid_t uid, gid_t gid, mode_t mode) {
    struct stat st;
    if (stat(path, &st) || !S_ISCHR(st.st_mode)) return fail(path);
    char dir[] = RUN "/node.XXXXXX", alias[sizeof(dir) + sizeof("/device")];
    if (!mkdtemp(dir)) return fail("mkdtemp HAL node");
    snprintf(alias, sizeof(alias), "%s/device", dir);
    int rc = mknod(alias, S_IFCHR | mode, st.st_rdev) || chown(alias, uid, gid) ||
             chmod(alias, mode) || mount(alias, path, NULL, MS_BIND, NULL);
    if (rc) perror(path);
    unlink(alias);
    rmdir(dir);
    return rc;
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    if (!strcmp(argv[1], "prepare-properties")) {
        if (private_mounts() || use_firmware_properties())
            return fail("PLP110 firmware properties");
        prepare_firmware_properties();
        return 0;
    }
    if (!strcmp(argv[1], "publish-keymint")) {
        if (wait_service("android.hardware.security.keymint.IKeyMintDevice/default"))
            return 1;
        return __system_property_set("sys.plp110.keymint", "ready");
    }
    const char *program;
    uid_t uid = 1000;
    gid_t groups[] = {1000, 1026};
    size_t group_count = 2;
    bool alarm = false, keymint = false, nfc = false, omapi = false;
    if (!strcmp(argv[1], "keymint")) {
        program = "/vendor/bin/hw/android.hardware.security.onekeymint-service-qti";
        keymint = true;
    } else if (!strcmp(argv[1], "ese")) {
        program = "/odm/bin/hw/android.hardware.secure_element-service.qti";
        groups[1] = 1027;
        nfc = true;
    } else if (!strcmp(argv[1], "omapi")) {
        program = ASSETS "/omapi/se_omapi";
        uid = groups[0] = 0;
        group_count = 1;
        omapi = true;
    } else if (!strcmp(argv[1], "weaver")) {
        if (wait_service("android.se.omapi.ISecureElementService/default")) return 1;
        program = "/odm/bin/hw/android.hardware.weaver-service.nxp";
        uid = 2911;
        alarm = nfc = true;
    } else {
        return 2;
    }

    if (private_mounts() ||
        mount(ASSETS "/vendor", "/vendor", NULL, MS_BIND, NULL) ||
        mount(ASSETS "/odm", "/odm", NULL, MS_BIND, NULL) ||
        mount(RUN "/firmware", "/vendor/firmware_mnt", NULL, MS_BIND, NULL) ||
        mount("/dev/binder", "/dev/vndbinder", NULL, MS_BIND, NULL))
        return fail("PLP110 OEM HAL filesystem");

    /* Recovery owns /metadata. Do not pin its filesystem in the private
     * view after Recovery unmounts it for Format Data. */
    if (umount2("/metadata", UMOUNT_NOFOLLOW) && errno != EINVAL && errno != ENOENT)
        return fail("PLP110 private /metadata unmount");

    if (node("/dev/smcinvoke", 1000, 1026, 0660) ||
        (nfc && (node("/dev/nq-nci", 1027, 1027, 0660) ||
                 node("/dev/dma_heap/qcom,qseecom-ta", 1000, 1000, 0444) ||
                 node("/dev/dma_heap/qcom,qseecom", 1000, 1000, 0444)))) return 1;

    if (keymint && (use_firmware_properties() ||
                    system("/system/bin/resetprop -n vendor.sys.listeners.registered true")))
        return fail("PLP110 KeyMint firmware properties");

    if ((alarm && prctl(PR_SET_KEEPCAPS, 1L, 0L, 0L, 0L)) ||
        prctl(PR_SET_NO_NEW_PRIVS, 1L, 0L, 0L, 0L) ||
        setgroups(group_count, groups) || setgid(uid) || setuid(uid))
        return fail("OEM HAL uid/gid");
    if (alarm) {
        struct __user_cap_header_struct header = {_LINUX_CAPABILITY_VERSION_3, 0};
        struct __user_cap_data_struct caps[2] = {{0}, {0}};
        caps[CAP_WAKE_ALARM / 32].effective = 1U << (CAP_WAKE_ALARM % 32);
        caps[CAP_WAKE_ALARM / 32].permitted = 1U << (CAP_WAKE_ALARM % 32);
        caps[CAP_WAKE_ALARM / 32].inheritable = 1U << (CAP_WAKE_ALARM % 32);
        if (syscall(SYS_capset, &header, caps) ||
            prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, CAP_WAKE_ALARM, 0L, 0L))
            return fail("OEM HAL WAKE_ALARM");
    }
    char *const args[] = {"/system/bin/linker64", (char *)program, NULL};
    char *const env[] = {
        "PATH=/system/bin:/vendor/bin", "ANDROID_ROOT=/system", "ANDROID_DATA=/data",
        omapi ? "LD_LIBRARY_PATH=" ASSETS "/omapi:/system/lib64:/odm/lib64:/odm/lib64/hw:/vendor/lib64:/vendor/lib64/hw"
              : "LD_LIBRARY_PATH=/system/lib64:/odm/lib64:/odm/lib64/hw:/vendor/lib64:/vendor/lib64/hw",
        NULL,
    };
    execve(args[0], args, env);
    return fail(program);
}
