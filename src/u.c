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
    if (epoch > 0) printf("%s %ld:%s-%s (%s)\n", name, epoch, version, release, arch);
    else printf("%s %s-%s (%s)\n", name, version, release, arch);
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


