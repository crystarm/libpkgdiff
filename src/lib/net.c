
#include <stdio.h>
#include "net.h"
#include "ansi.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <curl/curl.h>

#include "pkgdiff.h"
#include "util.h"


struct CachedJson {
    char  branch[64];
    char *data; 
};
static struct CachedJson s_cache[8];
static int s_cache_count = 0;

static char *cache_get_copy(const char *branch) {
    if (!branch || !*branch) return NULL;
    for (int i = 0; i < s_cache_count; ++i) {
        if (strcmp(s_cache[i].branch, branch) == 0 && s_cache[i].data) {
            char *dup = strdup(s_cache[i].data);
            if (dup) {
                char msg[192];
                snprintf(msg, sizeof(msg),
                         "Using cached JSON for branch '%s' (%.2f MB)",
                         branch, (double)strlen(dup) / 1048576.0);
                note(msg);
            }
            return dup;
        }
    }
    return NULL;
}

static void cache_put(const char *branch, const char *json_text) {
    if (!branch || !*branch || !json_text) return;
    for (int i = 0; i < s_cache_count; ++i) {
        if (strcmp(s_cache[i].branch, branch) == 0) {
            free(s_cache[i].data);
            s_cache[i].data = strdup(json_text);
            return;
        }
    }
    if (s_cache_count < (int)(sizeof(s_cache)/sizeof(s_cache[0]))) {
        strncpy(s_cache[s_cache_count].branch, branch,
                sizeof(s_cache[s_cache_count].branch) - 1);
        s_cache[s_cache_count].branch[sizeof(s_cache[s_cache_count].branch) - 1] = '\0';
        s_cache[s_cache_count].data = strdup(json_text);
        ++s_cache_count;
    } else {
        free(s_cache[0].data);
        for (int j = 1; j < s_cache_count; ++j) s_cache[j-1] = s_cache[j];
        strncpy(s_cache[s_cache_count-1].branch, branch,
                sizeof(s_cache[s_cache_count-1].branch) - 1);
        s_cache[s_cache_count-1].branch[sizeof(s_cache[s_cache_count-1].branch) - 1] = '\0';
        s_cache[s_cache_count-1].data = strdup(json_text);
    }
}


static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryBuffer *mem = (struct MemoryBuffer*)userp;
    char *ptr = (char*)realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) return 0;
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;
    return realsize;
}


static void cache_paths(const char *branch, char *json_path, size_t json_sz,
                        char *meta_path, size_t meta_sz) {
    const char *xdg = getenv("XDG_CACHE_HOME");
    char dir[512];
    if (!xdg || !*xdg) {
        const char *home = getenv("HOME");
        if (home && *home) snprintf(dir, sizeof(dir), "%s/.cache/pkgdiff", home);
        else snprintf(dir, sizeof(dir), ".pkgdiff-cache");
    } else {
        snprintf(dir, sizeof(dir), "%s/pkgdiff", xdg);
    }
    struct stat st;
    if (stat(dir, &st) != 0) {
        #ifdef _WIN32
        _mkdir(dir);
        #else
        mkdir(dir, 0700);
        #endif
    }
    int n1 = snprintf(json_path, json_sz, "%s/%s.json", dir, branch);
    if (n1 < 0 || (size_t)n1 >= json_sz) { warn("cache path too long (json)"); json_path[json_sz-1] = 0; }
int n2 = snprintf(meta_path, meta_sz, "%s/%s.meta", dir, branch);
    if (n2 < 0 || (size_t)n2 >= meta_sz) { warn("cache path too long (meta)"); meta_path[meta_sz-1] = 0; }
}

                        static int read_file_to_buf(const char *path, char **out, size_t *out_len) {
                            FILE *f = fopen(path, "rb");
                            if (!f) return -1;
                            fseek(f, 0, SEEK_END);
                            long n = ftell(f);
                            if (n < 0) { fclose(f); return -1; }
                            fseek(f, 0, SEEK_SET);
                            char *buf = (char*)malloc((size_t)n + 1);
                            if (!buf) { fclose(f); return -1; }
                            if (fread(buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(buf); return -1; }
                            buf[n] = '\0';
                            fclose(f);
                            if (out) *out = buf;
                            if (out_len) *out_len = (size_t)n;   
                            return 0;
                        }

                        static void write_text_file(const char *path, const char *text) {
                            FILE *f = fopen(path, "wb");
                            if (!f) return;
                            fwrite(text, 1, strlen(text), f);
                            fclose(f);
                        }

                        struct ResponseHeaders {
                            char etag[256];
                            char last_modified[256];
                        };

                        static size_t header_cb(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t total = size * nitems;
    struct ResponseHeaders *h = (struct ResponseHeaders*)userdata;
    char line[512];
    size_t copy = total < sizeof(line)-1 ? total : sizeof(line)-1;
    memcpy(line, buffer, copy);
    line[copy] = '\0';

    if (strncasecmp(line, "ETag:", 5) == 0) {
        const char *p = line + 5;
        while (*p == ' ' || *p == '\t') ++p;
        size_t n = strcspn(p, "\r\n");
        if (n >= sizeof(h->etag)) n = sizeof(h->etag) - 1;
        memcpy(h->etag, p, n);
        h->etag[n] = '\0';
    } else if (strncasecmp(line, "Last-Modified:", 14) == 0) {
        const char *p = line + 14;
        while (*p == ' ' || *p == '\t') ++p;
        size_t n = strcspn(p, "\r\n");
        if (n >= sizeof(h->last_modified)) n = sizeof(h->last_modified) - 1;
        memcpy(h->last_modified, p, n);
        h->last_modified[n] = '\0';
    }
    return total;
}


                        
                        
char *fetch_packages_json(const char *branch) {
    if (!branch) { fail("fetch_packages_json: branch is NULL"); return NULL; }

    
    {
        char *cached = cache_get_copy(branch);
        if (cached) return cached;
    }

    
    char json_path[512], meta_path[512];
    cache_paths(branch, json_path, sizeof(json_path), meta_path, sizeof(meta_path));

    
    struct stat st;
    if (stat(json_path, &st) == 0) {
        time_t now = time(NULL);
        if (now != (time_t)-1 && (now - st.st_mtime) < 2*60*60) {
            char *buf = NULL; size_t blen = 0;
            if (read_file_to_buf(json_path, &buf, &blen) == 0 && buf) {
                do { char _msg[160]; snprintf(_msg, sizeof(_msg), "Using cached sources for branch %s (younger than 2 hours)", branch ? branch : "?"); note(_msg); } while(0);
                cache_put(branch, buf);
                return buf;
            }
        }
    }

    
    char url[512];
    snprintf(url, sizeof(url), "https://rdb.altlinux.org/api/export/branch_binary_packages/%s", branch);

    struct MemoryBuffer chunk = (struct MemoryBuffer){0};
    struct ResponseHeaders rh = {{0},{0}};
    struct curl_slist *hdrs = NULL;
    CURL *curl = NULL;
    CURLcode res;
    long code = 0;

    
    char etag[256] = {0}, last_mod[256] = {0};
    {
        char *meta = NULL; size_t mlen = 0;
        if (read_file_to_buf(meta_path, &meta, &mlen) == 0 && meta) {
            char *save = NULL;
            for (char *line = strtok_r(meta, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
                if (strncasecmp(line, "ETag:", 5) == 0) {
                    const char *p = line + 5; while (*p==' '||*p=='\t') ++p;
                    snprintf(etag, sizeof(etag), "%s", p);
                } else if (strncasecmp(line, "Last-Modified:", 14) == 0) {
                    const char *p = line + 14; while (*p==' '||*p=='\t') ++p;
                    snprintf(last_mod, sizeof(last_mod), "%s", p);
                }
            }
            free(meta);
        }
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) { fail("curl initialization failed"); curl_global_cleanup(); goto try_disk_fallback; }

    {
        char headline[256];
        snprintf(headline, sizeof(headline), "Connecting to " C_GREEN "rdb.altlinux.org" C_RESET);
        print_tight_box(headline);
        char endpoint[256];
        snprintf(endpoint, sizeof(endpoint), "Endpoint: GET /api/export/branch_binary_packages/%s", branch);
        note(endpoint);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "altpkgdiff/1.9");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&rh);

    if (etag[0]) {
        char b[320]; snprintf(b, sizeof(b), "If-None-Match: %s", etag);
        hdrs = curl_slist_append(hdrs, b);
    }
    if (last_mod[0]) {
        char b[360]; snprintf(b, sizeof(b), "If-Modified-Since: %s", last_mod);
        hdrs = curl_slist_append(hdrs, b);
    }
    if (hdrs) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

    note("Requesting branch list...");
    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    if (res == CURLE_OK && code == 304) {
        
        char *filedata = NULL; size_t flen = 0;
        if (read_file_to_buf(json_path, &filedata, &flen) == 0 && filedata) {
            ok("Local cache is up to date (HTTP 304)");
            cache_put(branch, filedata);
            if (hdrs) curl_slist_free_all(hdrs);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return filedata;
        }
        
    }

    if (res == CURLE_OK && chunk.data) {
        
        if (rh.etag[0] || rh.last_modified[0]) {
            char meta_buf[640] = {0};
            snprintf(meta_buf, sizeof(meta_buf), "ETag: %s\nLast-Modified: %s\n",
                     rh.etag[0] ? rh.etag : "",
                     rh.last_modified[0] ? rh.last_modified : "");
            if (meta_buf[0]) write_text_file(meta_path, meta_buf);
        }
        
        write_text_file(json_path, chunk.data);

        char msg[128];
        snprintf(msg, sizeof(msg), "Received %.2f MB for branch '%s'",
                 (double)chunk.size/1048576.0, branch);
        note(msg);
        note("Verifying JSON structure...");
        if (chunk.size == 0 || chunk.data[0] != '{') {
            warn("Unexpected response format (not a JSON object)");
        } else {
            ok("Looks like valid JSON");
        }
        cache_put(branch, chunk.data);
        if (hdrs) curl_slist_free_all(hdrs);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return chunk.data; 
    }

try_disk_fallback:;
    {
        char *filedata = NULL; size_t flen = 0;
        if (read_file_to_buf(json_path, &filedata, &flen) == 0 && filedata) {
            warn("Network unavailable — using disk cache");
            cache_put(branch, filedata);
            if (hdrs) curl_slist_free_all(hdrs);
            if (curl) curl_easy_cleanup(curl);
            curl_global_cleanup();
            return filedata;
        }
    }
    if (hdrs) curl_slist_free_all(hdrs);
    if (curl) curl_easy_cleanup(curl);
    curl_global_cleanup();
    return NULL;
}