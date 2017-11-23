// author : lizs
// 2017.2.22

#ifndef MOM_SCHEDULER_H
#define MOM_SCHEDULER_H

#include <functional>
#include <map>
#include "defines.h"
#include "data/any.h"

namespace VK {
	namespace Net {
		typedef uint32_t timer_id_t;
		typedef uint64_t timer_period_t;
		const timer_id_t INVALID_TIMER_ID = 0;

		// scheduler based on libuv
		class NET_EXPORT Scheduler final {
		public:
			typedef std::function<void(any)> timer_cb_t;

			Scheduler() : m_seed(INVALID_TIMER_ID) {}

			~Scheduler();

			timer_id_t invoke(timer_period_t delay, timer_cb_t cb);
			timer_id_t invoke(timer_period_t delay, timer_period_t period, timer_cb_t cb);
			timer_id_t invoke(timer_period_t delay, timer_cb_t cb, any usr_data);
			timer_id_t invoke(timer_period_t delay, timer_period_t period, timer_cb_t cb, any usr_data);
			bool cancel(timer_id_t id);
			void cancel_all();

		private:
			typedef struct {
				uv_timer_t timer;
				timer_id_t id;
				timer_period_t period;
				timer_cb_t cb;
				Scheduler* scheduler;
				any user_data;
			} timer_req_t;

			timer_id_t m_seed;
#pragma warning(push)
#pragma warning(disable:4251)
			std::map<timer_id_t, timer_req_t*> m_timers;
#pragma warning(pop)
		};
	}
}

#endif
