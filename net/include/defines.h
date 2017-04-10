// author : lizs
// 2017.2.22
#pragma once

#include "uv.h"
#include <memory>
#include <functional>
#include "mem_pool.h"
#include "singleton.h"

#ifdef LINK_DYN
#ifdef NET_EXPORT
		#define NET_API __declspec( dllexport )
#else
		#define NET_API __declspec( dllimport )
#endif
#else
	#define NET_API
#endif


#pragma region("配置")
#define MESSAGE_TRACK_ENABLED false
#define TRACK_MESSAGE_AS_BINARY true
#define MONITOR_ENABLED true
#define CBUF_RESERVED_SIZE 32
#define MAX_PACKAGE_SIZE 1024
#define KEEP_ALIVE_INTERVAL 10 * 1000
#define KEEP_ALIVE_COUNTER_DEAD_LINE 5
#pragma endregion

#pragma region("typedefs")

typedef int16_t ops_t;
typedef uint8_t byte_t;
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
	Push,
	Request,
	Response,
	Ping,
	Pong,
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


#define PRINT_MESSAGE_AS_BINARY(buf, len, format, ...)         \
 do {                                                     \
    fprintf(stdout,                                       \
            format,										\
			##__VA_ARGS__);                                       \
	printf("[");                                      \
	for (auto i = 0; i < len; ++i) {			\
		printf("%02x ", buf[i]);				\
	}													\
	printf("]\n");                                      \
 } while (0)


#define PRINT_MESSAGE_AS_STRING(buf, len, format, ...)         \
 do {                                                     \
    fprintf(stdout,                                       \
            format,										\
			##__VA_ARGS__);                                       \
	printf("[");                                      \
		printf(buf);				\
	printf("]\n");                                      \
 } while (0)

#if TRACK_MESSAGE_AS_BINARY
#define PRINT_MESSAGE PRINT_MESSAGE_AS_BINARY
#else
#define PRINT_MESSAGE PRINT_MESSAGE_AS_STRING
#endif

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
	Singleton<Monitor>::instance().start(); \
	r = uv_run(loop, UV_RUN_DEFAULT);	\
	if(r)												\
		LOG_UV_ERR(r);						\
	MAKE_VALGRIND_HAPPY();		\
}while(0)

namespace VK {
	namespace Net {
		class Session;
		class CircularBuf;
		class Monitor;
	}
}

typedef VK::Net::Session session_t;
typedef VK::Net::CircularBuf cbuf_t;
typedef std::shared_ptr<cbuf_t> cbuf_ptr_t;
typedef VK::Net::MemoryPool<cbuf_t> cbuf_pool_t;
typedef VK::Net::Monitor monitor_t;
typedef std::function<void(bool)> send_cb_t;
typedef std::function<void(bool, session_t*)> open_cb_t;
typedef std::function<void(session_t*)> close_cb_t;
typedef std::function<void(error_no_t, cbuf_ptr_t)> req_cb_t;
typedef std::function<void(session_t*, cbuf_ptr_t)> push_handler_t;
typedef std::function<void(session_t*, cbuf_ptr_t, req_cb_t)> req_handler_t;

typedef struct {
	uv_write_t req;
	uv_buf_t buf;
	send_cb_t cb;
	cbuf_ptr_t pcb;

	void clear() {
		pcb = nullptr;
		cb = nullptr;
	}
} write_req_t;

typedef VK::Net::Singleton<VK::Net::MemoryPool<write_req_t, 1>> wr_pool_t;

typedef struct {
	uv_connect_t req;
	session_t* session;
	std::function<void(bool, session_t*)> cb;
} connect_req_t;

typedef struct {
	uv_shutdown_t req;
	session_t* session;
	std::function<void(session_t*)> cb;
} shutdown_req_t;

typedef struct {
	uv_getaddrinfo_t req;
	std::function<void(bool, sockaddr *)> cb;
} getaddr_req_t;

// mom protocols
#pragma pack(1)

typedef struct {
	node_id_t nid;
	node_type_t ntype;
} node_register_t;

typedef struct {
	node_id_t nid;
} node_unregister_t;

typedef struct {
	///node_type_t ntype;
	node_id_t nid;
} coordinate_rep_t;

typedef struct {
	node_type_t ntype;
} coordinate_req_t;

// REQ包头
typedef struct {
	ops_t ops;
	node_id_t nid;
	node_id_t gate_id;
	session_id_t sid;
} req_header_t;

// PUSH包头
typedef struct {
	ops_t ops;
	node_id_t nid;
} push_header_t;

typedef union {
	req_header_t req;
	push_header_t push;
} header_t;

// ping/pong 包头为空

// 广播
typedef struct {
	node_type_t ntype;
} broadcast_push_t;

// 组播
typedef struct {
	node_type_t ntype;
} multicast_push_t;

#pragma pack()

// 长度+包头
#define HEADER_SIZE sizeof(header_t) + sizeof(cbuf_len_t)
static_assert(CBUF_RESERVED_SIZE > HEADER_SIZE, "Reserved size too small.");

struct node_info_t {
	node_register_t reg;
	session_id_t sid;

	node_info_t(node_register_t& reg, session_id_t sid) : reg(reg), sid(sid) { }
};

// 系统命令 < 0
// MOM命令
enum MomOps : short {
	Invalid = SHRT_MIN,
	// 注册至网关
	Register2Gate,
	// 注册至协调服
	Register2Coordination,
	// 反注册
	UnregisterFromCoordination,
	// 协调（请求分配一个目标服务ID）
	Coordinate,
	// 广播
	Broadcast,
	// 组播
	Multicast,
};

enum MomError : error_no_t {
	ME_Begin = NE_End,
	ME_ReadHeader,
	ME_WriteHeader,
	ME_InvlidOps,
	ME_ReadRegisterInfo,
	ME_ReadUnregisterInfo,
	ME_TargetNodeNotExist,
	ME_NodeIdAlreadyExist,
	ME_NoHandler,
	ME_NotImplemented,
	ME_TargetSessionNotExist,
	ME_MomClientsNotReady,
	ME_CoordinationServerNotExist,
	ME_TargetServerNotExist,
	ME_ReadCoordinateInfo,
	ME_ReadBroadcastInfo,
	ME_End,
};

enum NodeType {
	NT_Game = 1,
	NT_Gate,
	NT_Coordination,
};
