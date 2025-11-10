#ifndef U_H
#define U_H
#include <stddef.h>
#include <jansson.h>
#include "pkgdiff.h"

struct MemoryBuffer { char *data; size_t size; };

int visible_len(const char *s);
void print_tight_box(const char *line);
void note(const char *msg);
void ok(const char *msg);
void warn(const char *msg);
void fail(const char *msg);
void print_pkg_line(json_t *pkg);
void append_preview(json_t *preview, json_t *pkg, size_t limit);
void compute_preview_widths(json_t *arr, int *wname, int *warch, int *wevr);
void print_pkg_line_aligned(json_t *pkg, int wname, int warch, int wevr);
int write_full_unique_lists(const char *file_only1, const char *file_only2, json_t *map1, json_t *map2);
int arch_matches(const char *arch_filter, const char *arch);

void log_parsing_start(void);
void log_parsing_done(void);
void log_indexing_start(void);
void log_indexing_done(void);
void log_fetched_count(const char *which, size_t n, const char *branch);

void compute_versions_widths(json_t *arr, const char *branch1, const char *branch2, int *wname, int *warch, int *wcol1, int *wcol2);
void print_version_pair_aligned(const char *name, const char *arch,
                                 const char *branch1, const char *s1,
                                 const char *branch2, const char *s2,
                                 int differ,
                                 int wname, int warch, int wcol1, int wcol2);



void pkgdiff_get_sources_dir(char *out, size_t out_sz);   
void pkgdiff_get_results_dir(char *out, size_t out_sz);   

int ensure_dir_all(const char *path, int mode);

void join_path(char *dst, size_t dst_sz, const char *dir, const char *name);

#endif
