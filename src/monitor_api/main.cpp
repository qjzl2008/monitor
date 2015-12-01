#include "monitor_api.h"
#include <iostream>
#include <errno.h>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <algorithm>
#include <stdio.h>
#include <set>
#include <stdint.h>
#include <sys/time.h>

using namespace std;

void costtimeTest();
void threadTest();
int main(int argc, char** argv)
{
	//CLEAR_ALL_DATE();
	//DEBUG_LIST();
	//return 0;
	//costtimeTest();
	//DEBUG_LIST();
	map<uint32_t, uint32_t> tmpMap = DEBUG_LIST();
	auto it = tmpMap.begin();
	for (; it != tmpMap.end(); ++it)
	{
		cout << "attrid:" << it->first << ", value:" << it->second << endl;
	}
	threadTest();
	return 0;
}

void costtimeTest()
{
	struct timeval start_tv, end_tv;
	gettimeofday(&start_tv, NULL);
	for (int i = 0; i < 1000000; ++i)
	{
		MONITOR_API(1, 1);
	}
	gettimeofday(&end_tv, NULL);
	cout << "start sec:" << start_tv.tv_sec << ", usec:" << start_tv.tv_usec << endl;
	cout << "end   sec:" << end_tv.tv_sec   << ", usec:" << end_tv.tv_usec   << endl;
	cout << "cost time:" << (end_tv.tv_sec - start_tv.tv_sec) * 1000 + (end_tv.tv_usec - start_tv.tv_usec) / 1000 << endl;
}

void* threadfunc(void* index)
{
	int iIndex = *(int*)index;
	cout << "start:" << iIndex << endl;
	for (int i = 0; i < 10000; ++i)
	{
		int ret = MONITOR_API(iIndex, 1);
		if (ret != RetCode_OK)
		{
			cout << "ret:" << ret << endl;
		}
		sleep(1);
	}
}
void threadTest()
{

	int indexList[11];
	pthread_t tidList[11];
	for (int i = 0; i < 11; ++i)
	{
		indexList[i] = i;
	}
	//cout << "start:" << g_globalTest << endl;
	for (int i = 1; i <= 10; ++i)
	{
		//cout << "create thread " << i << endl;
		int iret = pthread_create(&tidList[i], NULL, &threadfunc, &indexList[i]);
		if (iret != 0)
		{
			cout << "create threa err" << endl;
		}
	}
	for (int i = 1; i < 11; ++i)
	{
		pthread_join(tidList[i], NULL);
	}
}

bool isprime(int num)
{
	if (num <= 0)
		return false;
	for (int i = 2; i < num; ++i)
	{
		if (num % i == 0)
			return false;
	}
	return true;
}

void hashtest()
{
	//cout << "3%1" << 3 % 1 << 3 % 2;
	bool indexList[10000] = { false };
	int prime_table[] = {9733, 9739, 9743, 9749, 9767, 9769, 9781, 9787, 9791, 9803, 9811, 9817, 9829, 9833, 9839, 9851, 9857, 9859, 9871, 9883, 9887, 9901, 9907, 9923, 9929, 9931, 9941, 9949, 9967, 9973 };
	//cout << "prime_table size:" << sizeof(prime_table) / sizeof(int) << endl;
	int first_count = 0, second_count = 0;
	set<int> hash_set;
	for (int i = 10001; i < 20000; ++i)
	{
		int has_value = i% prime_table[0];
		if (has_value >= 10000 || has_value < 0)
		{
			cout << "has_value:" << has_value << ", index:" << i << endl;
			continue;
		}
		if (indexList[has_value] == true)
		{
			//cout << "first confilct has_value:" << has_value << ", index:" << i << endl;
			++first_count;
			has_value = i% prime_table[1];
			hash_set.insert(has_value);
			if (indexList[has_value] == true)
			{
				++second_count;
				//cout << "second confilct has_value:" << has_value << ", index:" << i << endl;
				continue;
			}
		}
		indexList[has_value] = true;
	}
	cout << "first count:" << first_count << ", second:" << second_count << ", hash_size:" << hash_set.size() << endl;

}