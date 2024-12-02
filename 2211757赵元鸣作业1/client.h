#pragma once
#pragma warning(disable:4996)
#pragma comment(lib,"ws2_32.lib")
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<stdio.h>
using namespace std;


void PrintClients(char* c_list);
char* setIP();
int charLen(char* a);
bool isExitting(char* a);
bool OtherExitting(char* a);
int clientStart();
DWORD WINAPI RecMsg(LPVOID);
