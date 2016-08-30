#pragma once
#include <pthread.h>
#include "ikcp.h"
#include <stdio.h>
#include "time_simple.h"
#include "unp_simple.h"

class transer
{
public:
	transer(IUINT32 conv, void *user, int mode);
	~transer();
	void DoWork();
	int Decode(const char* buff);
private:
	 // http connect info
	bool m_containHead;
	char m_buffer[MAXLINE];
	int m_posBuffer;
	// kcpcb
	ikcpcb* m_kcp;
public:
	// protect data in deque
	pthread_mutex_t m_mutex;
};

