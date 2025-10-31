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

