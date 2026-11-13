/* Read AVB property descriptors from the running slot. No write or AVB policy change. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t n6_be64(const unsigned char *p) {
    uint64_t n = 0;
    for (int i = 0; i < 8; ++i) n = (n << 8) | p[i];
    return n;
}

static int n6_vbmeta_prop(const char *path, const char *key, char *out, size_t capacity) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    unsigned char header[256];
    unsigned char *desc = NULL;
    int ok = 0;
    if (fread(header, 1, sizeof(header), f) != sizeof(header) ||
        memcmp(header, "AVB0", 4)) goto done;
    const uint64_t auth = n6_be64(header + 12), aux = n6_be64(header + 20);
    const uint64_t offset = n6_be64(header + 96), size = n6_be64(header + 104);
    if (auth > (1u << 20) || aux > (1u << 20) ||
        offset > aux || !size || size > aux - offset) goto done;
    desc = malloc((size_t)size);
    if (!desc || fseek(f, (long)(sizeof(header) + auth + offset), SEEK_SET) ||
        fread(desc, 1, (size_t)size, f) != size) goto done;
    for (uint64_t pos = 0; size - pos >= 16;) {
        const unsigned char *d = desc + pos;
        const uint64_t tag = n6_be64(d), len = n6_be64(d + 8);
        if ((len & 7) || len > size - pos - 16) break;
        if (tag == 0 && len >= 18) {
            const uint64_t kn = n6_be64(d + 16), vn = n6_be64(d + 24);
            if (kn == strlen(key) && kn <= len - 18 &&
                vn < capacity && vn <= len - 18 - kn &&
                !memcmp(d + 32, key, (size_t)kn) && d[32 + kn] == 0 &&
                d[33 + kn + vn] == 0 && !memchr(d + 33 + kn, 0, (size_t)vn)) {
                memcpy(out, d + 33 + kn, (size_t)vn);
                out[vn] = 0;
                ok = 1;
                break;
            }
        }
        pos += len + 16;
    }
done:
    free(desc);
    fclose(f);
    return ok;
}
