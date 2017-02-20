#pragma once
#include <functional>
#include <map>
#include "uv_plus.h"

class Scheduler;

#pragma pack(1)

typedef struct
{
	uv_timer_t timer;
	u_int id;
	Scheduler * scheduler;
	std::function<void()> cb;
} timer_req_t;

#pragma pack()

// timer based on libuv
class Scheduler
{
public:
	const u_int INVALID_TIMER_ID = 0;

	Scheduler() : m_seed(INVALID_TIMER_ID){}
	virtual ~Scheduler();

	u_int invoke(uint64_t delay, std::function<void()> cb);
	u_int invoke(uint64_t delay, uint64_t period, std::function<void()> cb);
	bool cancel(u_int id);
	

private:
	u_int m_seed;
	std::map<u_int, timer_req_t*> m_timers;
};