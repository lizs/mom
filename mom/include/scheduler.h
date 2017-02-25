// author : lizs
// 2017.2.22

#pragma once
#include <functional>
#include <map>
#include "bull.h"
#include <memory>

namespace Bull {
	typedef uint32_t timer_id_t;
	typedef uint64_t timer_period_t;

	// scheduler based on libuv
	class Scheduler final {
	public:
		typedef std::function<void()> timer_cb_t;

		const timer_id_t INVALID_TIMER_ID = 0;

		Scheduler() : m_seed(INVALID_TIMER_ID) {}

		~Scheduler();

		timer_id_t invoke(timer_period_t delay, timer_cb_t cb);
		timer_id_t invoke(timer_period_t delay, timer_period_t period, timer_cb_t cb);
		bool cancel(timer_id_t id);
		void cancel_all();

	private:
		typedef struct {
			uv_timer_t timer;
			timer_id_t id;
			timer_period_t period;
			timer_cb_t cb;
			Scheduler* scheduler;
		} timer_req_t;

		timer_id_t m_seed;
		std::map<timer_id_t, timer_req_t*> m_timers;
	};

	inline Scheduler::~Scheduler() {
		cancel_all();
	}

	inline timer_id_t Scheduler::invoke(timer_period_t delay, timer_cb_t cb) {
		return this->invoke(delay, 0, cb);
	}

	inline timer_id_t Scheduler::invoke(timer_period_t delay, timer_period_t period, std::function<void()> cb) {
		int r;
		timer_req_t* treq;
		uv_loop_t* loop;

		treq = new timer_req_t();
		treq->cb = cb;
		treq->id = ++m_seed;
		treq->period = period;
		treq->scheduler = this;

		loop = uv_default_loop();

		r = uv_timer_init(loop, &treq->timer);
		if (r) {
			LOG("uv_timer_init error : %s\n", uv_strerror(r));
			delete treq;
			return INVALID_TIMER_ID;
		}

		r = uv_timer_start(&treq->timer,
		                   [](uv_timer_t* handle) {
			                   auto tr = reinterpret_cast<timer_req_t*>(handle);
			                   auto fun = tr->cb;

			                   if (!tr->period)
				                   tr->scheduler->cancel(tr->id);

			                   if (fun)
				                   fun();
		                   },
		                   delay, period);

		if (r) {
			LOG_UV_ERR(r);
			delete treq;
			return INVALID_TIMER_ID;
		}

		m_timers.insert(m_timers.end(), std::make_pair(m_seed, treq));
		return m_seed;
	}

	inline bool Scheduler::cancel(timer_id_t id) {
		auto it = m_timers.find(id);
		if (m_timers.end() == it) return false;

		it->second->timer.data = it->second;
		uv_close(reinterpret_cast<uv_handle_t*>(&it->second->timer), [](uv_handle_t* handle) {
			         auto tr = reinterpret_cast<timer_req_t*>(handle->data);
			         delete tr;
		         });

		m_timers.erase(it);
		return true;
	}

	inline void Scheduler::cancel_all() {
		for (auto kv : m_timers) {
			kv.second->timer.data = kv.second;
			uv_close(reinterpret_cast<uv_handle_t*>(&kv.second->timer), [](uv_handle_t* handle) {
				         auto tr = reinterpret_cast<timer_req_t*>(handle->data);
				         delete tr;
			         });
		}

		m_timers.clear();
	}
}
