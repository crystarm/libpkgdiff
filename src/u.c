#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <jansson.h>
#include "pkgdiff.h"
#include "u.h"



 int visible_len(const char *s) {
    int w = 0;
    for (const char *p = s; *p; ++p) {
        if (*p == '\x1b' && *(p+1) == '[') {
            ++p;
            while (*p && *p != 'm') ++p;
        } else {
            w++;
        }
    }
    return w;
}



 void print_tight_box(const char *line) {
    int inner = visible_len(line);
    if (inner < 0) inner = 0;
    printf(C_BOLD C_MAGENTA "+");
    for (int i=0;i<inner+2;i++) putchar('-');
    printf("+" C_RESET "\n");
    printf(C_BOLD C_MAGENTA "|" C_RESET " ");
    fputs(line, stdout);
    printf(" " C_BOLD C_MAGENTA "|" C_RESET "\n");
    printf(C_BOLD C_MAGENTA "+");
    for (int i=0;i<inner+2;i++) putchar('-');
    printf("+" C_RESET "\n");
}



 void note(const char *msg) { printf(C_DIM "• " C_RESET "%s\n", msg); fflush(stdout); }


 void ok(const char *msg)   { printf(C_GREEN "✔ %s" C_RESET "\n", msg); fflush(stdout); }


 void warn(const char *msg) { printf(C_YELLOW "▲ %s" C_RESET "\n", msg); fflush(stdout); }


 void fail(const char *msg) { fprintf(stderr, C_RED "✖ %s" C_RESET "\n", msg); fflush(stderr); }



 void print_pkg_line(json_t *pkg) {
    
    const char *name = json_string_value(json_object_get(pkg, "name"));
    const char *version = json_string_value(json_object_get(pkg, "version"));
    const char *release = json_string_value(json_object_get(pkg, "release"));
    const char *arch = json_string_value(json_object_get(pkg, "arch"));
    long epoch = 0;
    json_t *e = json_object_get(pkg, "epoch");
    if (json_is_integer(e)) epoch = json_integer_value(e);
    if (!name) name = "(unknown)";
    if (!version) version = "?";
    if (!release) release = "?";
    if (!arch) arch = "noarch";

    char evr[256];
    if (epoch > 0) snprintf(evr, sizeof(evr), "%ld:%s-%s", epoch, version, release);
    else snprintf(evr, sizeof(evr), "%s-%s", version, release);

    // Pretty, uniform line with bullet, colors, and tabs to align columns
    printf("• " C_BOLD "%s" C_RESET "\t[" C_GREEN "%s" C_RESET "]\t" C_YELLOW "%s" C_RESET "\n",
           name, arch, evr);
    
}

 static void pad_spaces(int n) { for (int i = 0; i < n; ++i) putchar(' '); }

 void compute_preview_widths(json_t *arr, int *wname, int *warch, int *wevr) {
    *wname = *warch = *wevr = 0;
    size_t n = json_array_size(arr);
    for (size_t i = 0; i < n; ++i) {
        json_t *pkg = json_array_get(arr, i);
        const char *name = json_string_value(json_object_get(pkg, "name"));
        const char *version = json_string_value(json_object_get(pkg, "version"));
        const char *release = json_string_value(json_object_get(pkg, "release"));
        const char *arch = json_string_value(json_object_get(pkg, "arch"));
        long epoch = 0;
        json_t *e = json_object_get(pkg, "epoch");
        if (json_is_integer(e)) epoch = json_integer_value(e);
        if (!name) name = "(unknown)";
        if (!version) version = "?";
        if (!release) release = "?";
        if (!arch) arch = "noarch";

        char evr[256];
        if (epoch > 0) snprintf(evr, sizeof(evr), "%ld:%s-%s", epoch, version, release);
        else snprintf(evr, sizeof(evr), "%s-%s", version, release);

        int ln = (int)strlen(name);
        int la = (int)strlen(arch);
        int lv = (int)strlen(evr);
        if (ln > *wname) *wname = ln;
        if (la > *warch) *warch = la;
        if (lv > *wevr)  *wevr  = lv;
    }
 }

 void print_pkg_line_aligned(json_t *pkg, int wname, int warch, int wevr) {
    (void)wevr;  // width of EVR reserved for future use
    const char *name = json_string_value(json_object_get(pkg, "name"));
    const char *version = json_string_value(json_object_get(pkg, "version"));
    const char *release = json_string_value(json_object_get(pkg, "release"));
    const char *arch = json_string_value(json_object_get(pkg, "arch"));
    long epoch = 0;
    json_t *e = json_object_get(pkg, "epoch");
    if (json_is_integer(e)) epoch = json_integer_value(e);
    if (!name) name = "(unknown)";
    if (!version) version = "?";
    if (!release) release = "?";
    if (!arch) arch = "noarch";

    char evr[256];
    if (epoch > 0) snprintf(evr, sizeof(evr), "%ld:%s-%s", epoch, version, release);
    else snprintf(evr, sizeof(evr), "%s-%s", version, release);

    // • NAME···  [arch]··  EVR··
    printf("• " C_BOLD "%s" C_RESET, name);
    pad_spaces(wname - (int)strlen(name) + 2);
    printf("[" C_GREEN "%s" C_RESET "]", arch);
    pad_spaces(warch - (int)strlen(arch) + 2);
    printf(C_YELLOW "%s" C_RESET "\n", evr);
 }




 void append_preview(json_t *preview, json_t *pkg, size_t limit) {
    if (json_array_size(preview) < limit) {
        json_array_append(preview, pkg);
    }
}



 int write_full_unique_lists(const char *file_only1, const char *file_only2, json_t *map1, json_t *map2) {
    FILE *f1 = fopen(file_only1, "wb");
    if (!f1) { fprintf(stderr, "Failed to open %s for writing\n", file_only1); return -1; }
    FILE *f2 = fopen(file_only2, "wb");
    if (!f2) { fprintf(stderr, "Failed to open %s for writing\n", file_only2); fclose(f1); return -1; }
    fputs("[\n", f1);
    fputs("[\n", f2);
    void *iter = json_object_iter(map1);
    int first = 1;
    while (iter) {
        const char *key = json_object_iter_key(iter);
        json_t *pkg1 = json_object_iter_value(iter);
        if (!json_object_get(map2, key)) {
            char *dump = json_dumps(pkg1, JSON_COMPACT);
            if (dump) {
                if (!first) fputs(",\n", f1);
                fputs("  ", f1); fputs(dump, f1);
                free(dump);
                first = 0;
            }
        }
        iter = json_object_iter_next(map1, iter);
    }
    fputs("\n]\n", f1);
    iter = json_object_iter(map2);
    first = 1;
    while (iter) {
        const char *key = json_object_iter_key(iter);
        if (!json_object_get(map1, key)) {
            json_t *pkg2 = json_object_iter_value(iter);
            char *dump = json_dumps(pkg2, JSON_COMPACT);
            if (dump) {
                if (!first) fputs(",\n", f2);
                fputs("  ", f2); fputs(dump, f2);
                free(dump);
                first = 0;
            }
        }
        iter = json_object_iter_next(map2, iter);
    }
    fputs("\n]\n", f2);
    fclose(f1);
    fclose(f2);
    return 0;
}



 int arch_matches(const char *arch_filter, const char *arch) {
    if (!arch_filter || !*arch_filter) return 1;
    if (!arch || !*arch) return 0;
    return strcmp(arch_filter, arch) == 0;
}



void log_parsing_start(void)  { note("Parsing JSON..."); }
void log_parsing_done(void)   { ok("JSON parsed"); }
void log_indexing_start(void) { note("Indexing packages..."); }
void log_indexing_done(void)  { ok("Index built"); }
void log_fetched_count(const char *which, size_t n, const char *branch) {
    printf(C_ACCENT "Fetched %zu packages from %s (%s)\n" C_RESET, n, which, branch ? branch : "?");
    fflush(stdout);
}




 void compute_versions_widths(json_t *arr, const char *branch1, const char *branch2, int *wname, int *warch, int *wcol1, int *wcol2) {
    *wname = *warch = *wcol1 = *wcol2 = 0;
    if (!arr) return;
    size_t n = json_array_size(arr);
    for (size_t i = 0; i < n; ++i) {
        json_t *pv = json_array_get(arr, i);
        const char *name = json_string_value(json_object_get(pv, "name")); if (!name) name="(unknown)";
        const char *arch = json_string_value(json_object_get(pv, "arch")); if (!arch) arch="noarch";
        const char *s1 = json_string_value(json_object_get(pv, branch1)); if (!s1) s1="?";
        const char *s2 = json_string_value(json_object_get(pv, branch2)); if (!s2) s2="?";
        int ln = (int)strlen(name);
        int la = (int)strlen(arch);
        int l1 = 1 + (int)strlen(branch1) + 2 + (int)strlen(s1) + 1; // "(b1: s1)"
        int l2 = 1 + (int)strlen(branch2) + 2 + (int)strlen(s2) + 1; // "(b2: s2)"
        if (ln > *wname) *wname = ln;
        if (la > *warch) *warch = la;
        if (l1 > *wcol1) *wcol1 = l1;
        if (l2 > *wcol2) *wcol2 = l2;
    }
 }

 void print_version_pair_aligned(const char *name, const char *arch,
                                 const char *branch1, const char *s1,
                                 const char *branch2, const char *s2,
                                 int differ,
                                 int wname, int warch, int wcol1, int wcol2) {
    (void)wcol2;  // not needed for trailing pad
    if (!name) name="(unknown)";
    if (!arch) arch="noarch";
    if (!s1) s1="?";
    if (!s2) s2="?";
    int len_name = (int)strlen(name);
    int len_arch = (int)strlen(arch);
    int len_col1 = 1 + (int)strlen(branch1) + 2 + (int)strlen(s1) + 1; // "(b1: s1)"
    

    // • NAME··  [arch]··  (b1: s1)··  (b2: s2)  [=]
    printf("• " C_BOLD "%s" C_RESET, name);
    for (int i=0;i< (wname - len_name + 2); ++i) putchar(' ');
    printf("[" C_GREEN "%s" C_RESET "]", arch);
    for (int i=0;i< (warch - len_arch + 2); ++i) putchar(' ');

    // First column with colors
    printf("(" C_MAGENTA "%s" C_RESET ": %s%s%s)", branch1, differ ? C_YELLOW : C_GREEN, s1, C_RESET);
    for (int i=0;i< (wcol1 - len_col1 + 2); ++i) putchar(' ');

    // Second column with colors
    printf("(" C_MAGENTA "%s" C_RESET ": %s%s%s)", branch2, differ ? C_YELLOW : C_GREEN, s2, C_RESET);
putchar('\n');
 }


#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Safe string helpers */
static void safe_strcpy(char *dst, size_t n, const char *src) {
    if (!dst || n == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    snprintf(dst, n, "%s", src);
    dst[n-1] = '\0';
}

void join_path(char *dst, size_t dst_sz, const char *dir, const char *name) {
    if (!dst || dst_sz == 0) return;
    if (!dir || !*dir) { safe_strcpy(dst, dst_sz, name ? name : ""); return; }
    if (!name || !*name) { safe_strcpy(dst, dst_sz, dir); return; }
    size_t dlen = strlen(dir);
    int need_slash = (dlen > 0 && dir[dlen-1] != '/');
    snprintf(dst, dst_sz, need_slash ? "%s/%s" : "%s%s", dir, name);
    dst[dst_sz-1] = '\0';
}

static int ensure_dir_one(const char *path, int mode) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }
    #ifdef _WIN32
    return _mkdir(path);
    #else
    return mkdir(path, (mode_t)mode);
    #endif
}

int ensure_dir_all(const char *path, int mode) {
    if (!path || !*path) return -1;
    char tmp[1024];
    size_t n = strlen(path);
    if (n >= sizeof(tmp)) return -1;
    strcpy(tmp, path);
    /* strip trailing slash */
    if (n > 1 && tmp[n-1]=='/') tmp[n-1] = '\0';
    char *p = tmp;
    if (*p == '/') ++p; /* absolute path: skip first slash */
    for (; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (ensure_dir_one(tmp, mode) != 0) return -1;
            *p = '/';
        }
    }
    return ensure_dir_one(tmp, mode);
}

/* Resolve $XDG dirs or HOME fallbacks. Allow overrides via env:
   LIBPKGDIFF_SOURCES_DIR and LIBPKGDIFF_RESULTS_DIR. */

void pkgdiff_get_sources_dir(char *out, size_t out_sz) {
    const char *override = getenv("LIBPKGDIFF_SOURCES_DIR");
    if (override && *override) { safe_strcpy(out, out_sz, override); return; }

    const char *xdg = getenv("XDG_CACHE_HOME");
    if (xdg && *xdg) {
        snprintf(out, out_sz, "%s/libpkgdiff/sources", xdg);
    } else {
        const char *home = getenv("HOME");
        if (home && *home) snprintf(out, out_sz, "%s/.cache/libpkgdiff/sources", home);
        else snprintf(out, out_sz, ".pkgdiff-cache/sources");
    }
    out[out_sz-1] = '\0';
}

void pkgdiff_get_results_dir(char *out, size_t out_sz) {
    const char *override = getenv("LIBPKGDIFF_RESULTS_DIR");
    if (override && *override) { safe_strcpy(out, out_sz, override); return; }

    const char *xdg = getenv("XDG_DATA_HOME");
    if (xdg && *xdg) {
        snprintf(out, out_sz, "%s/libpkgdiff/results", xdg);
    } else {
        const char *home = getenv("HOME");
        if (home && *home) snprintf(out, out_sz, "%s/.local/share/libpkgdiff/results", home);
        else snprintf(out, out_sz, "./libpkgdiff-results");
    }
    out[out_sz-1] = '\0';
}

