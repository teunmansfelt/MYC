#include <stdarg.h>
typedef va_list va_list_t;
#include <stdio.h>

#include "myc/log.h"

static inline void print_header(const char *label, const char *func_name, const char *file_path, int line_nr) 
{
    fprintf(stderr, "%s:   In function '"MYC_FMT_BOLD("%s")"'   (file: "MYC_FMT_BOLD("%s")" | line: "MYC_FMT_BOLD("%d")"):\n",
            label, func_name, file_path, line_nr);
}

static void print_message(const char *message_fmt, va_list_t args)
{
    const size_t buffer_size = 1 + snprintf(NULL, 0, "  | %s\n", message_fmt);
    char fmt[buffer_size];
    snprintf(fmt, buffer_size, "  | %s\n", message_fmt);
    vfprintf(stderr, fmt, args);
}

void _myc_private_log(const char *message_fmt, ...)
{
    va_list_t args;
    va_start(args, message_fmt);
    print_message(message_fmt, args);
    va_end(args);
}

void _myc_private_log_error(const char *func_name, const char *file_path, int line_nr, const char *message_fmt, ...)
{
    print_header("["MYC_FMT_RED("ERROR")"]", func_name, file_path, line_nr);
    
    va_list_t args;
    va_start(args, message_fmt);
    print_message(message_fmt, args);
    va_end(args);
}

void _myc_private_log_trace(const char *func_name, const char *file_path, int line_nr, const char *message_fmt, ...)
{
    print_header("("MYC_FMT_RED("trace")")", func_name, file_path, line_nr);
    
    va_list_t args;
    va_start(args, message_fmt);
    print_message(message_fmt, args);
    va_end(args);
}

void _myc_private_log_warn(const char *func_name, const char *file_path, int line_nr, const char *message_fmt, ...)
{
    print_header("["MYC_FMT_YELLOW("WARNING")"]", func_name, file_path, line_nr);
    
    va_list_t args;
    va_start(args, message_fmt);
    print_message(message_fmt, args);
    va_end(args);
}

void _myc_private_log_info(const char *func_name, const char *file_path, int line_nr, const char *message_fmt, ...)
{
    print_header("["MYC_FMT_GREEN("INFO")"]", func_name, file_path, line_nr);
    
    va_list_t args;
    va_start(args, message_fmt);
    print_message(message_fmt, args);
    va_end(args);
}

void _myc_private_log_debug(const char *func_name, const char *file_path, int line_nr, const char *message_fmt, ...)
{
    print_header("(debug)", func_name, file_path, line_nr);
    
    va_list_t args;
    va_start(args, message_fmt);
    print_message(message_fmt, args);
    va_end(args);
}

void _myc_private_log_todo(const char *func_name, const char *file_path, int line_nr, const char *message_fmt, ...)
{
    print_header("["MYC_FMT_BLUE("TODO")"]", func_name, file_path, line_nr);
    
    va_list_t args;
    va_start(args, message_fmt);
    print_message(message_fmt, args);
    va_end(args);
}

void _myc_private_log_assert_failed(const char *func_name, const char *file_path, int line_nr, const char *check, const char *message_fmt, ...)
{
    print_header("["MYC_FMT_RED("FAILED ASSERTION")"]", func_name, file_path, line_nr);

    const size_t buffer_size = 1 + snprintf(NULL, 0, "Assertion '"MYC_FMT_RED("%s")"' failed!   =>   %s", check, message_fmt);
    char ext_message_fmt[buffer_size];
    snprintf(ext_message_fmt, buffer_size, "Assert '"MYC_FMT_RED("%s")"' failed!   =>   %s", check, message_fmt);

    va_list_t args;
    va_start(args, message_fmt);
    print_message(ext_message_fmt, args);
    va_end(args);
}

void _myc_private_log_unreachable(const char *func_name, const char *file_path, int line_nr, const char *message_fmt, ...)
{
    print_header("["MYC_FMT_RED("UNREACHABLE CODE")"]", func_name, file_path, line_nr);

    const size_t buffer_size = 1 + snprintf(NULL, 0, "Broken control flow   =>   %s", message_fmt);
    char ext_message_fmt[buffer_size];
    snprintf(ext_message_fmt, buffer_size, "Broken control flow   =>   %s", message_fmt);

    va_list_t args;
    va_start(args, message_fmt);
    print_message(ext_message_fmt, args);
    va_end(args);
}