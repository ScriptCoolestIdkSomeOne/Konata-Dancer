#include "TrayAnimation.h"
#include <windows.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <memory>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "msimg32.lib")

using namespace Gdiplus;

class KonataChan {
public:
    void Show();
    void Hide();
    bool IsVisible();
};
extern KonataChan* g_konataWindow;
#define ID_TRAY_SHOW 1001
#define ID_TRAY_HIDE 1002
#define ID_TRAY_EXIT 1003

extern void KonataShow();
extern void KonataHide();
extern bool KonataIsVisible();

class TrayPopupMenu {
private:
    HWND m_hwnd;
    bool m_isVisible;
    int m_selectedIndex;
    int m_menuWidth;
    int m_menuHeight;

    static const int ITEM_HEIGHT = 30;
    static const int MENU_PADDING = 10;

    struct MenuItem {
        std::wstring text;
        int id;
        bool isSeparator;
        MenuItem(const std::wstring& t, int i, bool sep = false) : text(t), id(i), isSeparator(sep) {}
    };

    std::vector<MenuItem> m_items;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        TrayPopupMenu* pThis = nullptr;
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<TrayPopupMenu*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        }
        else {
            pThis = reinterpret_cast<TrayPopupMenu*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
        if (pThis) return pThis->HandleMessage(hwnd, msg, wParam, lParam);
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rect;
            GetClientRect(hWnd, &rect);

            //double buf
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

            //bg
            HBRUSH bgBrush = CreateSolidBrush(RGB(30, 30, 35));
            FillRect(memDC, &rect, bgBrush);
            DeleteObject(bgBrush);

            //boorrdeeryy
            HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(70, 70, 80));
            HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
            HPEN oldPen = (HPEN)SelectObject(memDC, borderPen);
            Rectangle(memDC, 0, 0, rect.right, rect.bottom);

            //fonty
            HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            HFONT oldFont = (HFONT)SelectObject(memDC, hFont);
            SetBkMode(memDC, TRANSPARENT);

            int y = 2;
            int itemIndex = 0;

            for (const auto& item : m_items) {
                if (item.isSeparator) {
                    HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(70, 70, 80));
                    HPEN oldSepPen = (HPEN)SelectObject(memDC, sepPen);
                    MoveToEx(memDC, 10, y + ITEM_HEIGHT / 2, NULL);
                    LineTo(memDC, m_menuWidth - 10, y + ITEM_HEIGHT / 2);
                    SelectObject(memDC, oldSepPen);
                    DeleteObject(sepPen);
                }
                else {
                    //highlightin'
                    if (itemIndex == m_selectedIndex) {
                        HBRUSH hoverBrush = CreateSolidBrush(RGB(70, 110, 160));
                        RECT itemRect = { 2, y + 1, m_menuWidth - 2, y + ITEM_HEIGHT - 1 };
                        FillRect(memDC, &itemRect, hoverBrush);
                        DeleteObject(hoverBrush);
                        SetTextColor(memDC, RGB(255, 255, 255));
                    }
                    else {
                        SetTextColor(memDC, RGB(220, 220, 230));
                    }

                    //text
                    RECT textRect = { MENU_PADDING + 15, y, m_menuWidth - MENU_PADDING, y + ITEM_HEIGHT };
                    DrawTextW(memDC, item.text.c_str(), -1, &textRect,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                    itemIndex++;
                }
                y += ITEM_HEIGHT;
            }

            SelectObject(memDC, oldFont);
            SelectObject(memDC, oldPen);
            SelectObject(memDC, oldBrush);
            DeleteObject(hFont);
            DeleteObject(borderPen);

            BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_MOUSEMOVE: {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            int newIndex = -1;
            int y = 2;
            int itemIdx = 0;

            for (const auto& item : m_items) {
                if (!item.isSeparator) {
                    if (pt.x >= 0 && pt.x < m_menuWidth && pt.y >= y && pt.y < y + ITEM_HEIGHT) {
                        newIndex = itemIdx;
                        break;
                    }
                    itemIdx++;
                }
                y += ITEM_HEIGHT;
            }

            if (newIndex != m_selectedIndex) {
                m_selectedIndex = newIndex;
                InvalidateRect(hWnd, NULL, TRUE);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            int y = 2;
            int itemIdx = 0;

            for (const auto& item : m_items) {
                if (!item.isSeparator) {
                    if (pt.x >= 0 && pt.x < m_menuWidth && pt.y >= y && pt.y < y + ITEM_HEIGHT) {
                        extern KonataChan* g_konataWindow;
                        if (g_konataWindow) {
                            switch (item.id) {
                            case ID_TRAY_SHOW:
                                g_konataWindow->Show();
                                break;
                            case ID_TRAY_HIDE:
                                g_konataWindow->Hide();
                                break;
                            case ID_TRAY_EXIT:
                                PostQuitMessage(0);
                                break;
                            }
                        }
                        DestroyWindow(hWnd);
                        return 0;
                    }
                    itemIdx++;
                }
                y += ITEM_HEIGHT;
            }
            DestroyWindow(hWnd);
            return 0;
        }

        case WM_KILLFOCUS:
            DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:
            m_isVisible = false;
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

public:
    TrayPopupMenu() : m_hwnd(nullptr), m_isVisible(false), m_selectedIndex(-1) {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = L"TrayPopupMenuClass";
        RegisterClassEx(&wc);
    }

    void Show(int x, int y) {
        // Очищаем старые пункты
        m_items.clear();

        // Добавляем только 3 пункта
        extern KonataChan* g_konataWindow;
        if (g_konataWindow && g_konataWindow->IsVisible()) {
            m_items.push_back(MenuItem(L"hide Konata", ID_TRAY_HIDE));
        }
        else {
            m_items.push_back(MenuItem(L"show yourself Konata", ID_TRAY_SHOW));
        }
        m_items.push_back(MenuItem(L"", 0, true));
        m_items.push_back(MenuItem(L"Exit", ID_TRAY_EXIT));

        m_menuWidth = 200;
        m_menuHeight = m_items.size() * ITEM_HEIGHT + 4;

        m_hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"TrayPopupMenuClass", L"", WS_POPUP,
            x, y, m_menuWidth, m_menuHeight,
            nullptr, nullptr, GetModuleHandle(nullptr), this);

        if (m_hwnd) {
            //you don't dare to go outside of this idk shit
            RECT desktop;
            SystemParametersInfo(SPI_GETWORKAREA, 0, &desktop, 0);

            if (x + m_menuWidth > desktop.right) {
                x = desktop.right - m_menuWidth - 5;
            }
            if (y + m_menuHeight > desktop.bottom) {
                y = desktop.bottom - m_menuHeight - 5;
            }
            if (x < desktop.left) x = desktop.left + 5;
            if (y < desktop.top) y = desktop.top + 5;

            SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, m_menuWidth, m_menuHeight, SWP_SHOWWINDOW);

            ShowWindow(m_hwnd, SW_SHOW);
            UpdateWindow(m_hwnd);
            SetCapture(m_hwnd);
            m_isVisible = true;
        }
    }
};

static std::vector<HICON> gifFrames;
static int currentFrame = 0;
static NOTIFYICONDATA nid = { sizeof(nid) };
static HWND trayHwnd = nullptr;
static UINT_PTR timerId = 0;
static HANDLE workerThread = nullptr;
static bool stopThread = false;
extern KonataChan* g_konataWindow;

Bitmap* BlackPixelRemoval(Bitmap* source, BYTE tolerance = 30) {
    int width = source->GetWidth();
    int height = source->GetHeight();
    Bitmap* result = new Bitmap(width, height, PixelFormat32bppARGB);

    BitmapData srcData, dstData;
    Rect rect(0, 0, width, height);

    source->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &srcData);
    result->LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &dstData);

    BYTE* srcPixels = (BYTE*)srcData.Scan0;
    BYTE* dstPixels = (BYTE*)dstData.Scan0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * srcData.Stride + x * 4;
            BYTE r = srcPixels[idx + 2];
            BYTE g = srcPixels[idx + 1];
            BYTE b = srcPixels[idx];

            if (r < tolerance && g < tolerance && b < tolerance) {
                dstPixels[idx + 3] = 0;  //transparent
            }
            else {
                dstPixels[idx] = b;
                dstPixels[idx + 1] = g;
                dstPixels[idx + 2] = r;
                dstPixels[idx + 3] = srcPixels[idx + 3];
            }
        }
    }

    source->UnlockBits(&srcData);
    result->UnlockBits(&dstData);
    return result;
}
HICON LoadJpgAsIcon(const std::wstring& path, float scale = 1.0f) {
    std::unique_ptr<Bitmap> source(new Bitmap(path.c_str()));
    if (source->GetLastStatus() != Ok) return NULL;

    std::unique_ptr<Bitmap> processedBitmap(BlackPixelRemoval(source.get()));
    if (!processedBitmap) return NULL;
    int iconSize = GetSystemMetrics(SM_CXICON);
    std::unique_ptr<Bitmap> canvas(new Bitmap(iconSize, iconSize, PixelFormat32bppARGB));
    Graphics graphics(canvas.get());
    //some shit that i googled
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    graphics.SetSmoothingMode(SmoothingModeHighQuality);
    graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    float newWidth = (float)iconSize * scale;
    float newHeight = (float)iconSize * scale;
    float posX = (iconSize - newWidth) / 2.0f;
    float posY = (iconSize - newHeight) / 2.0f;

    Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255));
    graphics.DrawLine(&pen, 0, 0, 200, 200);
    graphics.DrawImage(processedBitmap.get(), posX, posY, newWidth, newHeight);
    HICON hIcon = NULL;
    canvas->GetHICON(&hIcon);
    return hIcon;
}

bool LoadAllFrames() {
    for (int i = 0; i <= 284; i++) {
        wchar_t filename[MAX_PATH];
        swprintf(filename, MAX_PATH, L"frames\\Konata.gif_frame_%05d.jpg", i);

        if (GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES) {
            HICON hIcon = LoadJpgAsIcon(filename);
            if (hIcon) {
                gifFrames.push_back(hIcon);
            }
        }
    }
    return !gifFrames.empty();
}


LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
        nid.uCallbackMessage = WM_USER + 100;
        nid.hIcon = gifFrames.empty() ? LoadIcon(NULL, IDI_APPLICATION) : gifFrames[0];
        wcscpy_s(nid.szTip, L"Konata Chan");
        Shell_NotifyIcon(NIM_ADD, &nid);

        timerId = SetTimer(hwnd, 1, 33, NULL);
        return 0;
    }

    case WM_TIMER: {
        if (!gifFrames.empty()) {
            currentFrame = (currentFrame + 1) % gifFrames.size();
            nid.hIcon = gifFrames[currentFrame];
            Shell_NotifyIcon(NIM_MODIFY, &nid);
        }
        return 0;
    }

    case WM_USER + 100: {
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);

            // Показываем ПРОСТОЕ кастомное меню
            static TrayPopupMenu* trayMenu = nullptr;
            if (!trayMenu) {
                trayMenu = new TrayPopupMenu();
            }
            trayMenu->Show(pt.x, pt.y);
        }
        else if (lParam == WM_LBUTTONUP) {
            // Левый клик - показать/скрыть
            if (g_konataWindow) {
                if (g_konataWindow->IsVisible()) {
                    g_konataWindow->Hide();
                }
                else {
                    g_konataWindow->Show();
                }
            }
        }
        return 0;
    }

    case WM_DESTROY: {
        if (timerId) KillTimer(hwnd, timerId);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

DWORD WINAPI TrayThreadProc(LPVOID lpParam) {
    /*GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok) {
        return 1;
    }*/

    if (!LoadAllFrames()) {
        return 1;
    }

    WNDCLASS wc = {};
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"trayanimclass";
    RegisterClass(&wc);

    trayHwnd = CreateWindow(L"trayanimclass", NULL, 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!trayHwnd) {
        return 1;
    }

    MSG msg;
    while (!stopThread && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    for (HICON h : gifFrames) {
        if (h) DestroyIcon(h);
    }
    gifFrames.clear();

    //GdiplusShutdown(gdiplusToken);
    return 0;
}

void StartTrayAnimation() {
    stopThread = false;
    workerThread = CreateThread(NULL, 0, TrayThreadProc, NULL, 0, NULL);
}

void StopTrayAnimation() {
    if (!workerThread) return;
    stopThread = true;
    if (trayHwnd) {
        PostMessage(trayHwnd, WM_QUIT, 0, 0);
    }
    if (workerThread) {
        WaitForSingleObject(workerThread, 5000);
        CloseHandle(workerThread);
        workerThread = nullptr;
    }
    trayHwnd = nullptr;
}