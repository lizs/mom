#include "scheduler.h"

Scheduler::~Scheduler()
{
}

u_int Scheduler::invoke(uint64_t delay, std::function<void()> cb)
{
	return this->invoke(delay, 0, cb);
}

u_int Scheduler::invoke(uint64_t delay, uint64_t period, std::function<void()> cb)
{
	int r;
	timer_req_t* treq;
	uv_loop_t* loop;

	treq = new timer_req_t();
	treq->cb = cb;
	treq->id = ++m_seed;
	treq->scheduler = this;

	loop = uv_default_loop();

	r = uv_timer_init(loop, &treq->timer);
	if (r)
	{
		LOG("uv_timer_init error : %s\n", uv_strerror(r));
		delete treq;
		return INVALID_TIMER_ID;
	}

	if(period != 0)
	{
		r = uv_timer_start(&treq->timer,
			[](uv_timer_t* handle)
		{
			timer_req_t* tr = (timer_req_t*)handle;
			if (tr->cb)
				tr->cb();
		},
			delay, period);
	}
	else
	{
		r = uv_timer_start(&treq->timer,
			[](uv_timer_t* handle)
		{
			timer_req_t* tr = (timer_req_t*)handle;
			if (tr->cb)
				tr->cb();

			tr->scheduler->cancel(tr->id);
		},
			delay, period);
	}

	if (r)
	{
		LOG_UV_ERR(r);
		delete treq;
		return INVALID_TIMER_ID;
	}

	m_timers.insert(m_timers.end(), std::make_pair(m_seed, treq));
	return m_seed;
}

bool Scheduler::cancel(u_int id)
{
	uv_loop_t* loop;

	auto it = m_timers.find(id);
	if (m_timers.end() == it) return false;

	loop = uv_default_loop();

	uv_close((uv_handle_t*)&it->second->timer, [](uv_handle_t*)
	         {
	         });

	free(it->second);

	m_timers.erase(it);
	return true;
}