/* win32.h
 *
 */

#ifndef WIN32_H
#define WIN32_H

#include <Winsock2.h>
#include <errno.h>

#if defined(_MT) || defined(_DLL)
# define set_errno(x)    (*_errno()) = (x)
#else
# define set_errno(x)    errno = (x)
#endif

//#define EADDRINUSE  WSAEADDRINUSE

#pragma warning(disable : 4996)

#if defined(_MSC_VER)
// for MSVC 6.0
typedef          __int64    int64_t;
typedef unsigned __int64   uint64_t;
typedef          int        int32_t;
typedef unsigned int       uint32_t;
typedef          short      int16_t;
typedef unsigned short     uint16_t;
typedef          char        int8_t;
typedef unsigned char       uint8_t;
#define inline __inline
#endif // _WIN32 && _MSC_VER

#define pid_t int
#define EWOULDBLOCK        EAGAIN
#define EAFNOSUPPORT       47
typedef int socklen_t;
typedef int ssize_t;

#define O_BLOCK 0
#define O_NONBLOCK 1
#define F_GETFL 3
#define F_SETFL 4

#define IOV_MAX 1024
struct iovec {
	u_long iov_len;  
	char FAR* iov_base;
};
struct msghdr
{
	void	*msg_name;			/* Socket name			*/
	int		 msg_namelen;		/* Length of name		*/
	struct iovec *msg_iov;		/* Data blocks			*/
	int		 msg_iovlen;		/* Number of blocks		*/
	void	*msg_accrights;		/* Per protocol magic (eg BSD file descriptor passing) */ 
	int		 msg_accrightslen;	/* Length of rights list */
};

int fcntl(SOCKET s, int cmd, int val);
int inet_aton(register const char *cp, struct in_addr *addr);

#define close(s) closesocket(s)

#if (NTDDI_VERSION < NTDDI_LONGHORN)
inline int inet_pton(int af, register const char *cp, struct in_addr *addr)
{
    if(af != AF_INET) {
		WSASetLastError(WSAEPFNOSUPPORT);
		return -1;
    }
    return inet_aton(cp, addr);
}
#endif

inline size_t write(int s, void *buf, size_t len)
{
	DWORD dwBufferCount = 0;
	WSABUF wsabuf = { len, (char *)buf} ;
	if(WSASend(s, &wsabuf, 1, &dwBufferCount, 0, NULL, NULL) == 0) {
		return dwBufferCount;
	}
	if(WSAGetLastError() == WSAECONNRESET) return 0;
	return -1;
}

inline size_t read(int s, void *buf, size_t len)
{
	DWORD flags = 0;
	DWORD dwBufferCount;
	WSABUF wsabuf = { len, (char *)buf };
	if(WSARecv((SOCKET)s, 
		&wsabuf, 
		1, 
		&dwBufferCount, 
		&flags, 
		NULL, 
		NULL
	) == 0) {
		return dwBufferCount;
	}
	if(WSAGetLastError() == WSAECONNRESET) return 0;
	return -1;
}

inline int sendmsg(int s, const struct msghdr *msg, int flags)
{
	DWORD dwBufferCount;
	if(WSASendTo((SOCKET) s,
		(LPWSABUF)msg->msg_iov,
		(DWORD)msg->msg_iovlen,
		&dwBufferCount,
		flags,
		msg->msg_name,
		msg->msg_namelen,
		NULL,
		NULL
	) == 0) {
		return dwBufferCount;
	}
	if(WSAGetLastError() == WSAECONNRESET) return 0;
	return -1;
}

#if 0
#undef errno
#define errno werrno()
inline int werrno()
{
	int error = WSAGetLastError();
	if(error == 0) error = *_errno();

	switch(error) {
		default:
			return error;
		case WSAEPFNOSUPPORT:
			set_errno(EAFNOSUPPORT);
			return EAFNOSUPPORT;
		case WSA_IO_PENDING:
		case WSATRY_AGAIN:
			set_errno(EAGAIN);
			return EAGAIN;
		case WSAEWOULDBLOCK:
			set_errno(EWOULDBLOCK);
			return EWOULDBLOCK;
		case WSAEMSGSIZE:
			set_errno(E2BIG);
			return E2BIG;
		case WSAECONNRESET:
			set_errno(0);
			return 0;
	}
}
#endif

#if _MSC_VER < 1300
#define strtoll(p, e, b) ((*(e) = (char*)(p) + (((b) == 10) ? strspn((p), "0123456789") : 0)), _atoi64(p))
#else
#define strtoll(p, e, b) _strtoi64(p, e, b) 
#endif


#ifndef snprintf
#define snprintf _snprintf
#endif

#ifndef strtoull
#define strtoull strtoul
#endif


#endif
