#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

class RemoteAccessClient {
private:
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    bool isConnected;
    
public:
    RemoteAccessClient() : isConnected(false) {
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
    
    ~RemoteAccessClient() {
        cleanup();
        WSACleanup();
    }
    
    bool connectToServer(const std::string& serverIP, int port) {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed" << std::endl;
            return false;
        }
        
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
            std::cerr << "Invalid address" << std::endl;
            return false;
        }
        
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Connection failed" << std::endl;
            return false;
        }
        
        isConnected = true;
        std::cout << "Connected to server " << serverIP << ":" << port << std::endl;
        return true;
    }
    
    void startInteractiveSession() {
        if (!isConnected) {
            std::cerr << "Not connected to server" << std::endl;
            return;
        }
        
        std::string command;
        char buffer[4096];
        
        std::cout << "Remote Access Client - Type 'exit' to quit" << std::endl;
        std::cout << "Remote Shell> ";
        
        while (std::getline(std::cin, command)) {
            if (command.empty()) {
                std::cout << "Remote Shell> ";
                continue;
            }
            
            // Send command to server
            if (send(clientSocket, command.c_str(), command.length(), 0) == SOCKET_ERROR) {
                std::cerr << "Send failed" << std::endl;
                break;
            }
            
            if (command == "exit") {
                break;
            }
            
            // Receive response from server
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0) {
                std::cerr << "Connection lost" << std::endl;
                break;
            }
            
            buffer[bytesReceived] = '\0';
            std::cout << buffer;
            
            if (buffer[bytesReceived - 1] != '\n') {
                std::cout << std::endl;
            }
            
            std::cout << "Remote Shell> ";
        }
    }
    
    bool sendCommand(const std::string& command) {
        if (!isConnected) {
            return false;
        }
        
        if (send(clientSocket, command.c_str(), command.length(), 0) == SOCKET_ERROR) {
            return false;
        }
        
        return true;
    }
    
    std::string receiveResponse() {
        if (!isConnected) {
            return "";
        }
        
        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived <= 0) {
            return "";
        }
        
        buffer[bytesReceived] = '\0';
        return std::string(buffer);
    }
    
    void cleanup() {
        isConnected = false;
        if (clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
        }
    }
    
private:
    WSADATA wsaData;
};

int main(int argc, char* argv[]) {
    std::string serverIP = "127.0.0.1"; // Default to localhost
    int port = 4444; // Default port
    
    // Parse command line arguments
    if (argc >= 2) {
        serverIP = argv[1];
    }
    if (argc >= 3) {
        port = std::stoi(argv[2]);
    }
    
    RemoteAccessClient client;
    
    std::cout << "Attempting to connect to " << serverIP << ":" << port << std::endl;
    
    if (!client.connectToServer(serverIP, port)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }
    
    client.startInteractiveSession();
    
    std::cout << "Connection closed" << std::endl;
    return 0;
}