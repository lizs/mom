// author : lizs
// 2017.2.21

#pragma once
#include "session.h"

namespace Bull {



	template <uint16_t Capacity = 64>
	class Packer {
	public:
		enum CircularBufCapacity {
			Value = Capacity,
		};

		explicit Packer()
			: m_serial(0), m_pattern(Pattern::Raw) { }

		template <typename ... Args>
		uint16_t pack(CircularBuf<Capacity>& cbuf, Args ... args) {
			auto previousTail = cbuf.get_tail();
			cbuf.write(args...);
			return (uint16_t)(cbuf.get_tail() - previousTail);
		}

		template <>
		uint16_t pack(CircularBuf<Capacity>& cbuf) {
			return 0;
		}

		uint16_t unpack(CircularBuf<Capacity>& cbuf) {
			auto previousHead = cbuf.get_head();
			auto pattern = static_cast<Pattern>(cbuf.template read<pattern_t>());
			switch (pattern) {

				case Raw: break;
				case Push: break;
				case Request:
					cbuf.template read<serial_t>();
					break;
				case Response: break;

				default: break;
			}

			return (uint16_t)(cbuf.get_head() - previousHead);
		}

		void on_message(const char* data, uint16_t len, Session<Packer>* session) const {
			//LOG("%d : %s : %d", m_serial, data, len);
		}
		
	private:
		static uint16_t g_serial;
		// optional ( push package needn't )
		uint16_t m_serial;
		Pattern m_pattern;
	};

	template <uint16_t Capacity>
	uint16_t Packer<Capacity>::g_serial = 0;
}
