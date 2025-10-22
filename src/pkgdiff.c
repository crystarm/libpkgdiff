#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include "pkgdiff.h"

void hello_pkgdiff() {
    printf("pkgdiff library says hello!\n");
}

struct MemoryBuffer {
    char *data;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    struct MemoryBuffer *mem = (struct MemoryBuffer *)userp;

    char *ptr = realloc(mem->data, mem->size + real_size + 1);
    if (!ptr) {
        fprintf(stderr, "realloc failed\n");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = 0;

    return real_size;
}

char *fetch_packages_json(const char *branch) {
    CURL *curl;
    CURLcode res;
    struct MemoryBuffer chunk = {0};

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "curl initialization failed\n");
        return NULL;
    }

    char url[256];
    snprintf(url, sizeof(url),
             "https://rdb.altlinux.org/api/export/branch_binary_packages/%s", branch);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "altpkgdiff/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.data);
        chunk.data = NULL;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return chunk.data;
}

static void print_snippet(const char *s, size_t max) {
    if (!s) return;
    fprintf(stderr, "Response snippet (%zu chars):\n", max);
    for (size_t i = 0; s[i] && i < max; ++i) {
        fputc(s[i], stderr);
    }
    fputc('\n', stderr);
}

void json_parse_demo(const char *json_data) {
    json_error_t error;
    json_t *root = json_loads(json_data, 0, &error);
    if (!root) {
        fprintf(stderr, "JSON parse error: %s (at line %d, col %d)\n", error.text, error.line, error.column);
        print_snippet(json_data, 400);
        return;
    }

    json_t *arr = NULL;
    if (json_is_array(root)) {
        arr = root;
    } else if (json_is_object(root)) {
        // Try common keys
        const char *keys[] = {"packages", "result", "data", "items"};
        for (size_t i = 0; i < sizeof(keys)/sizeof(keys[0]); ++i) {
            json_t *maybe = json_object_get(root, keys[i]);
            if (maybe && json_is_array(maybe)) { arr = maybe; break; }
        }
        if (!arr) {
            // Some APIs wrap under { "branch": "...", "packages": [...] } or { "list": [...] }
            json_t *maybe = json_object_get(root, "list");
            if (maybe && json_is_array(maybe)) arr = maybe;
        }
    }

    if (!arr) {
        fprintf(stderr, "Unsupported JSON schema: expected array or object with array field\n");
        print_snippet(json_data, 400);
        json_decref(root);
        return;
    }

    size_t count = json_array_size(arr);
    printf("Total packages: %zu\n", count);

    for (size_t i = 0; i < count && i < 5; ++i) {
        json_t *pkg = json_array_get(arr, i);
        if (!json_is_object(pkg)) continue;

        json_t *name = json_object_get(pkg, "name");
        json_t *version = json_object_get(pkg, "version");
        json_t *release = json_object_get(pkg, "release");
        json_t *arch = json_object_get(pkg, "arch");

        if (json_is_string(name) && json_is_string(version) &&
            json_is_string(release) && json_is_string(arch)) {
            printf("Package: %s, Version: %s-%s, Arch: %s\n",
                   json_string_value(name),
                   json_string_value(version),
                   json_string_value(release),
                   json_string_value(arch));
        }
    }

    json_decref(root);
}
