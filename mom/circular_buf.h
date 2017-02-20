#pragma once
#include "uv_plus.h"

typedef struct
{
	u_short pack_desired_size;

	u_short head;
	u_short tail;
	u_short cap;

	char* body;
} circular_buf_t;

typedef struct
{
	u_short len;
	char* body;
} package_t;

static void init_package(char* data, u_short len, package_t* package)
{
	package->body = (char*)malloc(len+1);
	package->len = len;

	// ensure end with '\0'
	package->body[len] = 0;

	memcpy(package->body, data, len);
}

static void free_package(package_t* package)
{
	free(package->body);
	free(package);
}

static package_t* make_package(char* data, u_short len)
{
	package_t* pack = (package_t*)malloc(sizeof(package_t));
	init_package(data, len, pack);
	return pack;
}

static u_short get_cbuf_len(circular_buf_t* cbuf)
{
	return cbuf->tail - cbuf->head;
}

static char* get_cbuf_head(circular_buf_t* cbuf)
{
	return cbuf->body + cbuf->head;
}

static char* get_cbuf_tail(circular_buf_t* cbuf)
{
	return cbuf->body + cbuf->tail;
}

static void arrange_cbuf(circular_buf_t* cbuf)
{
	if (cbuf->tail * 1.2 > cbuf->cap)
	{
		ASSERT(cbuf->tail - cbuf->head < 2);
		memcpy(cbuf->body, get_cbuf_head(cbuf), get_cbuf_len(cbuf));
	}
}

static void reset_circular_buffer(circular_buf_t* cbuf)
{
	cbuf->pack_desired_size = 0;
	cbuf->head = 0;
	cbuf->tail = 0;
	ZeroMemory(cbuf->body, cbuf->cap);
}

static void init_circular_buffer(u_short capacity, circular_buf_t* cbuf)
{
	cbuf->cap = capacity;
	cbuf->body = (char*)malloc(capacity);

	reset_circular_buffer(cbuf);
}

static circular_buf_t* make_circular_buffer(u_short capacity)
{
	circular_buf_t* cbuf = (circular_buf_t *)malloc(sizeof(circular_buf_t));
	init_circular_buffer(capacity, cbuf);
	return cbuf;
}
