#include "RespondingGuy.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <string>

#pragma comment(lib, "psapi.lib")

static HANDLE g_killerThread = NULL;
static bool g_isResponding = false;

HWND FindNotRespondingDialog() {
    HWND hWnd = FindWindow(NULL, L"Konata Dancer");
    if (hWnd) {
        wchar_t title[256];
        GetWindowText(hWnd, title, 256);
        if (wcsstr(title, L"Not Responding") || wcsstr(title, L"Не отвечает")) {
            return hWnd;
        }
    }

    hWnd = FindWindow(L"#32770", NULL);
    if (hWnd) {
        wchar_t title[256];
        GetWindowText(hWnd, title, 256);
        if (wcsstr(title, L"Not Responding") || wcsstr(title, L"Не отвечает")) {
            return hWnd;
        }
    }

    return NULL;
}

void AutoClickCloseButton() {
    for (int attempt = 0; attempt < 30; attempt++) {
        HWND hDialog = FindNotRespondingDialog();
        if (hDialog) {
            HWND hButton = FindWindowEx(hDialog, NULL, L"Button", L"Close the program");
            if (!hButton) hButton = FindWindowEx(hDialog, NULL, L"Button", L"Закрыть программу");
            if (!hButton) hButton = FindWindowEx(hDialog, NULL, L"Button", L"Close program");
            if (!hButton) hButton = FindWindowEx(hDialog, NULL, L"Button", L"Close");
            if (!hButton) hButton = FindWindowEx(hDialog, NULL, L"Button", L"Закрыть");

            if (hButton) {
                SetForegroundWindow(hDialog);
                SendMessage(hButton, BM_CLICK, 0, 0);
                return;
            }
        }
        Sleep(500);
    }
}

DWORD WINAPI KillerThreadProc(LPVOID lpParam) {
    Sleep(500);
    AutoClickCloseButton();
    Sleep(1000);
    TerminateProcess(GetCurrentProcess(), 0);
    return 0;
}

void DontRespond(HWND hwnd) {
    if (g_isResponding) return;
    g_isResponding = true;

    if (hwnd && IsWindow(hwnd)) {
        ShowWindow(hwnd, SW_SHOW);
        SetForegroundWindow(hwnd);
        BringWindowToTop(hwnd);
        SetWindowText(hwnd, L"Konata Dancer (Not Responding)");
        SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_WAIT));
        UpdateWindow(hwnd);
        RedrawWindow(hwnd, NULL, NULL, RDW_UPDATENOW);
        KillTimer(hwnd, 1);
    }

    g_killerThread = CreateThread(NULL, 0, KillerThreadProc, NULL, 0, NULL);
    while (true) {
        Sleep(INFINITE);
    }
}

void SelfDestruct() {
    TerminateProcess(GetCurrentProcess(), 0);
}

void CleanupRespondingGuy() {
    if (g_killerThread) {
        CloseHandle(g_killerThread);
        g_killerThread = NULL;
    }
    g_isResponding = false;
}