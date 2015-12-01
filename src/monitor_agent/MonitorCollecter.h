#ifndef __MONITOR_COLLECTER_H__
#define __MONITOR_COLLECTER_H__
#include "../monitor_defs.h"
class MonitorCollecter
{
public:
	MonitorCollecter();
	~MonitorCollecter();

	bool Init();
	void Run();
	inline void Stop(){ m_exit = true; }

	inline int GetLastErrCode(){ return m_last_errcode; }

private:
	int InitShm();
	void OnExit();

	// 休息
	void Rest(uint32_t uiLastMinTime);

	// 收集用户自定义属性
	void CollectUserData();

	// 收集系统性能属性
	void CollectSysData();
	
	// 获取当前分钟级别时间戳 eg:16:36:xx == 16:36:00
	uint32_t GetCurMinTimestamps();
private:
	void* m_ptr_shm;
	int m_last_errcode;
	bool m_exit;

};

#endif
