#pragma once
#pragma comment(lib,"ws2_32.lib")
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<winsock.h>
#include<stdio.h>
#include<iostream>
#include<cstring>

struct CInfo {
	int num;
	char name[10];
	bool online = false;
	int len;
};

void setSIP(char* p);
bool isName(char* a);
bool cmpStr(char* a, char* b);
bool wantCList(char* a);
bool ClientExit(char* a);
bool isWorldChat(char* a);
DWORD WINAPI recMsg(LPVOID arg);

int serverStart();