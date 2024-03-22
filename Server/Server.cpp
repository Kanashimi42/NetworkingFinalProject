#include <iostream>
#include <string>
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

WSADATA socketData;
SOCKET clientConnection;
sockaddr_in serverAddress;
#define MAX_BUFFER_SIZE 256

int main() {
    system("title Client");

    WSAStartup(MAKEWORD(2, 2), &socketData);

    clientConnection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    serverAddress.sin_family = AF_INET;

    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);
    serverAddress.sin_port = htons(20000);
    connect(clientConnection, (SOCKADDR*)&serverAddress, sizeof(serverAddress));

    string userOrder;
    char serverResponse[MAX_BUFFER_SIZE];

    while (true) {
        getline(cin, userOrder);

        if (userOrder == "exit")
            break;

        send(clientConnection, userOrder.c_str(), userOrder.length(), 0);

        int responseSize = recv(clientConnection, serverResponse, MAX_BUFFER_SIZE - 1, 0);
        if (responseSize > 0) {
            serverResponse[responseSize] = '\0';
            cout << serverResponse << endl;
        }
        else {
            cout << "No response from server." << endl;
        }

        responseSize = recv(clientConnection, serverResponse, MAX_BUFFER_SIZE - 1, 0);
        if (responseSize > 0) {
            serverResponse[responseSize] = '\0';
            cout << serverResponse << endl;
        }
        else {
            cout << "No response from server." << endl;
        }

    }

    closesocket(clientConnection);
    WSACleanup();

    return 0;
}
