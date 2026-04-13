#include <windows.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <commctrl.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

using namespace Gdiplus;

std::vector<HICON> gifFrames;
int currentFrame = 0;
NOTIFYICONDATA nid = { sizeof(nid) };
HWND g_trayHwnd = nullptr;

Bitmap* TrayRemoveBlackPixels(Bitmap* source, BYTE tolerance = 30) {
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
                dstPixels[idx + 3] = 0;
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

HICON TrayLoadJpgAsIcon(const std::wstring& path, float scale = 2.0f) {
    Bitmap source(path.c_str());
    if (source.GetLastStatus() != Ok) return NULL;

    Bitmap* processedBitmap = TrayRemoveBlackPixels(&source);
    if (!processedBitmap) return NULL;

    int iconSize = GetSystemMetrics(SM_CXSMICON);
    Bitmap canvas(iconSize, iconSize, PixelFormat32bppARGB);
    Graphics graphics(&canvas);

    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    float newWidth = (float)iconSize * scale;
    float newHeight = (float)iconSize * scale;
    float posX = (iconSize - newWidth) / 2.0f;
    float posY = (iconSize - newHeight) / 2.0f;

    graphics.Clear(Color(0, 0, 0, 0));
    graphics.DrawImage(processedBitmap, posX, posY, newWidth, newHeight);

    HICON hIcon = NULL;
    canvas.GetHICON(&hIcon);
    delete processedBitmap;
    return hIcon;
}

void TrayLoadAllFrames() {
    for (int i = 0; i <= 284; ++i) {
        wchar_t filename[MAX_PATH];
        swprintf(filename, MAX_PATH, L"frames\\Konata.gif_frame_%05d.jpg", i);

        if (GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES) {
            HICON hIcon = TrayLoadJpgAsIcon(filename);
            if (hIcon) gifFrames.push_back(hIcon);
        }
    }
}

class AnimationController {
private:
    int frameRate = 30;
    DWORD lastFrameTime = 0;

public:
    void UpdateFrame() {
        DWORD now = GetTickCount();
        if (now - lastFrameTime >= 1000 / frameRate) {
            if (!gifFrames.empty()) {
                currentFrame = (currentFrame + 1) % gifFrames.size();
                nid.hIcon = gifFrames[currentFrame];
                Shell_NotifyIcon(NIM_MODIFY, &nid);
            }
            lastFrameTime = now;
        }
    }
};

AnimationController animController;

LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
        nid.uCallbackMessage = WM_APP + 1;
        nid.hIcon = gifFrames[0];
        lstrcpy(nid.szTip, L"Konata Dancing");
        Shell_NotifyIcon(NIM_ADD, &nid);
        SetTimer(hwnd, 1, 33, NULL); // ~30 FPS
        return 0;

    case WM_TIMER:
        if (wParam == 1) {
            animController.UpdateFrame();
        }
        return 0;

    case WM_APP + 1:
        if (lParam == WM_LBUTTONDBLCLK) {
            DestroyWindow(hwnd);
        }
        else if (lParam == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 1, L"Exit");

            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
            PostMessage(hwnd, WM_NULL, 0, 0);
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Экспортируемая функция для запуска из WinMain
int RunTrayAnimation(HINSTANCE hInst) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok) {
        return 1;
    }

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    TrayLoadAllFrames();
    if (gifFrames.empty()) {
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"TrayAnimClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(L"TrayAnimClass", NULL, 0, 0, 0, 0, 0, NULL, NULL, hInst, NULL);
    if (!hwnd) {
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    g_trayHwnd = hwnd;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    for (HICON h : gifFrames) DestroyIcon(h);
    GdiplusShutdown(gdiplusToken);
    return 0;
}