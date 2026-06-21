#define _POSIX_C_SOURCE 200809L

#include <toolcommon/output.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef _WIN32
#  include <io.h>
#  define isatty _isatty
#  define fileno _fileno
#else
#  include <unistd.h>
#endif

#define BOLD "\033[1m"
#define RESET "\033[0m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define WHITE "\033[37m"

static bool should_use_color(FILE* stream) {
    const char* no_color = getenv("NO_COLOR");
    if (no_color && no_color[0] != '\0') {
        return false;
    }
    return isatty(fileno(stream));
}

static void tool_output_meta(ToolOutputType type, FILE** out_stream, const char** out_prefix, const char** out_color) {
    FILE* stream = stdout;
    const char* prefix = "";
    const char* color = "";

    switch (type) {
    case TOOL_OUTPUT_ERROR:
        stream = stderr;
        prefix = "error: ";
        color = BOLD RED;
        break;
    case TOOL_OUTPUT_WARN:
        stream = stderr;
        prefix = "warning: ";
        color = BOLD YELLOW;
        break;
    case TOOL_OUTPUT_INFO:
        stream = stdout;
        prefix = "info: ";
        color = BOLD BLUE;
        break;
    case TOOL_OUTPUT_DEBUG:
        stream = stdout;
        prefix = "debug: ";
        color = BOLD WHITE;
        break;
    }

    *out_stream = stream;
    *out_prefix = prefix;
    *out_color = color;
}

static void tool_output_write_prefix(FILE* stream, const char* prefix, const char* color) {
    const char* reset = RESET;
    bool use_color = should_use_color(stream);

    if (use_color && color[0] != '\0') {
        fprintf(stream, "%s%s%s", color, prefix, reset);
    } else {
        fprintf(stream, "%s", prefix);
    }
}

void tool_output_valist(ToolOutputType type, const char* fmt, va_list args) {
    FILE* stream = stdout;
    const char* prefix = "";
    const char* color = "";
    tool_output_meta(type, &stream, &prefix, &color);
    tool_output_write_prefix(stream, prefix, color);

    vfprintf(stream, fmt, args);
    fprintf(stream, "\n");
}

void tool_output(ToolOutputType type, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    tool_output_valist(type, fmt, args);
    va_end(args);
}

void tool_output_inline_valist(ToolOutputType type, const char* fmt, va_list args) {
    FILE* stream = stdout;
    const char* prefix = "";
    const char* color = "";
    tool_output_meta(type, &stream, &prefix, &color);
    fprintf(stream, "\r");
    tool_output_write_prefix(stream, prefix, color);
    vfprintf(stream, fmt, args);
    fflush(stream);
}

void tool_output_inline(ToolOutputType type, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    tool_output_inline_valist(type, fmt, args);
    va_end(args);
}

void tool_output_inline_finish(ToolOutputType type) {
    FILE* stream = (type == TOOL_OUTPUT_ERROR || type == TOOL_OUTPUT_WARN) ? stderr : stdout;
    fprintf(stream, "\n");
    fflush(stream);
}

void tool_print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

void tool_puts(const char* str) {
    fputs(str, stdout);
}

void tool_putc(char c) {
    fputc(c, stdout);
}
