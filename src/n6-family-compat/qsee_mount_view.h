#ifndef N6_QSEE_MOUNT_VIEW_H
#define N6_QSEE_MOUNT_VIEW_H

#include <errno.h>
#include <sched.h>
#include <sys/mount.h>

static int n6_prepare_qsee_mount_view(const char *const *paths) {
    if (unshare(CLONE_NEWNS) ||
        mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL))
        return -1;

    /* Detach only inside the private copy, including any stacked mounts. */
    for (; *paths; ++paths) {
        while (umount2(*paths, MNT_DETACH | UMOUNT_NOFOLLOW) == 0) {}
        if (errno != EINVAL && errno != ENOENT) return -1;
    }
    return 0;
}

#endif
