#include"client.h"
#include"server.h"
#include <iostream>

int main() {
	cout << " input c to start a client\tinput s to start a server\n";
	char inp;
	cin >> inp;
	cin.get();
	if (inp == 's')
		serverStart();
	else
		if (inp == 'c')
			clientStart();
	return 0;
}

