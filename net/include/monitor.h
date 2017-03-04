#pragma once
#include "net.h"
#include "scheduler.h"

namespace VK {
	namespace Net {
		class Monitor {
			const timer_period_t DefaultPeriod = 5 * 1000;
		public:
			Monitor()
				: m_readed(0),
				  m_wroted(0), m_writingQueued(0), m_timerID(INVALID_TIMER_ID) {}

			uint64_t get_readed() const {
				return m_readed;
			}

			uint64_t get_wroted() const {
				return m_wroted;
			}

			uint64_t get_writing() const {
				return m_writingQueued;
			}

			void inc_readed() {
				++m_readed;
			}

			void inc_wroted() {
				++m_wroted;
			}

			void inc_writing() {
				++m_writingQueued;
			}

			void dec_writing() {
				--m_writingQueued;
			}

			void start() {
#if MONITOR_ENABLED
				// performance monitor
				m_timerID = m_scheduler.invoke(DefaultPeriod, DefaultPeriod, [this]() {
					                               LOG("Read : %llu /s Write : %llu /s, Queued : %llu", get_readed() * 1000 / DefaultPeriod, get_wroted() * 1000 / DefaultPeriod, get_writing()) ;
					                               reset_readed();
					                               reset_wroted();
				                               });
#endif
			}

			void stop() {
				if (m_timerID != INVALID_TIMER_ID)
					m_scheduler.cancel(m_timerID);

				m_timerID = INVALID_TIMER_ID;
			}

			void reset_readed() {
				m_readed = 0;
			}

			void reset_wroted() {
				m_wroted = 0;
			}

		private:
			// record packages readed since last reset
			// shared between sessions
			uint64_t m_readed;
			// record packages wroted since last reset
			// shared between sessions
			uint64_t m_wroted;
			// record packages writing
			uint64_t m_writingQueued;

			Scheduler m_scheduler;
			timer_id_t m_timerID;
		};
	}
}
