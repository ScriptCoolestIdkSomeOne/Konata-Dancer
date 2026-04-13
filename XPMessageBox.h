#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <functional>
#include <cstdlib> 
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")

namespace XPMessageBox {

    using namespace Gdiplus;

    enum class IconType {
        None,
        Information,
        Question,
        Warning,
        Error
    };

    enum class ButtonType {
        OK,
        OKCancel,
        YesNo,
        YesNoCancel,
        RetryCancel,
        AbortRetryIgnore
    };

    enum class DialogResult {
        OK = IDOK,
        Cancel = IDCANCEL,
        Yes = IDYES,
        No = IDNO,
        Abort = IDABORT,
        Retry = IDRETRY,
        Ignore = IDIGNORE
    };

    enum class WindowMode {
        Dialog,
        Regular
    };

    struct ButtonAction {
        DialogResult buttonId;
        std::function<void()> action;
    };

    const Gdiplus::Color XP_TITLE_START(16, 56, 128);
    const Gdiplus::Color XP_TITLE_END(49, 106, 197);
    const Gdiplus::Color XP_INACTIVE_START(93, 105, 118);
    const Gdiplus::Color XP_INACTIVE_END(140, 150, 160);
    const Gdiplus::Color XP_BTN_TOP(240, 245, 250);
    const Gdiplus::Color XP_BTN_BOTTOM(190, 205, 225);
    const Gdiplus::Color XP_BTN_HOVER_TOP(255, 255, 255);
    const Gdiplus::Color XP_BTN_HOVER_BOTTOM(210, 225, 245);
    const Gdiplus::Color XP_BTN_PRESS_TOP(140, 160, 190);
    const Gdiplus::Color XP_BTN_PRESS_BOTTOM(100, 120, 150);
    const Gdiplus::Color XP_BORDER_COLOR(58, 110, 165);
    const Gdiplus::Color XP_BORDER_LIGHT(255, 255, 255);
    const Gdiplus::Color XP_BORDER_DARK(49, 106, 197);

    struct ButtonInfo {
        RECT rect = { 0 };
        bool hover = false;
        bool pressed = false;
        int id = 0;
        std::wstring text;
    };

    struct MsgBoxState {
        bool isDragging = false;
        POINT dragOffset = { 0, 0 };
        bool isActive = true;
        bool isResizing = false;
        int resizeEdge = 0;

        RECT closeRect = { 0 };
        bool closeHover = false;
        bool closePressed = false;

        std::vector<ButtonInfo> buttons;
        ButtonType buttonType = ButtonType::OK;
        IconType iconType = IconType::Question;
        DialogResult defaultButton = DialogResult::OK;
        DialogResult result = DialogResult::Cancel;

        int minWidth = 250;
        int minHeight = 120;
        int width = 300;
        int height = 140;
        int titleBarHeight = 28;
        int borderSize = 2;
        int buttonMargin = 20;
        int buttonWidth = 80;
        int buttonHeight = 26;
        float textScale = 1.0f;
        float buttonScale = 1.0f;
        int titleFontSize = 14;
        int messageFontSize = 12;
        int buttonFontSize = 14;
        int iconSize = 32;
        int messageMarginLeft = 58;

        std::wstring message;
        std::wstring title;
        std::function<void(DialogResult)> callback = nullptr;

        std::vector<ButtonAction> buttonActions;

        WindowMode mode = WindowMode::Dialog;

        static bool classRegistered;
        static int windowCount;

        void AddButtons() {
            buttons.clear();

            switch (buttonType) {
            case ButtonType::OK:
                buttons.push_back({ {}, false, false, IDOK, L"OK" });
                break;
            case ButtonType::OKCancel:
                buttons.push_back({ {}, false, false, IDOK, L"OK" });
                buttons.push_back({ {}, false, false, IDCANCEL, L"Cancel" });
                break;
            case ButtonType::YesNo:
                buttons.push_back({ {}, false, false, IDYES, L"Yes" });
                buttons.push_back({ {}, false, false, IDNO, L"No" });
                break;
            case ButtonType::YesNoCancel:
                buttons.push_back({ {}, false, false, IDYES, L"Yes" });
                buttons.push_back({ {}, false, false, IDNO, L"No" });
                buttons.push_back({ {}, false, false, IDCANCEL, L"Cancel" });
                break;
            case ButtonType::RetryCancel:
                buttons.push_back({ {}, false, false, IDRETRY, L"Retry" });
                buttons.push_back({ {}, false, false, IDCANCEL, L"Cancel" });
                break;
            case ButtonType::AbortRetryIgnore:
                buttons.push_back({ {}, false, false, IDABORT, L"Abort" });
                buttons.push_back({ {}, false, false, IDRETRY, L"Retry" });
                buttons.push_back({ {}, false, false, IDIGNORE, L"Ignore" });
                break;
            }
        }

        void CalculateButtonPositions() {
            int scaledWidth = static_cast<int>(buttonWidth * buttonScale);
            int scaledHeight = static_cast<int>(buttonHeight * buttonScale);
            int totalWidth = buttons.size() * scaledWidth + (buttons.size() - 1) * 10;
            int startX = (width - totalWidth) / 2;
            int btnY = height - scaledHeight - buttonMargin;

            for (size_t i = 0; i < buttons.size(); i++) {
                int btnX = startX + i * (scaledWidth + 10);
                buttons[i].rect = { btnX, btnY, btnX + scaledWidth, btnY + scaledHeight };
            }
        }

        void CalculateSize(HDC hdc) {
            int msgFontSize = static_cast<int>(12 * textScale);
            HFONT hFont = CreateFont(msgFontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

            //calculator for some shit
            int iconArea = 40 + static_cast<int>(20 * textScale);
            RECT msgRect = { iconArea + 10, 45, 1000, 0 };
            DrawTextW(hdc, message.c_str(), -1, &msgRect, DT_CALCRECT | DT_LEFT | DT_WORDBREAK);

            //sheesh so this is a title resizing function???
            int titleFontSize = static_cast<int>(14 * textScale);
            HFONT hTitleFont = CreateFont(titleFontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            HFONT hOldTitleFont = (HFONT)SelectObject(hdc, hTitleFont);
            RECT titleRect = { 0, 0, 0, 0 };
            DrawTextW(hdc, title.c_str(), -1, &titleRect, DT_CALCRECT | DT_LEFT | DT_SINGLELINE);
            SelectObject(hdc, hOldTitleFont);
            DeleteObject(hTitleFont);

            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);

            //math?
            int requiredWidth = max(msgRect.right - msgRect.left + iconArea + 20, titleRect.right - titleRect.left + 100);
            requiredWidth = max(requiredWidth, minWidth);

            //more math
            int requiredHeight = msgRect.bottom - msgRect.top + 80;
            requiredHeight = max(requiredHeight, minHeight);

            //you can't do this young man
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);

            width = min(requiredWidth, screenWidth * 9 / 10);
            height = min(requiredHeight, screenHeight * 9 / 10);

            CalculateButtonPositions();

            int closeSize = static_cast<int>(22 * textScale);
            int closeX = width - closeSize - static_cast<int>(8 * textScale);
            int closeY = (titleBarHeight - closeSize) / 2;
            closeRect = { closeX, closeY, closeX + closeSize, closeY + closeSize };
        }

        void UpdateButtonHover(POINT pt) {
            for (auto& btn : buttons) {
                bool hover = PtInRect(&btn.rect, pt);
                if (hover != btn.hover) {
                    btn.hover = hover;
                }
            }
        }

        ButtonInfo* GetButtonAtPoint(POINT pt) {
            for (auto& btn : buttons) {
                if (PtInRect(&btn.rect, pt)) {
                    return &btn;
                }
            }
            return nullptr;
        }

        int GetScaledButtonWidth() const {
            return static_cast<int>(buttonWidth * buttonScale);
        }

        int GetScaledButtonHeight() const {
            return static_cast<int>(buttonHeight * buttonScale);
        }

        int GetScaledTitleFontSize() const {
            return static_cast<int>(titleFontSize * textScale);
        }

        int GetScaledMessageFontSize() const {
            return static_cast<int>(messageFontSize * textScale);
        }

        int GetScaledButtonFontSize() const {
            return static_cast<int>(buttonFontSize * textScale);
        }

        int GetScaledIconSize() const {
            return static_cast<int>(iconSize * textScale);
        }

        int GetScaledMessageMarginLeft() const {
            return static_cast<int>(messageMarginLeft + (textScale - 1.0f) * 20);
        }
    };

    bool MsgBoxState::classRegistered = false;
    int MsgBoxState::windowCount = 0;

    //additional shit
    inline void DrawGradientRect(HDC hdc, RECT rect, Gdiplus::Color color1, Gdiplus::Color color2, bool horizontal = true) {
        Graphics graphics(hdc);
        LinearGradientBrush brush(Rect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top),
            color1, color2, horizontal ? LinearGradientModeHorizontal : LinearGradientModeVertical);
        graphics.FillRectangle(&brush, (INT)rect.left, (INT)rect.top, (INT)rect.right - (INT)rect.left, (INT)rect.bottom - (INT)rect.top);
    }

    inline void DrawXPButton(HDC hdc, const ButtonInfo& btn, bool isDefault = false, float buttonScale = 1.0f, float textScale = 1.0f) {
        Gdiplus::Color topColor, bottomColor;

        if (btn.pressed) {
            topColor = XP_BTN_PRESS_TOP;
            bottomColor = XP_BTN_PRESS_BOTTOM;
        }
        else if (btn.hover) {
            topColor = XP_BTN_HOVER_TOP;
            bottomColor = XP_BTN_HOVER_BOTTOM;
        }
        else {
            topColor = XP_BTN_TOP;
            bottomColor = XP_BTN_BOTTOM;
        }

        DrawGradientRect(hdc, btn.rect, topColor, bottomColor, false);

        //rect shitty
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 128));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, btn.rect.left, btn.rect.top, btn.rect.right, btn.rect.bottom);

        //3d magek
        if (!btn.pressed) {
            HPEN hLightPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            SelectObject(hdc, hLightPen);
            MoveToEx(hdc, btn.rect.left + 1, btn.rect.bottom - 1, NULL);
            LineTo(hdc, btn.rect.left + 1, btn.rect.top + 1);
            LineTo(hdc, btn.rect.right - 1, btn.rect.top + 1);
            DeleteObject(hLightPen);
        }

        //fat font
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));

        int fontSize = static_cast<int>(14 * textScale);
        HFONT hFont = CreateFont(fontSize, 0, 0, 0, isDefault ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        DrawTextW(hdc, btn.text.c_str(), -1, const_cast<RECT*>(&btn.rect), DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hFont);
        DeleteObject(hPen);
    }

    inline bool PlayMP3(const std::wstring& filePath, bool wait = false, bool loop = false) {
        std::wstring cmd = L"open \"" + filePath + L"\" type mpegvideo alias mp3_temp";

        if (mciSendStringW(cmd.c_str(), NULL, 0, NULL) != 0) {
            cmd = L"open \"" + filePath + L"\" type MPEGVideo alias mp3_temp";
            if (mciSendStringW(cmd.c_str(), NULL, 0, NULL) != 0) {
                return false;
            }
        }

        cmd = L"play mp3_temp";
        if (loop) cmd += L" repeat";
        if (wait) cmd += L" wait";

        return (mciSendStringW(cmd.c_str(), NULL, 0, NULL) == 0);
    }

    inline void StopMP3() {
        mciSendStringW(L"stop mp3_temp", NULL, 0, NULL);
        mciSendStringW(L"close mp3_temp", NULL, 0, NULL);
    }

    inline void DrawCloseBtn(HDC hdc, RECT rect, bool pressed, bool hover) {
        Gdiplus::Color topColor, bottomColor;

        if (pressed) {
            topColor = Gdiplus::Color(180, 60, 60);
            bottomColor = Gdiplus::Color(140, 40, 40);
        }
        else if (hover) {
            topColor = Gdiplus::Color(255, 100, 100);
            bottomColor = Gdiplus::Color(200, 70, 70);
        }
        else {
            topColor = Gdiplus::Color(220, 100, 100);
            bottomColor = Gdiplus::Color(180, 70, 70);
        }

        DrawGradientRect(hdc, rect, topColor, bottomColor, false);

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(128, 0, 0));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));

        HFONT hFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Marlett");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        DrawTextW(hdc, L"r", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hFont);
        DeleteObject(hPen);
    }

    inline void DrawIcon(HDC hdc, RECT rect, IconType iconType, float textScale = 1.0f) {
        HBRUSH hIconBrush;
        std::wstring iconText;

        switch (iconType) {
        case IconType::Information:
            hIconBrush = CreateSolidBrush(RGB(0, 100, 200));
            iconText = L"i";
            break;
        case IconType::Question:
            hIconBrush = CreateSolidBrush(RGB(0, 0, 200));
            iconText = L"?";
            break;
        case IconType::Warning:
            hIconBrush = CreateSolidBrush(RGB(200, 150, 0));
            iconText = L"!";
            break;
        case IconType::Error:
            hIconBrush = CreateSolidBrush(RGB(200, 0, 0));
            iconText = L"X";
            break;
        default:
            return;
        }

        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hIconBrush);
        Ellipse(hdc, rect.left, rect.top, rect.right, rect.bottom);

        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        int fontSize = static_cast<int>(28 * textScale);
        HFONT hIconFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hIconFont);
        DrawTextW(hdc, iconText.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hIconFont);
        DeleteObject(hIconBrush);
    }

    inline void DrawXPFrame(HDC hdc, RECT rect) {
        // Основная рамка
        HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(58, 110, 165));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        //rect
        Rectangle(hdc, 0, 0, rect.right, rect.bottom);

        //outside light rect
        HPEN hLightPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        SelectObject(hdc, hLightPen);
        Rectangle(hdc, 1, 1, rect.right - 1, rect.bottom - 1);

        //inner darkrect
        HPEN hDarkPen = CreatePen(PS_SOLID, 1, RGB(49, 106, 197));
        SelectObject(hdc, hDarkPen);
        Rectangle(hdc, 2, 2, rect.right - 2, rect.bottom - 2);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBorderPen);
        DeleteObject(hLightPen);
        DeleteObject(hDarkPen);
    }

    inline int GetResizeEdge(POINT pt, RECT rect, int borderSize) {
        int edge = 0;
        if (pt.x < rect.left + borderSize) edge |= 1;
        if (pt.x > rect.right - borderSize) edge |= 2;
        if (pt.y < rect.top + borderSize) edge |= 4;
        if (pt.y > rect.bottom - borderSize) edge |= 8;
        return edge;
    }

    //window proc
    inline LRESULT CALLBACK MsgBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        static MsgBoxState* state = nullptr;

        if (msg == WM_NCCREATE) {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            state = (MsgBoxState*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        state = (MsgBoxState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (!state && msg != WM_DESTROY) return DefWindowProc(hwnd, msg, wParam, lParam);

        switch (msg) {
        case WM_CREATE: {
            SetTimer(hwnd, 1, 50, NULL);
            return 0;
        }

        case WM_SIZE: {
            if (state) {
                state->width = LOWORD(lParam);
                state->height = HIWORD(lParam);

                state->CalculateButtonPositions();

                int closeSize = 22;
                int closeX = state->width - closeSize - 8;
                int closeY = (state->titleBarHeight - closeSize) / 2;
                state->closeRect = { closeX, closeY, closeX + closeSize, closeY + closeSize };

                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        }

        case WM_GETMINMAXINFO: {
            if (state) {
                MINMAXINFO* mmi = (MINMAXINFO*)lParam;
                mmi->ptMinTrackSize.x = state->minWidth;
                mmi->ptMinTrackSize.y = state->minHeight;
            }
            break;
        }

        case WM_MOUSEMOVE: {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };

            bool newCloseHover = PtInRect(&state->closeRect, pt);
            if (newCloseHover != state->closeHover) {
                state->closeHover = newCloseHover;
                InvalidateRect(hwnd, NULL, FALSE);
            }

            state->UpdateButtonHover(pt);

            if (state->isDragging) {
                POINT cursor;
                GetCursorPos(&cursor);
                SetWindowPos(hwnd, NULL, cursor.x - state->dragOffset.x,
                    cursor.y - state->dragOffset.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };

            if (PtInRect(&state->closeRect, pt)) {
                state->closePressed = true;
                InvalidateRect(hwnd, NULL, FALSE);
                SetCapture(hwnd);
            }
            else if (ButtonInfo* btn = state->GetButtonAtPoint(pt)) {
                btn->pressed = true;
                InvalidateRect(hwnd, NULL, FALSE);
                SetCapture(hwnd);
            }
            else if (pt.y < state->titleBarHeight) {
                state->isDragging = true;
                state->dragOffset = pt;
                SetCapture(hwnd);
            }
            break;
        }

        case WM_LBUTTONUP: {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };

            if (state->closePressed) {
                if (PtInRect(&state->closeRect, pt)) {
                    state->result = DialogResult::Cancel;
                    DestroyWindow(hwnd);
                }
                state->closePressed = false;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            else if (ButtonInfo* btn = state->GetButtonAtPoint(pt)) {
                if (btn->pressed) {
                    DialogResult result = static_cast<DialogResult>(btn->id);
                    state->result = result;

                    //call callback if exists maybe
                    if (state->callback) {
                        state->callback(result);
                    }

                    //apply
                    for (const auto& action : state->buttonActions) {
                        if (action.buttonId == result && action.action) {
                            action.action();
                        }
                    }

                    DestroyWindow(hwnd);
                }
                btn->pressed = false;
                InvalidateRect(hwnd, NULL, FALSE);
            }

            state->isDragging = false;
            ReleaseCapture();
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            HBRUSH hBgBrush = CreateSolidBrush(RGB(236, 233, 216));
            FillRect(hdc, &clientRect, hBgBrush);
            DeleteObject(hBgBrush);

            DrawXPFrame(hdc, clientRect);

            RECT titleRect = { state->borderSize, state->borderSize,
                              clientRect.right - state->borderSize, state->titleBarHeight + state->borderSize };

            if (state->isActive) {
                DrawGradientRect(hdc, titleRect, XP_TITLE_START, XP_TITLE_END, true);
            }
            else {
                DrawGradientRect(hdc, titleRect, XP_INACTIVE_START, XP_INACTIVE_END, true);
            }

            //title text
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            int titleFontSize = static_cast<int>(14 * state->textScale);
            HFONT hTitleFont = CreateFont(titleFontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hTitleFont);
            RECT textRect = titleRect;
            textRect.left += 10;
            textRect.right -= static_cast<int>(35 * state->textScale);
            DrawTextW(hdc, state->title.c_str(), -1, &textRect, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

            DrawCloseBtn(hdc, state->closeRect, state->closePressed, state->closeHover);

            int iconSize = static_cast<int>(32 * state->textScale);
            int iconY = 45;
            RECT iconRect = { 18, iconY, 18 + iconSize, iconY + iconSize };
            DrawIcon(hdc, iconRect, state->iconType, state->textScale);

            //msgs
            int msgFontSize = static_cast<int>(12 * state->textScale);
            HFONT hMsgFont = CreateFont(msgFontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            SelectObject(hdc, hMsgFont);
            SetTextColor(hdc, RGB(0, 0, 0));
            int msgMarginLeft = 18 + iconSize + 12;
            RECT msgRect = { msgMarginLeft, 45, state->width - 15, state->height - 65 };
            DrawTextW(hdc, state->message.c_str(), -1, &msgRect, DT_LEFT | DT_WORDBREAK);

            //btns
            for (const auto& btn : state->buttons) {
                bool isDefault = (static_cast<int>(state->defaultButton) == btn.id);
                DrawXPButton(hdc, btn, isDefault, state->buttonScale, state->textScale);
            }

            //line under the title
            HPEN hLinePen = CreatePen(PS_SOLID, 1, RGB(49, 106, 197));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hLinePen);
            MoveToEx(hdc, titleRect.left + 2, titleRect.bottom, NULL);
            LineTo(hdc, titleRect.right - 2, titleRect.bottom);

            HPEN hLightLinePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            SelectObject(hdc, hLightLinePen);
            MoveToEx(hdc, titleRect.left + 2, titleRect.bottom + 1, NULL);
            LineTo(hdc, titleRect.right - 2, titleRect.bottom + 1);

            SelectObject(hdc, hOldPen);
            DeleteObject(hLinePen);
            DeleteObject(hLightLinePen);

            SelectObject(hdc, hOldFont);
            DeleteObject(hTitleFont);
            DeleteObject(hMsgFont);

            EndPaint(hwnd, &ps);
            break;
        }

        case WM_TIMER: {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);

            bool newCloseHover = PtInRect(&state->closeRect, pt);
            if (newCloseHover != state->closeHover) {
                state->closeHover = newCloseHover;
                InvalidateRect(hwnd, NULL, FALSE);
            }

            state->UpdateButtonHover(pt);
            break;
        }

        case WM_ACTIVATE: {
            if (state) {
                state->isActive = (LOWORD(wParam) != WA_INACTIVE);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        }

        case WM_CLOSE: {
            if (state) {
                state->result = DialogResult::Cancel;
                DestroyWindow(hwnd);
            }
            break;
        }

        case WM_DESTROY: {
            KillTimer(hwnd, 1);
            if (state) {
                PostQuitMessage(static_cast<int>(state->result));
            }
            break;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    inline ULONG_PTR InitGdiPlus() {
        GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        return gdiplusToken;
    }

    inline void ShutdownGdiPlus(ULONG_PTR token) {
        if (token) {
            GdiplusShutdown(token);
        }
    }

    void PlaySoundFile(const std::wstring& soundPath) {
        PlaySoundW(soundPath.c_str(), NULL, SND_FILENAME | SND_ASYNC);
    }

    inline bool RegisterWindowClass(HINSTANCE hInstance) {
        if (MsgBoxState::classRegistered) return true;

        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = MsgBoxProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"XPMessageBox";

        if (RegisterClassEx(&wc)) {
            MsgBoxState::classRegistered = true;
            return true;
        }
        return false;
    }

    //fluent interface
    struct MessageBoxBuilder {
        HWND parent = NULL;
        std::wstring message;
        std::wstring title;
        IconType icon = IconType::Question;
        ButtonType buttons = ButtonType::OK;
        DialogResult defaultButton = DialogResult::OK;
        WindowMode mode = WindowMode::Dialog;
        int customWidth = 0;
        int customHeight = 0;
        float textScale = 1.0f;
        float buttonScale = 1.0f;
        std::function<void(DialogResult)> callback = nullptr;
        std::vector<ButtonAction> buttonActions;
        std::wstring soundFile;

        MessageBoxBuilder& SetParent(HWND hwnd) { parent = hwnd; return *this; }
        MessageBoxBuilder& SetMessage(const std::wstring& msg) { message = msg; return *this; }
        MessageBoxBuilder& SetTitle(const std::wstring& ttl) { title = ttl; return *this; }
        MessageBoxBuilder& SetIcon(IconType type) { icon = type; return *this; }
        MessageBoxBuilder& SetButtons(ButtonType type) { buttons = type; return *this; }
        MessageBoxBuilder& SetDefaultButton(DialogResult btn) { defaultButton = btn; return *this; }
        MessageBoxBuilder& SetMode(WindowMode m) { mode = m; return *this; }
        MessageBoxBuilder& SetSize(int width, int height) { customWidth = width; customHeight = height; return *this; }
        MessageBoxBuilder& SetTextScale(float scale) { textScale = scale; return *this; }
        MessageBoxBuilder& SetButtonScale(float scale) { buttonScale = scale; return *this; }
        MessageBoxBuilder& SetCallback(std::function<void(DialogResult)> cb) { callback = cb; return *this; }
        MessageBoxBuilder& SetSound(const std::wstring& soundPath) {
            PlaySoundW(soundPath.c_str(), NULL, SND_FILENAME | SND_ASYNC);
            return *this;
        }
        MessageBoxBuilder& SetMP3Sound(const std::wstring& mp3Path) {
            soundFile = mp3Path;
            return *this;
        }

        MessageBoxBuilder& SetSoundSync(const std::wstring& soundPath) {
            PlaySoundW(soundPath.c_str(), NULL, SND_FILENAME | SND_SYNC);
            return *this;
        }

        //add some shit
        MessageBoxBuilder& OnButton(DialogResult buttonId, std::function<void()> action) {
            buttonActions.push_back({ buttonId, action });
            return *this;
        }

        DialogResult Show() {
            ULONG_PTR gdiplusToken = InitGdiPlus();

            HINSTANCE hInstance = GetModuleHandle(NULL);
            if (!RegisterWindowClass(hInstance)) {
                ShutdownGdiPlus(gdiplusToken);
                return DialogResult::Cancel;
            }

            if (!soundFile.empty()) {
                PlayMP3(soundFile, false, false);
            }

            MsgBoxState* state = new MsgBoxState();
            state->message = message;
            state->title = title;
            state->iconType = icon;
            state->buttonType = buttons;
            state->defaultButton = defaultButton;
            state->mode = mode;
            state->callback = callback;
            state->textScale = textScale;
            state->buttonScale = buttonScale;
            state->buttonActions = buttonActions;
            state->AddButtons();

            if (customWidth > 0 && customHeight > 0) {
                state->width = customWidth;
                state->height = customHeight;
                state->CalculateButtonPositions();

                int closeSize = static_cast<int>(22 * textScale);
                int closeX = state->width - closeSize - static_cast<int>(8 * textScale);
                int closeY = (state->titleBarHeight - closeSize) / 2;
                state->closeRect = { closeX, closeY, closeX + closeSize, closeY + closeSize };
            }
            else {
                HDC hdc = GetDC(NULL);
                state->CalculateSize(hdc);
                ReleaseDC(NULL, hdc);
            }

            DWORD dwStyle = WS_POPUP;
            DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_TOPMOST;

            if (mode == WindowMode::Regular) {
                dwStyle |= WS_MAXIMIZEBOX | WS_SIZEBOX;
            }

            HWND hwnd = CreateWindowEx(
                dwExStyle,
                L"XPMessageBox",
                title.c_str(),
                dwStyle,
                (GetSystemMetrics(SM_CXSCREEN) - state->width) / 2,
                (GetSystemMetrics(SM_CYSCREEN) - state->height) / 2,
                state->width, state->height,
                parent, NULL, hInstance, state
            );

            if (!hwnd) {
                delete state;
                ShutdownGdiPlus(gdiplusToken);
                return DialogResult::Cancel;
            }

            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);

            MSG msg;
            DialogResult result = DialogResult::Cancel;
            while (GetMessage(&msg, NULL, 0, 0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (msg.message == WM_QUIT) {
                    result = static_cast<DialogResult>(msg.wParam);
                    break;
                }
            }

            delete state;
            ShutdownGdiPlus(gdiplusToken);
            return result;
        }
    };

    inline DialogResult Show(HWND parent, const std::wstring& message, const std::wstring& title,
        IconType icon = IconType::Question,
        ButtonType buttons = ButtonType::OK,
        DialogResult defaultBtn = DialogResult::OK,
        WindowMode mode = WindowMode::Dialog,
        int customWidth = 0, int customHeight = 0,
        float textScale = 1.0f,
        float buttonScale = 1.0f,
        std::function<void(DialogResult)> callback = nullptr,
        std::vector<ButtonAction> buttonActions = {}) {

        return MessageBoxBuilder()
            .SetParent(parent)
            .SetMessage(message)
            .SetTitle(title)
            .SetIcon(icon)
            .SetButtons(buttons)
            .SetDefaultButton(defaultBtn)
            .SetMode(mode)
            .SetSize(customWidth, customHeight)
            .SetTextScale(textScale)
            .SetButtonScale(buttonScale)
            .SetCallback(callback)
            .Show();
    }

    //additional for running
    inline void RunScene(const std::string& scenePath) {
        ShellExecuteA(NULL, "open", scenePath.c_str(), NULL, NULL, SW_SHOW);
    }

    inline void RunSceneW(const std::wstring& scenePath) {
        ShellExecuteW(NULL, L"open", scenePath.c_str(), NULL, NULL, SW_SHOW);
    }

    inline void RunExecutable(const std::wstring& exePath, const std::wstring& params = L"") {
        ShellExecuteW(NULL, L"open", exePath.c_str(), params.c_str(), NULL, SW_SHOW);
    }

    inline void ExeAndRunCpp(const std::wstring& cppPath) {
        //without file extension
        std::wstring exeName = cppPath;
        size_t dotPos = exeName.find_last_of(L".");
        if (dotPos != std::wstring::npos) {
            exeName = exeName.substr(0, dotPos);
        }
        //or just run .exe file
        RunExecutable(exeName + L".exe");
    }

    //additional funcs for some shit
    inline int ShowInt(HWND parent, const std::wstring& message, const std::wstring& title,
        IconType icon = IconType::Question,
        ButtonType buttons = ButtonType::OK,
        DialogResult defaultBtn = DialogResult::OK,
        WindowMode mode = WindowMode::Dialog,
        int customWidth = 0, int customHeight = 0,
        float textScale = 1.0f,
        float buttonScale = 1.0f,
        std::function<void(DialogResult)> callback = nullptr) {

        DialogResult result = Show(parent, message, title, icon, buttons, defaultBtn, mode,
            customWidth, customHeight, textScale, buttonScale, callback);
        return static_cast<int>(result);
    }

    inline int ShowSimple(HWND parent, const std::wstring& message, const std::wstring& title) {
        return static_cast<int>(Show(parent, message, title));
    }
}

//macros's's''s's
#define XPMessageBox(parent, message, title) \
        static_cast<int>(XPMessageBox::Show(parent, message, title, \
            XPMessageBox::IconType::Question, \
            XPMessageBox::ButtonType::OK, \
            XPMessageBox::DialogResult::OK, \
            XPMessageBox::WindowMode::Dialog))

#define XPMessageBoxWindow(parent, message, title, width, height) \
        static_cast<int>(XPMessageBox::Show(parent, message, title, \
            XPMessageBox::IconType::Question, \
            XPMessageBox::ButtonType::OK, \
            XPMessageBox::DialogResult::OK, \
            XPMessageBox::WindowMode::Regular, width, height))

#define XPMessageBoxAuto(parent, message, title) \
        static_cast<int>(XPMessageBox::Show(parent, message, title, \
            XPMessageBox::IconType::Question, \
            XPMessageBox::ButtonType::OK, \
            XPMessageBox::DialogResult::OK, \
            XPMessageBox::WindowMode::Dialog))