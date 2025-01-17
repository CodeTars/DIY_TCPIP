#include <stdarg.h>
#include "dbg.h"
#include "sys_plat.h"

void dbg_print(int module, int s_level, const char *file, const char *func, int line, const char *fmt, ...)
{
    if (module >= s_level)
    {
        static const char *title[] = {
            [DBG_LEVEL_ERROR] = DBG_STYLE_ERROR "error",
            [DBG_LEVEL_WARNING] = DBG_STYLE_WARNING "warning",
            [DBG_LEVEL_INFO] = "info",
            [DBG_LEVEL_NONE] = "none",
        };
        char *end = file + strlen(file) - 1;
        while (*end != '/' && *end != '\\')
        {
            --end;
        }
        ++end;
        printf("%s at %s-%s-%d: ", title[s_level], end, func, line);

        char buffer[128];
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buffer, fmt, ap);
        va_end(ap);

        printf("%s\n" DBG_STYLE_RESET, buffer);
    }
}