/**
 * The MIT License (MIT)
 *
 * Copyright (c) <2019> <Kartik Aiyer>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cutils/logger.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#define LOG_MAX_LINE_LENGTH                      (512)

#ifdef USE_STD_PRINTF
#define debugf     printf
#else
#define debugf
#endif

SIMPLE_LOGGER_DEF(general_logger, "general", true);

uint32_t logger_write_string(logger_t * p_logger,
                             void *p_private,
                             const char *fmt, char *p_buffer, uint32_t buffer_size, va_list args)
{
  size_t fmtLen = strlen(fmt);
  uint32_t index = 0;

  if (fmtLen < (buffer_size - 1) / 2) {
    if (p_logger->fnPrefix) {
      index += p_logger->fnPrefix(p_logger,
                                  p_private, p_buffer + index, buffer_size - index - (uint32_t) fmtLen);
      if (index < buffer_size - 2) {
        p_buffer[index++] = ':';
        p_buffer[index++] = ' ';
      } else {
        return index;
      }
    }
    index += vsprintf(p_buffer + index, fmt, args);
  }

  return index;
}

uint32_t SimpleLoggerPrefix(logger_t * p_logger, void *p_private, char *p_string, uint32_t string_size)
{
  int size = 0;
  simple_logger_t *p_simple = (simple_logger_t *) p_logger;

  size = sprintf(p_string, "%s", p_simple->pPrefix);
  return (size > 0) ? (uint32_t) size : 0;
}

void console_log(logger_t *logger, void *p_private, char *str, ...)
{
  char buf[LOG_MAX_LINE_LENGTH];
  va_list args;

  va_start(args, str);
  int index = 0;

  if (logger && logger->isEnabled) {
    time_t now = time(NULL);
    struct tm ts = *localtime(&now);

    index = sprintf(buf, "[%04u/%02u/%02u-%02u:%02u:%02u]: ",
                    ts.tm_year,
                    ts.tm_mon,
                    ts.tm_mday,
                    ts.tm_hour,
                    ts.tm_min,
                    ts.tm_sec);
    uint32_t bytes_written = logger_write_string(logger, p_private, str, buf + index, sizeof(buf) - index, args);
    sprintf(buf + index + bytes_written, "\n");
    debugf(buf);
  }
  va_end(args);
}

void console_log_raw(char* str, ...)
{
  va_list args;
  va_start(args, str);
  debugf(str, args);
  va_end(args);
}
