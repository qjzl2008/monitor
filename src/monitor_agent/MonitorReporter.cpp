#include "MonitorReporter.h"
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <iostream>
#include "../../libs/tinyxml/tinyxml.h"

extern std::map<uint32_t/*time*/, std::map<uint32_t/*attr*/, uint32_t/*value*/> > g_collect_data;
extern pthread_mutex_t g_data_mutex;

using namespace std;
using namespace mysqlpp;

MonitorReporter::MonitorReporter()
: m_exit(false)
, m_last_err_code(0)
, m_db_conn(NULL)
{
}


MonitorReporter::~MonitorReporter()
{
}

bool MonitorReporter::Init()
{
	bool bRet = LoadConfig("../conf/monitor_agent_cfg.xml");
	if (!bRet)
	{
		return bRet;
	}

	// 连接db
	bRet = InitDb();
	if (!bRet)
	{
		return bRet;
	}
	return true;
}

void MonitorReporter::Run()
{
	while (!m_exit)
	{
		if (m_wait_report_data.size() == 0)
		{
			if (pthread_mutex_lock(&g_data_mutex) != 0)
			{
				// TODO
			}
			m_wait_report_data.swap(g_collect_data);
			if (pthread_mutex_unlock(&g_data_mutex) != 0)
			{
				// TODO
			}
		}
		cout << "wait report_data size:" << m_wait_report_data.size() << endl;
		ReportData();
		// 每10s进行一次db上报
		sleep(10);
	}
}

bool MonitorReporter::LoadConfig(std::string strFilePath)
{
	TiXmlDocument doc;
	//LOG(INFO) << "load: " << filePath;
	bool flag = doc.LoadFile(strFilePath.c_str());
	do 
	{
		if (!flag)
		{
			//LOG(ERROR) << "Error load" << path << " failed:" << doc.ErrorDesc();
			m_last_err_code = -1;
			m_last_err_msg = "load " + strFilePath + " failed:" + doc.ErrorDesc();
			return false;
			//throw new std::exception();
		}
		try
		{

			TiXmlElement* parentXml = doc.RootElement()->FirstChildElement("Dbcfg");
			m_db_host = parentXml->Attribute("host");
			m_db_port = parentXml->Attribute("port");
			m_db_user = parentXml->Attribute("user");
			m_db_pwd = parentXml->Attribute("pwd");
			m_db_name = parentXml->Attribute("dbName");
		}
		catch (std::exception* e)
		{
			m_last_err_code = -2;
			m_last_err_msg = std::string("parm missing");
			flag = false;
		}
	} while (false);
	return flag;
}

bool MonitorReporter::InitDb()
{
	if (m_db_conn == NULL)
	{
		m_db_conn = new mysqlpp::Connection();
	}
	m_db_conn->disconnect();
	uint32_t dwPort = strtoul(m_db_port.c_str(), NULL, 10);
	if (!m_db_conn->connect(m_db_name.c_str(), m_db_host.c_str(), m_db_user.c_str(), m_db_pwd.c_str(), dwPort))
	{
		cout << "connect db err:" << m_db_conn->error() << endl;
		m_last_err_code = -3;
		m_last_err_msg = m_db_conn->error();
		return false;
	}
	return true;
}

void MonitorReporter::ReportData()
{
	if (m_wait_report_data.size() == 0)
	{
		return;
	}

	auto timeIt = m_wait_report_data.begin();
	for (; timeIt != m_wait_report_data.end(); )
	{
		auto attrIt = timeIt->second.begin();
		for (; attrIt != timeIt->second.end(); )
		{
			bool bRet = UpdateAttrValue(timeIt->first, attrIt->first, attrIt->second);
			if (bRet)
			{
				timeIt->second.erase(attrIt++);
				continue;
			}
			++attrIt;
		}
		if (timeIt->second.size() == 0)
		{
			m_wait_report_data.erase(timeIt++);
			continue;
		}
		++timeIt;
	}
	if (m_wait_report_data.size() == 0)
	{
		std::map<uint32_t/*time*/, std::map<uint32_t/*attr*/, uint32_t/*value*/> > tmpMap;
		m_wait_report_data.swap(tmpMap);
	}
}

bool MonitorReporter::UpdateAttrValue(uint32_t uiTime, uint32_t uiAttrId, uint32_t uiValue, int iRetryTimes)
{
	if (iRetryTimes > 3)
	{
		cout << "update db over times" << endl;
		return false;
	}
	int iRet = QueryRecordExsit(uiTime, uiAttrId);
	if (iRet == -1)
	{
		return UpdateAttrValue(uiTime, uiAttrId, uiValue, iRetryTimes + 1);
	}
	if (iRet == 0)
	{
		iRet = InsertRecord(uiTime, uiAttrId, uiValue);
		if (iRet == -1)
		{
			return UpdateAttrValue(uiTime, uiAttrId, uiValue, iRetryTimes + 1);			
		}
		return true;
	}
	iRet = UpdateRecord(uiTime, uiAttrId, uiValue);
	if (iRet == -1)
	{
		return UpdateAttrValue(uiTime, uiAttrId, uiValue, iRetryTimes + 1);
	}
	return true;
}

int MonitorReporter::QueryRecordExsit(uint32_t uiTime, uint32_t uiAttrId)
{
	uint64_t uiKey = 0;
	{
		char buff[128] = {0};
		snprintf(buff, 128, "%u%u", uiAttrId, uiTime);
		uiKey = strtoull(buff, NULL, 10);
	}
	string strsql;
	char buff[2048];
	snprintf(buff, 2048, "select * from monitor_report_list where report_id=%lu", uiKey);
	strsql = buff;
	cout << "sql:" << strsql << endl;
	Query query = m_db_conn->query(strsql);
	StoreQueryResult queryResult = query.store();
	if (!queryResult)
	{
		cout << "sql failed: " << query.error() << endl;
		return -1;
	}
	return queryResult.empty() ? 0 : 1;
}

int MonitorReporter::UpdateRecord(uint32_t uiTime, uint32_t uiAttrId, uint32_t uiValue)
{
	uint64_t uiKey = 0;
	{
		char buff[128] = { 0 };
		snprintf(buff, 128, "%u%u", uiAttrId, uiTime);
		uiKey = strtoull(buff, NULL, 10);
	}
	string strsql;
	char buff[2048];
	snprintf(buff, 2048, "update monitor_report_list set attr_value=attr_value+%u where report_id=%lu", uiValue, uiKey);
	strsql = buff;
	cout << "sql:" << strsql << endl;
	Query updateQuery = m_db_conn->query(strsql);
	if (!updateQuery.exec())
	{
		cout << "sql failed: " << updateQuery.error() << endl;
		return -1;
	}
	return 0;
}

int MonitorReporter::InsertRecord(uint32_t uiTime, uint32_t uiAttrId, uint32_t uiValue)
{
	uint64_t uiKey = 0;
	{
		char buff[128] = { 0 };
		snprintf(buff, 128, "%u%u", uiAttrId, uiTime);
		uiKey = strtoull(buff, NULL, 10);
	}
	string strsql;
	char buff[2048];
	snprintf(buff, 2048, "insert into monitor_report_list(report_id, attr_id, attr_value, report_time) values(%lu, %u, %u, %u)", uiKey, uiAttrId, uiValue, uiTime);
	strsql = buff;
	cout << "sql:" << strsql << endl;
	Query query = m_db_conn->query(strsql);
	if (!query.exec())
	{
		cout << "sql failed: " << query.error() << endl;
		return -1;
	}
	return 0;
}
