#include "../monitor_defs.h"
#include "MonitorCollecter.h"
#include "MonitorReporter.h"
#include <iostream>
#include <map>

using namespace std;
MonitorCollecter collecter;
MonitorReporter reporter;

std::map<uint32_t/*time*/, std::map<uint32_t/*attr*/, uint32_t/*value*/> > g_collect_data;
pthread_mutex_t g_data_mutex;

void* CollectFunc(void*)
{
	collecter.Run();
}

void* ReporterFunc(void*)
{
	reporter.Run();
}

int main(int argc, char** argv)
{
	bool bRet = collecter.Init();
	if (!bRet)
	{
		cout << "Init agent err, err code:" << collecter.GetLastErrCode() << endl;
		exit(0);
	}
	bRet = reporter.Init();
	if (!bRet)
	{
		cout << "Init reporter err, err code:" << reporter.GetLastErrCode() << endl;
		exit(0);
	}

	if (pthread_mutex_init(&g_data_mutex, NULL) != 0)
	{
		cout << "Init data mutext err" << endl;
		exit(0);
	}


	pthread_t collecterContext, reportContext;
	int iRet= pthread_create(&collecterContext, NULL, CollectFunc, NULL);
	if ( iRet != 0)
	{
		cout << "start collecter err" << endl;
		exit(0);
	}
	iRet = pthread_create(&reportContext, NULL, ReporterFunc, NULL);
	if (iRet != 0)
	{
		cout << "start reporter err" << endl;
		exit(0);
	}
	pthread_join(collecterContext, NULL);
	pthread_join(reportContext, NULL);
	return 0;
}

