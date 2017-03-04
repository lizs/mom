#pragma once
#include "../net.h"

// mom protocols

typedef short ops_t;

#pragma pack(1)

typedef struct {
	node_id_t nid;
	node_type_t ntype;
} node_register_t;

typedef struct {
	node_id_t nid;
} node_unregister_t;

typedef struct {
	///node_type_t ntype;
	node_id_t nid;
} coordinate_rep_t;

typedef struct {
	node_type_t ntype;
} coordinate_req_t;

typedef struct {
	ops_t ops;
	node_id_t nid;
	node_id_t gate_id;
	session_id_t sid;
} route_t;

#pragma pack()

struct node_info_t {
	node_register_t reg;
	session_id_t sid;

	node_info_t(node_register_t& reg, session_id_t sid) : reg(reg), sid(sid) { }
};

// 系统命令 < 0
// MOM命令
enum MomOps : short {
	Invalid = SHRT_MIN,
	// 注册至网关
	Register2Gate,
	// 注册至协调服
	Register2Coordination,
	// 反注册
	UnregisterFromCoordination,
	// 协调（请求分配一个目标服务ID）
	Coordinate,
};

enum MomError : error_no_t {
	ME_Begin = NE_End,
	ME_ReadRoute,
	ME_WriteRoute,
	ME_InvlidOps,
	ME_ReadRegisterInfo,
	ME_ReadUnregisterInfo,
	ME_TargetNodeNotExist,
	ME_NodeIdAlreadyExist,
	ME_NoHandler,
	ME_NotImplemented,
	ME_TargetSessionNotExist,
	ME_MomClientsNotReady,
	ME_CoordinationServerNotExist,
	ME_TargetServerNotExist,
	ME_ReadCoordinateInfo,
	ME_End,
};

static const char* mom_str_err(error_no_t error_no) {
	if(error_no <= NE_End) {
		return net_str_err(error_no);
	}
	
	switch (static_cast<MomError>(error_no)) {
		case ME_Begin: return "ME_Begin";
		case ME_ReadRoute: return "Read route failed.";
		case ME_WriteRoute: return "Write route failed.";
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

enum NodeType {
	NT_Mom= 1,
	NT_Game,
	NT_Gate,
	NT_Coordination,
};
