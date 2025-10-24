#pragma once
#include <Windows.h>
#include <mutex>
#include <cstdio>

class Logger {
public:
    static void Init();      // AllocConsole, redirect stdout
    static void Shutdown();  // FreeConsole
    static void Log(const char* format, ...);   // printf-style, thread-safe with mutex
    static void Error(const char* format, ...);
    static void Warning(const char* format, ...);
    
private:
    static std::mutex m_mutex;
    static FILE* m_console;
    static bool m_initialized;
    
    // NEW: Separate into locked and unlocked function
    static void PrintWithPrefix(const char* prefix, const char* format, va_list args); // This will now contain the lock
    static void PrintUnsafe(const char* prefix, const char* format, va_list args);    // This only does the printf
};
