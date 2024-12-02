#include"server.h"
#include"client.h"
#include <windows.h>
#include <ws2tcpip.h> 
#include <ctime>


int s_port = 8860;
char s_ip[16];
const int s_buffer_size = 1024;
const int max_online = 10;
int cur_online_cnt = 0;
int sock_adr = sizeof(SOCKADDR_IN);
char s_send_buffer[s_buffer_size];
char s_input_buffer[s_buffer_size];
char s_rec_buffer[s_buffer_size];
CInfo c_info[max_online];
SOCKET s_sock;
SOCKET c_socks[max_online];
SOCKADDR_IN s_adr;
SOCKADDR_IN c_adrs[max_online];
HANDLE c_threads[max_online];





void setSIP(char* p) {
	cout << "input ip\n";
	char ip[16];
	cin.getline(ip, sizeof(ip));
	int idx = 0;
	while (ip[idx] != '\0') {
		p[idx] = ip[idx];
		idx++;
	}
}

bool isName(char* a) {
	if (a[0] == 'n')
		return true;
	else
		return false;
}



bool cmpStr(char* a, char* b) {
	int i = 0;
	bool flag = true;
	while (a[i] != '\0' && b[i] != '\0') {
		if (a[i] != b[i]) {
			flag = false;
			break;
		}
		i++;
	}
	if (a[i] != '\0' || b[i] != '\0')
		flag = false;
	return flag;
}

bool wantCList(char* a) {
	const char* cmd = "showlist\0";
	if (!strcmp(cmd, a))
		return true;
	else
		return false;
}

bool isWorldChat(char* a) {
	const char* temp = "world\0";
	if (!strcmp(temp, a))
		return true;
	else
		return false;
}

bool ClientExit(char* a) {
	int l1 = int(a[25]);
	int l2 = int(a[25 + l1 + 1]);
	int len = 0;
	while (a[len] != '\0')
		len++;
	if ((len == (25 + 1 + l1 + 1 + l2 + 4)) && a[25 + 1 + l2 + 1 + 2] == 'e' && a[25 + 1 + l1 + 1 + 2 + 1] == 'x' && a[25 + 1 + l1 + 1 + 2 + 2] == 'i' && a[25 + 1 + l1 + 1 + 2 + 3] == 't')
		return true;
	else
		return false;
}

DWORD WINAPI recMsg(LPVOID arg) {
	CInfo* ci = (CInfo*)arg;
	int num = ci->num;
	while (1) {
		
		if (recv(c_socks[num], s_rec_buffer, s_buffer_size, 0) < 0) {
			cout << "step11: " << SOCKET_ERROR << ".\n";
		}
		else {
			if (s_rec_buffer[0] == 'e') {
				int len = int(s_rec_buffer[1]);
				char* name = new char[len + 1];
				for (int index = 0; index < len; index++)
					name[index] = s_rec_buffer[index + 2];
				name[len] = '\0';
				cout << " Client\t" << name << " \tleft\n";
				cur_online_cnt -= 1;
				cout << "there are " << cur_online_cnt << "\t  client(s) left\n";

				for (int i = 0; i < max_online; i++) {
					if (i == num || c_socks[i] == INVALID_SOCKET)
						continue;
					else
						send(c_socks[i], s_rec_buffer, sizeof(s_rec_buffer), 0);
				}
				closesocket(c_socks[num]);
				c_info[num].online = false;
				break;
			}
			else{
				if (wantCList(s_rec_buffer)) {
					char c_list[s_buffer_size];
					memset(c_list, 0, s_buffer_size);
					c_list[0] = 's';
					int pos_idx = 1;
					for (int temp = 0; temp < max_online; temp++) {
						if (c_info[temp].online) {
							c_list[pos_idx] = char(c_info[temp].len);
							pos_idx += 1;
							strcat(c_list, c_info[temp].name);
							pos_idx += c_info[temp].len;
						}
					}
					send(c_socks[num], c_list, sizeof(c_list), 0);
				}
				else {
					char t_str[26];
					int idx = 0;
					for (; idx < 25; idx++)
						t_str[idx] = s_rec_buffer[idx];
					t_str[25] = '\0';
					int len = int(s_rec_buffer[idx]);
					idx += 1;
					char* obj_name = new char[len + 1];
					for (; idx < 26 + len; idx++)
						obj_name[idx - 26] = s_rec_buffer[idx];
					obj_name[len] = '\0';
					len = int(s_rec_buffer[idx]);
					idx += 1;
					int lastpos_idx = len + idx;
					int timerPost = idx - 1;
					char* sender_name = new char[len + 2];
					sender_name[0] = char(len);
					for (; idx < lastpos_idx; idx++)
						sender_name[idx - timerPost] = s_rec_buffer[idx];
					sender_name[len + 1] = '\0';

					char tempBuf[s_buffer_size];
					len = 0;
					char mess[s_buffer_size];
					while (s_rec_buffer[idx] != '\0') {
						mess[len] = s_rec_buffer[idx];
						len++;
						idx++;
					}
					mess[len] = '\0';
					memset(tempBuf, 0, s_buffer_size);
					strcat(tempBuf, t_str);
					strcat(tempBuf, sender_name);
					strcat(tempBuf, mess);

					if (isWorldChat(obj_name)) {
						cout << "Client\t" << sender_name + 1 << "\tsend world msg\t" << "At: " << t_str;
						int idx = 0;
						while (mess[idx] != '\0') {
							cout << mess[idx];
							idx++;
						}
						cout << endl;

						// 发送世界短信给所有在线客户端
						for (int j = 0; j < max_online; j++) {
							if (c_info[j].online) {
								// 构建要发送的消息
								char worldMsg[s_buffer_size];
								memset(worldMsg, 0, s_buffer_size);
								strcpy(worldMsg, t_str);
								strcat(worldMsg, sender_name);
								strcat(worldMsg, mess);
								// 发送消息
								send(c_socks[j], worldMsg, sizeof(worldMsg), 0);
							}
						}
					}
					else {

						int index = 0;
						for (; index < max_online; index++) {
							if (c_info[index].online) {
								if (cmpStr(c_info[index].name, obj_name)) {

									send(c_socks[c_info[index].num], tempBuf, sizeof(tempBuf), 0);
									break;
								}
							}
						}
						if (index == max_online)
						{
							char* temp = new char(strlen(sender_name));
							temp = sender_name + 1;
							for (int j = 0; j < max_online; j++)
							{
								if (c_info[j].online) {
									cout << j << " " << c_info[j].name << " " << c_info[j].num << endl;
									if (strcmp(c_info[j].name, temp) == 0)
									{
										cout << strcmp(c_info[j].name, temp) << endl;
										send(c_socks[c_info[j].num], "obj choose fail,name wrong", sizeof(tempBuf), 0);
										break;
									}
								}
							}

						}

					}
				}
			}
		}
	}
	return 0;
}

int serverStart() {
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		
		cout << " step 12 fail\n";
		system("pause");
		return -1;
	}
	s_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (s_sock == INVALID_SOCKET)
	{
		cout << "step 13 fail" << WSAGetLastError() << endl;
		return 0;
	}
	s_adr.sin_family = AF_INET;
	setSIP(s_ip);
	inet_pton(AF_INET, s_ip, &s_adr.sin_addr.S_un.S_addr);
	s_port = 8860;
	s_adr.sin_port = htons(s_port);
	bind(s_sock, (SOCKADDR*)&s_adr, sizeof(SOCKADDR));
	while (1) {
		if (cur_online_cnt == 0)
			cout << "Listening.\n";
		
		listen(s_sock, max_online);

		
		int i = 0;
		for (; i < max_online; i++)
			if (c_socks[i] == 0)
				break;
		c_socks[i] = accept(s_sock, (SOCKADDR*)&c_adrs[i], &sock_adr);
		
		if (c_socks[i] != INVALID_SOCKET) {
			c_info[i].num = i;
			
			cur_online_cnt += 1;
			cout << "current online clients' num: " << cur_online_cnt << endl;

			memset(s_rec_buffer, 0, s_buffer_size);
			recv(c_socks[i], s_rec_buffer, s_buffer_size, 0);
			if (isName(s_rec_buffer)) {
				c_info[i].len = int(s_rec_buffer[1]);
				int idxOfName = 2;
				while (s_rec_buffer[idxOfName] != '\0') {
					c_info[i].name[idxOfName - 2] = s_rec_buffer[idxOfName];
					idxOfName++;
				}
				c_info[i].name[int(s_rec_buffer[1]) + 1] = '\0';
				c_info[i].online = true;
				for (int j = 0; j < 5; j++)
					if (c_info[j].online == true)
						cout << "clients: " << c_info[j].name << endl;
			}

			time_t timer = time(0);

			char* first_p = ctime(&timer);
			memset(s_send_buffer, 0, s_buffer_size);
			strcat(s_send_buffer, first_p);
			strcat(s_send_buffer, " successfully enter chatroom");
			send(c_socks[i], s_send_buffer, s_buffer_size, 0);

			memset(s_send_buffer, 0, s_buffer_size);
			c_threads[i] = CreateThread(NULL, NULL, recMsg, &c_info[i], 0, NULL);
			if (c_threads[i] == 0) {
				cout << "step 22 fail\n";
				cur_online_cnt -= 1;
				return -1;
			}
		}
		else {
			cout << "step 33 fail.\n";
		}
	}

	closesocket(s_sock);
	WSACleanup();
	return 0;
}

