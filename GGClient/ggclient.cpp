#include <opencv2\core\core.hpp>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include "ggclient.h"
#include <iostream>

using namespace std;

const int CLIENT_PORT = 12345; // Connection port.
const char READY      = '1';   // Ready flag character.

SOCKET client;
string ip;

/* Initialize client socket.
Pop up error message and return false if an error occurs.
*/
bool init_socket() {
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(iResult != NO_ERROR) {
		MessageBox(0, "WSAStartup function failed", "Error", MB_OK);
		return false;
	}

	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(client == INVALID_SOCKET) {
		MessageBox(0, "socket function failed", "Error", MB_OK);
		WSACleanup();
		return false;
	}
	return true;
}

/* Connect to server.
Pop up error message and return false if an error occurs.
*/
bool connect_server() {
	// Input ip address if IP is not set in `config.ini`.
	if(ip.empty())  {
		cout << "Please input server IP address: ";
		getline(cin, ip);
	}

	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(ip.c_str());
	clientService.sin_port = htons(CLIENT_PORT);

	// Connect to server.
	int iResult = connect(client, (SOCKADDR *)&clientService, sizeof(clientService));
	if(iResult == SOCKET_ERROR) {
		MessageBox(0, "connect function failed", "Error", MB_OK);
		iResult = closesocket(client);
		if(iResult == SOCKET_ERROR)
			MessageBox(0, "closesocket function failed", "Error", MB_OK);
		WSACleanup();
		return false;
	}
	return true;
}

/* Send a number (4 bytes int) to server.
Return false if an error occurs (e.g. connection is broken).
*/
bool send_int(const int &num) {
	if(send(client, (char*)&num, 4, 0) != 4)
		return false;
	else
		return true;
}

/* Close client socket.
*/
bool close_socket() {
	int iResult = closesocket(client);
	if(iResult == SOCKET_ERROR) {
		WSACleanup();
		return false;
	}

	WSACleanup();
	return true;
}

/* Check if server is realy to receive image.
Return true if server is ready.
*/
bool server_ready() {
	char c = '0';
	recv(client, &c, 1, 0);
	if(c == READY)
		return true;
	else
		return false;
}

/* Send image data to server.
Send the length of data first, then send image data.
Return false if not successful.
*/
bool send_img(const char* img_data, int length) {
	send_int(length);
	int sent = 0;
	while(length > 0) {
		int result = send(client, img_data + sent, length, 0);
		if(result == SOCKET_ERROR)
			return false;
		length -= result;
	}
	return true;
}

/* Send src points found in template to server.
Return false if not successful.
*/
bool send_src_points(vector<int> &src_points) {
	// Send src points to server.
	for(const auto &p : src_points) {
		if(!send_int(p))
			return false;
	}
	return true;
}