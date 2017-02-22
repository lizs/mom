// author : lizs
// 2017.2.21

#pragma once
#include "session.h"

namespace Bull {
	template <uint16_t Capacity = 64>
	class DefaultPacker {
	public:
		enum CircularBufCapacity {
			Value = Capacity,
		};

		ushort pack(CircularBuf<Capacity>& cbuf) {
			return 0;
		}

		ushort unpack(CircularBuf<Capacity>& cbuf) {
			return 0;
		}

		void on_message(const char* data, uint16_t len, Session<DefaultPacker>* session) const {
			LOG("%s : %d", data, len);
		}
	};

	template <uint16_t Capacity = 64>
	class SerialPacker {
	public:
		enum CircularBufCapacity {
			Value = Capacity,
		};

		explicit SerialPacker()
			: m_serial(0) { }

		ushort pack(CircularBuf<Capacity>& cbuf) {
			cbuf.write(++g_serial);
			return sizeof(uint16_t);
		}

		ushort unpack(CircularBuf<Capacity>& cbuf) {
			m_serial = cbuf.template read<uint16_t>();
			return sizeof(uint16_t);
		}

		void on_message(const char* data, uint16_t len, Session<SerialPacker>* session) const {
			//LOG("%d : %s : %d", m_serial, data, len);
		}

	private:
		static ushort g_serial;
		ushort m_serial;
	};

	template <uint16_t Capacity>
	uint16_t SerialPacker<Capacity>::g_serial = 0;
}
