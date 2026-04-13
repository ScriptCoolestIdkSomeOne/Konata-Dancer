#pragma once
#include <windows.h>
#include <vector>
#include <string>

struct MenuItem {
    int id;
    std::wstring text;
    bool isSeparator;
    bool isDisabled;
    std::vector<MenuItem> subItems;

    MenuItem() : id(0), isSeparator(false), isDisabled(false) {}
    MenuItem(int _id, const std::wstring& _text) : id(_id), text(_text), isSeparator(false), isDisabled(false) {}
    MenuItem(bool sep) : id(0), isSeparator(sep), isDisabled(false) {}
};

class KonataMenu {
private:
    HWND m_hwnd;
    HWND m_parentWnd;
    std::vector<MenuItem> m_menuItems;
    std::vector<MenuItem> m_effectSubmenu;
    bool m_isVisible;
    bool m_inSubmenu;
    POINT m_menuPos;
    int m_selectedIndex;
    int m_subSelectedIndex;

    //callbacks
    void* m_callbackTarget;
    void(__cdecl* m_onScaleChange)(void*, int);
    void(__cdecl* m_onFpsChange)(void*, int);
    void(__cdecl* m_onSaturationChange)(void*, float);
    void(__cdecl* m_onFrameSizeChange)(void*, int, int);
    void(__cdecl* m_onEffectChange)(void*);
    void(__cdecl* m_onPrize)(void*);
    void(__cdecl* m_onExit)(void*);

    void(__cdecl* m_onApplyEffect)(void*, int);

    //state
    float m_currentSaturation;
    float m_hueShift;
    float m_oldFilm;
    float m_termIntensity;
    bool m_inverted;

    void BuildMenu();
    void DrawMenu(HDC hdc);
    void DrawSubmenu(HDC hdc, const std::vector<MenuItem>& items, int x, int y, int selected);
    int GetMenuItemHeight() { return 30; }
    int GetMenuWidth(const std::vector<MenuItem>& items);
    void HandleClick(int x, int y);
    void HandleSubmenuClick(const std::vector<MenuItem>& items, int id);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
    KonataMenu();
    ~KonataMenu();

    void SetCallbacks(
        void* target,
        void(__cdecl* onScaleChange)(void*, int),
        void(__cdecl* onFpsChange)(void*, int),
        void(__cdecl* onSaturationChange)(void*, float),
        void(__cdecl* onFrameSizeChange)(void*, int, int),
        void(__cdecl* onEffectChange)(void*),
        void(__cdecl* onPrize)(void*),
        void(__cdecl* onExit)(void*)
    );

    void SetApplyEffectCallback(void(__cdecl* onApplyEffect)(void*, int)) {
        m_onApplyEffect = onApplyEffect;
    }

    void UpdateSaturation(float saturation);
    void UpdateHueShift(float shift);
    void UpdateOldFilm(float film);
    void UpdateTerminator(float term);
    void UpdateInverted(bool inverted);

    void Show(HWND parent, int x, int y);
    void Hide();
    HWND GetHWND() { return m_hwnd; }
};