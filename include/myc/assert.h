#ifndef _MYC_ASSERT_H_
#define _MYC_ASSERT_H_

#ifndef __FUNC_NAME__
#if __STDC_VERSION__ >= 199901L
    #define __FUNC_NAME__ __func__
#else
    #define __FUNC_NAME__ "N/A"
#endif
#endif

#ifndef _MYC_DISABLE_ASSERTIONS
    #include <stdlib.h>

    #define MYC_ASSERT(CHECK, ...)                                                                  \
        if (!(CHECK)) {                                                                             \
            _myc_private_log_assert_failed(__FUNC_NAME__, __FILE__, __LINE__, #CHECK, __VA_ARGS__); \
            abort();                                                                                \
        }

    #define MYC_UNREACHABLE(...)                                                                    \
        _myc_private_log_unreachable(__FUNC_NAME__, __FILE__, __LINE__, __VA_ARGS__);               \
        abort();
    
    __attribute__((format(printf, 5, 6)))
    void _myc_private_log_assert_failed(const char *func_name, const char *file_path, int line_nr, const char *check, const char *message_fmt, ...);

    __attribute__((format(printf, 4, 5)))
    void _myc_private_log_unreachable(const char *func_name, const char *file_path, int line_nr, const char *message_fmt, ...);
#else
    #define MYC_ASSERT(CHECK, ...)
    #define MYC_UNREACHABLE(...) __builtin_unreachable()
#endif // _MYC_DIABLE_ASSERRTIONS

#endif // _MYC_ASSERT_H_