//#include "stdafx.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/listener.h"
#include "event2/util.h"
#include "event2/event.h"
#include <event2/event-config.h>
#include <WinSock2.h>
#include <iostream>

#pragma warning(disable:4996)

#define IP_ADDRESS ("127.0.0.1")
#define PORT (9951)


int m_isrun = 0;
int tcp_connect_server(const char* server_ip, int port);
void cmd_msg_cb(int fd, char* msg);
void socket_read_cb(int fd, short events, void *arg);
DWORD WINAPI Fun1Proc(LPVOID lpParameter)
{
	char t_cin[1024];
	int sockfd = (int)lpParameter;
	while (1) {
		memset(t_cin, 0, 1024);

		std::cin >> t_cin;

		if (strcmp(t_cin, "exit") == 0) {
			break;
		}
		cmd_msg_cb(sockfd, t_cin);
	}
	exit(1);
	return 0;
}
int main(int argc, char** argv)
{
	//加载套接字库  
	WSADATA wsaData;
	int iRet = 0;
	iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iRet != 0)
	{
		return -1;
	}

	if (2 != LOBYTE(wsaData.wVersion) || 2 != HIBYTE(wsaData.wVersion))
	{
		WSACleanup();
		return -1;
	}
	//两个参数依次是服务器端的IP地址、端口号  
	int sockfd = tcp_connect_server(IP_ADDRESS, PORT);
	if (sockfd == -1)
	{
		perror("tcp_connect error ");
		return -1;
	}
	printf("connect to server successful\n");
	struct event_base* base = event_base_new();
	struct event *ev_sockfd = event_new(base, sockfd, EV_READ | EV_PERSIST, socket_read_cb, NULL);
	event_add(ev_sockfd, NULL);
	//创建线程发送数据
	HANDLE hThread1 = CreateThread(NULL, 0, Fun1Proc, (void*)sockfd, 0, NULL);
	CloseHandle(hThread1);
	event_base_dispatch(base);
	WSACleanup();
	printf("finished \n");
	return 0;
}
void cmd_msg_cb(int fd, char* msg)
{
	//把终端的消息发送给服务器端  
	int ret = send(fd, (const char*)msg, strlen((char*)msg), 0);
	if (ret <= 0)
	{
		perror("read fail ");
		return;
	}
	printf("send:%s\n", (char*)msg);
}
void socket_read_cb(int fd, short events, void *arg)
{
	char msg[1024];
	//为了简单起见，不考虑读一半数据的情况  
	int len = recv(fd, msg, sizeof(msg) - 1, 0);
	if (len <= 0)
	{
		perror("read fail ");
		exit(1);
	}
	msg[len] = '\0';
	printf("recv %s from server\n", msg);
}
int tcp_connect_server(const char* server_ip, int port)
{
	int sockfd, status, save_errno;
	SOCKADDR_IN  server_addr;

	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip);
	server_addr.sin_port = htons(port);

	sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		return sockfd;


	status = ::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if (status == -1)
	{
		save_errno = errno;   //清理  
		closesocket(sockfd);
		errno = save_errno; //the close may be error  
		return -1;
	}

	evutil_make_socket_nonblocking(sockfd);

	return sockfd;
}