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

#define TIME_INTEVAL 2000
#define TIME_EXIT 1000
#define TIME_RECVINTEERVAL 1
#define TIME_UPDATE_INTERVAL 10

int					sockfd;
struct sockaddr_in	servaddr, grpaddr, cliaddr;

enum WorKStatus
{	
	WORK_SEND,
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

int getFileSize(FILE* fp)
{
	int ret = 0;
	if (NULL != fp)
	{
		fseek(fp, 0L, SEEK_END);
		ret = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
	}
	return ret;
}

void Fwrite(void* ptr, size_t size, size_t n, FILE* fp)
{
	int ret = fwrite(ptr, size, n, fp);
	if (-1 == ret)
	{
		printf("ERROR: fwrite, error number %d.\n", errno);
	}
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

	std::string filename;
	FILE* fp = NULL;
	int posBuff = 0;
	bool getHead = false;
	IUINT32 lastRecv = iclock();
	IUINT32 lastUpdate = iclock();
	bool hasError = true;

	// for record msg had sent
	FILE* msg_fp = fopen("send.txt", "w");

	for (;;)
	{

		if (iclock() - lastRecv > TIME_INTEVAL)
		{
			printf("INFO: time is up.\n");
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
			ikcp_input(kcp1, mesg, n);
		}

		if (false == getHead)
		{
			posBuff = ikcp_recv(kcp1, mesg, sizeof(mesg));
			if (posBuff <= 0)
			{
				// not recv head, continue loop
				continue;
			}
			// recv head, didn't need to enter this if again.
			getHead = true;
			printf("\n***********************************************\n");
			printf("INFO: kcprecv buff\n");
			printf("%s\n", mesg);
			printf("\n***********************************************\n");

			n = decode(mesg, posBuff, filename);

			if (0 == n)
			{
				fp = fopen(filename.c_str(), "rb");
				if (NULL == fp)
				{
					posBuff = snprintf(mesg, sizeof(mesg), "HTTP/1.0 400 Bad Request\r\n\r\n");
					n = ikcp_send(kcp1, mesg, posBuff);
					if (n < 0)
					{
						printf("ERROR: ikcp_send.\n");
						// break for loop
						break;
					}
					else
					{
						printf("\n***********************************************\n");
						printf("INFO: send buff\n");
						printf("%s\n", mesg);
						printf("\n***********************************************\n");
						// has send everythin
						hasError = true;
					}
				}
				else
				{
					fileSize = getFileSize(fp);
					posBuff = snprintf(mesg, sizeof(mesg), "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n\r\n", fileSize);
					total = fread(mesg + posBuff, 1, sizeof(mesg)-posBuff, fp);
					posBuff += total;
					printf("\n***********************************************\n");
					printf("INFO: send content %d send buff\n", total);
					printf("%s\n", mesg);
					printf("\n***********************************************\n");
					n = ikcp_send(kcp1, mesg, posBuff);
					Fwrite(mesg, 1, posBuff, msg_fp);

					if (n < 0)
					{
						printf("ERROR: ikcp_send.\n");
						// break while
						break;
					}
					else
					{
						// 进入下列的send
						hasError = false;
					}
				}
			}
			else
			{
				// break for loop
				printf("ERROR: when decode recv buffer.\n");
				break;
			}
		}


		if (false == hasError)
		{
			IUINT32 ts = iclock();
			while (iclock() - ts < 5)
			{
				posBuff = fread(mesg, 1, sizeof(mesg), fp);
				if (0 == posBuff)
				{
					printf("ERROR: error fread 0 byte, maye end of file.\n");
					fclose(fp);
					hasError = true;
					// 继续下一次循环
					break;
				}
				n = ikcp_send(kcp1, mesg, posBuff);
				//  record what was sent
				Fwrite(mesg, 1, posBuff, msg_fp);
				if (n < 0)
				{
					printf("ERROR: ikcp_send %d.\n", n);
					hasError = true;
					// 继续下一次循环
					break;
				}
				else
				{
					printf("INFO: kcp_send %d.\n", posBuff);
					total += posBuff;
				}
			}
		}
	}
	printf("INFO: total resend %d.\n", kcp1->xmit);
	fclose(msg_fp);
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

	Setnonblocking(sockfd);

	Bind(sockfd, (SA *)&servaddr, sizeof(servaddr));
	while (true)
	{
		printf("INFO: can accept new data.\n");
		dg_echo(sockfd, (SA *)&cliaddr, sizeof(cliaddr), 0);
	}
	return 0;
}

