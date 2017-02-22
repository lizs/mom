#pragma once
#include "bull.h"

#if MONITOR_ENABLED
class Monitor {
public:
	// record packages readed since last reset
	// shared between sessions
	static uint64_t g_readed;
	// record packages wroted since last reset
	// shared between sessions
	static uint64_t g_wroted;
};


uint64_t Monitor::g_readed = 0;
uint64_t Monitor::g_wroted = 0;
#endif
