#include <iostream>
#include <sys/stat.h>
#include <sys/socket.h>
#include <math.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sstream>

#include "ikcp.h"
#include "unp_simple.h"
#include "time_simple.h"

#define TIME_INTEVAL 50000
#define TIME_EXIT 1000
#define TIME_RECVINTEERVAL 1
#define TIME_UPDATE_INTERVAL 10

int					sockfd;
struct sockaddr_in	servaddr, grpaddr, cliaddr;

enum WorKStatus
{	
	WORK_SEND,
	WORK_KCPRECV,
	WORK_DECODE,
	WORK_UPDATEBUFF,
	WORK_ERROR,
	WORK_COMPLETE
};

int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
	Sendto(sockfd, buf, len, 0, (SA *)&cliaddr, sizeof(cliaddr));
	return 0;
}

int decode(const char* buff, int len, std::string& filename)
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
	std::stringstream is(std::string(pos+strlen("GET")));
	is >> filename;
	printf("INFO: filename --- %s\n", filename.c_str());
	delete[] iBuff;
	return 0;
}

void
dg_echo(int sockfd, SA *pcliaddr, socklen_t clilen, int mode)
{
	int			n;
	socklen_t	len;
	char		mesg[MAXLINE];
	int			total = 0;
	int			fileSize;


	ikcpcb *kcp1 = ikcp_create(0x11223344, (void*)0);
	kcp1->output = udp_output;
	ikcp_setmtu(kcp1, 5600);

	ikcp_wndsize(kcp1, 128, 128);

	// 判断测试用例的模式
	if (mode == 0) {
		// 默认模式
		ikcp_nodelay(kcp1, 0, 10, 0, 0);
	}
	else if (mode == 1) {
		// 普通模式，关闭流控等
		ikcp_nodelay(kcp1, 0, 10, 0, 1);
	}
	else {
		// 启动快速模式
		// 第二个参数 nodelay-启用以后若干常规加速将启动
		// 第三个参数 interval为内部处理时钟，默认设置为 10ms
		// 第四个参数 resend为快速重传指标，设置为2
		// 第五个参数 为是否禁用常规流控，这里禁止
		ikcp_nodelay(kcp1, 1, 10, 2, 1);
		kcp1->rx_minrto = 10;
		kcp1->fastresend = 1;
	}

	WorKStatus workStatus = WORK_KCPRECV;
	std::string filename;
	bool notComplete = true;
	bool connected = false;
	FILE* fp = NULL;
	int posBuff = 0;
	bool notSend = false;
	IUINT32 lastRecv = iclock();

	IUINT32 lastUpdate = iclock();
	ikcp_update(kcp1, lastUpdate);

	while (notComplete)
	{

		if (iclock() - lastRecv > TIME_INTEVAL && true == connected)
		{
			printf("ERROR: time is up.\n");
			break;
		}

		if (iclock() - lastUpdate > TIME_UPDATE_INTERVAL)
		{
			lastUpdate = iclock();
			ikcp_update(kcp1, lastUpdate);
		}

		len = clilen;
		n = recvfrom(sockfd, mesg, sizeof(mesg), 0, pcliaddr, &len);
		if (n > 0)
		{
			// update the last recv time
			lastRecv = iclock();
			printf("INFO: recv a package.\n");
			ikcp_input(kcp1, mesg, n);
		}

		switch (workStatus)
		{
		case WORK_KCPRECV:
			//printf("INFO: in kcprecv.\n");
			posBuff = ikcp_recv(kcp1, mesg, sizeof(mesg));
			if (posBuff > 0)
			{
				printf("\n***********************************************\n");
				printf("INFO: kcprecv buff\n");
				printf("%s\n", mesg);
				printf("\n***********************************************\n");
				connected = true;
				workStatus = WORK_DECODE;
			}
			else if (true == connected)
			{
				workStatus = WORK_UPDATEBUFF;
			}
			else
			{
				workStatus = WORK_KCPRECV;
			}
			break;
		case WORK_DECODE:
			//printf("INFO: in decode.\n");
			n = decode(mesg, posBuff, filename);
			if (0 == n)
			{
				fp = fopen(filename.c_str(), "r");
				if (NULL == fp)
				{
					n = snprintf(mesg, sizeof(mesg), "HTTP/1.0 400 Bad Request\r\n\r\n");
					n = ikcp_send(kcp1, mesg, n);
					if (n < 0)
					{
						printf("ERROR: ikcp_send.\n");
						workStatus = WORK_ERROR;
					}
					else
					{
						printf("\n***********************************************\n");
						printf("INFO: send buff\n");
						printf("%s\n", mesg);
						printf("\n***********************************************\n");
						workStatus = WORK_COMPLETE;
					}
				}
				else
				{
					fseek(fp, 0L, SEEK_END);
					fileSize = ftell(fp);
					fseek(fp, 0L, SEEK_SET);
					n = snprintf(mesg, sizeof(mesg), "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n\r\n", fileSize);
					total = fread(mesg + n, 1, sizeof(mesg)-n, fp);
					n += total;
					printf("\n***********************************************\n");
					printf("INFO: send content %d send buff\n",
						n - strlen("HTTP/1.0 200 OK\r\nContent-Length: %d\r\n\r\n"));
					printf("%s\n", mesg);
					printf("\n***********************************************\n");
					n = ikcp_send(kcp1, mesg, n);

					if (n < 0)
					{
						workStatus = WORK_ERROR;
						printf("ERROR: ikcp_send.\n");
					}
					else
					{
						workStatus = WORK_KCPRECV;
					}
				}
			}
			else
			{
				workStatus = WORK_ERROR;
			}
			break;
		case WORK_SEND:
			//printf("INFO: in send.\n");
			n = ikcp_send(kcp1, mesg, posBuff);
			if (n < 0)
			{
				workStatus = WORK_SEND;
			}
			else
			{
				total += posBuff;
				workStatus = WORK_KCPRECV;
			}
			break;
		case WORK_UPDATEBUFF:
			//printf("INFO: in updatebuff.\n");
			if (NULL == fp)
			{
				workStatus = WORK_KCPRECV;
				break;
			}
			//printf("INFO: in update buff.\n");
			posBuff = fread(mesg, 1, sizeof(mesg), fp);
			if (0 == posBuff)
			{
				printf("ERROR: error fread 0 byte, maye end of file.\n");
				workStatus = WORK_ERROR;
			}
			else
			{
				printf("INFO: send %d bytes.\n", posBuff);
				workStatus = WORK_SEND;
			}
			break;
		case WORK_ERROR:
			workStatus = WORK_KCPRECV;
			break;
		case WORK_COMPLETE:
			workStatus = WORK_KCPRECV;
			break;
		default:
			printf("ERROR: unknown.\n");
			break;
		}
	}
	ikcp_release(kcp1);
	printf("INFO: total sent %d\n", total);
}

int main()
{

	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	struct timeval tv_out;
	tv_out.tv_sec = 0;
	tv_out.tv_usec = 10000; // wait for 10ms
	Setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));

	Bind(sockfd, (SA *)&servaddr, sizeof(servaddr));
	while (true)
	{
		printf("INFO: can accept new data.\n");
		dg_echo(sockfd, (SA *)&cliaddr, sizeof(cliaddr), 0);
	}
	return 0;
}

