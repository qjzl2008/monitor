#ifndef __MONITOR_DEFS_H__
#define __MONITOR_DEFS_H__
#include <stddef.h>
#include <stdint.h>

#define SHM_KEY 0x3868
#define MAX_NODE_COUNT	10000

enum RetCode
{
	RetCode_OK = 0,
	RetCode_ShmGetErr = 1,
	RetCode_ShmMatErr = 2,
	RetCode_OutMemory = 3,
	RetCode_CasErr = 4,
	RetCode_AttridErr = 5,
	RetCode_RetryOverTimes = 6,
};

typedef struct
{
	uint32_t uiAttr;
	uint32_t uiValue;
}AttrNode;

typedef union
{
	AttrNode attrNode;
	// 此字段用于cas时判断
	uint64_t uiCasValue;
}CasNode;

#endif