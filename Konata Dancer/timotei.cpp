#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <ctime>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_SHOW 1002
#define ID_TRAY_HIDE 1003

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

//json shit
class SimpleJSON {
private:
    std::map<std::string, std::string> values;
    std::map<std::string, SimpleJSON> objects;

    void skipWhitespace(const std::string& str, size_t& pos) {
        while (pos < str.length() && (str[pos] == ' ' || str[pos] == '\n' || str[pos] == '\r' || str[pos] == '\t')) {
            pos++;
        }
    }

    std::string parseString(const std::string& str, size_t& pos) {
        if (str[pos] != '"') return "";
        pos++;

        std::string result;
        while (pos < str.length() && str[pos] != '"') {
            if (str[pos] == '\\' && pos + 1 < str.length()) {
                pos++;
                switch (str[pos]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                default: result += str[pos];
                }
            }
            else {
                result += str[pos];
            }
            pos++;
        }
        pos++;
        return result;
    }

    bool parseObject(const std::string& str, size_t& pos) {
        skipWhitespace(str, pos);
        if (str[pos] != '{') return false;
        pos++;

        values.clear();
        objects.clear();

        while (pos < str.length()) {
            skipWhitespace(str, pos);
            if (str[pos] == '}') {
                pos++;
                return true;
            }

            std::string key = parseString(str, pos);
            if (key.empty()) return false;

            skipWhitespace(str, pos);
            if (str[pos] != ':') return false;
            pos++;

            skipWhitespace(str, pos);

            if (str[pos] == '{') {
                SimpleJSON obj;
                if (obj.parseObject(str, pos)) {
                    objects[key] = obj;
                }
            }
            else if (str[pos] == '"') {
                std::string value = parseString(str, pos);
                values[key] = "\"" + value + "\"";
            }
            else {
                std::string value;
                while (pos < str.length() && str[pos] != ',' && str[pos] != '}') {
                    value += str[pos];
                    pos++;
                }
                value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
                values[key] = value;
            }

            skipWhitespace(str, pos);
            if (str[pos] == ',') {
                pos++;
                continue;
            }
            else if (str[pos] == '}') {
                pos++;
                return true;
            }
            else {
                return false;
            }
        }
        return false;
    }

public:
    void setString(const std::string& key, const std::string& value) {
        values[key] = "\"" + value + "\"";
    }

    void setInt(const std::string& key, int value) {
        values[key] = std::to_string(value);
    }

    void setFloat(const std::string& key, float value) {
        values[key] = std::to_string(value);
    }

    void setBool(const std::string& key, bool value) {
        values[key] = value ? "true" : "false";
    }

    void setObject(const std::string& key, const SimpleJSON& obj) {
        objects[key] = obj;
    }

    std::string getString(const std::string& key, const std::string& defaultValue = "") {
        if (values.find(key) != values.end()) {
            std::string val = values[key];
            if (!val.empty() && val[0] == '"') {
                val = val.substr(1, val.length() - 2);
            }
            return val;
        }
        return defaultValue;
    }

    int getInt(const std::string& key, int defaultValue = 0) {
        if (values.find(key) != values.end()) {
            return std::stoi(values[key]);
        }
        return defaultValue;
    }

    float getFloat(const std::string& key, float defaultValue = 0.0f) {
        if (values.find(key) != values.end()) {
            return std::stof(values[key]);
        }
        return defaultValue;
    }

    bool getBool(const std::string& key, bool defaultValue = false) {
        if (values.find(key) != values.end()) {
            return values[key] == "true";
        }
        return defaultValue;
    }

    SimpleJSON getObject(const std::string& key) {
        if (objects.find(key) != objects.end()) {
            return objects[key];
        }
        return SimpleJSON();
    }

    bool hasKey(const std::string& key) {
        return values.find(key) != values.end() || objects.find(key) != objects.end();
    }

    std::string serialize(int indent = 0) {
        std::string result = "{";
        bool first = true;

        for (auto& pair : values) {
            if (!first) result += ",";
            result += "\n" + std::string(indent + 2, ' ') + "\"" + pair.first + "\": " + pair.second;
            first = false;
        }

        for (auto& pair : objects) {
            if (!first) result += ",";
            result += "\n" + std::string(indent + 2, ' ') + "\"" + pair.first + "\": " + pair.second.serialize(indent + 2);
            first = false;
        }

        result += "\n" + std::string(indent, ' ') + "}";
        return result;
    }

    bool parse(const std::string& json) {
        size_t pos = 0;
        return parseObject(json, pos);
    }
};

struct GifFrame {
    Bitmap* bitmap;
    UINT delay;
};

//Konata chan
class KonataChan {
private:
    HWND hwnd;
    std::vector<GifFrame> frames;
    UINT currentFrame;
    ULONG_PTR gdiplusToken;
    bool isDragging;
    POINT dragStart;
    int scale;
    int fps;
    std::wstring gifPath;
    int windowWidth, windowHeight;
    UINT_PTR timerId;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        KonataChan* pThis = nullptr;
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<KonataChan*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        }
        else {
            pThis = reinterpret_cast<KonataChan*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
        if (pThis) return pThis->HandleMessage(hwnd, msg, wParam, lParam);
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void DanceAnimatedKonata(const std::wstring& path) {//GIF animation shit
        //clear old frames
        for (auto& frame : frames) {
            if (frame.bitmap) {
                delete frame.bitmap;
                frame.bitmap = nullptr;
            }
        }
        frames.clear();

        Image* gifImage = new Image(path.c_str());
        if (gifImage->GetLastStatus() != Ok) {
            delete gifImage;
            return;
        }

        GUID frameDimension;
        gifImage->GetFrameDimensionsList(&frameDimension, 1);
        UINT frameCount = gifImage->GetFrameCount(&frameDimension);
        if (frameCount == 0) {
            delete gifImage;
            return;
        }

        UINT size = gifImage->GetPropertyItemSize(PropertyTagFrameDelay);
        UINT delay = 1000 / fps;

        if (size > 0) {
            PropertyItem* propertyItem = (PropertyItem*)malloc(size);
            gifImage->GetPropertyItem(PropertyTagFrameDelay, size, propertyItem);

            for (UINT i = 0; i < frameCount; i++) {
                gifImage->SelectActiveFrame(&frameDimension, i);
                Bitmap* frameBitmap = new Bitmap(gifImage->GetWidth(), gifImage->GetHeight(), PixelFormat32bppARGB);
                Graphics graphics(frameBitmap);
                graphics.Clear(Color(0, 0, 0, 0));
                graphics.DrawImage(gifImage, 0, 0, gifImage->GetWidth(), gifImage->GetHeight());

                GifFrame frame;
                frame.bitmap = frameBitmap;
                frame.delay = delay;
                if (propertyItem && i < propertyItem->length / sizeof(long)) {
                    long* delays = (long*)propertyItem->value;
                    UINT originalDelay = delays[i] * 10;
                    if (originalDelay >= 10) frame.delay = originalDelay;
                }
                frames.push_back(frame);
            }
            free(propertyItem);
        }
        else {
            for (UINT i = 0; i < frameCount; i++) {
                gifImage->SelectActiveFrame(&frameDimension, i);
                Bitmap* frameBitmap = new Bitmap(gifImage->GetWidth(), gifImage->GetHeight(), PixelFormat32bppARGB);
                Graphics graphics(frameBitmap);
                graphics.Clear(Color(0, 0, 0, 0));
                graphics.DrawImage(gifImage, 0, 0, gifImage->GetWidth(), gifImage->GetHeight());

                GifFrame frame;
                frame.bitmap = frameBitmap;
                frame.delay = delay;
                frames.push_back(frame);
            }
        }
        delete gifImage;

        if (!frames.empty() && frames[0].bitmap) {
            windowWidth = frames[0].bitmap->GetWidth() * scale / 100;
            windowHeight = frames[0].bitmap->GetHeight() * scale / 100;
            SetWindowPos(hwnd, nullptr, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
    }

    void SaveConfig() {
        SimpleJSON config;

        RECT rect;
        GetWindowRect(hwnd, &rect);
        config.setInt("konata_x", rect.left);
        config.setInt("konata_y", rect.top);
        config.setInt("konata_scale", scale);
        config.setInt("konata_fps", fps);
        config.setInt("last_updated", (int)time(nullptr));

        //read settings
        std::ifstream readFile("notes.json");
        if (readFile.is_open()) {
            std::stringstream buffer;
            buffer << readFile.rdbuf();
            std::string jsonStr = buffer.str();
            readFile.close();

            SimpleJSON existingConfig;
            if (existingConfig.parse(jsonStr)) {
                std::string notes = existingConfig.getString("notes", "");
                if (!notes.empty()) {
                    config.setString("notes", notes);
                }

                int notesX = existingConfig.getInt("notes_x", 200);
                int notesY = existingConfig.getInt("notes_y", 200);
                config.setInt("notes_x", notesX);
                config.setInt("notes_y", notesY);
            }
        }

        std::ofstream file("notes.json");
        if (file.is_open()) {
            file << config.serialize();
            file.close();
        }
    }

    void LoadConfig() {
        std::ifstream file("notes.json");
        if (!file.is_open()) return;

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();
        file.close();

        SimpleJSON config;
        if (config.parse(jsonStr)) {
            int x = config.getInt("konata_x", 100);
            int y = config.getInt("konata_y", 100);
            int newScale = config.getInt("konata_scale", 100);
            int newFps = config.getInt("konata_fps", 30);

            SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

            if (newScale != scale) {
                scale = newScale;
                if (!frames.empty()) {
                    windowWidth = frames[0].bitmap->GetWidth() * scale / 100;
                    windowHeight = frames[0].bitmap->GetHeight() * scale / 100;
                    SetWindowPos(hwnd, nullptr, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);
                    InvalidateRect(hwnd, nullptr, TRUE);
                }
            }

            if (newFps != fps) {
                fps = newFps;
                if (timerId) {
                    KillTimer(hwnd, timerId);
                }
                if (!frames.empty()) {
                    for (auto& frame : frames) {
                        frame.delay = 1000 / fps;
                    }
                    timerId = SetTimer(hwnd, 1, frames[0].delay, nullptr);
                }
            }
        }
    }

    void framelogicshet(HDC hdc) {//draw
        if (frames.empty() || currentFrame >= frames.size()) return;

        Graphics graphics(hdc);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

        Bitmap* currentBitmap = frames[currentFrame].bitmap;
        if (!currentBitmap) return;

        RECT rect;
        GetClientRect(hwnd, &rect);
        int drawWidth = currentBitmap->GetWidth() * scale / 100;
        int drawHeight = currentBitmap->GetHeight() * scale / 100;
        int x = (rect.right - drawWidth) / 2;
        int y = (rect.bottom - drawHeight) / 2;

        graphics.Clear(Color(0, 0, 0, 0));

        graphics.DrawImage(currentBitmap, x, y, drawWidth, drawHeight);
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rect;
            GetClientRect(hWnd, &rect);

            //double buffering(idk google it if forgot what it is)
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

            framelogicshet(memDC);

            //copy to matrix?!?!1/
            BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1; //don't you dare to vanish if front of my eyes

        case WM_MOUSEMOVE: {
            if (isDragging) {
                POINT mousePos;
                GetCursorPos(&mousePos);
                SetWindowPos(hWnd, nullptr,
                    mousePos.x - dragStart.x,
                    mousePos.y - dragStart.y,
                    0, 0,
                    SWP_NOSIZE | SWP_NOZORDER);
                SaveConfig();
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            isDragging = true;
            POINT mousePos;
            GetCursorPos(&mousePos);
            RECT winRect;
            GetWindowRect(hWnd, &winRect);
            dragStart.x = mousePos.x - winRect.left;
            dragStart.y = mousePos.y - winRect.top;
            SetCapture(hWnd);
            return 0;
        }

        case WM_LBUTTONUP: {
            isDragging = false;
            ReleaseCapture();
            SaveConfig();
            return 0;
        }

        case WM_RBUTTONDOWN: {
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 1, L"Scale 50%");
            AppendMenu(hMenu, MF_STRING, 2, L"Scale 75%");
            AppendMenu(hMenu, MF_STRING, 3, L"Scale 100%");
            AppendMenu(hMenu, MF_STRING, 4, L"Scale 125%");
            AppendMenu(hMenu, MF_STRING, 5, L"Scale 150%");
            AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(hMenu, MF_STRING, 10, L"FPS 15");
            AppendMenu(hMenu, MF_STRING, 11, L"FPS 24");
            AppendMenu(hMenu, MF_STRING, 12, L"FPS 30");
            AppendMenu(hMenu, MF_STRING, 13, L"FPS 60");
            AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(hMenu, MF_STRING, 99, L"close");

            POINT mousePos;
            GetCursorPos(&mousePos);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                mousePos.x, mousePos.y, 0, hWnd, nullptr);

            //int oldScale = scale;
            bool needReload = false;

            switch (cmd) {
            case 1: scale = 50; needReload = true; break;
            case 2: scale = 75; needReload = true; break;
            case 3: scale = 100; needReload = true; break;
            case 4: scale = 125; needReload = true; break;
            case 5: scale = 150; needReload = true; break;
            case 10:
                fps = 15;
                if (!frames.empty()) {
                    for (auto& frame : frames) frame.delay = 1000 / fps;
                    if (timerId) KillTimer(hWnd, timerId);
                    timerId = SetTimer(hWnd, 1, frames[0].delay, nullptr);
                }
                SaveConfig();
                break;
            case 11:
                fps = 24;
                if (!frames.empty()) {
                    for (auto& frame : frames) frame.delay = 1000 / fps;
                    if (timerId) KillTimer(hWnd, timerId);
                    timerId = SetTimer(hWnd, 1, frames[0].delay, nullptr);
                }
                SaveConfig();
                break;
            case 12:
                fps = 30;
                if (!frames.empty()) {
                    for (auto& frame : frames) frame.delay = 1000 / fps;
                    if (timerId) KillTimer(hWnd, timerId);
                    timerId = SetTimer(hWnd, 1, frames[0].delay, nullptr);
                }
                SaveConfig();
                break;
            case 13:
                fps = 60;
                if (!frames.empty()) {
                    for (auto& frame : frames) frame.delay = 1000 / fps;
                    if (timerId) KillTimer(hWnd, timerId);
                    timerId = SetTimer(hWnd, 1, frames[0].delay, nullptr);
                }
                SaveConfig();
                break;
            case 99: DestroyWindow(hWnd); break;
            }

            if (needReload && !frames.empty()) {
                windowWidth = frames[0].bitmap->GetWidth() * scale / 100;
                windowHeight = frames[0].bitmap->GetHeight() * scale / 100;
                SetWindowPos(hWnd, nullptr, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);
                InvalidateRect(hWnd, nullptr, TRUE);
                SaveConfig();
            }

            DestroyMenu(hMenu);
            return 0;
        }

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int oldScale = scale;
            scale = max(25, min(200, scale + (delta > 0 ? 10 : -10)));
            if (oldScale != scale && !frames.empty()) {
                windowWidth = frames[0].bitmap->GetWidth() * scale / 100;
                windowHeight = frames[0].bitmap->GetHeight() * scale / 100;
                SetWindowPos(hWnd, nullptr, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);
                InvalidateRect(hWnd, nullptr, TRUE);
                SaveConfig();
            }
            return 0;
        }

        case WM_TIMER: {
            if (!frames.empty()) {
                currentFrame = (currentFrame + 1) % frames.size();
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_DESTROY: {
            if (timerId) {
                KillTimer(hWnd, timerId);
                timerId = 0;
            }
            SaveConfig();
            //clear
            for (auto& frame : frames) {
                if (frame.bitmap) {
                    delete frame.bitmap;
                    frame.bitmap = nullptr;
                }
            }
            frames.clear();
            PostQuitMessage(0);
            return 0;
        }
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

public:
    KonataChan(const std::wstring& path)
        : hwnd(nullptr), currentFrame(0), isDragging(false), scale(100), fps(30),
        gifPath(path), windowWidth(400), windowHeight(400), timerId(0) {

        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wc.lpszClassName = L"KonataChan";
        RegisterClassEx(&wc);

        //windows with transparent shit + WS_EX_TOOLWINDOW убирает кнопку с панели задач
        hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            L"KonataChan", L"Konata Dancer", WS_POPUP,
            100, 100, windowWidth, windowHeight,
            nullptr, nullptr, GetModuleHandle(nullptr), this);

        if (!hwnd) return;

        //prozrac'nost'
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

        DanceAnimatedKonata(gifPath);

        LoadConfig();

        if (!frames.empty()) {
            timerId = SetTimer(hwnd, 1, frames[0].delay, nullptr);
        }
    }

    ~KonataChan() {
        for (auto& frame : frames) {
            if (frame.bitmap) {
                delete frame.bitmap;
                frame.bitmap = nullptr;
            }
        }
        frames.clear();
        GdiplusShutdown(gdiplusToken);
    }

    void Show() {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }

    void Hide() {
        ShowWindow(hwnd, SW_HIDE);
    }

    HWND GetHWND() {
        return hwnd;
    }

    bool IsVisible() {
        return IsWindowVisible(hwnd) != FALSE;
    }
};

//notes settings
class NotesWindow {
private:
    HWND hwnd;
    HWND editControl;
    std::wstring jsonPath;
    bool isDragging;
    POINT dragStart;
    int notesWidth;
    int notesHeight;
    int editX, editY, editWidth, editHeight;
    Image* backgroundImage;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        NotesWindow* pThis = nullptr;

        if (msg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<NotesWindow*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        }
        else {
            pThis = reinterpret_cast<NotesWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (pThis) {
            return pThis->HandleMessage(hwnd, msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void LoadNotesFromJson() {
        std::ifstream file(jsonPath);
        if (!file.is_open()) return;

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();
        file.close();

        SimpleJSON config;
        if (config.parse(jsonStr)) {
            std::string notes = config.getString("notes", "");
            if (!notes.empty()) {
                std::wstring wnotes(notes.begin(), notes.end());
                SetWindowText(editControl, wnotes.c_str());
            }

            int x = config.getInt("notes_x", 200);
            int y = config.getInt("notes_y", 200);
            SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    void SaveNotesToJson() {
        SimpleJSON config;

        //read info from json
        std::ifstream readFile("notes.json");
        if (readFile.is_open()) {
            std::stringstream buffer;
            buffer << readFile.rdbuf();
            std::string jsonStr = buffer.str();
            readFile.close();

            SimpleJSON existingConfig;
            if (existingConfig.parse(jsonStr)) {
                int konataX = existingConfig.getInt("konata_x", 100);
                int konataY = existingConfig.getInt("konata_y", 100);
                int konataScale = existingConfig.getInt("konata_scale", 100);
                int konataFps = existingConfig.getInt("konata_fps", 30);

                config.setInt("konata_x", konataX);
                config.setInt("konata_y", konataY);
                config.setInt("konata_scale", konataScale);
                config.setInt("konata_fps", konataFps);
            }
        }

        //save otes
        int textLength = GetWindowTextLength(editControl);
        if (textLength > 0) {
            std::vector<wchar_t> buffer(textLength + 1);
            GetWindowText(editControl, buffer.data(), textLength + 1);
            std::wstring wnotes(buffer.data());
            std::string notes(wnotes.begin(), wnotes.end());
            config.setString("notes", notes);
        }
        else {
            config.setString("notes", "");
        }

        //save everyting
        RECT rect;
        GetWindowRect(hwnd, &rect);
        config.setInt("notes_x", rect.left);
        config.setInt("notes_y", rect.top);
        config.setInt("last_updated", (int)time(nullptr));

        std::ofstream file(jsonPath);
        if (file.is_open()) {
            file << config.serialize();
            file.close();
        }
    }

    void DrawBackground(HDC hdc) {
        if (!backgroundImage) {
            backgroundImage = new Image(L"windows_xp_smth.png");
        }

        Graphics graphics(hdc);

        if (backgroundImage && backgroundImage->GetLastStatus() == Ok) {
            RECT rect;
            GetClientRect(hwnd, &rect);
            graphics.DrawImage(backgroundImage, 0, 0, rect.right, rect.bottom);
        }
        else {
            SolidBrush brush(Color(255, 240, 240, 240));
            RECT rect;
            GetClientRect(hwnd, &rect);
            graphics.FillRectangle(&brush, 0, 0, rect.right, rect.bottom);

            Font font(L"Arial", 12);
            SolidBrush textBrush(Color(255, 255, 0, 0));
            graphics.DrawString(L"windows_xp_smth.png not found", -1, &font, PointF(10, 10), &textBrush);
        }

        Pen pen(Color(128, 0, 0, 0), 1);
        pen.SetDashStyle(DashStyleDash);
        graphics.DrawRectangle(&pen, editX, editY, editWidth, editHeight);
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rect;
            GetClientRect(hWnd, &rect);
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

            DrawBackground(memDC);
            BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_MOUSEMOVE: {
            if (isDragging) {
                POINT mousePos;
                GetCursorPos(&mousePos);
                SetWindowPos(hWnd, nullptr,
                    mousePos.x - dragStart.x,
                    mousePos.y - dragStart.y,
                    0, 0,
                    SWP_NOSIZE | SWP_NOZORDER);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            isDragging = true;
            POINT mousePos;
            GetCursorPos(&mousePos);
            RECT winRect;
            GetWindowRect(hWnd, &winRect);
            dragStart.x = mousePos.x - winRect.left;
            dragStart.y = mousePos.y - winRect.top;
            SetCapture(hWnd);
            return 0;
        }

        case WM_LBUTTONUP: {
            isDragging = false;
            ReleaseCapture();
            SaveNotesToJson();
            return 0;
        }

        case WM_CLOSE:
        case WM_DESTROY: {
            SaveNotesToJson();
            if (backgroundImage) delete backgroundImage;
            DestroyWindow(hWnd);
            return 0;
        }
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

public:
    NotesWindow(const std::wstring& jsonFilePath)
        : hwnd(nullptr), editControl(nullptr), jsonPath(jsonFilePath),
        isDragging(false), notesWidth(621), notesHeight(523),
        editX(13), editY(54), editWidth(580), editHeight(438),
        backgroundImage(nullptr) {

        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wc.lpszClassName = L"NotesWindow";
        RegisterClassEx(&wc);

        hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"NotesWindow", L"Notes",
            WS_POPUP,
            200, 200, notesWidth, notesHeight,
            nullptr, nullptr, GetModuleHandle(nullptr), this);

        if (!hwnd) return;

        editControl = CreateWindowEx(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER,
            editX, editY, editWidth, editHeight,
            hwnd, nullptr, GetModuleHandle(nullptr), nullptr);

        HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        SendMessage(editControl, WM_SETFONT, (WPARAM)hFont, TRUE);

        LoadNotesFromJson();
    }

    void Show() {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }

    void Hide() {
        ShowWindow(hwnd, SW_HIDE);
    }

    HWND GetHWND() {
        return hwnd;
    }

    bool IsVisible() {
        return IsWindowVisible(hwnd) != FALSE;
    }
};

//additional shit
bool FileExists(const std::wstring& path) {
    std::ifstream file(path.c_str());
    return file.good();
}

static NOTIFYICONDATA g_nid;
static KonataChan* g_konataWindow = nullptr;
static NotesWindow* g_notesWindow = nullptr;
static HWND g_hTrayHwnd = nullptr;

void ShowTrayContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, L"start dancing");
    AppendMenu(hMenu, MF_STRING, ID_TRAY_HIDE, L"stop it");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"exit");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

//for ninja tray windpw
LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            ShowTrayContextMenu(hwnd);
        }
        else if (lParam == WM_LBUTTONDBLCLK) {
            if (g_konataWindow && g_notesWindow) {
                if (g_konataWindow->IsVisible()) {
                    g_konataWindow->Hide();
                    g_notesWindow->Hide();
                }
                else {
                    g_konataWindow->Show();
                    g_notesWindow->Show();
                }
            }
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_EXIT:
            if (g_konataWindow) delete g_konataWindow;
            if (g_notesWindow) delete g_notesWindow;
            Shell_NotifyIcon(NIM_DELETE, &g_nid);
            PostQuitMessage(0);
            return 0;
        case ID_TRAY_SHOW:
            if (g_konataWindow) g_konataWindow->Show();
            if (g_notesWindow) g_notesWindow->Show();
            return 0;
        case ID_TRAY_HIDE:
            if (g_konataWindow) g_konataWindow->Hide();
            if (g_notesWindow) g_notesWindow->Hide();
            return 0;
        }
        return 0;

    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//checking and just the main entrance
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::wstring gifPath = L"Konata.gif";

    if (!FileExists(gifPath)) {
        MessageBox(nullptr, L"Konata.gif not found\nplease place Konata.gif in the program folder",
            L"error", MB_OK);
        return 1;
    }

    //create thy ninja
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TrayWindowClass";
    RegisterClassEx(&wc);

    g_hTrayHwnd = CreateWindowEx(0, L"TrayWindowClass", L"", WS_OVERLAPPED,
        0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);

    //create thy windows
    g_konataWindow = new KonataChan(gifPath);
    g_notesWindow = new NotesWindow(L"notes.json");

    ZeroMemory(&g_nid, sizeof(NOTIFYICONDATA));
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = g_hTrayHwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = (HICON)LoadImage(NULL, L"konata.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    if (!g_nid.hIcon) {
        g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    wcscpy_s(g_nid.szTip, L"Konata Chan");
    Shell_NotifyIcon(NIM_ADD, &g_nid);

    //cycle
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    delete g_konataWindow;
    delete g_notesWindow;

    return 0;
}

/*magiv number note

const int DEFAULT_SCALE = 100;
const int MIN_SCALE = 25;
const int MAX_SCALE = 200;
const int SCALE_STEP = 10;
const int ZOOM_PRESET_50 = 50;
const int ZOOM_PRESET_75 = 75;

const int FPS_SLOW = 15;
const int FPS_CINEMA = 24;
const int FPS_STANDARD = 30;
const int FPS_SMOOTH = 60;

const int MENU_CLOSE_ID = 99;
const int MENU_EXIT_COMMAND = 99;

const int NOTES_WINDOW_WIDTH = 621;
const int NOTES_WINDOW_HEIGHT = 523;
const int NOTES_EDITOR_OFFSET_X = 13;
const int NOTES_EDITOR_OFFSET_Y = 54;*/