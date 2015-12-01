#ifndef __MONITOR_API_H__
#define __MONITOR_API_H__
#define MONITOR_VERSION "1.0"
#include "../monitor_defs.h"
#include <map>

int MONITOR_API(uint32_t attrId, uint32_t value);

int CLEAR_ALL_DATE();

inline const char* CURRENT_VERSION(){ return MONITOR_VERSION; }

// µ˜ ‘”√
std::map<uint32_t, uint32_t> DEBUG_LIST();


#endif