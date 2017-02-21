//#pragma once
//#include "uv_plus.h"
//#include "circular_buf.h"
//
//#pragma pack(1)
//typedef struct
//{
//	ushort len;
//	char* body;
//} package_t;
//#pragma pack()
//
//template<ushort Capacity = 128>
//class Package
//{
//public:
//
//private:
//	CircularBuf<Capacity> m_cbuf;
//};
//
//
//static void init_package(char* data, u_short len, package_t* package)
//{
//	package->body = (char*)malloc(len+1);
//	package->len = len;
//
//	// ensure end with '\0'
//	package->body[len] = 0;
//
//	memcpy(package->body, data, len);
//}
//
//static void free_package(package_t* package)
//{
//	free(package->body);
//	free(package);
//}
//
//static package_t* make_package(char* data, u_short len)
//{
//	package_t* pack = (package_t*)malloc(sizeof(package_t));
//	init_package(data, len, pack);
//	return pack;
//}