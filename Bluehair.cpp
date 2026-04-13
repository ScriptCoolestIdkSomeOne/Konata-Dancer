#include "Bluehair.h"
#include <vector>

#ifndef COLOR_LISTBOXTEXT
#define COLOR_LISTBOXTEXT 24
#endif

HHOOK g_hook = NULL;
HHOOK g_msgHook = NULL;
std::vector<HWND> g_hookedWindows;

void textinshit() {
    int textElements[] = {
        COLOR_WINDOWTEXT,
        COLOR_MENUTEXT,
        COLOR_CAPTIONTEXT,
        COLOR_INACTIVECAPTIONTEXT,
        COLOR_BTNTEXT,
        COLOR_INFOTEXT,
        COLOR_HIGHLIGHTTEXT,
        COLOR_GRAYTEXT,
        COLOR_HOTLIGHT,
        COLOR_LISTBOXTEXT
    };

    COLORREF blueColors[10];
    for (int i = 0; i < 10; i++) {
        blueColors[i] = RGB(75, 200, 244);
    }

    SetSysColors(10, textElements, blueColors);

    SendMessageTimeout(HWND_BROADCAST, WM_SYSCOLORCHANGE,
        0, 0, SMTO_ABORTIFHUNG, 5000, NULL);
    SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE,
        0, (LPARAM)"BluehairMode", SMTO_ABORTIFHUNG, 5000, NULL);
}

LRESULT CALLBACK bluehair(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC originalProc = (WNDPROC)GetProp(hwnd, L"BluehairProc");

    switch (uMsg) {
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(75, 200, 244));
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_NCPAINT: {
        LRESULT result = CallWindowProc(originalProc, hwnd, uMsg, wParam, lParam);
        HDC hdc = GetWindowDC(hwnd);

        wchar_t title[256];
        GetWindowTextW(hwnd, title, 255);

        RECT rect;
        GetWindowRect(hwnd, &rect);

        int captionHeight = GetSystemMetrics(SM_CYCAPTION);
        RECT captionRect = { 0, 0, rect.right - rect.left, captionHeight };

        SetTextColor(hdc, RGB(75, 200, 244));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, title, -1, &captionRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        ReleaseDC(hwnd, hdc);
        return result;
    }

    case WM_ERASEBKGND:
        return 1;
    }

    if (originalProc) {
        return CallWindowProc(originalProc, hwnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK createhook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HCBT_CREATEWND) {
        HWND hwnd = (HWND)wParam;

        WNDPROC originalProc = (WNDPROC)SetWindowLongPtr(
            hwnd, GWLP_WNDPROC, (LONG_PTR)bluehair);

        SetProp(hwnd, L"BluehairProc", (HANDLE)originalProc);

        g_hookedWindows.push_back(hwnd);

        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }

    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

LRESULT CALLBACK gtmsghook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSG* msg = (MSG*)lParam;
        if (msg->message == WM_CTLCOLORSTATIC ||
            msg->message == WM_CTLCOLOREDIT ||
            msg->message == WM_CTLCOLORBTN) {
            HDC hdc = (HDC)msg->wParam;
            SetTextColor(hdc, RGB(75, 200, 244));
            SetBkMode(hdc, TRANSPARENT);
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL CALLBACK itslikechristmasmiracle(HWND hwnd, LPARAM lParam) {
    if (IsWindowVisible(hwnd)) {
        WNDPROC originalProc = (WNDPROC)SetWindowLongPtr(
            hwnd, GWLP_WNDPROC, (LONG_PTR)bluehair);
        SetProp(hwnd, L"BluehairProc", (HANDLE)originalProc);

        InvalidateRect(hwnd, NULL, TRUE);
        RedrawWindow(hwnd, NULL, NULL,
            RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
    }
    return TRUE;
}

void EnableBluehair() {
    if (g_hook) return;

    textinshit();
    g_hook = SetWindowsHookEx(WH_CBT, createhook, GetModuleHandle(NULL), 0);
    g_msgHook = SetWindowsHookEx(WH_GETMESSAGE, gtmsghook, GetModuleHandle(NULL), 0);
    EnumWindows(itslikechristmasmiracle, 0);
}

void DisableBluehair() {
    if (!g_hook) return;

    UnhookWindowsHookEx(g_hook);
    UnhookWindowsHookEx(g_msgHook);
    g_hook = NULL;
    g_msgHook = NULL;

    int elements[] = { COLOR_WINDOWTEXT, COLOR_MENUTEXT, COLOR_CAPTIONTEXT,
                       COLOR_INACTIVECAPTIONTEXT, COLOR_BTNTEXT, COLOR_INFOTEXT,
                       COLOR_HIGHLIGHTTEXT, COLOR_GRAYTEXT, COLOR_HOTLIGHT, COLOR_LISTBOXTEXT };
    COLORREF black[10];
    for (int i = 0; i < 10; i++) black[i] = RGB(0, 0, 0);
    SetSysColors(10, elements, black);

    SendMessageTimeout(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0, SMTO_ABORTIFHUNG, 5000, NULL);

    for (HWND hwnd : g_hookedWindows) {
        if (IsWindow(hwnd)) {
            WNDPROC originalProc = (WNDPROC)GetProp(hwnd, L"BluehairProc");
            if (originalProc) {
                SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)originalProc);
                RemoveProp(hwnd, L"BluehairProc");
            }
        }
    }
    g_hookedWindows.clear();
}