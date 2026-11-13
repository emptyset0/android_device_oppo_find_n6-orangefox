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

static void prepare_firmware_properties(void) {
    char slot[PROP_VALUE_MAX] = {0};
    __system_property_get("ro.boot.slot_suffix", slot);
    if (strcmp(slot, "_a") && strcmp(slot, "_b")) {
        fputs("N6: cannot identify the running slot; keeping existing private KeyMint properties\n", stderr);
        return;
    }
    const char *byname = "/dev/block/bootdevice/by-name/";
    const struct {
        const char *partition, *key, *property;
        bool date;
    } props[] = {
        {"vbmeta", "com.android.build.boot.security_patch", "ro.build.version.security_patch", true},
        {"vbmeta_vendor", "com.android.build.vendor.security_patch", "ro.vendor.build.security_patch", true},
        {"vbmeta", "com.android.build.boot.os_version", "ro.build.version.release", false},
    };
    for (size_t i = 0; i < sizeof(props) / sizeof(props[0]); ++i) {
        char path[128], value[PROP_VALUE_MAX] = {0};
        snprintf(path, sizeof(path), "%s%s%s", byname, props[i].partition, slot);
        bool valid = n6_vbmeta_prop(path, props[i].key, value, sizeof(value));
        if (props[i].date)
            valid = valid && strlen(value) == 10 && value[4] == '-' && value[7] == '-' &&
                    strspn(value, "0123456789-") == 10;
        else
            valid = valid && value[0] >= '0' && value[0] <= '9' &&
                    strspn(value, "0123456789.") == strlen(value);
        if (!valid || !set_private_prop(props[i].property, value))
            fprintf(stderr, "N6: no usable %s from %s; leaving the existing property\n",
                    props[i].key, props[i].partition);
    }
}
