#include"client.h"
#include<iostream>
#include <windows.h>
#include <ws2tcpip.h> 
#include<cstring>
#include<ctime>




const int c_buffer_size = 1024;
char c_send_buffer[c_buffer_size];
char c_input_buffer[c_buffer_size];
char c_rev_buffer[c_buffer_size];
char usr_name[c_buffer_size];
SOCKET rem_sock, local_sock;
SOCKADDR_IN rem_adr, local_adr;



void PrintClients(char* c_list) {
	int list_len = charLen(c_list);
	int pos_idx = 1;
	int num = 1;
	while (pos_idx < list_len) {
		int len = int(c_list[pos_idx]);
		pos_idx += 1;
		int lp = pos_idx + len;
		cout << num << ". ";
		num += 1;
		for (; pos_idx < lp; pos_idx += 1)
			cout << c_list[pos_idx];
		cout << endl;
	}
}

char* setIP() {
	char* my_ip = new char[16];
	cin >> my_ip;
	return my_ip;
}

int charLen(char* a) {
	int idx = 0;
	while (a[idx] != '\0')
		idx++;
	return idx;
}
bool isExitting(char* a) {
	if (a[0] == 'e' && a[1] == 'x' && a[2] == 'i' && a[3] == 't')
		return true;
	else
		return false;
}
bool OtherExitting(char* a) {
	if (a[0] == 'e')
		return true;
	else
		return false;
}
DWORD WINAPI RecMsg(LPVOID) {
	while (1) {
		::memset(c_rev_buffer, 0, c_buffer_size);
		if (recv(local_sock, c_rev_buffer, c_buffer_size, 0) < 0) {
			cout << "step 1 fail. Errcode: " << SOCKET_ERROR << ".\n";
			system("pause");
		}
		else if (c_rev_buffer[0] == 's')
			PrintClients(c_rev_buffer);
		else if (OtherExitting(c_rev_buffer)) {
			int len = int(c_rev_buffer[1]);
			char* exittingOne = new char[len + 1];
			for (int i = 0; i < len; i++)
				exittingOne[i] = c_rev_buffer[i + 2];
			exittingOne[len] = '\0';
			cout << "Client " << exittingOne << " exit from chat.\n";
		}
		
		else {
			char t_str[26];
			int idx = 0;
			for (; idx < 25; idx++)
				t_str[idx] = c_rev_buffer[idx];
			t_str[idx] = '\0';
			int len = int(c_rev_buffer[idx]);
			idx += 1;
			char* name = new char[len + 1];
			for (; idx < 26 + len; idx++)
				name[idx - 26] = c_rev_buffer[idx];
			name[idx - 26] = '\0';
			while (c_rev_buffer[idx] == '\0')
				idx++;
			cout << t_str<< "\t Client  " << name << "\t send:\t";
			while (c_rev_buffer[idx] != '\0') {
				cout << c_rev_buffer[idx];
				idx++;
			}
			cout << endl;
		}
	}
	return 0;
}





int clientStart() {
	
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		
		cout << "step 3 fail\n";
		system("pause");
		return -1;
	}

	
	local_sock = socket(AF_INET, SOCK_STREAM, 0);
	
	if (local_sock == INVALID_SOCKET) {
		cout << "step 4 fail:" << WSAGetLastError() << endl;
		WSACleanup();
		return -2;
	}

	
	local_adr.sin_family = AF_INET;
	local_adr.sin_port = htons(8870);
	inet_pton(AF_INET, "127.0.0.1", &local_adr.sin_addr.S_un.S_addr);


	
	rem_adr.sin_family = AF_INET;
	cout << "input IP\n";
	char* obj_ip = setIP();
	cin.get();
	rem_adr.sin_port = htons(8860);
	inet_pton(AF_INET, obj_ip, &rem_adr.sin_addr.S_un.S_addr);

	
	cout << "input nickname\n";
	char inputName[c_buffer_size];
	cin.getline(inputName, c_buffer_size);
	usr_name[0] = char(charLen(inputName));
	int idxOfInput = 0;
	while (inputName[idxOfInput] != '\0') {
		usr_name[idxOfInput + 1] = inputName[idxOfInput];
		idxOfInput++;
	}
	usr_name[idxOfInput + 2] = '\0';
	
	char* sendName = new char[int(usr_name[0]) + 4];
	sendName[0] = 'n';
	sendName[1] = usr_name[0];
	int l = usr_name[0];
	for (int ind = 0; ind < l; ind++)
		sendName[2 + ind] = usr_name[1 + ind];
	sendName[l + 3] = '\0';

	
	if (connect(local_sock, (SOCKADDR*)&rem_adr, sizeof(rem_adr)) != SOCKET_ERROR) {
		send(local_sock, sendName, int(usr_name[0] + 2), 0);
		HANDLE recv_thread = CreateThread(NULL, NULL, RecMsg, NULL, 0, NULL);
		while (1) {
			memset(c_input_buffer, 0, c_buffer_size);
			char input[c_buffer_size];
			cin.getline(input, c_buffer_size);

			if (!strcmp(input, "getc_list\0")) {
				send(local_sock, input, 11, 0);
			}
			else {
				if (isExitting(input)) {
					char tmp[c_buffer_size];
					memset(tmp, 0, c_buffer_size);
					tmp[0] = 'e';
					strcat(tmp, usr_name);
					strcat(tmp, "exit\0");
					send(local_sock, tmp, sizeof(tmp), 0);
					cout << "Exit done.\n";
					closesocket(local_sock);
					closesocket(rem_sock);
					WSACleanup();
					system("pause");
					return 0;
				}
				else {
					int i = 0;
					while (input[i] != ':' && input[i] != '\0')
						i++;
					char* objC = new char[i + 2];
					objC[0] = char(i);
					for (int j = 1; j <= i; j++)
						objC[j] = input[j - 1];
					objC[i + 1] = '\0';
					i += 1;
					int j = i;
					while (input[i] != '\0') {
						c_input_buffer[i - j] = input[i];
						i += 1;
					}
					c_input_buffer[i - j] = '\0';

					time_t now = time(0);
					char* timer = ctime(&now);
					memset(c_send_buffer, 0, c_buffer_size);
					strcat(c_send_buffer, timer);
					strcat(c_send_buffer, objC);
					strcat(c_send_buffer, usr_name);
					strcat(c_send_buffer, c_input_buffer);
					send(local_sock, c_send_buffer, c_buffer_size, 0);
					memset(c_send_buffer, '\0', c_buffer_size);
					memset(c_input_buffer, '\0', c_buffer_size);
				}
			}
		}
	}
	else {
		cout << "step 5 fail: " << SOCKET_ERROR << ".\n";
		return 0;
	}
	closesocket(local_sock);
	closesocket(rem_sock);
	WSACleanup();
	system("pause");
	return 0;
}