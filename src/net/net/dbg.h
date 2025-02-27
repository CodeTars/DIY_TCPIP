#ifndef DBG_H
#define DBG_H
#include "net_cfg.h"

void dbg_print(int module, int s_level, const char *file, const char *func, int line, const char *fmt, ...);

#define DBG_LEVEL_NONE 0
#define DBG_LEVEL_ERROR 1
#define DBG_LEVEL_WARNING 2
#define DBG_LEVEL_INFO 3

#define DBG_STYLE_RESET "\033[0m"    // 复位显示
#define DBG_STYLE_ERROR "\033[31m"   // 红色显示
#define DBG_STYLE_WARNING "\033[33m" // 黄色显示

#define dbg_info(module, fmt, ...) dbg_print(module, DBG_LEVEL_INFO, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define dbg_warning(module, fmt, ...) dbg_print(module, DBG_LEVEL_WARNING, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define dbg_error(module, fmt, ...) dbg_print(module, DBG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define dbg_assert(expr, msg)                                                                                            \
    if (!(expr)) {                                                                                                       \
        dbg_print(DBG_LEVEL_ERROR, DBG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, "assert failed: " #expr ", " msg); \
        while (1) {                                                                                                      \
            sys_sleep(1000);                                                                                             \
        }                                                                                                                \
    }

#define DBG_DISP_ENABLED(module) (module >= DBG_LEVEL_INFO)

#endif