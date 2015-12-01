#ifndef __MONITOR_REPORTER_H__
#define __MONITOR_REPORTER_H__
#include <string>
#define MYSQLPP_MYSQL_HEADERS_BURIED
#include <mysql++/mysql++.h>

class MonitorReporter
{
public:
	MonitorReporter();
	~MonitorReporter();
	bool Init();
	void Run();
	inline int GetLastErrCode(){ return m_last_err_code; }
	inline std::string GetLastErrMsg(){ return m_last_err_msg; }


private:
	bool LoadConfig(std::string strFilePath);
	bool InitDb();
	void ReportData();
	bool UpdateAttrValue(uint32_t uiTime, uint32_t uiAttrId, uint32_t uiValue, int iRetryTimes = 0);

	// 查询记录是否存在 -1-err 0-empty 1-exsit
	int QueryRecordExsit(uint32_t uiTime, uint32_t uiAttrId);
	// 更新记录
	int UpdateRecord(uint32_t uiTime, uint32_t uiAttrId, uint32_t uiValue);
	// 插入记录
	int InsertRecord(uint32_t uiTime, uint32_t uiAttrId, uint32_t uiValue);


private:
	bool m_exit;
	int m_last_err_code;
	std::string m_last_err_msg;
	std::string m_db_host;
	std::string m_db_port;
	std::string m_db_user;
	std::string m_db_pwd;
	std::string m_db_name;
	mysqlpp::Connection* m_db_conn;
	std::map<uint32_t/*time*/, std::map<uint32_t/*attr*/, uint32_t/*value*/> > m_wait_report_data;
};

#endif
