#pragma once
#include "ikcp.h"
class transer
{
public:
	transer(IUINT32 conv, void *user);
	~transer();
	void DoWork();
	void Nodelay(int nodelay, int interval, int resend, int nc);
	int Decode(const char* buff);
private:
	 // send info
	bool m_recvHead;
	ikcpcb* m_kcp;
};

transer::transer(IUINT32, void *user)
{
}

transer::~transer()
{
}