#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include "Common/BasicMacros.hpp"
#include "Common/Defer.hpp"
#include "Common/NtBase.hpp"
#include "Common/NtHandle.hpp"
#include "Common/NtEvent.hpp"
#include "Common/NtCriticalSection.hpp"
#include "Common/NtUtils.hpp"
#include "Common/StrUtils.hpp"
#include <algorithm> // std::min and std::max.
#include <math.h>

#ifdef _CONSOLE

inline void DebugLogImpl(const char * funcname, const char * type, const char * format, ...)
{
	static CriticalSection mutex;
	ScopedLock lock(mutex);

	static bool is_inited = false;
	static bool show_trace = false;

	if (!is_inited)
	{
		// It also looks in path to exe, but it's fine for a debug build.
		char buf[MAX_PATH];
		strcpy_s(buf, GetCommandLineA());
		_strlwr(buf);
		if (StringContains<AsciiToLower>(buf, "trace")) { show_trace = true; }
		is_inited = true;
	}

	if (!show_trace && type && StringEquals<AsciiToLower>(type, "TRACE")) { return; }

	static uint64_t prev_date = 0;
	SYSTEMTIME now = {0};
	GetSystemTime(&now);

	// Output current date once. First 8 bytes are current date, so we can compare it as a 64-bit integer.
	if (prev_date != *((uint64_t*)&now))
	{
		prev_date = *((uint64_t*)&now);
		printf("%04d/%02d/%02d ", now.wYear, now.wMonth, now.wDay);
	}

	printf("%02d:%02d:%02d.%03d ", now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
	printf("[%5d] ", GetThreadId(GetCurrentThread()));

	if (show_trace && funcname)
	{
		printf("[%s] ", funcname);
	}

	if (type && !StringEquals<AsciiToLower>(type, "TRACE") && !StringEquals<AsciiToLower>(type, "INFO"))
	{
		printf("[%s] ", type);
	}

	va_list argptr;
	va_start(argptr, format);
	vprintf(format, argptr);

	printf("\n");
	fflush(stdout);
}

#define DebugLog(...) DebugLogImpl(__FUNCTION__, "INFO", __VA_ARGS__)
#define DebugLogWarning(...) DebugLogImpl(__FUNCTION__, "WARNING", __VA_ARGS__)
#define DebugLogError(...) DebugLogImpl(__FUNCTION__, "ERROR", __VA_ARGS__)
#define TraceLog(...) DebugLogImpl(__FUNCTION__, "TRACE", __VA_ARGS__)
#define DebugThreadName(...) SetCurrentThreadDescriptionW(L ## __VA_ARGS__)

#else

#define DebugLog(...)
#define DebugLogWarning(...)
#define DebugLogError(...)
#define TraceLog(...)
#define DebugThreadName(...)

#endif

// Windows API stuff to enable EcoQoS (Efficiency Mode)
#define WIN32_LEAN_AND_MEAN
#include <wtypes.h>
#include <stdlib.h>

// Returns a pseudo handle interpreted as the current process handle
static HANDLE
SKIF_Util_GetCurrentProcess (void)
{
  // A pseudo handle is a special constant, currently (HANDLE)-1, that is interpreted as the current process handle.
  // For compatibility with future operating systems, it is best to call GetCurrentProcess instead of hard-coding this constant value.
  static HANDLE
         handle = GetCurrentProcess ( );
  return handle;
}

static BOOL
WINAPI
SKIF_Util_SetProcessInformation (HANDLE hProcess, PROCESS_INFORMATION_CLASS ProcessInformationClass, LPVOID ProcessInformation, DWORD ProcessInformationSize)
{
  // SetProcessInformation (Windows 8+)
  using SetProcessInformation_pfn =
    BOOL (WINAPI *)(HANDLE, PROCESS_INFORMATION_CLASS, LPVOID, DWORD);

  static SetProcessInformation_pfn
    SKIF_SetProcessInformation =
        (SetProcessInformation_pfn)GetProcAddress (LoadLibraryEx (L"kernel32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32),
        "SetProcessInformation");

  if (SKIF_SetProcessInformation == nullptr)
    return FALSE;

  return SKIF_SetProcessInformation (hProcess, ProcessInformationClass, ProcessInformation, ProcessInformationSize);
}

// Sets the power throttling execution speed (EcoQoS) of a process
//   1 = enable; 0 = disable; -1 = auto-managed
static BOOL
SKIF_Util_SetProcessPowerThrottling (HANDLE processHandle, INT state)
{
  PROCESS_POWER_THROTTLING_STATE throttlingState;
  ZeroMemory(&throttlingState, sizeof(throttlingState));

  throttlingState.Version     =                PROCESS_POWER_THROTTLING_CURRENT_VERSION;
  throttlingState.ControlMask = (state > -1) ? PROCESS_POWER_THROTTLING_EXECUTION_SPEED : 0;
  throttlingState.StateMask   = (state == 1) ? PROCESS_POWER_THROTTLING_EXECUTION_SPEED : 0;

  return SKIF_Util_SetProcessInformation (processHandle, ProcessPowerThrottling, &throttlingState, sizeof(throttlingState));
}

static void
SKIF_Util_EngageEffiencyMode (void)
{
	// Enable Efficiency Mode in Windows 11 (requires idle (low) priority + EcoQoS)
	SKIF_Util_SetProcessPowerThrottling (SKIF_Util_GetCurrentProcess ( ), 1);
	SetPriorityClass  (SKIF_Util_GetCurrentProcess ( ), IDLE_PRIORITY_CLASS);

	// Begin background processing mode
	SetThreadPriority (GetCurrentThread ( ), THREAD_MODE_BACKGROUND_BEGIN);
}
