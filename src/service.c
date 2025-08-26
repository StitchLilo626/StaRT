/**
 * @file service.c
 * @brief Minimal formatted output services (lightweight printf).
 * @version 1.0.2
 * @date 2025-08-26
 * @author
 *   StitchLilo626
 * @note
 *   History:
 *     - 2025-08-26 1.0.2 Translate comments to English.
 */

#include "start.h"
#include <stdarg.h>

/**
 * @brief Weak putc hook (user may override for UART / SWO / etc.).
 */
__weak void s_putc(char c)
{
    (void)c;
}

/**
 * @brief Very small vsnprintf-like formatter (supports %d %s %c).
 * @param buffer Output buffer.
 * @param size Buffer size (including terminator).
 * @param fmt Format string.
 * @param args Vararg list.
 */
int s_vsnprintf(char *buffer, size_t size, const char *fmt, va_list args)
{
    char *ptr       = buffer;
    const char *end = buffer + size - 1; /* Preserve space for NUL */

    while (*fmt && ptr < end)
    {
        if (*fmt == '%')
        {
            fmt++;
            if (*fmt == 'd')
            {
                int value = va_arg(args, int);
                char temp[12];
                int  len = 0;

                if (value < 0)
                {
                    if (ptr < end) *ptr++ = '-';
                    value = -value;
                }
                do
                {
                    temp[len++] = (char)('0' + (value % 10));
                    value /= 10;
                } while (value && len < (int)sizeof(temp));

                while (len-- && ptr < end)
                    *ptr++ = temp[len];
            }
            else if (*fmt == 's')
            {
                const char *str = va_arg(args, const char *);
                while (*str && ptr < end)
                    *ptr++ = *str++;
            }
            else if (*fmt == 'c')
            {
                char c = (char)va_arg(args, int);
                if (ptr < end) *ptr++ = c;
            }
            else
            {
                if (ptr < end) *ptr++ = '%';
                if (ptr < end) *ptr++ = *fmt;
            }
        }
        else
        {
            if (ptr < end) *ptr++ = *fmt;
        }
        fmt++;
    }

    *ptr = '\0';
    return (int)(ptr - buffer);
}

/**
 * @brief Lightweight printf forwarding to s_putc().
 */
void s_printf(const char *fmt, ...)
{
    char buffer[S_PRINTF_BUF_SIZE];
    va_list args;
    int length;

    va_start(args, fmt);
    length = s_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    for (int i = 0; i < length; i++)
        s_putc(buffer[i]);
}
