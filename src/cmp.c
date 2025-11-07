#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include <jansson.h>
#include "pkgdiff.h"
#include "u.h"



void compare_branches_interactive_min(const char *branch1, const char *branch2, const char *arch_filter) {
    if (arch_filter && *arch_filter) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Analyzing only architecture: %s", arch_filter);
        note(buf);
    }
    note("Fetching package lists...");
    printf("\n");
    char *json1 = fetch_packages_json(branch1);
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
    log_indexing_start();
    json_t *map1 = json_object();
    json_t *map2 = json_object();
size_t len1 = json_array_size(packages1), filt1 = 0;for (size_t i = 0; i < len1; ++i) {
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
    size_t len2 = json_array_size(packages2), filt2 = 0;for (size_t i = 0; i < len2; ++i) {
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
    
    
    log_indexing_done();
    printf("\n");
    log_fetched_count("branch1", len1, branch1);
    log_fetched_count("branch2", len2, branch2);
    printf("\n");
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
    ok("Differences computed");
    printf("\n" C_BOLD "Show 10-item previews of unique packages?\n" C_RESET);
    printf("  • Only in %s (not in %s): %zu\n", branch1, branch2, only1_count);
    printf("  • Only in %s (not in %s): %zu\n", branch2, branch1, only2_count);
    printf("[y/N]: ");char buf[16] = {0};
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
    }memset(buf, 0, sizeof(buf));
    printf("\nSave to JSON files? [y/N]: ");if (fgets(buf, sizeof(buf), stdin) && (buf[0] == 'y' || buf[0] == 'Y')) {
        char file1[256], file2[256];
        snprintf(file1, sizeof(file1), "%s_only.json", branch1);
        snprintf(file2, sizeof(file2), "%s_only.json", branch2);

        char line[512] = {0};
        printf("Enter two filenames separated by a space (defaults: %s and %s): ", file1, file2);if (fgets(line, sizeof(line), stdin)) {
            // tokenize by whitespace (simple)
            char *p = line;
            // trim trailing newline
            for (size_t i = 0; line[i]; ++i) { if (line[i]=='\n' || line[i]=='\r') { line[i]='\0'; break; } }
            // skip leading spaces
            while (*p==' ' || *p=='\t') ++p;
            if (*p) {
                // first token
                char *t1 = p;
                while (*p && *p!=' ' && *p!='\t') ++p;
                if (*p) { *p++ = '\0'; }
                if (t1[0]) strncpy(file1, t1, sizeof(file1)-1);

                // skip spaces to second token (optional)
                while (*p==' ' || *p=='\t') ++p;
                if (*p) {
                    char *t2 = p;
                    while (*p && *p!=' ' && *p!='\t') ++p;
                    *p = '\0';
                    if (t2[0]) strncpy(file2, t2, sizeof(file2)-1);
                }
            }
        }

        if (write_full_unique_lists(file1, file2, map1, map2) == 0) {
            printf(C_GREEN "Saved: %s and %s\n" C_RESET, file1, file2);
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

/* ===== New feature: common packages across two branches with version preview & JSON save ===== */

static __attribute__((unused)) void print_common_versions_line(json_t *pkg1, json_t *pkg2, const char *b1, const char *b2) {
    const char *name = json_string_value(json_object_get(pkg1, "name"));
    if (!name) name = "(unknown)";
    const char *arch = json_string_value(json_object_get(pkg1, "arch"));
    if (!arch) arch = "noarch";

    long e1 = 0, e2 = 0;
    json_t *e = json_object_get(pkg1, "epoch");
    if (json_is_integer(e)) e1 = json_integer_value(e);
    e = json_object_get(pkg2, "epoch");
    if (json_is_integer(e)) e2 = json_integer_value(e);

    const char *v1 = json_string_value(json_object_get(pkg1, "version")); if (!v1) v1 = "?";
    const char *r1 = json_string_value(json_object_get(pkg1, "release")); if (!r1) r1 = "?";
    const char *v2 = json_string_value(json_object_get(pkg2, "version")); if (!v2) v2 = "?";
    const char *r2 = json_string_value(json_object_get(pkg2, "release")); if (!r2) r2 = "?";

    char s1[256], s2[256];
    if (e1 > 0) snprintf(s1, sizeof(s1), "%ld:%s-%s", e1, v1, r1);
    else        snprintf(s1, sizeof(s1), "%s-%s", v1, r1);
    if (e2 > 0) snprintf(s2, sizeof(s2), "%ld:%s-%s", e2, v2, r2);
    else        snprintf(s2, sizeof(s2), "%s-%s", v2, r2);

    int differ = strcmp(s1, s2) != 0;
    if (differ) {
        printf("• " C_BOLD "%s" C_RESET " [" C_GREEN "%s" C_RESET "] versions: (" C_MAGENTA "%s" C_RESET ": " C_YELLOW "%s" C_RESET ") (" C_MAGENTA "%s" C_RESET ": " C_YELLOW "%s" C_RESET ")\n", 
               name, arch, b1, s1, b2, s2);
    } else {
        printf("• " C_BOLD "%s" C_RESET " [" C_GREEN "%s" C_RESET "] versions: (" C_MAGENTA "%s" C_RESET ": " C_GREEN "%s" C_RESET ") (" C_MAGENTA "%s" C_RESET ": " C_GREEN "%s" C_RESET ")  " C_DIM "[=]" C_RESET "\n", 
               name, arch, b1, s1, b2, s2);
    }
}

PKGDIFF_API void common_packages_interactive_min(const char *branch1, const char *branch2, const char *arch_filter, const char *save_path) {
    if (arch_filter && *arch_filter) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Analyzing only architecture: %s", arch_filter);
        note(buf);
    }
    note("Fetching package lists...");
    printf("\n");
    char *json1 = fetch_packages_json(branch1);
    char *json2 = fetch_packages_json(branch2);
    if (!json1 || !json2) {
        fail("Download failed (one of branches)"); 
        free(json1); free(json2); 
        return;
    }

    note("Parsing JSON...");
    json_error_t err1 = {0}, err2 = {0};
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
        fail("Unexpected JSON: 'packages' is not an array");
        json_decref(root1); json_decref(root2);
        return;
    }

    log_indexing_start();
    json_t *map1 = json_object();
    json_t *map2 = json_object();
size_t len1 = json_array_size(packages1);for (size_t i = 0; i < len1; ++i) {
        json_t *pkg = json_array_get(packages1, i);
        if (!json_is_object(pkg)) continue;
        const char *name = json_string_value(json_object_get(pkg, "name"));
        const char *arch = json_string_value(json_object_get(pkg, "arch"));
        if (!name || !arch) continue;
        if (!arch_matches(arch_filter, arch)) continue;
        char key[512]; snprintf(key, sizeof(key), "%s|%s", name, arch);
        json_object_set(map1, key, pkg);
    }
    size_t len2 = json_array_size(packages2);for (size_t i = 0; i < len2; ++i) {
        json_t *pkg = json_array_get(packages2, i);
        if (!json_is_object(pkg)) continue;
        const char *name = json_string_value(json_object_get(pkg, "name"));
        const char *arch = json_string_value(json_object_get(pkg, "arch"));
        if (!name || !arch) continue;
        if (!arch_matches(arch_filter, arch)) continue;
        char key[512]; snprintf(key, sizeof(key), "%s|%s", name, arch);
        json_object_set(map2, key, pkg);
    }

    
    log_indexing_done();
    printf("\n");
    log_fetched_count("branch1", len1, branch1);
    log_fetched_count("branch2", len2, branch2);
    printf("\n");
note("Computing common set...");
    size_t common_count = 0;
    const size_t PREVIEW_LIMIT = 10;
    json_t *preview = json_array();
    json_t *save_arr = json_array();

    void *iter = json_object_iter(map1);
    while (iter) {
        const char *key = json_object_iter_key(iter);
        json_t *pkg1 = json_object_iter_value(iter);
        json_t *pkg2 = json_object_get(map2, key);
        if (pkg2) {
            const char *name = json_string_value(json_object_get(pkg1, "name"));
            const char *arch = json_string_value(json_object_get(pkg1, "arch"));
            long e1 = 0, e2 = 0;
            json_t *e = json_object_get(pkg1, "epoch"); if (json_is_integer(e)) e1 = json_integer_value(e);
            e = json_object_get(pkg2, "epoch"); if (json_is_integer(e)) e2 = json_integer_value(e);
            const char *v1 = json_string_value(json_object_get(pkg1, "version")); if (!v1) v1 = "?";
            const char *r1 = json_string_value(json_object_get(pkg1, "release")); if (!r1) r1 = "?";
            const char *v2 = json_string_value(json_object_get(pkg2, "version")); if (!v2) v2 = "?";
            const char *r2 = json_string_value(json_object_get(pkg2, "release")); if (!r2) r2 = "?";

            char s1[256], s2[256];
            if (e1 > 0) snprintf(s1, sizeof(s1), "%ld:%s-%s", e1, v1, r1);
            else        snprintf(s1, sizeof(s1), "%s-%s", v1, r1);
            if (e2 > 0) snprintf(s2, sizeof(s2), "%ld:%s-%s", e2, v2, r2);
            else        snprintf(s2, sizeof(s2), "%s-%s", v2, r2);

            json_t *entry = json_object();
            json_object_set_new(entry, "name", json_string(name ? name : ""));
            json_object_set_new(entry, "arch", json_string(arch ? arch : "noarch"));
            json_object_set_new(entry, branch1, json_string(s1));
            json_object_set_new(entry, branch2, json_string(s2));
            json_object_set_new(entry, "different", json_boolean(strcmp(s1, s2) != 0));
            json_array_append_new(save_arr, entry);

            if (json_array_size(preview) < PREVIEW_LIMIT) {
                json_t *pv = json_object();
                json_object_set_new(pv, "name", json_string(name ? name : ""));
                json_object_set_new(pv, "arch", json_string(arch ? arch : "noarch"));
                json_object_set_new(pv, branch1, json_string(s1));
                json_object_set_new(pv, branch2, json_string(s2));
                json_array_append_new(preview, pv);
            }
            common_count++;
        }
        iter = json_object_iter_next(map1, iter);
    }

    ok("Common set computed");
    printf("\n" C_BOLD C_ACCENT "Packages present in BOTH %s and %s: %zu\n" C_RESET, branch1, branch2, common_count);
    printf(C_DIM "(Preview: first %d)\n" C_RESET, (int)PREVIEW_LIMIT);

    for (size_t i = 0; i < json_array_size(preview); ++i) {
        json_t *pv = json_array_get(preview, i);
        const char *name = json_string_value(json_object_get(pv, "name"));
        const char *arch = json_string_value(json_object_get(pv, "arch"));
        const char *s1 = json_string_value(json_object_get(pv, branch1));
        const char *s2 = json_string_value(json_object_get(pv, branch2));
        int differ = (s1 && s2) ? strcmp(s1, s2) != 0 : 1;
        if (differ) {
            printf("• " C_BOLD "%s" C_RESET " [" C_GREEN "%s" C_RESET "] versions: (" C_MAGENTA "%s" C_RESET ": " C_YELLOW "%s" C_RESET ") (" C_MAGENTA "%s" C_RESET ": " C_YELLOW "%s" C_RESET ")\n", 
                   name ? name : "(unknown)", arch ? arch : "noarch", branch1, s1?s1:"?", branch2, s2?s2:"?");
        } else {
            printf("• " C_BOLD "%s" C_RESET " [" C_GREEN "%s" C_RESET "] versions: (" C_MAGENTA "%s" C_RESET ": " C_GREEN "%s" C_RESET ") (" C_MAGENTA "%s" C_RESET ": " C_GREEN "%s" C_RESET ")  " C_DIM "[=]" C_RESET "\n", 
                   name ? name : "(unknown)", arch ? arch : "noarch", branch1, s1?s1:"?", branch2, s2?s2:"?");
        }
    }

    int do_save = 0;
    char pathbuf[512] = {0};
    if (save_path && *save_path) {
        strncpy(pathbuf, save_path, sizeof(pathbuf)-1);
        do_save = 1;
    } else {
        printf("\nSave to JSON file? [y/N]: ");char ans[8] = {0};
        if (fgets(ans, sizeof(ans), stdin) && (ans[0]=='y' || ans[0]=='Y')) {
            printf("Enter filename (default: common-%s-%s.json): ", branch1, branch2);if (!fgets(pathbuf, sizeof(pathbuf), stdin)) { pathbuf[0] = '\0'; }
            size_t L = strlen(pathbuf); if (L > 0 && pathbuf[L-1] == '\n') pathbuf[L-1] = '\0';
            if (pathbuf[0] == '\0') {
                snprintf(pathbuf, sizeof(pathbuf), "common-%s-%s.json", branch1, branch2);
            }
            do_save = 1;
        }
    }

    if (do_save) {
        char *dump = json_dumps(save_arr, JSON_INDENT(2));
        if (!dump) {
            fail("Failed to serialize JSON");
        } else {
            FILE *f = fopen(pathbuf, "wb");
            if (!f) {
                fail("Failed to open output file for writing");
            } else {
                fwrite(dump, 1, strlen(dump), f);
                fclose(f);
                printf(C_GREEN "Saved: %s\n" C_RESET, pathbuf);

}
            free(dump);
        }
    }

    json_decref(preview);
    json_decref(save_arr);
    json_decref(map1);
    json_decref(map2);
    json_decref(root1);
    json_decref(root2);
}

