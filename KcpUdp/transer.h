#pragma once
#include <pthread.h>
#include <queue>
#include <sstream>
#include "ikcp.h"
#include <stdio.h>
#include "time_simple.h"
#include "unp_simple.h"

// for break connect 
#ifndef TIME_MAX_INTERVAL
#define TIME_MAX_INTERVAL 2000
#endif



class transer
{
public:
	transer(IUINT32 conv, void *user, int mode);
	~transer();
	void DoWork();
	int Decode(const char* buff, int len, std::string& filename);
private:
	// http connect info
	bool m_hasParseHead;
	char m_buffer[MAXLINE];
	int m_posBuffer;
	// file info
	std::string m_fileName;
	unsigned long long m_start;
	unsigned long long m_end;
	unsigned long long m_fileSize;
	// kcpcb
	ikcpcb* m_kcp;
	IUINT32 m_lastRecvTimeStamp;
public:
	// protect data in deque
	pthread_mutex_t m_mutex;
	std::queue<std::string> m_dataQueue;
};

