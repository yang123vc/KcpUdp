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

	// http connect info
	m_containHead = true;
	m_posBuffer = 0;
}

transer::~transer()
{
	ikcp_release(m_kcp);
}