// lizs 2017.3.8
#pragma once
#include "defines.h"
#include "circular_buf.h"
#include "singleton.h"

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

		static cbuf_ptr_t alloc_broadcast(cbuf_len_t size) {
			auto pcb = alloc_cbuf(size);
			auto broad_cast_header = pcb->pre_write_head<broadcast_push_t>();
			broad_cast_header->ntype = NT_Game;
			auto header = pcb->pre_write_head<push_header_t>();
			header->ops = Broadcast;

			return pcb;
		}

		template <typename T>
		static cbuf_ptr_t alloc_broadcast() {
			return alloc_broadcast(sizeof(T));
		}

		static cbuf_ptr_t alloc_push(ops_t ops, node_id_t nid, cbuf_len_t size) {
			auto pcb = alloc_cbuf(size);
			auto header = pcb->pre_write_head<push_header_t>();
			header->nid = nid;
			header->ops = ops;

			return pcb;
		}

		template <typename T>
		static cbuf_ptr_t alloc_push(ops_t ops, node_id_t nid) {
			return alloc_push(ops, nid, sizeof(T));
		}

		static cbuf_ptr_t alloc_request(ops_t ops, node_id_t nid, cbuf_len_t size) {
			auto pcb = alloc_cbuf(size);
			auto header = pcb->pre_write_head<req_header_t>();
			header->nid = nid;
			header->ops = ops;

			return pcb;
		}

		template <typename T>
		static cbuf_ptr_t alloc_request(ops_t ops, node_id_t nid) {
			return alloc_request(ops, nid, sizeof(T));
		}

		// ´ò°ü
		template <typename ... Args>
		static bool pack(cbuf_ptr_t pcb, Args ... args) {
			// 1 pattern
			// 2 serial ...
			if (!pcb->write_head(args...))
				return false;

			// rewrite the package size
			if (!pcb->write_head<pack_size_t>(pcb->get_len()))
				return false;

			return true;
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
