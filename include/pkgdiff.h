#ifndef PKGDIFF_H
#define PKGDIFF_H
#ifdef __cplusplus
extern "C" {
#endif
void hello_pkgdiff(void);
char *fetch_packages_json(const char *branch);
void compare_branches_interactive_min(const char *branch1, const char *branch2, const char *arch_filter);
#ifdef __cplusplus
}
#endif
#endif
