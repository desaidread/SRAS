#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <windows.h>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

class RemoteClient {
private:
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    bool isConnected;
    std::string serverIP;
    int serverPort;
    
public:
    RemoteClient() : isConnected(false), clientSocket(INVALID_SOCKET) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
    
    ~RemoteClient() {
        disconnect();
        WSACleanup();
    }
    
    bool connectToServer(const std::string& ip, int port) {
        serverIP = ip;
        serverPort = port;
        
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed" << std::endl;
            return false;
        }
        
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
            std::cerr << "Invalid IP address" << std::endl;
            return false;
        }
        
        // Устанавливаем таймаут подключения
        int timeout = 10000; // 10 секунд
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
        
        std::cout << "Connecting to " << ip << ":" << port << "..." << std::endl;
        
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Connection failed. Error: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
            return false;
        }
        
        isConnected = true;
        std::cout << "Connected successfully!" << std::endl;
        return true;
    }
    
    void disconnect() {
        if (isConnected && clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
            isConnected = false;
            std::cout << "Disconnected from server." << std::endl;
        }
    }
    
    bool reconnect() {
        disconnect();
        Sleep(2000); // Ждем 2 секунды перед переподключением
        return connectToServer(serverIP, serverPort);
    }
    
    void startInteractiveSession() {
        if (!isConnected) {
            std::cerr << "Not connected to server" << std::endl;
            return;
        }
        
        // Получаем приветственное сообщение
        char buffer[8192];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << buffer;
        }
        
        std::cout << "\n=== Remote Access Client ===" << std::endl;
        std::cout << "Type 'help' for available commands, 'exit' to quit" << std::endl;
        std::cout << "Connected to: " << serverIP << ":" << serverPort << std::endl;
        std::cout << "=============================\n" << std::endl;
        
        std::string command;
        while (isConnected) {
            std::cout << "remote> ";
            std::getline(std::cin, command);
            
            if (command.empty()) {
                continue;
            }
            
            // Локальные команды клиента
            if (command == "help") {
                showHelp();
                continue;
            }
            
            if (command == "clear" || command == "cls") {
                system("cls");
                continue;
            }
            
            if (command == "reconnect") {
                std::cout << "Attempting to reconnect..." << std::endl;
                if (reconnect()) {
                    std::cout << "Reconnected successfully!" << std::endl;
                } else {
                    std::cout << "Reconnection failed." << std::endl;
                    break;
                }
                continue;
            }
            
            if (command == "exit" || command == "quit") {
                sendCommand("exit");
                break;
            }
            
            // Отправляем команду на сервер
            if (!sendCommand(command)) {
                std::cout << "Connection lost. Attempting to reconnect..." << std::endl;
                if (reconnect()) {
                    std::cout << "Reconnected! Retrying command..." << std::endl;
                    sendCommand(command);
                } else {
                    std::cout << "Failed to reconnect. Exiting..." << std::endl;
                    break;
                }
            }
        }
    }
    
    bool sendCommand(const std::string& command) {
        if (!isConnected || clientSocket == INVALID_SOCKET) {
            return false;
        }
        
        // Отправляем команду
        if (send(clientSocket, command.c_str(), command.length(), 0) == SOCKET_ERROR) {
            isConnected = false;
            return false;
        }
        
        // Получаем ответ
        char buffer[8192];
        int totalReceived = 0;
        std::string response;
        
        // Читаем ответ порциями
        while (true) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesReceived <= 0) {
                if (bytesReceived == 0) {
                    // Сервер закрыл соединение
                    isConnected = false;
                }
                return false;
            }
            
            buffer[bytesReceived] = '\0';
            response += std::string(buffer);
            totalReceived += bytesReceived;
            
            // Проверяем, есть ли промпт в конце (признак окончания ответа)
            if (response.find("]> ") != std::string::npos) {
                break;
            }
            
            // Защита от бесконечного чтения
            if (totalReceived > 1024 * 1024) { // 1MB лимит
                break;
            }
        }
        
        // Выводим ответ (убираем промпт)
        size_t promptPos = response.find_last_of("]");
        if (promptPos != std::string::npos) {
            std::cout << response.substr(0, promptPos - response.substr(0, promptPos).find_last_of("[")) << std::endl;
        } else {
            std::cout << response << std::endl;
        }
        
        return true;
    }
    
    void showHelp() {
        std::cout << "\n=== Available Commands ===" << std::endl;
        std::cout << "Local commands:" << std::endl;
        std::cout << "  help        - Show this help" << std::endl;
        std::cout << "  clear/cls   - Clear screen" << std::endl;
        std::cout << "  reconnect   - Reconnect to server" << std::endl;
        std::cout << "  exit/quit   - Exit client" << std::endl;
        std::cout << "\nRemote commands:" << std::endl;
        std::cout << "  cd <path>   - Change directory" << std::endl;
        std::cout << "  pwd         - Show current directory" << std::endl;
        std::cout << "  dir/ls      - List directory contents" << std::endl;
        std::cout << "  sysinfo     - Show system information" << std::endl;
        std::cout << "  netstat     - Show network connections" << std::endl;
        std::cout << "  tasklist    - Show running processes" << std::endl;
        std::cout << "  download <file> - Download file content" << std::endl;
        std::cout << "  <any_cmd>   - Execute any system command" << std::endl;
        std::cout << "========================\n" << std::endl;
    }
    
    // Функция для сканирования портов
    void portScan(const std::string& targetIP, int startPort, int endPort) {
        std::cout << "Scanning " << targetIP << " ports " << startPort << "-" << endPort << "..." << std::endl;
        
        for (int port = startPort; port <= endPort; port++) {
            SOCKET testSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (testSocket == INVALID_SOCKET) continue;
            
            struct sockaddr_in testAddr;
            testAddr.sin_family = AF_INET;
            testAddr.sin_port = htons(port);
            inet_pton(AF_INET, targetIP.c_str(), &testAddr.sin_addr);
            
            // Устанавливаем короткий таймаут
            int timeout = 1000; // 1 секунда
            setsockopt(testSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
            setsockopt(testSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
            
            if (connect(testSocket, (struct sockaddr*)&testAddr, sizeof(testAddr)) == 0) {
                std::cout << "Port " << port << " is open" << std::endl;
            }
            
            closesocket(testSocket);
        }
        
        std::cout << "Port scan completed." << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::string serverIP = "127.0.0.1"; // По умолчанию локальный
    int serverPort = 4444;
    
    // Парсинг аргументов командной строки
    if (argc >= 2) {
        serverIP = argv[1];
    }
    if (argc >= 3) {
        serverPort = atoi(argv[2]);
    }
    
    std::cout << "=== Remote Access Client ===" << std::endl;
    std::cout << "Target: " << serverIP << ":" << serverPort << std::endl;
    
    // Специальный режим сканирования портов
    if (argc >= 4 && std::string(argv[3]) == "scan") {
        RemoteClient client;
        int startPort = (argc >= 5) ? atoi(argv[4]) : 1;
        int endPort = (argc >= 6) ? atoi(argv[5]) : 1000;
        client.portScan(serverIP, startPort, endPort);
        return 0;
    }
    
    RemoteClient client;
    
    // Попытка подключения с повторами
    int attempts = 0;
    const int maxAttempts = 5;
    
    while (attempts < maxAttempts) {
        if (client.connectToServer(serverIP, serverPort)) {
            client.startInteractiveSession();
            break;
        }
        
        attempts++;
        if (attempts < maxAttempts) {
            std::cout << "Retrying in 3 seconds... (" << attempts << "/" << maxAttempts << ")" << std::endl;
            Sleep(3000);
        }
    }
    
    if (attempts >= maxAttempts) {
        std::cout << "Failed to connect after " << maxAttempts << " attempts." << std::endl;
        std::cout << "\nUsage examples:" << std::endl;
        std::cout << "  " << argv[0] << " <IP> <PORT>                    - Connect to remote server" << std::endl;
        std::cout << "  " << argv[0] << " 192.168.1.100 4444             - Connect to specific IP" << std::endl;
        std::cout << "  " << argv[0] << " <IP> <PORT> scan <start> <end>  - Port scan" << std::endl;
        std::cout << "  " << argv[0] << " 192.168.1.100 0 scan 1 1000     - Scan ports 1-1000" << std::endl;
    }
    
    return 0;
}