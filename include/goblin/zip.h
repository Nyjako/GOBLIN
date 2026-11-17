/*
    GOBLIN by Kacper Tucholski
    https://github.com/Nyjako/GOBLIN

    zip.h — a minimal, STORE-only (uncompressed) ZIP reader/writer: a flat
    list of named entries, no DEFLATE, no encryption, no Zip64. Not a
    general-purpose zip library -- for reading/writing a small,
    uncompressed archive without vendoring a compression library.

    Unlike every other header here, this one is not standalone: it
    includes dyn_array.h.
*/

#ifndef GOBLIN_ZIP_H
#define GOBLIN_ZIP_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct goblin_zip_writer goblin_zip_writer_t;

/* NULL if `path` can't be opened for writing. */
goblin_zip_writer_t *goblin_zip_writer_open(const char *path);

/*
    Appends one stored entry under `name` (its path inside the archive,
    e.g. "META-INF/container.xml" -- pass it already using '/'
    separators). Returns false on I/O or allocation failure; the writer
    is left open but unusable.
*/
bool goblin_zip_writer_add(goblin_zip_writer_t *zip, const char *name, const unsigned char *data, size_t len);

/*
    Writes the central directory and closes the file, then frees `zip`
    either way. Returns false if this or any earlier add call failed.
*/
bool goblin_zip_writer_close(goblin_zip_writer_t *zip);

typedef struct goblin_zip_reader goblin_zip_reader_t;

/*
    Reads the whole file into memory and parses its central directory
    (scanning back from the end for the EOCD signature, to tolerate a
    trailing comment). NULL on any failure: not found, truncated, no
    valid EOCD record, or more than one disk.
*/
goblin_zip_reader_t *goblin_zip_reader_open(const char *path);

/*
    Looks up `name` and returns a pointer into the reader's own buffer
    (valid only until goblin_zip_reader_close -- copy it to keep it
    longer) plus its length via *out_len. NULL if `name` is missing,
    wasn't stored uncompressed, or fails its CRC-32 check.
*/
const unsigned char *goblin_zip_reader_get(goblin_zip_reader_t *zip, const char *name, size_t *out_len);

/* Number of entries in the archive. */
size_t goblin_zip_reader_entry_count(goblin_zip_reader_t *zip);

/* The name of the entry at `index` (0 .. entry_count - 1), or NULL if
    index is out of range. */
const char *goblin_zip_reader_entry_name(goblin_zip_reader_t *zip, size_t index);

void goblin_zip_reader_close(goblin_zip_reader_t *zip);

/*
    Extracts every entry from `zip_path` into `dest_dir` (parent
    directories created as needed). An entry name ending in '/' just
    creates that directory.

    Rejects the whole call before writing anything if any entry name is
    empty, absolute, or has a ".." segment -- without this check a
    crafted archive could write outside dest_dir ("Zip Slip"). Also
    fails on a CRC-32 mismatch or I/O error. Not transactional: entries
    already written before a later failure stay on disk.
*/
bool goblin_zip_extract_all(const char *zip_path, const char *dest_dir);

#ifdef __cplusplus
}
namespace goblin {
    static inline goblin_zip_writer_t *zip_writer_open(const char *path) { return goblin_zip_writer_open(path); }
    static inline bool zip_writer_add(goblin_zip_writer_t *zip, const char *name, const unsigned char *data, size_t len) { return goblin_zip_writer_add(zip, name, data, len); }
    static inline bool zip_writer_close(goblin_zip_writer_t *zip) { return goblin_zip_writer_close(zip); }
    static inline goblin_zip_reader_t *zip_reader_open(const char *path) { return goblin_zip_reader_open(path); }
    static inline const unsigned char *zip_reader_get(goblin_zip_reader_t *zip, const char *name, size_t *out_len) { return goblin_zip_reader_get(zip, name, out_len); }
    static inline size_t zip_reader_entry_count(goblin_zip_reader_t *zip) { return goblin_zip_reader_entry_count(zip); }
    static inline const char *zip_reader_entry_name(goblin_zip_reader_t *zip, size_t index) { return goblin_zip_reader_entry_name(zip, index); }
    static inline void zip_reader_close(goblin_zip_reader_t *zip) { goblin_zip_reader_close(zip); }
    static inline bool zip_extract_all(const char *zip_path, const char *dest_dir) { return goblin_zip_extract_all(zip_path, dest_dir); }
}
#endif

#ifdef GOBLIN_ZIP_IMPLEMENTATION

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dyn_array.h"

/* ---- CRC-32 (reflected, polynomial 0xEDB88320 -- the standard ZIP/PNG/
 * gzip variant) -- small and self-contained, no vendored dependency. ---- */

static uint32_t goblin_zip_crc32_table[256];
static bool goblin_zip_crc32_table_ready = false;

static void goblin_zip_crc32_init_table(void)
{
    if (goblin_zip_crc32_table_ready) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) {
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        goblin_zip_crc32_table[i] = c;
    }
    goblin_zip_crc32_table_ready = true;
}

static uint32_t goblin_zip_crc32_compute(const unsigned char *data, size_t len)
{
    goblin_zip_crc32_init_table();
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        c = goblin_zip_crc32_table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFu;
}

/* ---- little-endian field helpers ---- */

static void goblin_zip_put_u16(unsigned char *p, uint16_t v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
}

static void goblin_zip_put_u32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

static uint16_t goblin_zip_get_u16(const unsigned char *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t goblin_zip_get_u32(const unsigned char *p)
{
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24));
}

/* Fixed DOS date/time (1980-01-01) -- valid but meaningless; this
 * writer doesn't track real mtimes. */
#define GOBLIN_ZIP_DOS_TIME 0u
#define GOBLIN_ZIP_DOS_DATE 0x21u

/* ---- writer ---- */

typedef struct
{
    char *name;
    uint32_t crc;
    uint32_t size;
    uint32_t offset; /* where this entry's local header starts */
} goblin_zip_writer_entry_t;

GOBLIN_DYN_ARRAY_DECL(goblin_zip_writer_entry, goblin_zip_writer_entry_t)

struct goblin_zip_writer
{
    FILE *f;
    bool ok;
    goblin_zip_writer_entry_array entries;
};

goblin_zip_writer_t *goblin_zip_writer_open(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return NULL;

    goblin_zip_writer_t *zip = (goblin_zip_writer_t *)calloc(1, sizeof(*zip));
    if (!zip) {
        fclose(f);
        return NULL;
    }
    zip->f = f;
    zip->ok = true;
    goblin_zip_writer_entry_array_init(&zip->entries);
    return zip;
}

bool goblin_zip_writer_add(goblin_zip_writer_t *zip, const char *name, const unsigned char *data, size_t len)
{
    if (!zip || !zip->ok || !name) {
        if (zip) zip->ok = false;
        return false;
    }

    long offset_long = ftell(zip->f);
    if (offset_long < 0) {
        zip->ok = false;
        return false;
    }
    uint32_t offset = (uint32_t)offset_long;
    uint32_t crc = goblin_zip_crc32_compute(data, len);
    uint16_t name_len = (uint16_t)strlen(name);

    unsigned char header[30];
    goblin_zip_put_u32(header + 0, 0x04034b50u);
    goblin_zip_put_u16(header + 4, 20); /* version needed to extract */
    goblin_zip_put_u16(header + 6, 0);  /* general purpose bit flag */
    goblin_zip_put_u16(header + 8, 0);  /* compression method: stored */
    goblin_zip_put_u16(header + 10, GOBLIN_ZIP_DOS_TIME);
    goblin_zip_put_u16(header + 12, GOBLIN_ZIP_DOS_DATE);
    goblin_zip_put_u32(header + 14, crc);
    goblin_zip_put_u32(header + 18, (uint32_t)len);
    goblin_zip_put_u32(header + 22, (uint32_t)len);
    goblin_zip_put_u16(header + 26, name_len);
    goblin_zip_put_u16(header + 28, 0); /* extra field length */

    if (fwrite(header, 1, sizeof(header), zip->f) != sizeof(header) || fwrite(name, 1, name_len, zip->f) != name_len || (len > 0 && fwrite(data, 1, len, zip->f) != len)) {
        zip->ok = false;
        return false;
    }

    char *name_copy = (char *)malloc((size_t)name_len + 1);
    if (!name_copy) {
        zip->ok = false;
        return false;
    }
    memcpy(name_copy, name, (size_t)name_len + 1);

    goblin_zip_writer_entry_t entry;
    entry.name = name_copy;
    entry.crc = crc;
    entry.size = (uint32_t)len;
    entry.offset = offset;
    if (!goblin_zip_writer_entry_array_push(&zip->entries, entry)) {
        free(name_copy);
        zip->ok = false;
        return false;
    }
    return true;
}

bool goblin_zip_writer_close(goblin_zip_writer_t *zip)
{
    if (!zip) return false;
    bool ok = zip->ok;

    if (ok) {
        long cd_start_long = ftell(zip->f);
        if (cd_start_long < 0) {
            ok = false;
        } else {
            uint32_t cd_start = (uint32_t)cd_start_long;
            for (size_t i = 0; ok && i < zip->entries.count; i++) {
                goblin_zip_writer_entry_t *e = &zip->entries.items[i];
                uint16_t name_len = (uint16_t)strlen(e->name);
                unsigned char record[46];
                goblin_zip_put_u32(record + 0, 0x02014b50u);
                goblin_zip_put_u16(record + 4, 20); /* version made by */
                goblin_zip_put_u16(record + 6, 20); /* version needed to extract */
                goblin_zip_put_u16(record + 8, 0);  /* general purpose bit flag */
                goblin_zip_put_u16(record + 10, 0); /* compression method: stored */
                goblin_zip_put_u16(record + 12, GOBLIN_ZIP_DOS_TIME);
                goblin_zip_put_u16(record + 14, GOBLIN_ZIP_DOS_DATE);
                goblin_zip_put_u32(record + 16, e->crc);
                goblin_zip_put_u32(record + 20, e->size);
                goblin_zip_put_u32(record + 24, e->size);
                goblin_zip_put_u16(record + 28, name_len);
                goblin_zip_put_u16(record + 30, 0); /* extra field length */
                goblin_zip_put_u16(record + 32, 0); /* file comment length */
                goblin_zip_put_u16(record + 34, 0); /* disk number start */
                goblin_zip_put_u16(record + 36, 0); /* internal file attributes */
                goblin_zip_put_u32(record + 38, 0); /* external file attributes */
                goblin_zip_put_u32(record + 42, e->offset);
                if (fwrite(record, 1, sizeof(record), zip->f) != sizeof(record) || fwrite(e->name, 1, name_len, zip->f) != name_len) {
                    ok = false;
                }
            }

            if (ok) {
                long cd_end_long = ftell(zip->f);
                if (cd_end_long < 0) {
                    ok = false;
                } else {
                    uint32_t cd_size = (uint32_t)(cd_end_long - (long)cd_start);
                    unsigned char eocd[22];
                    goblin_zip_put_u32(eocd + 0, 0x06054b50u);
                    goblin_zip_put_u16(eocd + 4, 0); /* number of this disk */
                    goblin_zip_put_u16(eocd + 6, 0); /* disk where central directory starts */
                    goblin_zip_put_u16(eocd + 8, (uint16_t)zip->entries.count);
                    goblin_zip_put_u16(eocd + 10, (uint16_t)zip->entries.count);
                    goblin_zip_put_u32(eocd + 12, cd_size);
                    goblin_zip_put_u32(eocd + 16, cd_start);
                    goblin_zip_put_u16(eocd + 20, 0); /* comment length */
                    if (fwrite(eocd, 1, sizeof(eocd), zip->f) != sizeof(eocd)) {
                        ok = false;
                    }
                }
            }
        }
    }

    if (fclose(zip->f) != 0) {
        ok = false;
    }
    for (size_t i = 0; i < zip->entries.count; i++) {
        free(zip->entries.items[i].name);
    }
    goblin_zip_writer_entry_array_free(&zip->entries);
    free(zip);
    return ok;
}

/* ---- reader ---- */

typedef struct
{
    char *name;
    uint32_t offset; /* local header offset */
    uint32_t size;
    uint32_t crc;
    uint16_t method;
} goblin_zip_reader_entry_t;

GOBLIN_DYN_ARRAY_DECL(goblin_zip_reader_entry, goblin_zip_reader_entry_t)

struct goblin_zip_reader
{
    unsigned char *buf;
    size_t buf_len;
    goblin_zip_reader_entry_array entries;
};

static long goblin_zip_file_size(FILE *f)
{
    if (fseek(f, 0, SEEK_END) != 0) return -1;
    long size = ftell(f);
    if (size < 0 || fseek(f, 0, SEEK_SET) != 0) return -1;
    return size;
}

/* Scans backward from the end of `buf` for the EOCD signature, within
 * the format's max comment length (65535 bytes). Returns its offset,
 * or (size_t)-1 if not found. */
static size_t goblin_zip_find_eocd(const unsigned char *buf, size_t size)
{
    if (size < 22) return (size_t)-1;
    size_t max_back = size - 22;
    size_t max_comment = 65535;
    size_t earliest = (max_back > max_comment) ? max_back - max_comment : 0;
    for (size_t pos = max_back;; pos--) {
        if (goblin_zip_get_u32(buf + pos) == 0x06054b50u) return pos;
        if (pos == earliest) break;
    }
    return (size_t)-1;
}

goblin_zip_reader_t *goblin_zip_reader_open(const char *path)
{
    if (!path) return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    long size = goblin_zip_file_size(f);
    if (size < 22) {
        fclose(f);
        return NULL;
    }
    unsigned char *buf = (unsigned char *)malloc((size_t)size);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);
    if (read != (size_t)size) {
        free(buf);
        return NULL;
    }

    size_t eocd_pos = goblin_zip_find_eocd(buf, (size_t)size);
    if (eocd_pos == (size_t)-1) {
        free(buf);
        return NULL;
    }
    const unsigned char *eocd = buf + eocd_pos;
    uint16_t disk_number = goblin_zip_get_u16(eocd + 4);
    uint16_t cd_disk = goblin_zip_get_u16(eocd + 6);
    if (disk_number != 0 || cd_disk != 0) {
        free(buf); /* multi-disk archives aren't supported */
        return NULL;
    }
    uint16_t entry_count = goblin_zip_get_u16(eocd + 10);
    uint32_t cd_offset = goblin_zip_get_u32(eocd + 16);
    if (cd_offset >= (size_t)size) {
        free(buf);
        return NULL;
    }

    goblin_zip_reader_t *zip = (goblin_zip_reader_t *)calloc(1, sizeof(*zip));
    if (!zip) {
        free(buf);
        return NULL;
    }
    goblin_zip_reader_entry_array_init(&zip->entries);
    if (entry_count > 0 && !goblin_zip_reader_entry_array_reserve(&zip->entries, entry_count)) {
        free(zip);
        free(buf);
        return NULL;
    }

    size_t pos = cd_offset;
    for (size_t parsed = 0; parsed < entry_count; parsed++) {
        if (pos + 46 > (size_t)size || goblin_zip_get_u32(buf + pos) != 0x02014b50u) {
            break;
        }
        uint16_t method = goblin_zip_get_u16(buf + pos + 10);
        uint32_t crc = goblin_zip_get_u32(buf + pos + 16);
        uint32_t uncompressed_size = goblin_zip_get_u32(buf + pos + 24);
        uint16_t name_len = goblin_zip_get_u16(buf + pos + 28);
        uint16_t extra_len = goblin_zip_get_u16(buf + pos + 30);
        uint16_t comment_len = goblin_zip_get_u16(buf + pos + 32);
        uint32_t local_offset = goblin_zip_get_u32(buf + pos + 42);
        if (pos + 46 + name_len > (size_t)size) {
            break;
        }
        char *name = (char *)malloc((size_t)name_len + 1);
        if (!name) break;
        memcpy(name, buf + pos + 46, name_len);
        name[name_len] = '\0';

        goblin_zip_reader_entry_t entry;
        entry.name = name;
        entry.offset = local_offset;
        entry.size = uncompressed_size;
        entry.crc = crc;
        entry.method = method;
        if (!goblin_zip_reader_entry_array_push(&zip->entries, entry)) {
            free(name);
            break;
        }
        pos += 46 + name_len + extra_len + comment_len;
    }

    zip->buf = buf;
    zip->buf_len = (size_t)size;
    return zip;
}

const unsigned char *goblin_zip_reader_get(goblin_zip_reader_t *zip, const char *name, size_t *out_len)
{
    if (!zip || !name || !out_len) return NULL;
    for (size_t i = 0; i < zip->entries.count; i++) {
        goblin_zip_reader_entry_t *e = &zip->entries.items[i];
        if (strcmp(e->name, name) != 0) continue;
        if (e->method != 0) return NULL; /* this reader only understands stored entries */
        if ((uint64_t)e->offset + 30 > zip->buf_len) return NULL;
        uint16_t local_name_len = goblin_zip_get_u16(zip->buf + e->offset + 26);
        uint16_t local_extra_len = goblin_zip_get_u16(zip->buf + e->offset + 28);
        uint64_t data_offset = (uint64_t)e->offset + 30 + local_name_len + local_extra_len;
        if (data_offset + e->size > zip->buf_len) return NULL;
        const unsigned char *data = zip->buf + data_offset;
        if (goblin_zip_crc32_compute(data, e->size) != e->crc) return NULL; /* corrupted or truncated entry */
        *out_len = e->size;
        return data;
    }
    return NULL;
}

size_t goblin_zip_reader_entry_count(goblin_zip_reader_t *zip)
{
    return zip ? zip->entries.count : 0;
}

const char *goblin_zip_reader_entry_name(goblin_zip_reader_t *zip, size_t index)
{
    if (!zip || index >= zip->entries.count) return NULL;
    return zip->entries.items[index].name;
}

void goblin_zip_reader_close(goblin_zip_reader_t *zip)
{
    if (!zip) return;
    for (size_t i = 0; i < zip->entries.count; i++) {
        free(zip->entries.items[i].name);
    }
    goblin_zip_reader_entry_array_free(&zip->entries);
    free(zip->buf);
    free(zip);
}

/* ---- extraction ("unzip") ---- */

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
#include <errno.h>

static bool goblin_zip_mkdir_one(const char *path)
{
#ifdef _WIN32
    if (_mkdir(path) == 0) return true;
#else
    if (mkdir(path, 0755) == 0) return true;
#endif
    return errno == EEXIST;
}

/* mkdir -p over `path`'s '/'-separated components (mutates `path`
 * in-place, restores it before returning). Self-contained rather than
 * reusing appdirs.h's mkdir_p, to avoid requiring that header's own
 * IMPLEMENTATION macro too. */
static bool goblin_zip_mkdir_p(char *path)
{
    size_t len = strlen(path);
    if (len == 0) return false;
    for (size_t i = 1; i < len; i++) {
        if (path[i] != '/') continue;
        path[i] = '\0';
        bool ok = goblin_zip_mkdir_one(path);
        path[i] = '/';
        if (!ok) return false;
    }
    return goblin_zip_mkdir_one(path);
}

/* False for an empty, absolute (leading '/', '\\', or a drive letter),
 * or ".."-containing name. See goblin_zip_extract_all for why. */
static bool goblin_zip_entry_name_is_safe(const char *name)
{
    if (!name || !*name) return false;
    if (name[0] == '/' || name[0] == '\\') return false;
    if (name[1] == ':') return false;

    size_t len = strlen(name);
    size_t seg_start = 0;
    for (size_t i = 0; i <= len; i++) {
        if (i != len && name[i] != '/' && name[i] != '\\') continue;
        size_t seg_len = i - seg_start;
        if (seg_len == 2 && name[seg_start] == '.' && name[seg_start + 1] == '.') return false;
        seg_start = i + 1;
    }
    return true;
}

bool goblin_zip_extract_all(const char *zip_path, const char *dest_dir)
{
    if (!zip_path || !dest_dir || !*dest_dir) return false;

    goblin_zip_reader_t *zip = goblin_zip_reader_open(zip_path);
    if (!zip) return false;

    bool ok = true;
    size_t dest_dir_len = strlen(dest_dir);
    size_t count = goblin_zip_reader_entry_count(zip);

    for (size_t i = 0; ok && i < count; i++) {
        const char *name = goblin_zip_reader_entry_name(zip, i);
        if (!goblin_zip_entry_name_is_safe(name)) {
            ok = false;
            break;
        }

        size_t name_len = strlen(name);
        bool is_dir_marker = name[name_len - 1] == '/';

        char *full_path = (char *)malloc(dest_dir_len + 1 + name_len + 1);
        if (!full_path) {
            ok = false;
            break;
        }
        memcpy(full_path, dest_dir, dest_dir_len);
        full_path[dest_dir_len] = '/';
        memcpy(full_path + dest_dir_len + 1, name, name_len + 1);

        if (is_dir_marker) {
            full_path[dest_dir_len + 1 + name_len - 1] = '\0'; /* drop the trailing '/' -- mkdir_p doesn't need it */
            ok = goblin_zip_mkdir_p(full_path);
            free(full_path);
            continue;
        }

        /* The '/' just inserted guarantees a parent dir in full_path
         * (dest_dir itself if name has no further '/' of its own). */
        char *last_slash = strrchr(full_path, '/');
        *last_slash = '\0';
        bool dir_ok = goblin_zip_mkdir_p(full_path);
        *last_slash = '/';
        if (!dir_ok) {
            ok = false;
            free(full_path);
            break;
        }

        size_t data_len = 0;
        const unsigned char *data = goblin_zip_reader_get(zip, name, &data_len);
        if (!data) {
            ok = false;
            free(full_path);
            break;
        }

        FILE *out = fopen(full_path, "wb");
        if (!out || (data_len > 0 && fwrite(data, 1, data_len, out) != data_len)) {
            ok = false;
            if (out) fclose(out);
            free(full_path);
            break;
        }
        ok = fclose(out) == 0;
        free(full_path);
    }

    goblin_zip_reader_close(zip);
    return ok;
}

#endif /* GOBLIN_ZIP_IMPLEMENTATION */

#endif /* GOBLIN_ZIP_H */
