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
	// �жϲ���������ģʽ
	if (mode == 0) {
		// Ĭ��ģʽ
		ikcp_nodelay(m_kcp, 0, 10, 0, 0);
	}
	else if (mode == 1) {
		// ��ͨģʽ���ر����ص�
		ikcp_nodelay(m_kcp, 0, 10, 0, 1);
	}
	else {
		// ��������ģʽ
		// �ڶ������� nodelay-�����Ժ����ɳ�����ٽ�����
		// ���������� intervalΪ�ڲ�����ʱ�ӣ�Ĭ������Ϊ 10ms
		// ���ĸ����� resendΪ�����ش�ָ�꣬����Ϊ2
		// ��������� Ϊ�Ƿ���ó������أ������ֹ
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