#ifndef _MYC_LOG_H_
#define _MYC_LOG_H_

#if __STDC_VERSION__ >= 199901L
    #define __FUNC_NAME__ __func__
#else
    #define __FUNC_NAME__ "N/A"
#endif

#ifndef _MYC_LOG_LEVEL
    #define _MYC_LOG_LEVEL 3
#endif

#define MYC_FMT_BOLD(TEXT)   "\033[1m" TEXT "\033[22m"
#define MYC_FMT_ITALIC(TEXT) "\033[2m" TEXT "\033[22m"
#define MYC_FMT_RED(TEXT)    "\033[91m" TEXT "\033[39m"
#define MYC_FMT_YELLOW(TEXT) "\033[93m" TEXT "\033[39m"
#define MYC_FMT_GREEN(TEXT)  "\033[92m" TEXT "\033[39m"
#define MYC_FMT_BLUE(TEXT)   "\033[94m" TEXT "\033[39m"



#if _MYC_LOG_LEVEL > 0
    #define MYC_LOG(...)        _myc_private_log(__VA_ARGS__)
    #define MYC_LOG_ERROR(...)  _myc_private_log_error(__FUNC_NAME__, __FILE__, __LINE__, __VA_ARGS__)

    __attribute__((format(printf, 1, 2)))
    void _myc_private_log(const char *message_fmt, ...);

    __attribute__((format(printf, 4, 5)))
    void _myc_private_log_error(const char *func_name, const char *file_path, int line_nr, const char *message_fmt, ...);
#else
    #define MYC_LOG(...)
    #define MYC_LOG_ERROR(...)
#endif // _MYC_LOG_LEVEL > 0



#if _MYC_LOG_LEVEL > 1
    #define MYC_LOG_WARN(...) _myc_private_log_warn(__FUNC_NAME__, __FILE__, __LINE__, __VA_ARGS__)
    #define MYC_LOG_INFO(...) _myc_private_log_info(__FUNC_NAME__, __FILE__, __LINE__, __VA_ARGS__)

    __attribute__((format(printf, 4, 5)))
    void _myc_private_log_warn(const char *func_name, const char *file_path, int line_nr, const char *msg_fmt, ...);

    __attribute__((format(printf, 4, 5)))
    void _myc_private_log_info(const char *func_name, const char *file_path, int line_nr, const char *msg_fmt, ...);
#else
    #define MYC_LOG_WARN(...) 
    #define MYC_LOG_INFO(...)       
#endif // _MYC_LOG_LEVEL > 1



#if _MYC_LOG_LEVEL > 2
    #define MYC_LOG_TRACE(...) _myc_private_log_trace(__FUNC_NAME__, __FILE__, __LINE__, __VA_ARGS__)
    #define MYC_LOG_DEBUG(...) _myc_private_log_debug(__FUNC_NAME__, __FILE__, __LINE__, __VA_ARGS__)
    #define MYC_LOG_TODO(...)  _myc_private_log_todo(__FUNC_NAME__, __FILE__, __LINE__, __VA_ARGS__)

    __attribute__((format(printf, 4, 5)))
    void _myc_private_log_trace(const char *func_name, const char *file_path, int line_nr, const char *msg_fmt, ...);

    __attribute__((format(printf, 4, 5)))
    void _myc_private_log_debug(const char *func_name, const char *file_path, int line_nr, const char *msg_fmt, ...);

    __attribute__((format(printf, 4, 5)))
    void _myc_private_log_todo(const char *func_name, const char *file_path, int line_nr, const char *msg_fmt, ...);
#else
    #define MYC_LOG_TRACE(...)
    #define MYC_LOG_DEBUG(...)
    #define MYC_LOG_TODO(...)
#endif // _MYC_LOG_LEVEL > 2

#endif // _MYC_LOG_H_