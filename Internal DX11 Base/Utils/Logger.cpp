#include "pch.h"
#include "Logger.h"
#include <iostream>
#include <cstdarg>

std::mutex Logger::m_mutex;
FILE* Logger::m_console = nullptr;
bool Logger::m_initialized = false;

void Logger::Init() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) return;
    
    // Create console
    AllocConsole();
    SetConsoleTitleA("PS2 Cheat - Debug Console");
    
    // Redirect stdout
    freopen_s(&m_console, "CONOUT$", "w", stdout);
    
    // Make console window a bit larger
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow) {
        RECT rect;
        GetWindowRect(consoleWindow, &rect);
        SetWindowPos(consoleWindow, HWND_TOP, rect.left, rect.top, 800, 600, SWP_NOZORDER);
    }
    
    m_initialized = true;
    
    // Direct, simpler call without vaPrintf detour (avoids deadlock!)
    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("[%02d:%02d:%02d.%03d] [INFO] Logger initialized successfully!\n", 
           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    fflush(stdout);
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) return;
    
    // Log shutdown message before file is closed
    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("[%02d:%02d:%02d.%03d] [INFO] Logger shutting down.\n", 
           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    fflush(stdout);
    
    if (m_console) {
        fclose(m_console);
        m_console = nullptr;
    }
    
    FreeConsole();
    m_initialized = false;
}

void Logger::Log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    PrintWithPrefix("[INFO] ", format, args);
    va_end(args);
}

void Logger::Error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    PrintWithPrefix("[ERROR] ", format, args);
    va_end(args);
}

void Logger::Warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    PrintWithPrefix("[WARNING] ", format, args);
    va_end(args);
}

// NEW UNSAFE FUNCTION (without mutex!)
void Logger::PrintUnsafe(const char* prefix, const char* format, va_list args) {
    if (!m_initialized) return;
    
    // Timestamp
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    printf("[%02d:%02d:%02d.%03d] %s", 
           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, prefix);
    
    vprintf(format, args);
    printf("\n");
    fflush(stdout);
}

// ADAPTED PrintWithPrefix (now just a wrapper with lock)
void Logger::PrintWithPrefix(const char* prefix, const char* format, va_list args) {
    std::lock_guard<std::mutex> lock(m_mutex);
    PrintUnsafe(prefix, format, args);
}
