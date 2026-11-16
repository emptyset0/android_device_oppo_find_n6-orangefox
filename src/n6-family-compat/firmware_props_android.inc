static int set_private_prop(const char *name, const char *value) {
    int status = -1;
    const pid_t pid = fork();
    if (pid == 0) {
        execl("/system/bin/resetprop", "resetprop", "-n", name, value, (char *)NULL);
        _exit(127);
    }
    return pid > 0 && TEMP_FAILURE_RETRY(waitpid(pid, &status, 0)) == pid &&
           WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

/* X9U prepdecrypt likewise takes the OS release from the installed system. */
static int read_system_release(FILE *f, char *out, size_t capacity) {
    static const char *const keys[] = {
        "ro.build.version.release=", "ro.system.build.version.release=",
    };
    char *line = NULL;
    size_t space = 0;
    ssize_t len;
    bool found = false, valid = true;
    while ((len = getline(&line, &space, f)) >= 0) {
        for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
            size_t prefix = strlen(keys[i]);
            if ((size_t)len < prefix || memcmp(line, keys[i], prefix)) continue;
            size_t end = (size_t)len;
            if (end > prefix && line[end - 1] == '\n') --end;
            if (end > prefix && line[end - 1] == '\r') --end;
            size_t n = end - prefix;
            const char *value = line + prefix;
            if (!n || n >= capacity) { valid = false; break; }
            bool digit = false;
            for (size_t j = 0; j < n; ++j) {
                if (value[j] >= '0' && value[j] <= '9') digit = true;
                else if (value[j] == '.' && digit) digit = false;
                else { valid = false; break; }
            }
            if (!digit || (found && (strlen(out) != n || memcmp(out, value, n))))
                valid = false;
            if (valid) {
                memcpy(out, value, n);
                out[n] = '\0';
                found = true;
            }
            break;
        }
        if (!valid) break;
    }
    free(line);
    return valid && found && feof(f) && !ferror(f);
}

static int system_release(const char *slot, char *out, size_t capacity) {
    char dev[64];
    char dir[] = RUN "/system-version.XXXXXX";
    snprintf(dev, sizeof(dev), "/dev/block/mapper/system%s", slot);
    if (!mkdtemp(dir)) return 0;
    bool found = false;
    const unsigned long flags = MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC;
    /* MS_RDONLY alone does not prevent ext4 journal replay. */
    /* Fox creates the active mapper asynchronously after post-fs-data.
     * Retry the actual mount: a block node can exist before it is ready. */
    bool mounted = false;
    for (int attempt = 0; attempt < 50 && !mounted; ++attempt) {
        mounted = !mount(dev, dir, "erofs", flags, NULL) ||
                  !mount(dev, dir, "ext4", flags, "noload");
        if (!mounted) usleep(100000);
    }
    if (!mounted) {
        perror("N6: read-only system mount");
        goto done;
    }
    const int file_flags = O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK;
    int root = open(dir, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root >= 0) {
        int system = openat(root, "system", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        int fd = system >= 0 ? openat(system, "build.prop", file_flags) : -1;
        if (fd < 0 && errno == ENOENT) fd = openat(root, "build.prop", file_flags);
        if (system >= 0) close(system);
        close(root);
        if (fd >= 0) {
            struct stat st;
            FILE *f = !fstat(fd, &st) && S_ISREG(st.st_mode) ? fdopen(fd, "r") : NULL;
            if (f) {
                found = read_system_release(f, out, capacity);
                fclose(f);
            } else close(fd);
        }
    }
    if (umount2(dir, UMOUNT_NOFOLLOW)) {
        perror("N6: read-only system unmount");
        found = false;
    }
done:
    rmdir(dir);
    return found;
}

static int prepare_firmware_properties(void) {
    char slot[PROP_VALUE_MAX] = {0};
    __system_property_get("ro.boot.slot_suffix", slot);
    if (strcmp(slot, "_a") && strcmp(slot, "_b")) {
        fputs("N6: cannot identify the running slot; keeping existing private KeyMint properties\n", stderr);
        return 1;
    }
    const char *byname = "/dev/block/bootdevice/by-name/";
    const struct {
        const char *partition, *key, *property;
    } props[] = {
        {"vbmeta", "com.android.build.boot.security_patch", "ro.build.version.security_patch"},
        {"vbmeta_vendor", "com.android.build.vendor.security_patch", "ro.vendor.build.security_patch"},
    };
    for (size_t i = 0; i < sizeof(props) / sizeof(props[0]); ++i) {
        char path[128], value[PROP_VALUE_MAX] = {0};
        snprintf(path, sizeof(path), "%s%s%s", byname, props[i].partition, slot);
        bool valid = n6_vbmeta_prop(path, props[i].key, value, sizeof(value));
        valid = valid && strlen(value) == 10 && value[4] == '-' && value[7] == '-' &&
                strspn(value, "0123456789-") == 10;
        if (!valid || !set_private_prop(props[i].property, value))
            fprintf(stderr, "N6: no usable %s from %s; leaving the existing property\n",
                    props[i].key, props[i].partition);
    }
    char release[PROP_VALUE_MAX] = {0};
    if (!system_release(slot, release, sizeof(release)) ||
        !set_private_prop("ro.build.version.release", release)) {
        fputs("N6: cannot read installed system release for private KeyMint; refusing a boot-version fallback\n", stderr);
        return 1;
    }
    fprintf(stderr, "N6: private KeyMint system release=%s\n", release);
    return 0;
}
