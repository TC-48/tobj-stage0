#pragma once

#include <stdarg.h>

#if defined(__GNUC__) || defined(__clang__)
#  define TOOL_PRINTF_FUNCTION(FMT, ARGS) __attribute__((format(printf, FMT, ARGS)))
#else
#  define TOOL_PRINTF_FUNCTION(FMT, ARGS)
#endif

typedef enum ToolOutputType {
    TOOL_OUTPUT_ERROR,
    TOOL_OUTPUT_WARN,
    TOOL_OUTPUT_INFO,
    TOOL_OUTPUT_DEBUG,
} ToolOutputType;

void tool_output(ToolOutputType type, const char* fmt, ...) TOOL_PRINTF_FUNCTION(2, 3);
void tool_output_valist(ToolOutputType type, const char* fmt, va_list args);
void tool_output_inline(ToolOutputType type, const char* fmt, ...) TOOL_PRINTF_FUNCTION(2, 3);
void tool_output_inline_valist(ToolOutputType type, const char* fmt, va_list args);
void tool_output_inline_finish(ToolOutputType type);

void tool_print(const char* fmt, ...) TOOL_PRINTF_FUNCTION(1, 2);
void tool_puts(const char* str);
void tool_putc(char c);

#define tool_show_error(...) tool_output(TOOL_OUTPUT_ERROR, __VA_ARGS__)
#define tool_show_warn(...)  tool_output(TOOL_OUTPUT_WARN,  __VA_ARGS__)
#define tool_show_info(...)  tool_output(TOOL_OUTPUT_INFO,  __VA_ARGS__)
#define tool_show_error_inline(...) tool_output_inline(TOOL_OUTPUT_ERROR, __VA_ARGS__)
#define tool_show_warn_inline(...)  tool_output_inline(TOOL_OUTPUT_WARN,  __VA_ARGS__)
#define tool_show_info_inline(...)  tool_output_inline(TOOL_OUTPUT_INFO,  __VA_ARGS__)
#define tool_show_error_inline_finish() tool_output_inline_finish(TOOL_OUTPUT_ERROR)
#define tool_show_warn_inline_finish()  tool_output_inline_finish(TOOL_OUTPUT_WARN)
#define tool_show_info_inline_finish()  tool_output_inline_finish(TOOL_OUTPUT_INFO)

#ifndef NDEBUG
#  define tool_show_debug(...) tool_output(TOOL_OUTPUT_DEBUG, __VA_ARGS__)
#else
#  define tool_show_debug(...) ((void)0)
#endif
