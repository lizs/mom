// lizs 2017.3.8
#pragma once
#include "defines.h"
#include "circular_buf.h"
#include "singleton.h"
#include <vector>

namespace VK {
	namespace Net {
		static cbuf_ptr_t alloc_cbuf(cbuf_len_t len) {
			cbuf_ptr_t pcb(Singleton<cbuf_pool_t>::instance().alloc());
			pcb->reset(len);

			return pcb;
		}

		template <typename T>
		static cbuf_ptr_t alloc_cbuf() {
			return alloc_cbuf(sizeof(T));
		}

		// 打包
		template <typename ... Args>
		static std::vector<cbuf_ptr_t> pack(cbuf_ptr_t pcb, Args ... args) {
			auto ret = std::vector<cbuf_ptr_t>();

			// 1 pattern
			// 2 serial ...
			if (!pcb->write_head(args...))
				return ret;

			cbuf_len_t limit = MAX_PACKAGE_SIZE - sizeof(byte_t);
			if (pcb->get_len() > limit) {
				// 多包
				auto cnt = pcb->get_len() / limit;
				auto lastLen = pcb->get_len() % limit;
				if (lastLen != 0)
					++cnt;

				if (cnt == 1)
					goto Single;

				for (auto i = 0; i < cnt; ++i) {
					cbuf_ptr_t slice;
					auto packLen = limit;
					if (i == cnt - 1 && lastLen != 0)
						packLen = lastLen;

					slice = alloc_cbuf(packLen);
					slice->write_binary(pcb->get_head_ptr() + i * limit, packLen);

					if (!slice->write_head<byte_t>(cnt - i))
						return ret;

					if (!slice->write_head<pack_size_t>(slice->get_len()))
						return ret;

					ret.push_back(slice);
				}

				return ret;
			}
			else {
Single:
				// 单包
				if (!pcb->write_head<byte_t>(1))
					return ret;

				if (!pcb->write_head<pack_size_t>(pcb->get_len()))
					return ret;

				ret.push_back(pcb);
			}

			return std::move(ret);
		}

		static const char* net_str_err(error_no_t error_no) {
			switch (static_cast<NetError>(error_no)) {
				case Success: return "Success.";
				case NE_Write: return "Write failed.";
				case NE_Read: return "Read failed.";
				case NE_SerialConflict: return "Serial failed.";
				case NE_NoHandler: return "No handler.";
				case NE_ReadErrorNo: return "Read errno failed.";
				case NE_SessionClosed: return "Session closed.";
				case NE_End: return "NE_End.";
				default: return "Unknown";
			}
		}

		static const char* mom_str_err(error_no_t error_no) {
			if (error_no <= NE_End) {
				return net_str_err(error_no);
			}

			switch (static_cast<MomError>(error_no)) {
				case ME_Begin: return "ME_Begin";
				case ME_ReadHeader: return "Read header failed.";
				case ME_WriteHeader: return "Write header failed.";
				case ME_InvlidOps: return "Invalid ops.";
				case ME_ReadRegisterInfo: return "Read register information failed.";
				case ME_TargetNodeNotExist: return "Target node doesn't exist.";
				case ME_NodeIdAlreadyExist: return "Node with the same id already registered.";
				case ME_NoHandler: return "No handler found for target ops.";
				case ME_NotImplemented: return "Handler not implemented. ";
				case ME_TargetSessionNotExist: return "Target session doesn't exist. ";
				case ME_MomClientsNotReady: return "Mom clients not ready. ";
				case ME_ReadUnregisterInfo: return "Read unregister information failed. ";
				case ME_CoordinationServerNotExist: return "Coordination server not exist. ";
				case ME_TargetServerNotExist: return "Target server not exist. ";
				case ME_ReadCoordinateInfo: return "Read coordinate information failed.";
				case ME_End: return "ME_End";
				default: return "Unknown";
			}
		}
	}
}
