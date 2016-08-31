#include "transer.h"


void* Work(void* pVoid)
{
	transer* pThis = (transer*)pVoid;
	pThis->DoWork();
}

transer::transer(IUINT32 conv, void *user, int mode)
{
	m_kcp = ikcp_create(conv, user);
	if (NULL == m_kcp)
	{
		printf("ERROR: when ikcp_create.\n");
	}
	// 判断测试用例的模式
	if (mode == 0) {
		// 默认模式
		ikcp_nodelay(m_kcp, 0, 10, 0, 0);
	}
	else if (mode == 1) {
		// 普通模式，关闭流控等
		ikcp_nodelay(m_kcp, 0, 10, 0, 1);
	}
	else {
		// 启动快速模式
		// 第二个参数 nodelay-启用以后若干常规加速将启动
		// 第三个参数 interval为内部处理时钟，默认设置为 10ms
		// 第四个参数 resend为快速重传指标，设置为2
		// 第五个参数 为是否禁用常规流控，这里禁止
		ikcp_nodelay(m_kcp, 1, 10, 2, 1);
		m_kcp->rx_minrto = 10;
		m_kcp->fastresend = 1;
	}
	// file info
	m_start = m_end = m_fileSize = 0;

	// http connect info
	m_hasParseHead = false;
	m_posBuffer = 0;
	
	// initial mutex
}

transer::~transer()
{
	ikcp_release(m_kcp);
}

void transer::DoWork()
{
	m_lastRecvTimeStamp = iclock();
	int ret = 0;

	for (;;)
	{
		if (iclock() - m_lastRecvTimeStamp > TIME_MAX_INTERVAL)
		{
			printf("INFO: time is up.\n");
			break;
		}

		if (!m_dataQueue.empty())
		{
			std::string str;
			pthread_mutex_lock(&m_mutex);
			while (!m_dataQueue.empty())
			{
				str = m_dataQueue.front();
				ret = ikcp_input(m_kcp, str.c_str(), str.size());
				m_dataQueue.pop();
			}
			pthread_mutex_unlock(&m_mutex);
		}

		if (false == m_hasParseHead)
		{
			m_posBuffer = ikcp_recv(m_kcp, m_buffer, sizeof(m_buffer));
			if (m_posBuffer > 0)
			{
				// get a head
				m_hasParseHead = true;
				ret = Decode(m_buffer, m_posBuffer, m_fileName);
				if (0 == ret)
				{

				}
				else
				{

				}
			}
			else
			{}
		}
	}
}

int transer::Decode(const char* buff, int len, std::string& filename)
{
	char* iBuff = new char[len + 1];
	iBuff[len] = '\0';
	memcpy(iBuff, buff, len);
	char* pos = strstr(iBuff, "GET");

	if (NULL == pos)
	{
		delete[] iBuff;
		return -1;
	}
	std::stringstream is(std::string(pos + strlen("GET")));
	is >> filename;
	printf("INFO: filename --- %s\n", filename.c_str());
	
	// get start and end
	pos = strstr(iBuff, "Range: bytes =");
	if (NULL == pos)
	{
		m_start = m_end = 0;
	}
	else
	{
		m_start = atoi(pos + strlen("Range: bytes ="));

	}
	delete[] iBuff;
	return 0;
}