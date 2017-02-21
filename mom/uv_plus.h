#pragma once

#include "uv.h"

#pragma region("≈‰÷√")
#define MONITOR_ENABLED true
#pragma endregion

using ushort = unsigned short;

typedef enum {
	TCP = 0,
	UDP,
	PIPE
} stream_type;

#define LOG_UV_ERR(code)         \
 do {                                                     \
	if (code) {                                          \
		fprintf(stderr,                                       \
            "%s : %s\n",    \
			uv_err_name(code),				\
			uv_strerror(code));                                       \
	 }                                                       \
 } while (0)

#define LOG_UV_ERR_DETAIL(code)         \
 do {                                                     \
	if (code) {                                          \
		fprintf(stderr,                                       \
            "%s : %s [%s : %s : %d]\n",    \
			uv_err_name(code),				\
			uv_strerror(code),					\
            __FILE__,                                     \
			__FUNCTION__	,							\
            __LINE__);                                       \
	}                                                       \
 } while (0)

#define LOG(format, ...)         \
 do {                                                     \
    fprintf(stdout,                                       \
            format,										\
			##__VA_ARGS__);                                       \
	printf("\n");                                      \
 } while (0)

/* Die with fatal error. */
#define FATAL(msg)                                        \
  do {                                                    \
    fprintf(stderr,                                       \
            "Fatal error in %s on line %d: %s\n",         \
            __FILE__,                                     \
            __LINE__,                                     \
            msg);                                         \
    fflush(stderr);                                       \
    abort();                                              \
  } while (0)

#define ASSERT(expr)                                      \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Assertion failed in %s on line %d: %s\n",    \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr);                                       \
    abort();                                              \
  }                                                       \
 } while (0)


#if defined(__clang__) ||                                \
    defined(__GNUC__) ||                                 \
    defined(__INTEL_COMPILER) ||                         \
    defined(__SUNPRO_C)
# define UNUSED __attribute__((unused))
#else
# define UNUSED
#endif

/* This macro cleans up the main loop. This is used to avoid valgrind
* warnings about memory being "leaked" by the main event loop.
*/
#define MAKE_VALGRIND_HAPPY()                       \
  do {                                              \
    close_loop(uv_default_loop());                  \
    ASSERT(0 == uv_loop_close(uv_default_loop()));  \
  } while (0)

/* Fully close a loop */
UNUSED static void close_walk_cb(uv_handle_t* handle, void* arg) {
	if (!uv_is_closing(handle))
		uv_close(handle, NULL);
}

UNUSED static void close_loop(uv_loop_t* loop) {
	uv_walk(loop, close_walk_cb, NULL);
	uv_run(loop, UV_RUN_DEFAULT);
}
