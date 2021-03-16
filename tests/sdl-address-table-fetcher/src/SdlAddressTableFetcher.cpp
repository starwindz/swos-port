#include "SdlAddressTableFetcher.h"
#include <Windows.h>

static void **m_table;
static uint32_t m_tableSize;
static HHOOK m_hook;

LRESULT CALLBACK hookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HCBT_CREATEWND) {
        auto windowParams = reinterpret_cast<CBT_CREATEWND *>(lParam);
        auto name = windowParams->lpcs->lpszName;

        int skipWindow = 0;
        if (strstr(name, "SDL") && strstr(name, "ailure"))
            skipWindow = 1;

        UnhookWindowsHookEx(m_hook);
        return skipWindow;
    }

    return 0;
}

static void setupHook()
{
    m_hook = ::SetWindowsHookExA(WH_CBT, hookProc, 0, ::GetCurrentThreadId());
    if (!m_hook)
        ::MessageBoxA(NULL, "Failed to set hook!", "Horrible Error", MB_OK | MB_ICONERROR);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        setupHook();
        _putenv("SDL_DYNAMIC_API=.\\\\" SDL_ADDRESS_FETCHER_DLL);
    }

    return TRUE;
}

int32_t SDL_DYNAPI_entry(uint32_t apiVer, void *table, uint32_t tableSize)
{
    m_table = reinterpret_cast<void **>(table);
    m_tableSize = tableSize / sizeof(void *);
    return -1;  // return failure so that SDL fills the table internally
}

void **GetFunctionTable(uint32_t *tableSize)
{
    if (tableSize)
        *tableSize = m_tableSize;

    return m_table;
}
