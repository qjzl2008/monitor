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

	// ��Ϣ
	void Rest(uint32_t uiLastMinTime);

	// �ռ��û��Զ�������
	void CollectUserData();

	// �ռ�ϵͳ��������
	void CollectSysData();
	
	// ��ȡ��ǰ���Ӽ���ʱ��� eg:16:36:xx == 16:36:00
	uint32_t GetCurMinTimestamps();
private:
	void* m_ptr_shm;
	int m_last_errcode;
	bool m_exit;

};

#endif
