#pragma once
#include "defines.h"
#include "scheduler.h"
#include "logger.h"

namespace VK {
	namespace Net {
		class NET_EXPORT Monitor {
			const timer_period_t DefaultPeriod = 5 * 1000;
		public:
			Monitor()
				: m_readed(0),
				  m_wroted(0), m_pending(0), m_pcbCount(0), m_timerID(INVALID_TIMER_ID) {
			}

			uint64_t get_readed() const {
				return m_readed;
			}

			uint64_t get_wroted() const {
				return m_wroted;
			}

			uint64_t get_pending() const {
				return m_pending;
			}

			uint64_t get_pcb_count() const {
				return m_pcbCount;
			}

			void inc_readed() {
				++m_readed;
			}

			void inc_wroted() {
				++m_wroted;
			}

			void inc_pending() {
				++m_pending;
			}

			void dec_pending() {
				--m_pending;
			}

			void inc_pcb_count() {
				++m_pcbCount;
			}

			void dec_pcb_count() {
				--m_pcbCount;
			}

			void start() {
#if MONITOR_ENABLED
				// performance monitor
				m_timerID = m_scheduler.invoke(DefaultPeriod, DefaultPeriod, [this](any usr_data) {
					                               Logger::instance().info("Read : {} /s Write : {} /s, Pending : {} PCB : {}", get_readed() * 1000 / DefaultPeriod, get_wroted() * 1000 / DefaultPeriod, get_pending(), get_pcb_count());

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
			uint64_t m_pending;
			uint64_t m_pcbCount;

			Scheduler m_scheduler;
			timer_id_t m_timerID;
		};
	}
}
