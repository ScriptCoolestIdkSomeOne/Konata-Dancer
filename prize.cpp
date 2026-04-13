#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(linker, "/ENTRY:mainCRTStartup")
#include <windows.h>
#include <mmsystem.h>
#undef min
#undef max
#include <gdiplus.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#define _USE_MATH_DEFINES
#include <cmath>
#include <wincrypt.h>
#include <wingdi.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

using namespace Gdiplus;

static struct {
    bool active;
    HWND hwnd;
    UINT_PTR closeTimer;
    UINT_PTR animTimer;
    Gdiplus::Image* gif;
    UINT frameCount;
    UINT currentFrame;
    GUID frameDim;
    CRITICAL_SECTION cs;
    bool classRegistered;
} prize = { false, NULL, 0, 0, NULL, 0, 0, {0}, {0}, false };

static ULONG_PTR gdiToken = 0;
typedef RGBQUAD* PRGBQUAD;
static bool effectsRunning = false;
static HANDLE hEffectsThread = NULL;

HCRYPTPROV g_hProv = NULL;
DWORD g_xs = 0;

void InitRandom() {
    if (g_hProv == NULL) {
        if (!CryptAcquireContext(&g_hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_SILENT | CRYPT_VERIFYCONTEXT)) {
            g_hProv = NULL;
        }
    }
}

void CleanupRandom() {
    if (g_hProv) {
        CryptReleaseContext(g_hProv, 0);
        g_hProv = NULL;
    }
}
int Random() {
    if (g_hProv == NULL) {
        InitRandom();
        if (g_hProv == NULL) return rand();
    }
    int out;
    if (!CryptGenRandom(g_hProv, sizeof(out), (BYTE*)(&out))) {
        return rand();
    }
    return out & 0x7fffffff;
}
void SeedXorshift32(DWORD dwSeed) {
    g_xs = dwSeed;
}
DWORD Xorshift32() {
    g_xs ^= g_xs << 13;
    g_xs ^= g_xs >> 17;
    g_xs ^= g_xs << 5;
    return g_xs;
}

void getphyssize(int& width, int& height) {//pankoza is the goat
    DEVMODE devMode;
    devMode.dmSize = sizeof(DEVMODE);
    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode)) {
        width = devMode.dmPelsWidth;
        height = devMode.dmPelsHeight;
        return;
    }
    HDC hdc = GetDC(nullptr);
    if (hdc) {
        width = GetDeviceCaps(hdc, HORZRES);
        height = GetDeviceCaps(hdc, VERTRES);
        ReleaseDC(nullptr, hdc);
        return;
    }
    width = 1920;
    height = 1080;
}

POINT GetVirtualScreenPos() {
    POINT pt = { 0, 0 };
    return pt;
}

SIZE GetVirtualScreenSize() {
    SIZE sz;
    sz.cx = GetSystemMetrics(SM_CXSCREEN);
    sz.cy = GetSystemMetrics(SM_CYSCREEN);
    return sz;
}

int RotateDC(HDC hdc, int Angle, POINT ptCenter) {
    int nGraphicsMode = SetGraphicsMode(hdc, GM_ADVANCED);
    if (Angle != 0) {
        double fangle = (double)Angle / 180. * 3.14159265358979323846;
        XFORM xform;
        xform.eM11 = (float)cos(fangle);
        xform.eM12 = (float)sin(fangle);
        xform.eM21 = (float)-sin(fangle);
        xform.eM22 = (float)cos(fangle);
        xform.eDx = (float)(ptCenter.x - cos(fangle) * ptCenter.x + sin(fangle) * ptCenter.y);
        xform.eDy = (float)(ptCenter.y - cos(fangle) * ptCenter.y - sin(fangle) * ptCenter.x);
        SetWorldTransform(hdc, &xform);
    }
    return nGraphicsMode;
}

void Shader1(int t, int w, int h, PRGBQUAD prgbScreen) {
    for (int i = 0; i < w * h; i++) {
        COLORREF c = RGB(prgbScreen[i].rgbRed, prgbScreen[i].rgbGreen, prgbScreen[i].rgbBlue);
        c = (c * 2) % RGB(255, 255, 255);
        prgbScreen[i].rgbRed = GetRValue(c);
        prgbScreen[i].rgbGreen = GetGValue(c);
        prgbScreen[i].rgbBlue = GetBValue(c);
    }
}

void Shader2(int t, int w, int h, PRGBQUAD prgbScreen) {
    for (int i = 0; i < w * h; i++) {
        int r = prgbScreen[i].rgbRed;
        int g = prgbScreen[i].rgbGreen;
        int b = prgbScreen[i].rgbBlue;
        prgbScreen[i].rgbRed = (r + 100) % 256;
        prgbScreen[i].rgbGreen = ((r + g + b) / 4 + t) % 256;
        prgbScreen[i].rgbBlue = ((r + g + b) / 4 + i) % 256;
    }
}

void Shader3(int t, int w, int h, PRGBQUAD prgbScreen) {
    for (int i = 0; i < w * h; i++) {
        int r = prgbScreen[i].rgbRed;
        int g = prgbScreen[i].rgbGreen;
        int b = prgbScreen[i].rgbBlue;
        int val = (r + 1 > 0) ? (int)sqrt((double)(i >> (t / (r + 1)))) / 10 : 0;
        COLORREF c = RGB((2 * r) % 256, (b + t) % 256, (g + i) % 256);
        c = (c + val) % RGB(255, 255, 255);
        prgbScreen[i].rgbRed = GetRValue(c);
        prgbScreen[i].rgbGreen = GetGValue(c);
        prgbScreen[i].rgbBlue = GetBValue(c);
    }
}

void Shader4(int t, int w, int h, PRGBQUAD prgbScreen) {
    for (int i = 0; i < w * h; i++) {
        int r = prgbScreen[i].rgbRed;
        int g = prgbScreen[i].rgbGreen;
        int b = prgbScreen[i].rgbBlue;
        int gray = (r + g + b) / 3;
        COLORREF c = RGB(gray, gray, gray);
        c = (c + t - (int)sqrt((double)i)) % RGB(255, 255, 255);
        prgbScreen[i].rgbRed = GetRValue(c);
        prgbScreen[i].rgbGreen = GetGValue(c);
        prgbScreen[i].rgbBlue = GetBValue(c);
    }
}

void Shader5(int t, int w, int h, PRGBQUAD prgbScreen) {
    for (int i = 0; i < w * h; i++) {
        int val = Xorshift32() % (int)sqrt((double)(i + 1));
        int r = (int)prgbScreen[i].rgbRed - val;
        if (r < 0) r = 0;
        int g = (int)prgbScreen[i].rgbGreen - val;
        if (g < 0) g = 0;
        int b = (int)prgbScreen[i].rgbBlue - val;
        if (b < 0) b = 0;
        prgbScreen[i].rgbRed = r % 256;
        prgbScreen[i].rgbGreen = g % 256;
        prgbScreen[i].rgbBlue = b % 256;
    }
}

void Shader6(int t, int w, int h, PRGBQUAD prgbScreen) {
    for (int i = 0; i < w * h; i++) {
        RGBQUAD temp = prgbScreen[i];
        int idx = i / 3;
        if (idx < w * h) {
            prgbScreen[i] = prgbScreen[idx];
            prgbScreen[idx] = temp;
        }
    }
}

void Shader7(int t, int w, int h, PRGBQUAD prgbScreen) {
    for (int i = 0; i < w * h; i++) {
        int randPixel = Xorshift32() % (w * h);
        BYTE tempB = prgbScreen[i].rgbBlue;
        BYTE bVal = prgbScreen[randPixel].rgbBlue;
        prgbScreen[i].rgbRed = bVal;
        prgbScreen[i].rgbGreen = bVal;
        prgbScreen[i].rgbBlue = bVal;
        prgbScreen[randPixel].rgbRed = tempB;
        prgbScreen[randPixel].rgbGreen = tempB;
        prgbScreen[randPixel].rgbBlue = tempB;
    }
}

void Shader8(int t, int w, int h, PRGBQUAD prgbScreen) {
    t *= 10;
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            int idx = i * j;
            if (idx < w * h) {
                prgbScreen[idx].rgbRed = i % 256;
                prgbScreen[idx].rgbGreen = j % 256;
                prgbScreen[idx].rgbBlue = t % 256;
            }
        }
    }
}

void Shader9(int t, int w, int h, PRGBQUAD prgbScreen) {
    t *= 50;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int idx = i * w + j;
            if (idx < w * h) {
                prgbScreen[idx].rgbRed = (prgbScreen[idx].rgbRed + i / 10) % 256;
                prgbScreen[idx].rgbGreen = (prgbScreen[idx].rgbGreen + j / 10) % 256;
                prgbScreen[idx].rgbBlue = (prgbScreen[idx].rgbBlue + t) % 256;
            }
        }
    }
}

void Shader10(int t, int w, int h, PRGBQUAD prgbScreen) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int temp1 = (i + Xorshift32() % 11 - 5);
            if (temp1 < 0) temp1 = -temp1;
            int temp2 = (j + Xorshift32() % 11 - 5);
            if (temp2 < 0) temp2 = -temp2;
            int idx = (temp1 * w + temp2) % (w * h);
            int curIdx = i * w + j;
            if (curIdx < w * h && idx < w * h) {
                prgbScreen[curIdx] = prgbScreen[idx];
            }
        }
    }
}

void Shader11(int t, int w, int h, PRGBQUAD prgbScreen) {
    for (int i = 0; i < w * h; i++) {
        RGBQUAD temp = prgbScreen[i];
        int idx = i / 3 * 2;
        if (idx < w * h) {
            prgbScreen[i] = prgbScreen[idx];
            prgbScreen[idx] = temp;
        }
    }
}

void Shader12(int t, int w, int h, PRGBQUAD prgbScreen) {
    PRGBQUAD prgbTemp = (PRGBQUAD)malloc((w * h + w) * sizeof(RGBQUAD));
    if (!prgbTemp) return;
    memcpy(prgbTemp, prgbScreen, (w * h) * sizeof(RGBQUAD));
    for (int i = 0; i < h / 2; i++) {
        for (int j = 0; j < w; j++) {
            int idx = i * w + j;
            int newIdx = (int)((float)(i * w + j) + sqrt((double)(2 * (h / 2) * i - i * i))) % (w * h);
            if (idx < w * h && newIdx < w * h) {
                prgbScreen[idx] = prgbTemp[newIdx];
            }
        }
    }
    for (int i = h / 2; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int idx = i * w + j;
            int newIdx = (int)((float)(i * w + j) + sqrt((double)(2 * (h / 2) * i - i * i))) % (w * h);
            if (idx < w * h && newIdx < w * h) {
                prgbScreen[idx] = prgbTemp[newIdx];
            }
        }
    }
    free(prgbTemp);
    Sleep(50);
}

void Shader13(int t, int w, int h, PRGBQUAD prgbScreen) {
    PRGBQUAD prgbTemp = (PRGBQUAD)malloc((w * h + w) * sizeof(RGBQUAD));
    if (!prgbTemp) return;
    memcpy(prgbTemp, prgbScreen, (w * h) * sizeof(RGBQUAD));
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int idx = i * w + j;
            int newIdx = (unsigned int)((float)(j * w + i) + sqrt((double)(2 * h * i - i * i))) % (w * h);
            if (idx < w * h && newIdx < w * h) {
                prgbScreen[idx] = prgbTemp[newIdx];
            }
        }
    }
    free(prgbTemp);
    Sleep(100);
}

void Shader14(int t, int w, int h, PRGBQUAD prgbScreen) {
    PRGBQUAD prgbTemp = (PRGBQUAD)malloc((w * h + w) * sizeof(RGBQUAD));
    if (!prgbTemp) return;
    memcpy(prgbTemp, prgbScreen, (w * h) * sizeof(RGBQUAD));
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            int idx = i * w + j;
            int newIdx = (unsigned int)((float)(j * w + i) + sqrt((double)(2 * h * j - j * j))) % (w * h);
            if (idx < w * h && newIdx < w * h) {
                prgbScreen[idx] = prgbTemp[newIdx];
            }
        }
    }
    free(prgbTemp);
    Sleep(100);
}

void Shader15(int t, int w, int h, PRGBQUAD prgbScreen) {
    for (int i = 0; i < w * h; i++) {
        COLORREF c = (t * i) % RGB(255, 255, 255);
        prgbScreen[i].rgbRed = GetRValue(c);
        prgbScreen[i].rgbGreen = GetGValue(c);
        prgbScreen[i].rgbBlue = GetBValue(c);
    }
}

void Shader16(int t, int w, int h, PRGBQUAD prgbScreen) {
    for (int i = 0; i < w * h; i++) {
        BYTE val = Xorshift32() % 0x100;
        prgbScreen[i].rgbRed = val;
        prgbScreen[i].rgbGreen = val;
        prgbScreen[i].rgbBlue = val;
    }
}

void Payload1(int t, HDC hdcScreen) {
    POINT ptScreen = GetVirtualScreenPos();
    SIZE szScreen = GetVirtualScreenSize();

    HBRUSH brush = CreateSolidBrush(RGB(t % 256, (t / 2) % 256, (t / 2) % 256));
    if (brush) {
        HGDIOBJ oldBrush = SelectObject(hdcScreen, brush);
        PatBlt(hdcScreen, ptScreen.x, ptScreen.y, szScreen.cx, szScreen.cy, PATINVERT);
        SelectObject(hdcScreen, oldBrush);
        DeleteObject(brush);
    }
    Sleep(15);
}

void Payload2(int t, HDC hdcScreen) {
    POINT ptScreen = GetVirtualScreenPos();
    SIZE szScreen = GetVirtualScreenSize();
    t *= 10;

    BitBlt(hdcScreen, ptScreen.x, ptScreen.y, szScreen.cx, szScreen.cy,
        hdcScreen, ptScreen.x + t % szScreen.cx, ptScreen.y + t % szScreen.cy, NOTSRCERASE);

    HBRUSH brush = CreateSolidBrush(RGB(Random() % 256, Random() % 256, Random() % 256));
    if (brush) {
        HGDIOBJ oldBrush = SelectObject(hdcScreen, brush);
        PatBlt(hdcScreen, ptScreen.x, ptScreen.y, szScreen.cx, szScreen.cy, PATINVERT);
        SelectObject(hdcScreen, oldBrush);
        DeleteObject(brush);
    }
    Sleep(15);
}

void Payload3(int t, HDC hdcScreen) {
    POINT ptScreen = GetVirtualScreenPos();
    SIZE szScreen = GetVirtualScreenSize();

    HDC hcdc = CreateCompatibleDC(hdcScreen);
    if (!hcdc) return;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, szScreen.cx, szScreen.cy);
    if (!hBitmap) {
        DeleteDC(hcdc);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(hcdc, hBitmap);
    BitBlt(hcdc, ptScreen.x, ptScreen.y, szScreen.cx, szScreen.cy, hdcScreen, ptScreen.x, ptScreen.y, SRCCOPY);

    for (int i = ptScreen.x; i <= szScreen.cx / 10; i++) {
        for (int j = ptScreen.y; j <= szScreen.cy / 10; j++) {
            StretchBlt(hcdc, i * 10, j * 10, 10, 10, hcdc, i * 10, j * 10, 1, 1, SRCCOPY);
        }
    }

    BitBlt(hdcScreen, ptScreen.x, ptScreen.y, szScreen.cx, szScreen.cy, hcdc, ptScreen.x, ptScreen.y, SRCCOPY);

    SelectObject(hcdc, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hcdc);

    Sleep(100);
}

void Payload4(int t, HDC hdcScreen) {
    POINT ptScreen = GetVirtualScreenPos();
    SIZE szScreen = GetVirtualScreenSize();

    HDC hcdc = CreateCompatibleDC(hdcScreen);
    if (!hcdc) return;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, szScreen.cx, szScreen.cy);
    if (!hBitmap) {
        DeleteDC(hcdc);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(hcdc, hBitmap);
    BitBlt(hcdc, 0, 0, szScreen.cx, szScreen.cy, hdcScreen, 0, 0, SRCCOPY);

    BLENDFUNCTION blf = { 0 };
    blf.BlendOp = AC_SRC_OVER;
    blf.BlendFlags = 0;
    blf.SourceConstantAlpha = 128;
    blf.AlphaFormat = 0;

    AlphaBlend(hdcScreen, ptScreen.x + t % 200 + 10, ptScreen.y - t % 25,
        szScreen.cx, szScreen.cy, hcdc, ptScreen.x, ptScreen.y, szScreen.cx, szScreen.cy, blf);

    SelectObject(hcdc, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hcdc);

    Sleep(20);
}

void Payload5(int t, HDC hdcScreen) {
    POINT ptScreen = GetVirtualScreenPos();
    SIZE szScreen = GetVirtualScreenSize();
    t *= 3;

    int x = Random() % szScreen.cx;
    int y = Random() % szScreen.cy;
    BitBlt(hdcScreen, x, y, t % szScreen.cx, t % szScreen.cy,
        hdcScreen, (x + t / 2) % szScreen.cx, y % szScreen.cy, SRCCOPY);
}

void Payload6(int t, HDC hdcScreen) {
    POINT ptScreen = GetVirtualScreenPos();
    SIZE szScreen = GetVirtualScreenSize();

    HDC hcdc = CreateCompatibleDC(hdcScreen);
    if (!hcdc) return;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, szScreen.cx, szScreen.cy);
    if (!hBitmap) {
        DeleteDC(hcdc);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(hcdc, hBitmap);
    BitBlt(hcdc, 0, 0, szScreen.cx, szScreen.cy, hdcScreen, 0, 0, SRCCOPY);

    POINT p = { szScreen.cx / 2, szScreen.cy / 2 };

    BLENDFUNCTION blf = { 0 };
    blf.BlendOp = AC_SRC_OVER;
    blf.BlendFlags = 0;
    blf.SourceConstantAlpha = 128;
    blf.AlphaFormat = 0;

    RotateDC(hdcScreen, 10, p);
    AlphaBlend(hdcScreen, 0, t, szScreen.cx, szScreen.cy, hcdc, 0, 0, szScreen.cx, szScreen.cy, blf);

    SelectObject(hcdc, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hcdc);
}

void Payload7(int t, HDC hdcScreen) {
    POINT ptScreen = GetVirtualScreenPos();
    SIZE szScreen = GetVirtualScreenSize();

    HDC hcdc = CreateCompatibleDC(hdcScreen);
    if (!hcdc) return;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, szScreen.cx, szScreen.cy);
    if (!hBitmap) {
        DeleteDC(hcdc);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(hcdc, hBitmap);
    BitBlt(hcdc, 0, 0, szScreen.cx, szScreen.cy, hdcScreen, 0, 0, SRCCOPY);

    POINT p = { szScreen.cx / 2, szScreen.cy / 2 };

    BLENDFUNCTION blf = { 0 };
    blf.BlendOp = AC_SRC_OVER;
    blf.BlendFlags = 0;
    blf.SourceConstantAlpha = 128;
    blf.AlphaFormat = 0;

    if (t % 2 == 0) {
        RotateDC(hdcScreen, 1, p);
    }
    else {
        RotateDC(hdcScreen, -1, p);
    }

    SetBkColor(hdcScreen, RGB(Random() % 256, Random() % 256, Random() % 256));
    SetTextColor(hdcScreen, RGB(Random() % 256, Random() % 256, Random() % 256));

    TextOut(hdcScreen, Random() % szScreen.cx, Random() % szScreen.cy, L"KONAKONA", 8);

    AlphaBlend(hdcScreen, 0, 0, szScreen.cx, szScreen.cy, hcdc, 0, 0, szScreen.cx, szScreen.cy, blf);

    SelectObject(hcdc, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hcdc);
}

void Payload8(int t, HDC hdcScreen) {
    POINT ptScreen = GetVirtualScreenPos();
    SIZE szScreen = GetVirtualScreenSize();

    HDC hcdc = CreateCompatibleDC(hdcScreen);
    if (!hcdc) return;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, szScreen.cx, szScreen.cy);
    if (!hBitmap) {
        DeleteDC(hcdc);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(hcdc, hBitmap);
    BitBlt(hcdc, 0, 0, szScreen.cx, szScreen.cy, hdcScreen, 0, 0, SRCCOPY);

    int nGraphicsMode = SetGraphicsMode(hdcScreen, GM_ADVANCED);
    XFORM xform;
    xform.eDx = 0;
    xform.eDy = 0;
    xform.eM11 = 1;
    xform.eM12 = 0.1f;
    xform.eM21 = 0;
    xform.eM22 = 1;
    SetWorldTransform(hdcScreen, &xform);

    BLENDFUNCTION blf = { 0 };
    blf.BlendOp = AC_SRC_OVER;
    blf.BlendFlags = 0;
    blf.SourceConstantAlpha = 128;
    blf.AlphaFormat = 0;

    SetBkColor(hdcScreen, RGB(Random() % 256, Random() % 256, Random() % 256));
    SetTextColor(hdcScreen, RGB(Random() % 256, Random() % 256, Random() % 256));

    for (int i = 0; i < 5; i++) {
        TextOut(hdcScreen, Random() % szScreen.cx, Random() % szScreen.cy, L"Kona DANCER SHIT", 11);
    }

    AlphaBlend(hdcScreen, 0, 0, szScreen.cx, szScreen.cy, hcdc, 0, 0, szScreen.cx, szScreen.cy, blf);

    SelectObject(hcdc, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hcdc);

    Sleep(50);
}

void Payload9(int t, HDC hdcScreen) {
    POINT ptScreen = GetVirtualScreenPos();
    SIZE szScreen = GetVirtualScreenSize();

    POINT pt[3];
    pt[0] = { 0, 0 };
    pt[1] = { szScreen.cx, 0 };
    pt[2] = { 25, szScreen.cy };
    PlgBlt(hdcScreen, pt, hdcScreen, ptScreen.x, ptScreen.y, szScreen.cx + 25, szScreen.cy, NULL, 0, 0);

    HBRUSH brush = CreateSolidBrush(RGB(Random() % 256, Random() % 256, Random() % 256));
    if (brush) {
        HGDIOBJ oldBrush = SelectObject(hdcScreen, brush);
        PatBlt(hdcScreen, ptScreen.x, ptScreen.y, szScreen.cx, szScreen.cy, PATINVERT);
        SelectObject(hdcScreen, oldBrush);
        DeleteObject(brush);
    }

    Sleep(50);
}

void Payload10(int t, HDC hdcScreen) {
    POINT ptScreen = GetVirtualScreenPos();
    SIZE szScreen = GetVirtualScreenSize();

    t *= 30;

    RedrawWindow(NULL, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);

    HDC hcdc = CreateCompatibleDC(hdcScreen);
    if (!hcdc) return;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, szScreen.cx, szScreen.cy);
    if (!hBitmap) {
        DeleteDC(hcdc);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(hcdc, hBitmap);
    Sleep(50);
    BitBlt(hcdc, 0, 0, szScreen.cx, szScreen.cy, hdcScreen, 0, 0, NOTSRCCOPY);

    HBRUSH patternBrush = CreatePatternBrush(hBitmap);
    if (patternBrush) {
        HGDIOBJ oldBrush = SelectObject(hdcScreen, patternBrush);

        int x1 = t % szScreen.cx + 20;
        int y1 = t % szScreen.cy + 20;
        int x2 = t % szScreen.cx + t % 101 + 180;
        int y2 = t % szScreen.cy + t % 101 + 180;
        Ellipse(hdcScreen, x1, y1, x2, y2);

        BitBlt(hcdc, 0, 0, szScreen.cx, szScreen.cy, hdcScreen, 0, 0, NOTSRCCOPY);

        SelectObject(hdcScreen, patternBrush);
        x1 = t % szScreen.cx + 10;
        y1 = t % szScreen.cy + 10;
        x2 = t % szScreen.cx + t % 101 + 190;
        y2 = t % szScreen.cy + t % 101 + 190;
        Ellipse(hdcScreen, x1, y1, x2, y2);

        x1 = t % szScreen.cx;
        y1 = t % szScreen.cy;
        x2 = t % szScreen.cx + t % 101 + 200;
        y2 = t % szScreen.cy + t % 101 + 200;
        Ellipse(hdcScreen, x1, y1, x2, y2);

        BitBlt(hcdc, 0, 0, szScreen.cx, szScreen.cy, hdcScreen, 0, 0, NOTSRCCOPY);

        SelectObject(hdcScreen, patternBrush);
        Ellipse(hdcScreen, t % szScreen.cx, t % szScreen.cy,
            t % szScreen.cx + t % 101 + 200, t % szScreen.cy + t % 101 + 200);

        SelectObject(hdcScreen, oldBrush);
        DeleteObject(patternBrush);
    }

    SetBkColor(hdcScreen, RGB(Random() % 256, Random() % 256, Random() % 256));
    SetTextColor(hdcScreen, RGB(Random() % 256, Random() % 256, Random() % 256));

    for (int i = 0; i < 5; i++) {
        TextOut(hdcScreen, Random() % szScreen.cx, Random() % szScreen.cy, L"     ", 5);
    }

    SelectObject(hcdc, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hcdc);
}

class TextOutEffect {
private:
    int screenWidth;
    int screenHeight;
    int logicalWidth;
    int logicalHeight;
    const char* texts[4];

public:
    TextOutEffect() {
        srand(static_cast<unsigned>(time(nullptr)));
        texts[0] = "Dancerr";
        texts[1] = "KonaKona";
        texts[2] = "Konata-Dancer";
        texts[3] = "Konata";
    }

    bool initialize() {
        getphyssize(screenWidth, screenHeight);
        logicalWidth = GetSystemMetrics(SM_CXSCREEN);
        logicalHeight = GetSystemMetrics(SM_CYSCREEN);
        if (screenWidth <= 0 || screenHeight <= 0) {
            screenWidth = 1920;
            screenHeight = 1080;
        }
        return true;
    }

    void Run(DWORD maxDuration = 5000) {
        if (!initialize()) return;
        DWORD startTime = GetTickCount();

        while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000) &&
            (GetTickCount() - startTime) < maxDuration) {
            HDC hdc = GetDC(nullptr);
            if (!hdc) break;

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));

            HFONT font = CreateFontA(43, 32, 0, 0, FW_EXTRALIGHT, 0, 1, 0,
                ANSI_CHARSET, 0, 0, 0, 0, "Courier New");

            if (font) {
                SelectObject(hdc, font);
                for (int i = 0; i < 4; i++) {
                    TextOutA(hdc, rand() % logicalWidth, rand() % logicalHeight,
                        texts[i], (int)strlen(texts[i]));
                    Sleep(1);
                }
                DeleteObject(font);
            }
            ReleaseDC(nullptr, hdc);
        }
    }
};

class BlurEffect {
private:
    HDC hdc;
    HDC dcCopy;
    HBITMAP bmp;
    int screenWidth;
    int screenHeight;
    int logicalWidth;
    int logicalHeight;

public:
    BlurEffect() : hdc(nullptr), dcCopy(nullptr), bmp(nullptr),
        screenWidth(0), screenHeight(0), logicalWidth(0), logicalHeight(0) {
    }

    ~BlurEffect() { Cleanup(); }

    bool Initialize() {
        getphyssize(screenWidth, screenHeight);
        logicalWidth = GetSystemMetrics(SM_CXSCREEN);
        logicalHeight = GetSystemMetrics(SM_CYSCREEN);
        if (screenWidth <= 0 || screenHeight <= 0) {
            screenWidth = 1920;
            screenHeight = 1080;
        }
        hdc = GetDC(nullptr);
        if (!hdc) return false;
        dcCopy = CreateCompatibleDC(hdc);
        if (!dcCopy) { ReleaseDC(nullptr, hdc); return false; }

        BITMAPINFO bmpi = { 0 };
        bmpi.bmiHeader.biSize = sizeof(bmpi);
        bmpi.bmiHeader.biWidth = screenWidth;
        bmpi.bmiHeader.biHeight = screenHeight;
        bmpi.bmiHeader.biPlanes = 1;
        bmpi.bmiHeader.biBitCount = 32;
        bmpi.bmiHeader.biCompression = BI_RGB;

        void* bits = NULL;
        bmp = CreateDIBSection(hdc, &bmpi, DIB_RGB_COLORS, &bits, NULL, 0);
        if (!bmp || !bits) { DeleteDC(dcCopy); ReleaseDC(nullptr, hdc); return false; }

        SelectObject(dcCopy, bmp);
        return true;
    }

    void Run(DWORD maxDuration = 5000) {
        if (!Initialize()) return;
        DWORD startTime = GetTickCount();

        BLENDFUNCTION blur = { 0 };
        blur.BlendOp = AC_SRC_OVER;
        blur.SourceConstantAlpha = 15;

        while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000) &&
            (GetTickCount() - startTime) < maxDuration) {
            HDC hdcScreen = GetDC(nullptr);
            if (!hdcScreen) break;

            StretchBlt(dcCopy, -1, -1, screenWidth, screenHeight, hdcScreen, 0, 0, logicalWidth, logicalHeight, SRCCOPY);
            SetStretchBltMode(hdcScreen, HALFTONE);
            AlphaBlend(hdcScreen, 0, 0, logicalWidth, logicalHeight, dcCopy, 0, 0, screenWidth, screenHeight, blur);

            ReleaseDC(nullptr, hdcScreen);
        }
    }

    void Cleanup() {
        if (bmp) { DeleteObject(bmp); bmp = nullptr; }
        if (dcCopy) { DeleteDC(dcCopy); dcCopy = nullptr; }
        if (hdc) { ReleaseDC(nullptr, hdc); hdc = nullptr; }
    }
};

class MasherEffect {
private:
    int screenWidth;
    int screenHeight;
    int logicalWidth;
    int logicalHeight;

public:
    MasherEffect() : screenWidth(0), screenHeight(0), logicalWidth(0), logicalHeight(0) {
        srand(static_cast<unsigned>(time(nullptr)));
    }

    bool initialize() {
        getphyssize(screenWidth, screenHeight);
        logicalWidth = GetSystemMetrics(SM_CXSCREEN);
        logicalHeight = GetSystemMetrics(SM_CYSCREEN);
        if (screenWidth <= 0 || screenHeight <= 0) {
            screenWidth = 1920;
            screenHeight = 1080;
        }
        return true;
    }

    void Run(DWORD maxDuration = 5000) {
        if (!initialize()) return;
        DWORD startTime = GetTickCount();

        while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000) &&
            (GetTickCount() - startTime) < maxDuration) {
            HDC hdc = GetDC(HWND_DESKTOP);
            if (!hdc) break;

            int x1 = rand() % 10, y1 = rand() % 10;
            int w = rand() % logicalWidth;
            int h = rand() % logicalHeight;
            int x2 = rand() % 10, y2 = rand() % 10;

            StretchBlt(hdc, x1, y1, w, h, hdc, x2, y2, w, h, SRCCOPY);
            ReleaseDC(HWND_DESKTOP, hdc);
        }
    }
};

static LRESULT CALLBACK prizyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        EnterCriticalSection(&prize.cs);
        if (prize.gif && prize.frameCount) {
            prize.gif->SelectActiveFrame(&prize.frameDim, prize.currentFrame);
            RECT rc;
            GetClientRect(hwnd, &rc);
            Graphics graphics(hdc);
            graphics.DrawImage(prize.gif, 0, 0, rc.right, rc.bottom);
        }
        LeaveCriticalSection(&prize.cs);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_TIMER:
        if (wParam == prize.animTimer) {
            EnterCriticalSection(&prize.cs);
            prize.currentFrame = (prize.currentFrame + 1) % prize.frameCount;
            LeaveCriticalSection(&prize.cs);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == prize.closeTimer) {
            KillTimer(hwnd, prize.closeTimer);
            KillTimer(hwnd, prize.animTimer);
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, prize.closeTimer);
        KillTimer(hwnd, prize.animTimer);
        EnterCriticalSection(&prize.cs);
        if (prize.gif) {
            delete prize.gif;
            prize.gif = NULL;
        }
        prize.active = false;
        prize.hwnd = NULL;
        LeaveCriticalSection(&prize.cs);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void InitGdi() {
    if (!gdiToken) {
        GdiplusStartupInput in;
        GdiplusStartup(&gdiToken, &in, NULL);
    }
}

static bool LoadGif() {
    EnterCriticalSection(&prize.cs);
    if (prize.gif) {
        delete prize.gif;
        prize.gif = NULL;
    }
    prize.gif = Gdiplus::Image::FromFile(L"Congratulations_You've_Won.gif");
    if (!prize.gif || prize.gif->GetLastStatus() != Ok) {
        LeaveCriticalSection(&prize.cs);
        return false;
    }
    prize.gif->GetFrameDimensionsList(&prize.frameDim, 1);
    prize.frameCount = prize.gif->GetFrameCount(&prize.frameDim);
    prize.currentFrame = 0;
    LeaveCriticalSection(&prize.cs);
    return prize.frameCount > 0;    
}

static void ShowPrizeWindow() {
    if (!prize.classRegistered) {
        WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = prizyWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = L"prizyWindow";
        RegisterClassEx(&wc);
        prize.classRegistered = true;
    }

    int w = 400, h = 300;
    EnterCriticalSection(&prize.cs);
    if (prize.gif) {
        w = prize.gif->GetWidth();
        h = prize.gif->GetHeight();
    }
    LeaveCriticalSection(&prize.cs);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    prize.hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"PrizeWindow", L"",
        WS_POPUP, (sw - w) / 2, (sh - h) / 2, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (prize.hwnd) {
        ShowWindow(prize.hwnd, SW_SHOW);
        prize.animTimer = SetTimer(prize.hwnd, 1, 33, NULL);
        prize.closeTimer = SetTimer(prize.hwnd, 2, 3000, NULL);
    }
}

static void PlayMp3(const wchar_t* file, const wchar_t* alias) {
    wchar_t cmd[512];
    wsprintfW(cmd, L"close %s", alias);
    mciSendString(cmd, NULL, 0, NULL);
    wsprintfW(cmd, L"open \"%s\" type mpegvideo alias %s", file, alias);
    mciSendString(cmd, NULL, 0, NULL);
    wsprintfW(cmd, L"play %s", alias);
    mciSendString(cmd, NULL, 0, NULL);
}

DWORD WINAPI PlaySecondSoundThread(LPVOID) {
    Sleep(1000);
    PlayMp3(L"Congratulations_You've_Won.mp3", L"sound2");
    return 0;
}

DWORD WINAPI EffectsThread(LPVOID lpParam) {
    Sleep(500);

    TextOutEffect textEffect;
    textEffect.Run(4000);

    BlurEffect blurEffect;
    blurEffect.Run(4000);

    MasherEffect masherEffect;
    masherEffect.Run(4000);

    HDC hdcScreen = GetDC(NULL);
    if (hdcScreen) {
        int t = 0;
        DWORD startTime = GetTickCount();
        while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000) &&
            t < 500 &&
            (GetTickCount() - startTime) < 4000) {
            int w = GetSystemMetrics(SM_CXSCREEN);
            int h = GetSystemMetrics(SM_CYSCREEN);

            int payload = Random() % 10;
            switch (payload) {
            case 0: Payload1(t, hdcScreen); break;
            case 1: Payload2(t, hdcScreen); break;
            case 2: Payload3(t, hdcScreen); break;
            case 3: Payload4(t, hdcScreen); break;
            case 4: Payload5(t, hdcScreen); break;
            case 5: Payload6(t, hdcScreen); break;
            case 6: Payload7(t, hdcScreen); break;
            case 7: Payload8(t, hdcScreen); break;
            case 8: Payload9(t, hdcScreen); break;
            case 9: Payload10(t, hdcScreen); break;
            }
            t++;
        }
        ReleaseDC(NULL, hdcScreen);
    }

    effectsRunning = false;
    return 0;
}

bool ActivatePrize(HWND parent) {
    static bool initialized = false;
    if (!initialized) {
        InitializeCriticalSection(&prize.cs);
        InitRandom();
        srand(static_cast<unsigned>(time(nullptr)));
        SeedXorshift32(static_cast<DWORD>(time(nullptr)));
        initialized = true;
    }

    if (prize.active) return false;

    bool hasFiles = true;
    if (GetFileAttributesW(L"Congratulations_You've_Won.gif") == INVALID_FILE_ATTRIBUTES) hasFiles = false;
    if (GetFileAttributesW(L"konata-ding.mp3") == INVALID_FILE_ATTRIBUTES) hasFiles = false;
    if (GetFileAttributesW(L"Congratulations_You've_Won.mp3") == INVALID_FILE_ATTRIBUTES) hasFiles = false;

    if (hasFiles) {
        InitGdi();
        if (LoadGif()) {
            prize.active = true;
            PlayMp3(L"konata-ding.mp3", L"sound1");
            CreateThread(NULL, 0, PlaySecondSoundThread, NULL, 0, NULL);
            ShowPrizeWindow();
        }
    }

    //multiple threads
    if (!effectsRunning) {
        effectsRunning = true;
        hEffectsThread = CreateThread(NULL, 0, EffectsThread, NULL, 0, NULL);
    }

    return true;
}

bool IsPrizeActive() { return prize.active; }

void CleanupPrize() {
    effectsRunning = false;
    if (hEffectsThread) {
        WaitForSingleObject(hEffectsThread, 2000);
        CloseHandle(hEffectsThread);
        hEffectsThread = NULL;
    }

    if (prize.hwnd) DestroyWindow(prize.hwnd);
    EnterCriticalSection(&prize.cs);
    if (prize.gif) delete prize.gif;
    LeaveCriticalSection(&prize.cs);
    DeleteCriticalSection(&prize.cs);

    if (gdiToken) {
        GdiplusShutdown(gdiToken);
        gdiToken = 0;
    }

    CleanupRandom();
}

int main() {
    SetProcessDPIAware();

    ActivatePrize(NULL);

    //esc if this shit will somehow brokke and timer deosn't work
    while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
        Sleep(100);
    }

    CleanupPrize();
    return 0;
}