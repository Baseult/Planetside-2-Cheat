#pragma once
#define DEBUG 1

#include "pch.h"

extern DWORD WINAPI MainThread_Initialize(LPVOID dwModule);

namespace DX11Base
{
	using namespace std::chrono_literals;
	inline HMODULE g_hModule{};
	inline std::atomic_bool g_Running{};
	inline std::atomic_bool g_CleanupInProgress{};
}