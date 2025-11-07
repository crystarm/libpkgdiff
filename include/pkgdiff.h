#ifndef PKGDIFF_H
#define PKGDIFF_H

/* Coloring macros shared across modules */

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
#define C_ACCENT CSI "38;2;61;174;233m"  /* #3DAEE9 */

#if !defined(PKGDIFF_API)
#  if defined(__GNUC__) && !defined(_WIN32)
#    define PKGDIFF_API __attribute__((visibility("default")))
#  else
#    define PKGDIFF_API
#  endif
#endif
#ifdef __cplusplus
extern "C" {
#endif
PKGDIFF_API void hello_pkgdiff(void);
PKGDIFF_API char *fetch_packages_json(const char *branch);
PKGDIFF_API void compare_branches_interactive_min(const char *branch1, const char *branch2, const char *arch_filter);
PKGDIFF_API void common_packages_interactive_min(const char *branch1, const char *branch2, const char *arch_filter, const char *save_path);
#if !defined(PKGDIFF_API)
#  if defined(__GNUC__) && !defined(_WIN32)
#    define PKGDIFF_API __attribute__((visibility("default")))
#  else
#    define PKGDIFF_API
#  endif
#endif
#ifdef __cplusplus
}
#endif
#endif
