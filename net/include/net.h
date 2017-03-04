// author : lizs
// 2017.2.22
#pragma once

#include "uv.h"
#include <cstdint>

#pragma region("≈‰÷√")
#define MONITOR_ENABLED true
#pragma endregion

#pragma region("typedefs")

typedef uint16_t pack_size_t;
typedef uint16_t serial_t;
typedef uint16_t error_no_t;
typedef uint16_t node_id_t;
typedef uint8_t node_type_t;
typedef uint16_t session_id_t;
typedef uint16_t mask_t;
typedef uint16_t cbuf_len_t;
typedef int16_t cbuf_offset_t;
typedef uint8_t pattern_t;

#pragma endregion 

#pragma region("enums")

enum Pattern : pattern_t {
	Push = 0,
	Request = 1,
	Response = 2,
};

enum NetError : error_no_t {
	Success = 0,
	NE_Write,
	NE_Read,
	NE_SerialConflict,
	NE_NoHandler,
	NE_ReadErrorNo,
	NE_SessionClosed,
	NE_End,
};

static const char* net_str_err(error_no_t error_no) {
	switch (static_cast<NetError>(error_no)) {
		case Success: return "Success.";
		case NE_Write: return "Write failed.";
		case NE_Read: return "Read failed.";
		case NE_SerialConflict: return "Serial failed.";
		case NE_NoHandler: return "No handler.";
		case NE_ReadErrorNo: return "Read errno failed.";
		case NE_SessionClosed: return "Session closed.";
		case NE_End: return "NE_End.";
		default: return "Unknown";
	}
}

#pragma endregion 

#define NEW_CBUF() session_t::cbuf_pool_t::instance()->newElement()
#define RELEASE_CBUF(pcb) session_t::cbuf_pool_t::instance()->deleteElement(pcb)

//#define NEW_CBUF() new session_t::cbuf_t()
//#define RELEASE_CBUF(pcb) delete pcb


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
    auto r = uv_loop_close(uv_default_loop());  \
	LOG_UV_ERR(r);	\
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
	MONITOR->start(); \
	r = uv_run(loop, UV_RUN_DEFAULT);	\
	if(r)												\
		LOG_UV_ERR(r);						\
	MAKE_VALGRIND_HAPPY();		\
}while(0)

#define TYPE_DEFINES(TSession) \
typedef typename TSession::session_t session_t; \
typedef typename TSession::cbuf_t cbuf_t; \
typedef typename TSession::cbuf_pool_t cbuf_pool_t; \
typedef typename TSession::send_cb_t send_cb_t; \
typedef typename TSession::open_cb_t open_cb_t; \
typedef typename TSession::close_cb_t close_cb_t; \
typedef typename TSession::req_cb_t req_cb_t; \
typedef typename TSession::push_handler_t push_handler_t; \
typedef typename TSession::req_handler_t req_handler_t;

#define SESSION_TYPE_DEFINES \
typedef Session<Size> session_t; \
typedef CircularBuf<Size> cbuf_t; \
typedef Singleton<MemoryPool<cbuf_t>> cbuf_pool_t; \
typedef std::function<void(bool)> send_cb_t; \
typedef std::function<void(bool, session_t*)> open_cb_t; \
typedef std::function<void(session_t*)> close_cb_t; \
typedef std::function<void(error_no_t, cbuf_t*)> req_cb_t; \
typedef std::function<void(session_t*, cbuf_t*)> push_handler_t; \
typedef std::function<void(session_t*, cbuf_t*, req_cb_t)> req_handler_t;

#define MONITOR Singleton<Monitor>::instance()