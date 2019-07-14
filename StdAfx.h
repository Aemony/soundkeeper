#pragma once

#define _WIN32_WINNT 0x0600

#include <new>
#include <windows.h>
#include <strsafe.h>
#include <objbase.h>

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

#ifdef NDEBUG
#define DebugLog(...)
#define DebugLogError(...)
#else
#define DebugLog(...) printf(__VA_ARGS__); printf("\n");
#define DebugLogError(...) printf("[ERROR] "); printf(__VA_ARGS__); printf("\n");
#endif
