// Kartik Aiyer
#pragma once

#include <cutils/types.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Logger
{
  uint32_t (*fnPrefix)(struct _Logger *, void *pPrivate, char *pString, uint32_t stringSize);

  bool isEnabled;
  uint32_t log_level;
} logger_t;

typedef struct _SimpleLogger
{
  logger_t base;
  char *pPrefix;
} simple_logger_t;

uint32_t SimpleLoggerPrefix(logger_t *p_logger, void *p_private, char *p_string, uint32_t string_size);

#define SIMPLE_LOGGER_DECL(name) \
simple_logger_t sysLog_##name

#define SIMPLE_LOGGER_DEF(name, prefix, enabled)\
simple_logger_t sysLog_##name = { .base.isEnabled = (enabled),\
  .base.fnPrefix = SimpleLoggerPrefix,\
  .base.log_level = 0,\
  .pPrefix = (prefix),\
}

#define SIMPLE_LOGGER_STRUCT_INIT(name, prefix, enabled)\
sysLog_##name = {\
  .base.isEnabled = (enabled),\
  .base.fnPrefix = SimpleLoggerPrefix,\
  .base.log_level = 0,\
  .pPrefix = (prefix)\
}

#define SIMPLE_LOGGER_INIT(logger, prefix, enabled)\
{\
  (logger)->base.isEnabled = (enabled);\
  (logger)->base.fnPrefix = SimpleLoggerPrefix;\
  (logger)->base.log_level = 0;\
  (logger)->pPrefix = prefix;\
}

static inline simple_logger_t simple_logger_init(char *prefix, bool enabled)
{
  simple_logger_t logger;

  logger.base.isEnabled = enabled;
  logger.base.fnPrefix = SimpleLoggerPrefix;
  logger.base.log_level = 0;
  logger.pPrefix = prefix;
  return logger;
}

#define SIMPLE_LOGGER(name) sysLog_##name

extern SIMPLE_LOGGER_DECL(general_logger);

uint32_t logger_write_string(logger_t *p_logger,
                             void *p_private,
                             const char *fmt, char *p_buffer, uint32_t buffer_size, va_list args);

void console_log(logger_t *logger, void *p_private, char *str, ...);

void console_log_raw(char *str, ...);

/*
 * Can be used to build up a string.
 * Initialize n_written to zero on the first call
 */
#define INSERT_TO_STRING(p_str, n_written, fmt, ...)\
do{\
  int res = sprintf( p_str + n_written, fmt, ##__VA_ARGS__ );\
  n_written += (res > 0) ? res : 0;\
}while(0)

#define CLOG(str, ...)   console_log( &SIMPLE_LOGGER( general_logger ).base, NULL,(char*)"%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__ )
#define CHECK_RETURN(expr, retval) if( !(expr) ) { CLOG( #expr " failed " ); return retval; }
#define CHECK_RETURNF(expr, retval, str, ...) if(!(expr)) { CLOG( #expr " failed - " str, ##__VA_ARGS__ ); return retval;}
#define CHECK_RUN(expr, run_lines, str, ...) if(!(expr)) { CLOG( #expr " failed - " str, ##__VA_ARGS__); run_lines; }

#define CLOG_IF(cond, str, ...);  { if( (cond) ) CLOG( str, ##__VA_ARGS__ ); }
#define LOG_IF(LOGGER_MACRO, cond, str, ...); { if( (cond) ) LOGGER_MACRO(str, ##__VA_ARGS__ ); }

#define LOG_FIELD(x, f, MACRO)    MACRO(#x " = %" #f, x)
#define CLOG_FIELD(x, f)    LOG_FIELD( #x " = %" #f, x, CLOG)

/* NOTE: Compute the microseconds elapsed for a given function.
AmbaUtility_HighResolutionTimerStart() needs to be called somewhere earlier. */
#define TIMED_FUNC_US(retval, func, ...)\
{\
  UINT64 func##_start, func##_end;\
  AmbaUtility_GetHighResolutionTimeStamp(&func##_start);\
  func(__VA_ARGS__);\
  AmbaUtility_GetHighResolutionTimeStamp(&func##_end);\
  retval = func##_end - func##_start;\
}

/* NOTE: Compute the milliseconds elapsed for a given function. */
#define TIMED_FUNC_MS(retval, func, ...)\
{\
  uint32_t func##_start = task_get_ticks();\
  func(__VA_ARGS__);\
  retval = task_get_ticks() - func##_start;\
}

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

#define CUTILS_ASSERTF(cnd, str, ...) {\
  if(!(cnd)) {\
    CLOG(#cnd " Assertion failed @ %s line %u: " str, __FILE__, __LINE__, ##__VA_ARGS__);\
    *((volatile char*)0) =1;\
  }\
}

#ifdef __cplusplus
};
#endif
