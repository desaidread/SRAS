@echo off
echo ========================================
echo    Remote Access Tool - Build Script
echo ========================================
echo.

:: Check if MinGW is available
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: g++ compiler not found!
    echo Please install MinGW-w64 or add it to PATH
    pause
    exit /b 1
)

echo Compiler found: g++
echo.

:: Create output directory
if not exist "bin" mkdir bin

:: Build basic versions
echo Building basic server...
g++ -std=c++11 -Wall -O2 -o bin\server.exe server.cpp -lws2_32
if %errorlevel% neq 0 (
    echo ERROR: Failed to build server
    pause
    exit /b 1
)
echo ✓ Basic server built successfully

echo Building basic client...
g++ -std=c++11 -Wall -O2 -o bin\client.exe client.cpp -lws2_32
if %errorlevel% neq 0 (
    echo ERROR: Failed to build client
    pause
    exit /b 1
)
echo ✓ Basic client built successfully

:: Build advanced versions
echo Building advanced server...
g++ -std=c++11 -Wall -O2 -o bin\advanced_server.exe advanced_server.cpp -lws2_32 -lpsapi
if %errorlevel% neq 0 (
    echo ERROR: Failed to build advanced server
    pause
    exit /b 1
)
echo ✓ Advanced server built successfully

echo Building advanced client...
g++ -std=c++11 -Wall -O2 -o bin\advanced_client.exe advanced_client.cpp -lws2_32
if %errorlevel% neq 0 (
    echo ERROR: Failed to build advanced client
    pause
    exit /b 1
)
echo ✓ Advanced client built successfully

:: Build stealth versions
echo Building stealth server...
g++ -std=c++11 -Wall -O2 -DSTEALTH_MODE -mwindows -o bin\server_stealth.exe server.cpp -lws2_32
if %errorlevel% neq 0 (
    echo ERROR: Failed to build stealth server
    pause
    exit /b 1
)
echo ✓ Stealth server built successfully

echo Building advanced stealth server...
g++ -std=c++11 -Wall -O2 -DSTEALTH_MODE -mwindows -o bin\advanced_server_stealth.exe advanced_server.cpp -lws2_32 -lpsapi
if %errorlevel% neq 0 (
    echo ERROR: Failed to build advanced stealth server
    pause
    exit /b 1
)
echo ✓ Advanced stealth server built successfully

echo.
echo ========================================
echo           BUILD COMPLETED!
echo ========================================
echo.
echo Built files in 'bin' directory:
echo   - server.exe              (Basic server)
echo   - client.exe              (Basic client)
echo   - advanced_server.exe     (Advanced server)
echo   - advanced_client.exe     (Advanced client)
echo   - server_stealth.exe      (Stealth server)
echo   - advanced_server_stealth.exe (Advanced stealth server)
echo.
echo Usage examples:
echo   Start server: bin\server.exe
echo   Connect client: bin\client.exe [IP] [PORT]
echo   Advanced mode: bin\advanced_server.exe
echo   Stealth mode: bin\server_stealth.exe
echo.
echo WARNING: Use only for educational purposes!
echo.
pause