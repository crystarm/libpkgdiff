#ifndef U_H
#define U_H
#include <stddef.h>
#include <jansson.h>
#include "pkgdiff.h"
/* Internal shared struct used by curl write callback */
struct MemoryBuffer { char *data; size_t size; };
/* Internal helpers (not part of public API) */
int visible_len(const char *s);
void print_tight_box(const char *line);
void note(const char *msg);
void ok(const char *msg);
void warn(const char *msg);
void fail(const char *msg);
void print_pkg_line(json_t *pkg);
void append_preview(json_t *preview, json_t *pkg, size_t limit);
int write_full_unique_lists(const char *file_only1, const char *file_only2, json_t *map1, json_t *map2);
int arch_matches(const char *arch_filter, const char *arch);

void log_parsing_start(void);
void log_parsing_done(void);
void log_indexing_start(void);
void log_indexing_done(void);
void log_fetched_count(const char *which, size_t n, const char *branch);

#endif
