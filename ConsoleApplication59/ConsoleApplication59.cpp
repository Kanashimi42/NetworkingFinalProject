#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
float totalOrderCost = 0;
int totalOrderTime = 0;

using namespace std;

struct FoodItem {
    string dishName, dishType;
    float dishPrice;
};

#define MAX_BUFFER_LEN 256

string dataFileName = "menu.txt";
FoodItem* menuList;
int totalWaitingTime = 0;
int totalItems = 0;
vector<string> orderHistory;

WSADATA socketData;
SOCKET serverSocket, clientConnection;
sockaddr_in address;
string toLowercase(string str) {
    transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return tolower(c); });
    return str;
}

pair<string, float> calculateWaitTimeResponse(string order) {
    float orderCost = 0;
    int orderTime = 0;
    order = toLowercase(order);
    istringstream iss(order);
    string word;
    bool foundInMenu = false;
    while (iss >> word) {
        for (int i = 0; i < totalItems; i++) {
            string menuDishNameLower = toLowercase(menuList[i].dishName);
            if (menuDishNameLower.find(word) != string::npos) {
                foundInMenu = true;
                if (menuList[i].dishType == "MainCourse")
                    orderTime += 5;
                else if (menuList[i].dishType == "Appetizer")
                    orderTime += 4;
                else if (menuList[i].dishType == "Dessert")
                    orderTime += 3;
                else if (menuList[i].dishType == "Drink")
                    orderTime += 2;
                orderCost += menuList[i].dishPrice;
            }
        }
    }

    string response;
    if (!foundInMenu) {
        response = "We don't have these items in our menu.";
    }
    else {
        if (orderTime != 0) {
            response = "Please wait for " + to_string(orderTime) + " seconds";
        }
        else {
            response = "We don't have these items in our menu.";
        }
    }
    return make_pair(response, orderCost);
}


void processOrder(string order) {
    // Обработка заказа
    pair<string, float> response = calculateWaitTimeResponse(order);
    send(clientConnection, response.first.c_str(), response.first.length(), 0);
}

void receiveResponse() {
    char clientRequest[MAX_BUFFER_LEN];
    int requestSize = recv(clientConnection, clientRequest, MAX_BUFFER_LEN - 1, 0);
    if (requestSize > 0) {
        clientRequest[requestSize] = '\0';
        cout << "Received order: '" << clientRequest << "'" << endl;
        processOrder(clientRequest);
    }
    else {
        cout << "No request from client." << endl;
    }
}
string generateAllItemsResponse(string order) {
    string response;
    string orderLower = toLowercase(order);
    for (int i = 0; i < totalItems; i++) {
        string menuDishNameLower = toLowercase(menuList[i].dishName);
        if (menuDishNameLower.find(orderLower) != string::npos) {
            ostringstream priceStream;
            priceStream << fixed << setprecision(2) << menuList[i].dishPrice;
            response += menuList[i].dishName + "-" + priceStream.str() + "\n";
        }
    }
    if (response.empty()) {
        response = "We don't have these items in our menu.";
    }
    return response;
}

string getMenuInformation() {
    string menuInfo;
    for (int i = 0; i < totalItems; ++i) {
        menuInfo += menuList[i].dishName + " - " + menuList[i].dishType + " - $" + to_string(menuList[i].dishPrice) + "\n";
    }
    return menuInfo;
}

void terminateProgram(SOCKET connection) {
    cout << "Order History:\n";
    for (const auto& order : orderHistory) {
        cout << order << endl;
    }

    closesocket(connection);
}


DWORD WINAPI waitForExitCommand(LPVOID lpParam) {
    SOCKET serverSocket = (SOCKET)lpParam;
    string exitCommand;
    while (true) {
        cin >> exitCommand;
        if (exitCommand == "exit") {
            for (SOCKET connection = 0; connection <= serverSocket; ++connection) {
                terminateProgram(connection);
            }
            exit(0);
        }
    }
    return 0;
}

int main() {
    ifstream file(dataFileName);
    string jsonData((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    size_t position = 0;
    while ((position = jsonData.find("dishName", position)) != string::npos) {
        totalItems++;
        position += 8;
    }

    menuList = new FoodItem[totalItems];
    position = 0;
    int index = 0;

    while ((position = jsonData.find("dishName", position)) != string::npos) {
        size_t nameEnd = jsonData.find(",", position + 9);
        menuList[index].dishName = jsonData.substr(position + 9, nameEnd - position - 10);

        size_t typeStart = jsonData.find("dishType", nameEnd);
        size_t typeEnd = jsonData.find(",", typeStart + 10);
        menuList[index].dishType = jsonData.substr(typeStart + 10, typeEnd - typeStart - 11);

        size_t priceStart = jsonData.find("dishPrice", typeEnd);
        size_t priceEnd = jsonData.find(",", priceStart + 11);
        menuList[index++].dishPrice = stof(jsonData.substr(priceStart + 11, priceEnd - priceStart - 11));
        position = priceEnd;
    }

    WSAStartup(MAKEWORD(2, 2), &socketData);

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    address.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &address.sin_addr);
    address.sin_port = htons(20000);

    bind(serverSocket, (SOCKADDR*)&address, sizeof(address));
    listen(serverSocket, SOMAXCONN);

    fd_set readSet;
    int maxDescriptor, result;

    FD_ZERO(&readSet);
    FD_SET(serverSocket, &readSet);
    maxDescriptor = serverSocket;

    CreateThread(0, NULL, waitForExitCommand, NULL, 0, 0);

    while (true) {
        fd_set tempSet = readSet;
        result = select(maxDescriptor + 1, &tempSet, NULL, NULL, NULL);
        if (result < 0) {
            cout << "Error in select\n";
            break;
        }
        else if (result == 0) {
            cout << "Timeout\n";
            continue;
        }

        for (SOCKET connection = 1; connection <= maxDescriptor; ++connection) {
            if (FD_ISSET(connection, &tempSet)) {
                if (connection == serverSocket) {
                    clientConnection = accept(serverSocket, NULL, NULL);
                    FD_SET(clientConnection, &readSet);
                    maxDescriptor = max(maxDescriptor, clientConnection);
                    cout << "New client connected" << endl;
                }
                else {
                    totalWaitingTime = 0;
                    char buffer[MAX_BUFFER_LEN];
                    result = recv(connection, buffer, MAX_BUFFER_LEN, 0);
                    if (result <= 0) {
                        cout << "Client disconnected" << endl;
                        closesocket(connection);
                        FD_CLR(connection, &readSet);
                    }
                    else {
                        buffer[result] = '\0';
                        string order(buffer);
                        cout << "Received order: '" << order << "'" << endl;
                        if (order != "Client is waiting") {
                            orderHistory.push_back(order);
                        }
                        if (order == "Confirm") {
                            pair<string, float> waitTimeAndCost = calculateWaitTimeResponse(order);
                            string waitTimeResponse = waitTimeAndCost.first;
                            totalOrderCost = waitTimeAndCost.second;

                            totalOrderTime = totalOrderTime;

                            string timeWaitMessage = "Order confirmed. Estimated wait time: " + waitTimeResponse;
                            send(connection, timeWaitMessage.c_str(), timeWaitMessage.length(), 0);
                            Sleep(totalOrderTime * 1000);
                            totalOrderTime = 0;
                            totalOrderCost = 0;
                        }


                        else if (order == "getMenu") {
                            string menuInfo = getMenuInformation();
                            send(connection, menuInfo.c_str(), menuInfo.length(), 0);
                        }
                        else {
                            string waitTimeResponse = calculateWaitTimeResponse(order).first;
                            send(connection, waitTimeResponse.c_str(), waitTimeResponse.length(), 0);
                            Sleep(totalWaitingTime * 1000);
                            string allItemsResponse = generateAllItemsResponse(order);
                            send(connection, allItemsResponse.c_str(), allItemsResponse.length(), 0);
                        }
                    }
                }
            }
        }
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
