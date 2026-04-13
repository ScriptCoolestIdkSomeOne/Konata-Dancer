#include "menu.h"
#include <gdiplus.h>
#include <cmath>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#define MENU_WIDTH 220
#define MENU_ITEM_HEIGHT 30
#define MENU_BORDER 1
#define MENU_CORNER 5
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#define MENU_BG_COLOR Color(40, 40, 45)
#define MENU_BORDER_COLOR Color(80, 80, 95)
#define MENU_TEXT_COLOR Color(220, 220, 230)
#define MENU_HOVER_BG Color(70, 110, 160)
#define MENU_HOVER_TEXT Color(255, 255, 255)
#define MENU_DISABLED_TEXT Color(100, 100, 110)
#define MENU_SEPARATOR Color(80, 80, 95)

KonataMenu::KonataMenu()
    : m_hwnd(nullptr), m_parentWnd(nullptr), m_isVisible(false), m_inSubmenu(false),
    m_selectedIndex(-1), m_subSelectedIndex(-1), m_callbackTarget(nullptr),
    m_onScaleChange(nullptr), m_onFpsChange(nullptr), m_onSaturationChange(nullptr),
    m_onFrameSizeChange(nullptr), m_onEffectChange(nullptr), m_onPrize(nullptr), m_onExit(nullptr),
    m_currentSaturation(1.0f), m_hueShift(0.0f), m_oldFilm(0.0f), m_termIntensity(0.0f), m_inverted(false) {

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"KonataMenuOverlay";
    RegisterClassEx(&wc);

    m_hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        L"KonataMenuOverlay", L"", WS_POPUP,
        0, 0, MENU_WIDTH, 400, nullptr, nullptr, GetModuleHandle(nullptr), this);

    SetLayeredWindowAttributes(m_hwnd, 0, 245, LWA_ALPHA);
    BuildMenu();
}

KonataMenu::~KonataMenu() {
    if (m_hwnd && IsWindow(m_hwnd)) DestroyWindow(m_hwnd);
}

void KonataMenu::SetCallbacks(
    void* target,
    void(__cdecl* onScaleChange)(void*, int),
    void(__cdecl* onFpsChange)(void*, int),
    void(__cdecl* onSaturationChange)(void*, float),
    void(__cdecl* onFrameSizeChange)(void*, int, int),
    void(__cdecl* onEffectChange)(void*),
    void(__cdecl* onPrize)(void*),
    void(__cdecl* onExit)(void*)
) {
    m_callbackTarget = target;
    m_onScaleChange = onScaleChange;
    m_onFpsChange = onFpsChange;
    m_onSaturationChange = onSaturationChange;
    m_onFrameSizeChange = onFrameSizeChange;
    m_onEffectChange = onEffectChange;
    m_onPrize = onPrize;
    m_onExit = onExit;
}

void KonataMenu::UpdateSaturation(float saturation) {
    m_currentSaturation = saturation;
}

void KonataMenu::UpdateHueShift(float shift) {
    m_hueShift = shift;
}

void KonataMenu::UpdateOldFilm(float film) {
    m_oldFilm = film;
}

void KonataMenu::UpdateTerminator(float term) {
    m_termIntensity = term;
}

void KonataMenu::UpdateInverted(bool inverted) {
    m_inverted = inverted;
}

void KonataMenu::BuildMenu() {
    m_menuItems.clear();

    //scale
    m_menuItems.push_back(MenuItem(1, L"scale 50%"));
    m_menuItems.push_back(MenuItem(2, L"scale 75%"));
    m_menuItems.push_back(MenuItem(3, L"scale 100%"));
    m_menuItems.push_back(MenuItem(4, L"scale 125%"));
    m_menuItems.push_back(MenuItem(5, L"scale 150%"));
    m_menuItems.push_back(MenuItem(true)); //separator

    //FPS
    m_menuItems.push_back(MenuItem(10, L"FPS 15"));
    m_menuItems.push_back(MenuItem(11, L"FPS 24"));
    m_menuItems.push_back(MenuItem(12, L"FPS 30"));
    m_menuItems.push_back(MenuItem(13, L"FPS 60"));
    m_menuItems.push_back(MenuItem(true)); //separator

    //frame 
    m_menuItems.push_back(MenuItem(30, L"frame 160x120"));
    m_menuItems.push_back(MenuItem(31, L"frame 320x240"));
    m_menuItems.push_back(MenuItem(32, L"frame 480x360"));
    m_menuItems.push_back(MenuItem(33, L"frame 640x480"));
    m_menuItems.push_back(MenuItem(true)); // separator

    wchar_t satText[64];
    if (m_currentSaturation == 1.0f) {
        swprintf_s(satText, L"Saturation: Normal");
    }
    else {
        swprintf_s(satText, L"Saturation: %.1fx", m_currentSaturation);
    }
    MenuItem satItem(0, satText);
    satItem.isDisabled = true;
    m_menuItems.push_back(satItem);

    //Saturation
    m_menuItems.push_back(MenuItem(20, L"set Saturation 1.2x"));
    m_menuItems.push_back(MenuItem(21, L"set Saturation 1.5x"));
    m_menuItems.push_back(MenuItem(22, L"set Saturation 2.0x"));
    m_menuItems.push_back(MenuItem(23, L"reset Saturation"));
    m_menuItems.push_back(MenuItem(true)); // separator

    //effects submenu
    MenuItem effectsItem(1000, L"▶ Color Effects");
    m_effectSubmenu.clear();
    m_effectSubmenu.push_back(MenuItem(40, L"Reset All Effects"));
    m_effectSubmenu.push_back(MenuItem(true));
    m_effectSubmenu.push_back(MenuItem(41, L"Warm"));
    m_effectSubmenu.push_back(MenuItem(42, L"Cool"));
    m_effectSubmenu.push_back(MenuItem(43, L"Sepia 50%"));
    m_effectSubmenu.push_back(MenuItem(44, L"Sepia 100%"));
    m_effectSubmenu.push_back(MenuItem(45, L"Vintage"));
    m_effectSubmenu.push_back(MenuItem(46, L"Hue Shift +"));
    m_effectSubmenu.push_back(MenuItem(47, L"Hue Shift -"));
    m_effectSubmenu.push_back(MenuItem(48, L"Reset Hue"));
    m_effectSubmenu.push_back(MenuItem(49, L"Gamma 0.8"));
    m_effectSubmenu.push_back(MenuItem(50, L"Gamma 1.0"));
    m_effectSubmenu.push_back(MenuItem(51, L"Gamma 1.2"));
    m_effectSubmenu.push_back(MenuItem(52, L"Exposure +1"));
    m_effectSubmenu.push_back(MenuItem(53, L"Exposure -1"));
    m_effectSubmenu.push_back(MenuItem(54, L"Reset Exposure"));
    m_effectSubmenu.push_back(MenuItem(55, L"Old Film"));
    m_effectSubmenu.push_back(MenuItem(56, L"Terminator"));
    m_effectSubmenu.push_back(MenuItem(57, L"Black & White"));
    m_effectSubmenu.push_back(MenuItem(58, L"Sepia B&W"));
    m_effectSubmenu.push_back(MenuItem(59, L"Invert Colors"));
    effectsItem.subItems = m_effectSubmenu;
    m_menuItems.push_back(effectsItem);

    m_menuItems.push_back(MenuItem(true)); //separator
    m_menuItems.push_back(MenuItem(1007, L"prizy"));
    m_menuItems.push_back(MenuItem(true)); //separator
    m_menuItems.push_back(MenuItem(99, L"exose"));
}

int KonataMenu::GetMenuWidth(const std::vector<MenuItem>& items) {
    int maxWidth = 200;
    Graphics graphics(m_hwnd);
    Font font(L"Segoe UI", 12);
    StringFormat format;

    for (const auto& item : items) {
        if (!item.isSeparator) {
            RectF bounds;
            graphics.MeasureString(item.text.c_str(), item.text.length(), &font, PointF(0, 0), &format, &bounds);
            int width = (int)bounds.Width + 40;
            if (width > maxWidth) maxWidth = width;
        }
    }
    return maxWidth;
}

void GetDesktopRect(RECT* rect) {
    rect->left = 0;
    rect->top = 0;
    rect->right = GetSystemMetrics(SM_CXSCREEN);
    rect->bottom = GetSystemMetrics(SM_CYSCREEN);
}

void KonataMenu::DrawMenu(HDC hdc) {
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

    int menuWidth = GetMenuWidth(m_menuItems);
    int menuHeight = m_menuItems.size() * MENU_ITEM_HEIGHT;

    //clear bg
    SolidBrush clearBrush(Color(40, 40, 45));
    graphics.FillRectangle(&clearBrush, 0, 0, menuWidth, menuHeight);

    //draw background with rounded rect
    SolidBrush bgBrush(MENU_BG_COLOR);
    graphics.FillRectangle(&bgBrush, 0, 0, menuWidth, menuHeight);

    Pen borderPen(MENU_BORDER_COLOR, 1);
    graphics.DrawRectangle(&borderPen, 0, 0, menuWidth - 1, menuHeight - 1);

    //draw menu items
    Font font(L"Segoe UI", 12, FontStyleRegular);
    SolidBrush textBrush(MENU_TEXT_COLOR);
    SolidBrush hoverBrush(MENU_HOVER_BG);
    SolidBrush disabledBrush(MENU_DISABLED_TEXT);
    SolidBrush hoverTextBrush(MENU_HOVER_TEXT);
    Pen separatorPen(MENU_SEPARATOR);
    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    format.SetLineAlignment(StringAlignmentCenter);

    for (size_t i = 0; i < m_menuItems.size(); i++) {
        const auto& item = m_menuItems[i];
        int y = i * MENU_ITEM_HEIGHT;

        if (item.isSeparator) {
            graphics.DrawLine(&separatorPen, 10, y + MENU_ITEM_HEIGHT / 2,
                menuWidth - 10, y + MENU_ITEM_HEIGHT / 2);
            continue;
        }

        //highlight selected item
        if ((int)i == m_selectedIndex && !m_inSubmenu) {
            Rect itemRect(2, y + 1, menuWidth - 4, MENU_ITEM_HEIGHT - 2);
            graphics.FillRectangle(&hoverBrush, itemRect);
            Pen itemBorder(MENU_BORDER_COLOR, 1);
            graphics.DrawRectangle(&itemBorder, itemRect);
        }

        //draw text
        RectF textRect(15, y, menuWidth - 30, MENU_ITEM_HEIGHT);
        Brush* brush = item.isDisabled ? (Brush*)&disabledBrush :
            ((int)i == m_selectedIndex && !m_inSubmenu) ? (Brush*)&hoverTextBrush : (Brush*)&textBrush;

        graphics.DrawString(item.text.c_str(), -1, &font, textRect, &format, brush);

        //draw arrow for submenu
        if (item.id == 1000) {
            Font arrowFont(L"Segoe UI", 14, FontStyleBold);
            RectF arrowRect(menuWidth - 25, y, 20, MENU_ITEM_HEIGHT);
            graphics.DrawString(L"▶", -1, &arrowFont, arrowRect, &format, brush);
        }
    }

    //draw submenu if open
    if (m_inSubmenu && m_selectedIndex >= 0 && m_selectedIndex < (int)m_menuItems.size() &&
        m_menuItems[m_selectedIndex].id == 1000 && !m_effectSubmenu.empty()) {

        int submenuX = menuWidth;
        int submenuY = m_selectedIndex * MENU_ITEM_HEIGHT;

        //submenu checking
        RECT desktop;
        GetDesktopRect(&desktop);
        if (submenuX + GetMenuWidth(m_effectSubmenu) > desktop.right) {
            submenuX = menuWidth - GetMenuWidth(m_effectSubmenu);
        }
        if (submenuY + (int)m_effectSubmenu.size() * MENU_ITEM_HEIGHT > desktop.bottom) {
            submenuY = desktop.bottom - (int)m_effectSubmenu.size() * MENU_ITEM_HEIGHT;
        }
        if (submenuY < 0) submenuY = 0;

        DrawSubmenu(hdc, m_effectSubmenu, submenuX, submenuY, m_subSelectedIndex);
    }
}

void KonataMenu::DrawSubmenu(HDC hdc, const std::vector<MenuItem>& items, int x, int y, int selected) {
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

    int submenuWidth = GetMenuWidth(items);
    int submenuHeight = items.size() * MENU_ITEM_HEIGHT;

    SolidBrush bgBrush(MENU_BG_COLOR);
    graphics.FillRectangle(&bgBrush, x, y, submenuWidth, submenuHeight);

    //rect
    Pen borderPen(MENU_BORDER_COLOR, 2);
    graphics.DrawRectangle(&borderPen, x, y, submenuWidth - 1, submenuHeight - 1);

    Font font(L"Segoe UI", 12);
    SolidBrush textBrush(MENU_TEXT_COLOR);
    SolidBrush hoverBrush(MENU_HOVER_BG);
    SolidBrush hoverTextBrush(MENU_HOVER_TEXT);
    Pen separatorPen(MENU_SEPARATOR);
    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    format.SetLineAlignment(StringAlignmentCenter);

    for (size_t i = 0; i < items.size(); i++) {
        const auto& item = items[i];
        int itemY = y + i * MENU_ITEM_HEIGHT;

        if (item.isSeparator) {
            graphics.DrawLine(&separatorPen, x + 10, itemY + MENU_ITEM_HEIGHT / 2,
                x + submenuWidth - 10, itemY + MENU_ITEM_HEIGHT / 2);
            continue;
        }

        //highlightn'
        if ((int)i == selected) {
            Rect itemRect(x + 2, itemY + 1, submenuWidth - 4, MENU_ITEM_HEIGHT - 2);
            graphics.FillRectangle(&hoverBrush, itemRect);
            Pen itemBorder(MENU_BORDER_COLOR, 1);
            graphics.DrawRectangle(&itemBorder, itemRect);
        }

        //text
        RectF textRect(x + 15, itemY, submenuWidth - 30, MENU_ITEM_HEIGHT);
        Brush* brush = ((int)i == selected) ? (Brush*)&hoverTextBrush : (Brush*)&textBrush;
        graphics.DrawString(item.text.c_str(), -1, &font, textRect, &format, brush);
    }
}

void KonataMenu::HandleClick(int x, int y) {
    if (!m_isVisible) return;

    if (m_inSubmenu && m_selectedIndex >= 0 &&
        m_selectedIndex < (int)m_menuItems.size() &&
        m_menuItems[m_selectedIndex].id == 1000) {

        int menuWidth = GetMenuWidth(m_menuItems);

        //submenu sheesh
        int submenuX = menuWidth;
        int submenuY = m_selectedIndex * MENU_ITEM_HEIGHT;
        int submenuWidth = GetMenuWidth(m_effectSubmenu);

        RECT desktop;
        GetDesktopRect(&desktop);
        if (submenuX + submenuWidth > desktop.right) {
            submenuX = menuWidth - submenuWidth;
        }
        if (submenuY + (int)m_effectSubmenu.size() * MENU_ITEM_HEIGHT > desktop.bottom) {
            submenuY = desktop.bottom - (int)m_effectSubmenu.size() * MENU_ITEM_HEIGHT;
        }
        if (submenuY < 0) submenuY = 0;

        //click check
        if (x >= submenuX && x < submenuX + submenuWidth &&
            y >= submenuY && y < submenuY + (int)m_effectSubmenu.size() * MENU_ITEM_HEIGHT) {
            //index
            int visualIndex = (y - submenuY) / MENU_ITEM_HEIGHT;
            //fuck separators
            int arrayIndex = 0;
            int visualCounter = 0;

            for (size_t i = 0; i < m_effectSubmenu.size(); i++) {
                if (!m_effectSubmenu[i].isSeparator) {
                    if (visualCounter == visualIndex) {
                        arrayIndex = i;
                        break;
                    }
                    visualCounter++;
                }
                else {
                    if (visualCounter == visualIndex) {
                        return;
                    }
                    visualCounter++;
                }
            }

            if (arrayIndex >= 0 && arrayIndex < (int)m_effectSubmenu.size()) {
                const auto& item = m_effectSubmenu[arrayIndex];
                if (!item.isSeparator && !item.isDisabled) {
                    HandleSubmenuClick(m_effectSubmenu, item.id);
                    Hide();
                    return;
                }
            }
        }
    }

    int menuWidth = GetMenuWidth(m_menuItems);
    int menuHeight = (int)m_menuItems.size() * MENU_ITEM_HEIGHT;

    if (x >= 0 && x < menuWidth && y >= 0 && y < menuHeight) {
        int index = y / MENU_ITEM_HEIGHT;
        if (index >= 0 && index < (int)m_menuItems.size()) {
            const auto& item = m_menuItems[index];
            if (item.id == 1000) {
                if (m_inSubmenu && m_selectedIndex == index) {
                    m_inSubmenu = false;
                    m_selectedIndex = -1;
                    m_subSelectedIndex = -1;
                }
                else {
                    m_inSubmenu = true;
                    m_selectedIndex = index;
                    m_subSelectedIndex = -1;
                }
                InvalidateRect(m_hwnd, nullptr, TRUE);
            }
            else if (!item.isSeparator && !item.isDisabled) {
                HandleSubmenuClick(m_menuItems, item.id);
                Hide();
            }
        }
    }
    else {
        Hide();
    }
}

void KonataMenu::HandleSubmenuClick(const std::vector<MenuItem>& items, int id) {
    switch (id) {
    case 40: //Reset All Effects
    case 41: //Warm
    case 42: //Cool
    case 43: //Sepia 50%
    case 44: //Sepia 100%
    case 45: //Vintage
    case 46: //Hue Shift +
    case 47: //Hue Shift -
    case 48: //Reset Hue
    case 49: //Gamma 0.8
    case 50: //Gamma 1.0
    case 51: //Gamma 1.2
    case 52: //Exposure +1
    case 53: //Exposure -1
    case 54: //Reset Exposure
    case 55: //Old Film
    case 56: //Terminator
    case 57: //Black & White
    case 58: //Sepia B&W
    case 59: //Invert Colors
        if (m_onApplyEffect) {
            m_onApplyEffect(m_callbackTarget, id);
        }
        if (m_onEffectChange) {
            m_onEffectChange(m_callbackTarget);
        }
        return;
    }
    switch (id) {
    case 1: if (m_onScaleChange) m_onScaleChange(m_callbackTarget, 50); break;
    case 2: if (m_onScaleChange) m_onScaleChange(m_callbackTarget, 75); break;
    case 3: if (m_onScaleChange) m_onScaleChange(m_callbackTarget, 100); break;
    case 4: if (m_onScaleChange) m_onScaleChange(m_callbackTarget, 125); break;
    case 5: if (m_onScaleChange) m_onScaleChange(m_callbackTarget, 150); break;
    case 10: if (m_onFpsChange) m_onFpsChange(m_callbackTarget, 15); break;
    case 11: if (m_onFpsChange) m_onFpsChange(m_callbackTarget, 24); break;
    case 12: if (m_onFpsChange) m_onFpsChange(m_callbackTarget, 30); break;
    case 13: if (m_onFpsChange) m_onFpsChange(m_callbackTarget, 60); break;
    case 20: if (m_onSaturationChange) m_onSaturationChange(m_callbackTarget, 1.2f); break;
    case 21: if (m_onSaturationChange) m_onSaturationChange(m_callbackTarget, 1.5f); break;
    case 22: if (m_onSaturationChange) m_onSaturationChange(m_callbackTarget, 2.0f); break;
    case 23: if (m_onSaturationChange) m_onSaturationChange(m_callbackTarget, 1.0f); break;
    case 30: if (m_onFrameSizeChange) m_onFrameSizeChange(m_callbackTarget, 160, 120); break;
    case 31: if (m_onFrameSizeChange) m_onFrameSizeChange(m_callbackTarget, 320, 240); break;
    case 32: if (m_onFrameSizeChange) m_onFrameSizeChange(m_callbackTarget, 480, 360); break;
    case 33: if (m_onFrameSizeChange) m_onFrameSizeChange(m_callbackTarget, 640, 480); break;
    case 1007: if (m_onPrize) m_onPrize(m_callbackTarget); break;
    case 99: if (m_onExit) m_onExit(m_callbackTarget); break;
    }
}

void KonataMenu::Show(HWND parent, int x, int y) {
    m_parentWnd = parent;
    m_menuPos.x = x;
    m_menuPos.y = y;
    m_isVisible = true;
    m_inSubmenu = false;
    m_selectedIndex = -1;
    m_subSelectedIndex = -1;

    BuildMenu();

    int menuWidth = GetMenuWidth(m_menuItems);
    int menuHeight = m_menuItems.size() * MENU_ITEM_HEIGHT;

    //submenu tooo
    int totalWidth = menuWidth;
    if (!m_effectSubmenu.empty()) {
        totalWidth += GetMenuWidth(m_effectSubmenu) + 10;
    }

    //adjust pos
    RECT desktop;
    GetDesktopRect(&desktop);
    if (x + totalWidth > desktop.right) {
        x = desktop.right - totalWidth - 10;
    }
    if (x < 0) x = 10;
    if (y + menuHeight > desktop.bottom) {
        y = desktop.bottom - menuHeight - 10;
    }
    if (y < 0) y = 10;

    //window size
    SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, totalWidth + 20, menuHeight + 20,
        SWP_SHOWWINDOW);
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    SetCapture(m_hwnd);
}

void KonataMenu::Hide() {
    m_isVisible = false;
    m_inSubmenu = false;
    m_selectedIndex = -1;
    ReleaseCapture();
    ShowWindow(m_hwnd, SW_HIDE);
}

LRESULT CALLBACK KonataMenu::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    KonataMenu* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<KonataMenu*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else {
        pThis = reinterpret_cast<KonataMenu*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (pThis) return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT KonataMenu::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rect;
        GetClientRect(hWnd, &rect);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        HBRUSH bgBrush = CreateSolidBrush(RGB(40, 40, 45));
        FillRect(memDC, &rect, bgBrush);
        DeleteObject(bgBrush);
        DrawMenu(memDC);
        BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (m_isVisible) {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            if (m_inSubmenu && m_selectedIndex >= 0 &&
                m_selectedIndex < (int)m_menuItems.size() &&
                m_menuItems[m_selectedIndex].id == 1000) {

                int menuWidth = GetMenuWidth(m_menuItems);
                int submenuX = menuWidth;
                int submenuY = m_selectedIndex * MENU_ITEM_HEIGHT;
                int submenuWidth = GetMenuWidth(m_effectSubmenu);

                RECT desktop;
                GetDesktopRect(&desktop);
                if (submenuX + submenuWidth > desktop.right) {
                    submenuX = menuWidth - submenuWidth;
                }
                if (submenuY + (int)m_effectSubmenu.size() * MENU_ITEM_HEIGHT > desktop.bottom) {
                    submenuY = desktop.bottom - (int)m_effectSubmenu.size() * MENU_ITEM_HEIGHT;
                }
                if (submenuY < 0) submenuY = 0;

                if (pt.x >= submenuX && pt.x < submenuX + submenuWidth &&
                    pt.y >= submenuY && pt.y < submenuY + (int)m_effectSubmenu.size() * MENU_ITEM_HEIGHT) {

                    int visualIndex = (pt.y - submenuY) / MENU_ITEM_HEIGHT;

                    int arrayIndex = 0;
                    int visualCounter = 0;

                    for (size_t i = 0; i < m_effectSubmenu.size(); i++) {
                        if (visualCounter == visualIndex) {
                            arrayIndex = i;
                            break;
                        }
                        visualCounter++;
                    }

                    if (arrayIndex >= 0 && arrayIndex < (int)m_effectSubmenu.size() &&
                        !m_effectSubmenu[arrayIndex].isSeparator &&
                        arrayIndex != m_subSelectedIndex) {
                        m_subSelectedIndex = arrayIndex;
                        InvalidateRect(m_hwnd, nullptr, TRUE);
                    }
                }
                else if (m_subSelectedIndex != -1) {
                    m_subSelectedIndex = -1;
                    InvalidateRect(m_hwnd, nullptr, TRUE);
                }
            }

            int menuWidth = GetMenuWidth(m_menuItems);
            if (pt.x >= 0 && pt.x < menuWidth &&
                pt.y >= 0 && pt.y < (int)m_menuItems.size() * MENU_ITEM_HEIGHT) {
                int newIndex = pt.y / MENU_ITEM_HEIGHT;
                if (newIndex != m_selectedIndex && !m_menuItems[newIndex].isSeparator) {
                    m_selectedIndex = newIndex;
                    InvalidateRect(m_hwnd, nullptr, TRUE);
                }
            }
            else if (m_selectedIndex != -1 && !m_inSubmenu) {
                m_selectedIndex = -1;
                InvalidateRect(m_hwnd, nullptr, TRUE);
            }
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        HandleClick(pt.x, pt.y);
        return 0;
    }

    case WM_KILLFOCUS:
    case WM_ACTIVATE: {
        if (LOWORD(wParam) == WA_INACTIVE) {
            Hide();
        }
        return 0;
    }

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}