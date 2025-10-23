#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include <jansson.h>
#include "pkgdiff.h"

#define CSI "\x1b["
#define C_RESET  CSI "0m"
#define C_BOLD   CSI "1m"
#define C_DIM    CSI "2m"
#define C_RED    CSI "31m"
#define C_GREEN  CSI "32m"
#define C_YELLOW CSI "33m"
#define C_BLUE   CSI "34m"
#define C_MAGENTA CSI "35m"
#define C_CYAN   CSI "36m"

static FILE *open_art_file(void) {
    const char *env = getenv("PKGDIFF_ART");
    if (env && *env) {
        FILE *f = fopen(env, "rb");
        if (f) return f;
    }
    const char *candidates[] = {
        "picture.txt",
        "/usr/share/libpkgdiff/picture.txt",
        "/usr/local/share/libpkgdiff/picture.txt",
        NULL
    };
    for (int i = 0; candidates[i]; ++i) {
        FILE *f = fopen(candidates[i], "rb");
        if (f) return f;
    }
    return NULL;
}

static void wait_and_show_braille(void) {
    printf(C_BOLD C_CYAN "\nlibpgkdiff — Alt Package Comparator (interactive mode)" C_RESET "\n");
    printf("\nPlease maximize your terminal window, then press Enter to continue...");
    fflush(stdout);
    char buf[8];
    fgets(buf, sizeof(buf), stdin);
    FILE *pf = open_art_file();
    if (!pf) {
        printf("\n" C_YELLOW "(Art file not found. Set PKGDIFF_ART=/path/to/picture.txt to override.)" C_RESET "\n\n");
        return;
    }
    printf("\n");
    char chunk[4096];
    size_t n;
    while ((n = fread(chunk, 1, sizeof(chunk), pf)) > 0) {
        fwrite(chunk, 1, n, stdout);
    }
    fclose(pf);
    printf("\n\n");
    fflush(stdout);
}

void hello_pkgdiff(void) { wait_and_show_braille(); }

struct MemoryBuffer { char *data; size_t size; };

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    struct MemoryBuffer *mem = (struct MemoryBuffer *)userp;
    char *ptr = (char *)realloc(mem->data, mem->size + real_size + 1);
    if (!ptr) {
        fprintf(stderr, C_RED "realloc failed" C_RESET "\n");
        return 0;
    }
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = '\0';
    return real_size;
}

static void note(const char *msg) { printf(C_DIM "• " C_RESET "%s\n", msg); fflush(stdout); }
static void ok(const char *msg)   { printf(C_GREEN "✔ %s" C_RESET "\n", msg); fflush(stdout); }
static void warn(const char *msg) { printf(C_YELLOW "▲ %s" C_RESET "\n", msg); fflush(stdout); }
static void fail(const char *msg) { fprintf(stderr, C_RED "✖ %s" C_RESET "\n", msg); fflush(stderr); }

static int visible_len(const char *s) {
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

static void print_tight_box(const char *line) {
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

char *fetch_packages_json(const char *branch) {
    if (!branch) {
        fail("fetch_packages_json: branch is NULL");
        return NULL;
    }
    char headline[256];
    snprintf(headline, sizeof(headline), "Connecting to " C_GREEN "rdb.altlinux.org" C_RESET);
    print_tight_box(headline);
    char endpoint[512];
    snprintf(endpoint, sizeof(endpoint), "Endpoint: " C_RED "GET /api/export/branch_binary_packages/%s" C_RESET, branch);
    note(endpoint);
    CURL *curl = NULL;
    CURLcode res;
    struct MemoryBuffer chunk = (struct MemoryBuffer){0};
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        fail("curl initialization failed");
        curl_global_cleanup();
        return NULL;
    }
    char url[256];
    snprintf(url, sizeof(url), "https://rdb.altlinux.org/api/export/branch_binary_packages/%s", branch);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "altpkgdiff/1.9");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    note("Requesting branch list...");
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Request failed for branch '%s': %s", branch, curl_easy_strerror(res));
        fail(msg);
        free(chunk.data);
        chunk.data = NULL;
    } else {
        ok("Download complete");
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    if (chunk.data) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Received %.2f MB for branch '%s'", (double)chunk.size/1048576.0, branch);
        note(msg);
    }
    note("Verifying JSON structure...");
    if (!chunk.data || chunk.size == 0 || chunk.data[0] != '{') {
        warn("Unexpected response format (not a JSON object)");
    } else {
        ok("Looks like valid JSON");
    }
    return chunk.data;
}

static void append_preview(json_t *preview, json_t *pkg, size_t limit) {
    if (json_array_size(preview) < limit) {
        json_array_append(preview, pkg);
    }
}

static void print_pkg_line(json_t *pkg) {
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

static int write_full_unique_lists(const char *file_only1, const char *file_only2, json_t *map1, json_t *map2) {
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

static int arch_matches(const char *arch_filter, const char *arch) {
    if (!arch_filter || !*arch_filter) return 1;
    if (!arch || !*arch) return 0;
    return strcmp(arch_filter, arch) == 0;
}

void compare_branches_interactive_min(const char *branch1, const char *branch2, const char *arch_filter) {
    if (arch_filter && *arch_filter) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Analyzing only architecture: %s", arch_filter);
        note(buf);
    }
    note("Fetching package lists...");
    char *json1 = fetch_packages_json(branch1);
    printf("\n");
    char *json2 = fetch_packages_json(branch2);
    if (!json1 || !json2) {
        fail("Failed to fetch JSON data for one or both branches");
        free(json1); free(json2);
        return;
    }
    printf("\n");
    note("Parsing JSON...");
    json_error_t err1, err2;
    json_t *root1 = json_loads(json1, 0, &err1);
    json_t *root2 = json_loads(json2, 0, &err2);
    free(json1); free(json2);
    if (!root1 || !root2) {
        fail("JSON parse error");
        if (root1) json_decref(root1);
        if (root2) json_decref(root2);
        return;
    }
    ok("JSON parsed");
    json_t *packages1 = json_object_get(root1, "packages");
    json_t *packages2 = json_object_get(root2, "packages");
    if (!json_is_array(packages1) || !json_is_array(packages2)) {
        fail("Unexpected JSON schema: 'packages' arrays not found");
        json_decref(root1); json_decref(root2);
        return;
    }
    note("Indexing packages...");
    json_t *map1 = json_object();
    json_t *map2 = json_object();
    size_t len1 = json_array_size(packages1), filt1 = 0;
    for (size_t i = 0; i < len1; ++i) {
        json_t *pkg = json_array_get(packages1, i);
        if (!json_is_object(pkg)) continue;
        const char *name = json_string_value(json_object_get(pkg, "name"));
        const char *arch = json_string_value(json_object_get(pkg, "arch"));
        if (!name || !arch) continue;
        if (!arch_matches(arch_filter, arch)) continue;
        char key[512];
        snprintf(key, sizeof(key), "%s|%s", name, arch);
        json_object_set(map1, key, pkg);
        filt1++;
    }
    size_t len2 = json_array_size(packages2), filt2 = 0;
    for (size_t i = 0; i < len2; ++i) {
        json_t *pkg = json_array_get(packages2, i);
        if (!json_is_object(pkg)) continue;
        const char *name = json_string_value(json_object_get(pkg, "name"));
        const char *arch = json_string_value(json_object_get(pkg, "arch"));
        if (!name || !arch) continue;
        if (!arch_matches(arch_filter, arch)) continue;
        char key[512];
        snprintf(key, sizeof(key), "%s|%s", name, arch);
        json_object_set(map2, key, pkg);
        filt2++;
    }
    ok("Index built");
    printf("\n" C_BOLD "Fetched %zu packages from branch1 (%s)\n" C_RESET, filt1, branch1);
    printf(C_BOLD "Fetched %zu packages from branch2 (%s)\n\n" C_RESET, filt2, branch2);
    fflush(stdout);
    size_t only1_count = 0, only2_count = 0;
    json_t *only1_preview = json_array();
    json_t *only2_preview = json_array();
    const size_t PREVIEW_LIMIT = 10;
    note("Computing differences...");
    void *iter = json_object_iter(map1);
    while (iter) {
        const char *key = json_object_iter_key(iter);
        json_t *pkg1 = json_object_iter_value(iter);
        if (!json_object_get(map2, key)) {
            only1_count++;
            append_preview(only1_preview, pkg1, PREVIEW_LIMIT);
        }
        iter = json_object_iter_next(map1, iter);
    }
    iter = json_object_iter(map2);
    while (iter) {
        const char *key = json_object_iter_key(iter);
        if (!json_object_get(map1, key)) {
            only2_count++;
            append_preview(only2_preview, json_object_iter_value(iter), PREVIEW_LIMIT);
        }
        iter = json_object_iter_next(map2, iter);
    }
    ok("Diff computed");
    printf("\n" C_BOLD "Show 10-item previews of unique packages?\n" C_RESET);
    printf("  • Only in %s (not in %s): %zu\n", branch1, branch2, only1_count);
    printf("  • Only in %s (not in %s): %zu\n", branch2, branch1, only2_count);
    printf("[y/N]: ");
    fflush(stdout);
    char buf[16] = {0};
    if (fgets(buf, sizeof(buf), stdin) && (buf[0] == 'y' || buf[0] == 'Y')) {
        printf("\n" C_BOLD "Preview: only in %s (first %d)\n" C_RESET, branch1, (int)PREVIEW_LIMIT);
        for (size_t i = 0; i < json_array_size(only1_preview); ++i) {
            print_pkg_line(json_array_get(only1_preview, i));
        }
        printf("\n" C_BOLD "Preview: only in %s (first %d)\n" C_RESET, branch2, (int)PREVIEW_LIMIT);
        for (size_t i = 0; i < json_array_size(only2_preview); ++i) {
            print_pkg_line(json_array_get(only2_preview, i));
        }
    } else {
        printf("Skipping previews.\n");
    }
    printf("\n" C_YELLOW "These unique lists are large; writing them may take a long time." C_RESET "\n");
    printf("Save full lists now? [y/N]: ");
    fflush(stdout);
    memset(buf, 0, sizeof(buf));
    if (fgets(buf, sizeof(buf), stdin) && (buf[0] == 'y' || buf[0] == 'Y')) {
        char file1[256], file2[256];
        snprintf(file1, sizeof(file1), "%s_only.json", branch1);
        snprintf(file2, sizeof(file2), "%s_only.json", branch2);
        if (write_full_unique_lists(file1, file2, map1, map2) == 0) {
            ok("Files saved");
            printf("Saved: %s and %s\n", file1, file2);
            if (arch_filter && *arch_filter) {
                printf("(Both files contain only '%s' architecture entries)\n", arch_filter);
            }
        } else {
            fail("Failed to write one or both files");
        }
    } else {
        printf("Skipping saving full lists.\n");
    }
    json_decref(only1_preview);
    json_decref(only2_preview);
    json_decref(map1);
    json_decref(map2);
    json_decref(root1);
    json_decref(root2);
}
