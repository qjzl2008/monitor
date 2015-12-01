#include "monitor_api.h"
#include <string>
#include <memory.h>
#include <sys/shm.h>

#define MAX_CAS_RETRY_TIMES 30

static void* g_ptr_shm = NULL;

void onexit_clear()
{
	if (g_ptr_shm != NULL)
	{
		shmdt(g_ptr_shm);
	}
}

int init_shm()
{
	int shmid = shmget(SHM_KEY, sizeof(CasNode) * MAX_NODE_COUNT, 0666 | IPC_CREAT);
	if (shmid == -1)
	{
		return RetCode_ShmGetErr;
	}
	g_ptr_shm = shmat(shmid, 0, 0);
	if (g_ptr_shm == (void*)-1)
	{
		return RetCode_ShmMatErr;
	}
	atexit(onexit_clear);
	return RetCode_OK;
}

int cas_update_attr_value(CasNode* ptr_node, uint32_t attrId, uint32_t value, uint32_t retryTimes)
{
	// 重试多次后就不再重试
	if (retryTimes >= MAX_CAS_RETRY_TIMES)
	{
		return RetCode_RetryOverTimes;
	}
	CasNode oldNode = *ptr_node;
	// attr=0时表示还未记录过
	// node中的attr与待设置的attr不一致目前只可能发生在共享内存被篡改时
	if (oldNode.attrNode.uiAttr == 0 || ptr_node->attrNode.uiAttr != attrId)
	{
		CasNode newNode;
		newNode.attrNode.uiAttr = attrId;
		newNode.attrNode.uiValue = value;
		bool bRet = __sync_bool_compare_and_swap(&(ptr_node->uiCasValue), oldNode.uiCasValue, newNode.uiCasValue);
		if (!bRet)
		{
			return cas_update_attr_value(ptr_node, attrId, value, retryTimes + 1);
		}
		return RetCode_OK;
	}
	// 直接叠加value
	CasNode newNode;
	newNode.attrNode.uiAttr = attrId;
	newNode.attrNode.uiValue = ptr_node->attrNode.uiValue + value;
	bool bRet = __sync_bool_compare_and_swap(&(ptr_node->uiCasValue), oldNode.uiCasValue, newNode.uiCasValue);
	if (!bRet)
	{
		return cas_update_attr_value(ptr_node, attrId, value, retryTimes + 1);
	}
	return RetCode_OK;
}

int MONITOR_API(uint32_t attrId, uint32_t value)
{
	if (attrId == 0 || attrId > MAX_NODE_COUNT )
	{
		return RetCode_AttridErr;
	}
	int retCode = 0;
	if (g_ptr_shm == NULL)
	{
		//TODO 为了防止多线程需要加锁
		// 可能存在多个线程等待锁，所以再次判断
		if (g_ptr_shm == NULL)
		{
			retCode = init_shm();
			if (retCode != RetCode_OK)
			{
				return retCode;
			}
		}
	}
	// 共享内存首位node预留
	CasNode* ptr_node = (CasNode*)g_ptr_shm + attrId;
	return cas_update_attr_value(ptr_node, attrId, value, 0);
}

std::map<uint32_t, uint32_t>  DEBUG_LIST()
{
	if (g_ptr_shm == NULL)
	{
		init_shm();
	}
	std::map<uint32_t, uint32_t> tmpMap;
	for (int i = 0; i < MAX_NODE_COUNT; ++i)
	{
		CasNode* ptrNode = (CasNode*)g_ptr_shm + i;
		if (ptrNode->attrNode.uiAttr != 0)
		{
			tmpMap.insert(std::make_pair(ptrNode->attrNode.uiAttr, ptrNode->attrNode.uiValue));
		}
	}
	return tmpMap;
}

int CLEAR_ALL_DATE()
{
	// 调试函数，线程不安全
	if (g_ptr_shm == NULL)
	{
		int iRet = init_shm();
		if (iRet != RetCode_OK)
		{
			return iRet;
		}
	}
	memset(g_ptr_shm, 0, sizeof(CasNode) * MAX_NODE_COUNT);	
	return RetCode_OK;
}

