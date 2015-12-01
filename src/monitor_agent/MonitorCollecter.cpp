#include "MonitorCollecter.h"
#include <sys/shm.h>
#include <time.h>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <iostream>

extern std::map<uint32_t/*time*/, std::map<uint32_t/*attr*/, uint32_t/*value*/> > g_collect_data;
extern pthread_mutex_t g_data_mutex;

MonitorCollecter::MonitorCollecter()
: m_exit(false)
{

}


MonitorCollecter::~MonitorCollecter()
{
	if (m_ptr_shm != NULL)
	{
		shmdt(m_ptr_shm);
	}
}

void MonitorCollecter::Run()
{
	while (!m_exit)
	{
		uint32_t curTime = GetCurMinTimestamps();
		std::map<uint32_t, uint32_t> curCollectDataMap;
		// 共享内存首位有预留，所以从1开始
		for (int i = 1; i < MAX_NODE_COUNT; ++i)
		{
			CasNode* ptrNode = (CasNode*)m_ptr_shm + i;
			bool bRet = false;
			int iRetryTimes = 0;
			// 收集cas冲突时只重试50次，超过的待下次收集周期时收集
			while (!bRet && iRetryTimes <50)
			{
				volatile CasNode oldNode = *ptrNode;
				if (oldNode.attrNode.uiAttr == 0 || oldNode.attrNode.uiValue == 0)
				{
					break;
				}
				CasNode newNode;
				newNode.uiCasValue = 0;
				bRet = __sync_bool_compare_and_swap(&(ptrNode->uiCasValue), oldNode.uiCasValue, newNode.uiCasValue);
				if (bRet)
				{
					uint32_t uiAttrId = oldNode.attrNode.uiAttr;
					auto it = curCollectDataMap.find(uiAttrId);
					if (it == curCollectDataMap.end())
					{
						curCollectDataMap.insert(std::make_pair(uiAttrId, oldNode.attrNode.uiValue));
					}
					else
					{
						it->second += oldNode.attrNode.uiValue;
					}
					break;
				}
				++iRetryTimes;
			}
		}
		std::cout << "collecter collect data size:" << curCollectDataMap.size() << std::endl;
		if (curCollectDataMap.size() != 0)
		{
			if (pthread_mutex_lock(&g_data_mutex) != 0)
			{
				// TODO
			}
			auto timeIt = g_collect_data.find(curTime);
			if (timeIt == g_collect_data.end())
			{
				std::map<uint32_t/*attr*/, uint32_t/*value*/> tmpMap;
				g_collect_data.insert(make_pair(curTime, tmpMap));
				timeIt = g_collect_data.find(curTime);
			}
			auto collectIt = curCollectDataMap.begin();
			for (; collectIt != curCollectDataMap.end(); ++collectIt)
			{
				timeIt->second[collectIt->first] += collectIt->second;
			}
			if (pthread_mutex_unlock(&g_data_mutex) != 0)
			{
				// TODO
			}
		}
		// 休息
		Rest(curTime);
	}
}

bool MonitorCollecter::Init()
{
	int iRet = InitShm();
	if (iRet != RetCode_OK)
	{
		m_last_errcode = iRet;
		return false;
	}
	return true;
}

int MonitorCollecter::InitShm()
{
	int shmid = shmget(SHM_KEY, sizeof(CasNode) * MAX_NODE_COUNT, 0666 | IPC_CREAT);
	if (shmid == -1)
	{
		return RetCode_ShmGetErr;
	}
	m_ptr_shm = shmat(shmid, 0, 0);
	if (m_ptr_shm == (void*)-1)
	{
		return RetCode_ShmMatErr;
	}
	return RetCode_OK;
}

uint32_t MonitorCollecter::GetCurMinTimestamps()
{
	time_t cur_time;
	time(&cur_time);
	struct tm* time_info = localtime(&cur_time);
	time_info->tm_sec = 0;
	return mktime(time_info);
}

void MonitorCollecter::Rest(uint32_t uiLastMinTime)
{
	time_t curTime;
	time(&curTime);
	uint32_t curMinTime = GetCurMinTimestamps();
	uint32_t curSecTime = (uint32_t)curTime;
	uint32_t curIntervalTime = (curSecTime >= curMinTime) ? curSecTime - curMinTime : 0;
	// 默认休息10s
	uint32_t sleepTime = 10;
	do 
	{
		if (curIntervalTime >=50 && curIntervalTime < 59)
		{
			sleepTime = 59 - curIntervalTime;
			
		}
		// 确保收集在59秒执行一次
	} while (false);
	sleep(sleepTime);
}
