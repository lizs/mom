// author : lizs
// 2017.2.22
#pragma once

#include "uv.h"

#pragma region("配置")
#define MONITOR_ENABLED true
#pragma endregion

#pragma region("typedefs")

typedef uint16_t pack_size_t;
typedef uint32_t serial_t;
typedef uint16_t node_id_t;
typedef uint16_t session_id_t;
typedef uint16_t mask_t;
typedef uint16_t cbuf_len_t;
typedef uint8_t pattern_t;

#pragma endregion 

#pragma region("enums")

enum Pattern {
	Push = 0,
	Request = 1,
	Response = 2,
};

/*
Pattern(4 bit) NodesCnt(4bit) Command(8 bit)
*/
enum Mask {
	MaskPattern = 0xC000,
	MaskNodesCnt = 0xC000,
	MaskCmd = 0x00FF,
};

/*
暂支持最多64个MOM命令
*/
enum SysCommand {
	Null,
	KeepAlive,
	Register,
};

//static Pattern get_pattern(mask_t mask) {
//	return static_cast<Pattern>(mask & MaskPattern);
//}
//
//static mask_t make_pattern(Pattern pattern) {
//	return static_cast<mask_t>(pattern) << 12;
//}
//
////static bool get_error(mask_t mask) {
////	return mask & MaskError;
////}
////
////static mask_t make_error(bool error) {
////	return error ? MaskError : 0;
////}
//
//static uint8_t get_nodes_cnt(mask_t mask) {
//	return static_cast<uint8_t>(mask & MaskPattern) >> 8;
//}
//
//static mask_t make_nodes_cnt(uint8_t cnt) {
//	return static_cast<mask_t>(cnt) << 8;
//}
//
//static SysCommand get_cmd(mask_t mask) {
//	return static_cast<SysCommand>(mask & MaskCmd);
//}
//
//static mask_t make_cmd(SysCommand cmd) {
//	return static_cast<mask_t>(cmd);
//}

// MOM系统错误
// < 0 为系统错误码
// > 0 为业务错误码
enum SysError {
	Invalid = SHRT_MIN,
	ServerNotFound,
	Success = 0,
};

#pragma endregion 



#define PRINT_UV_ERR(code)         \
 do {                                                     \
	if (code) {                                          \
		fprintf(stderr,                                       \
            "%s : %s\n",    \
			uv_err_name(code),				\
			uv_strerror(code));                                       \
	 }                                                       \
 } while (0)

#define PRINT_UV_ERR_DETAIL(code)         \
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

#define LOG_UV_ERR(code) PRINT_UV_ERR_DETAIL(code)

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
		uv_close(handle, nullptr);
}

UNUSED static void close_loop(uv_loop_t* loop) {
	uv_walk(loop, close_walk_cb, nullptr);
	uv_run(loop, UV_RUN_DEFAULT);
}

#define RUN_UV_DEFAULT_LOOP()		\
do{												\
	int r;												\
	uv_loop_t* loop;							\
	loop = uv_default_loop();			\
	r = uv_run(loop, UV_RUN_DEFAULT);	\
	if(r)												\
		LOG_UV_ERR(r);						\
	MAKE_VALGRIND_HAPPY();		\
}while(0)