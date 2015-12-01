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
	// ���Զ�κ�Ͳ�������
	if (retryTimes >= MAX_CAS_RETRY_TIMES)
	{
		return RetCode_RetryOverTimes;
	}
	CasNode oldNode = *ptr_node;
	// attr=0ʱ��ʾ��δ��¼��
	// node�е�attr������õ�attr��һ��Ŀǰֻ���ܷ����ڹ����ڴ汻�۸�ʱ
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
	// ֱ�ӵ���value
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
		//TODO Ϊ�˷�ֹ���߳���Ҫ����
		// ���ܴ��ڶ���̵߳ȴ����������ٴ��ж�
		if (g_ptr_shm == NULL)
		{
			retCode = init_shm();
			if (retCode != RetCode_OK)
			{
				return retCode;
			}
		}
	}
	// �����ڴ���λnodeԤ��
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
	// ���Ժ������̲߳���ȫ
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

