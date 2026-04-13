#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_SHOW 1002
#define ID_TRAY_HIDE 1003
#define ID_TRAY_SETTINGS 1004
#define ID_TRAY_ABOUT 1005
#define ID_TRAY_BLUEHAIR 1006
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#define ID_TRAY_PRIZE 1007
#define MENU_BG_COLOR RGB(0, 0, 0)
#define MENU_TEXT_COLOR RGB(255, 255, 255)
#define MENU_HOVER_COLOR RGB(64, 64, 64)
#pragma warning(disable: 4073)
#pragma init_seg(lib)
#ifdef _DEBUG
#pragma comment(linker, "/ENTRY:WinMainCRTStartup")
#else
#pragma comment(linker, "/ENTRY:WinMainCRTStartup")
#endif
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
#include <commctrl.h>
#include "Bluehair.h"
#include "TrayAnimation.h"
#include "Colors.h"
#include "prize.h"
#include "RespondingGuy.h"
#include "menu.h"
#include "XPMessageBox.h"

void StartTrayAnimation();
void StopTrayAnimation();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    return WinMain(hInstance, hPrevInstance, NULL, nCmdShow);
}

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "msimg32.lib")

using namespace Gdiplus;

//simpel json
//TODO: make it more cooler/optimized
class SimpleJSON {
private:
    std::map<std::string, std::string> values;
    std::map<std::string, SimpleJSON> objects;

    void skipWhitespace(const std::string& str, size_t& pos) {
        while (pos < str.length() && (str[pos] == ' ' || str[pos] == '\n' || str[pos] == '\r' || str[pos] == '\t')) pos++;
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
            else result += str[pos];
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
            if (str[pos] == '}') { pos++; return true; }
            std::string key = parseString(str, pos);
            if (key.empty()) return false;
            skipWhitespace(str, pos);
            if (str[pos] != ':') return false;
            pos++;
            skipWhitespace(str, pos);
            if (str[pos] == '{') {
                SimpleJSON obj;
                if (obj.parseObject(str, pos)) objects[key] = obj;
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
            if (str[pos] == ',') { pos++; continue; }
            else if (str[pos] == '}') { pos++; return true; }
            else return false;
        }
        return false;
    }

public:
    void setString(const std::string& key, const std::string& value) { values[key] = "\"" + value + "\""; }
    void setInt(const std::string& key, int value) { values[key] = std::to_string(value); }
    void setFloat(const std::string& key, float value) { values[key] = std::to_string(value); }
    void setBool(const std::string& key, bool value) { values[key] = value ? "true" : "false"; }
    void setObject(const std::string& key, const SimpleJSON& obj) { objects[key] = obj; }

    std::string getString(const std::string& key, const std::string& defaultValue = "") {
        if (values.find(key) != values.end()) {
            std::string val = values[key];
            if (!val.empty() && val[0] == '"') val = val.substr(1, val.length() - 2);
            return val;
        }
        return defaultValue;
    }

    int getInt(const std::string& key, int defaultValue = 0) {
        if (values.find(key) != values.end()) return std::stoi(values[key]);
        return defaultValue;
    }

    float getFloat(const std::string& key, float defaultValue = 0.0f) {
        if (values.find(key) != values.end()) return std::stof(values[key]);
        return defaultValue;
    }

    bool getBool(const std::string& key, bool defaultValue = false) {
        if (values.find(key) != values.end()) return values[key] == "true";
        return defaultValue;
    }

    SimpleJSON getObject(const std::string& key) {
        if (objects.find(key) != objects.end()) return objects[key];
        return SimpleJSON();
    }

    bool hasKey(const std::string& key) { return values.find(key) != values.end() || objects.find(key) != objects.end(); }

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

    bool parse(const std::string& json) { size_t pos = 0; return parseObject(json, pos); }
};

struct GifFrame {
    Bitmap* bitmap;
    UINT delay;
};

class KonataChan {
private:
    int frameWidth;
    int frameHeight;
    int frameSize;
    bool isSaturating;
    std::vector<GifFrame> originalFrames;
    float currentSaturation;
    std::vector<GifFrame> frames;
    bool isApplyingEffect;
    CRITICAL_SECTION frameLock;
    HWND hwnd;
    UINT currentFrame;
    ULONG_PTR gdiplusToken;
    bool isDragging;
    bool m_initialized;
    POINT dragStart;
    int scale;
    int fps;
    std::wstring gifPath;
    int windowWidth, windowHeight;
    UINT_PTR timerId;
    float m_temperature = 0.0f;      //-1.0 (cold) до +1.0 (warm)
    float m_sepia = 0.0f;            //0.0 - 1.0
    float m_hueShift = 0.0f;         //0.0 - 1.0
    float m_gamma = 1.0f;            //0.5 - 2.0
    float m_exposure = 0.0f;         //-2.0 - +2.0 EV
    bool m_inverted = false;
    float m_oldFilm = 0.0f;          //0.0 - 1.0 (old film)
    float m_termIntensity = 0.0f;    //0.0 - 1.0 (terminator)
    bool m_blackWhite = false;
    RGBQUAD m_bwTint = { 255,255,255,0 };


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

    static DWORD WINAPI EffectThreadProc(LPVOID param) {
        KonataChan* pThis = reinterpret_cast<KonataChan*>(param);
        pThis->ProcessEffect();
        return 0;
    }

    void SaveOriginalFrames() {
        for (auto& frame : originalFrames) {
            if (frame.bitmap) {
                delete frame.bitmap;
                frame.bitmap = nullptr;
            }
        }
        originalFrames.clear();

        for (auto& frame : frames) {
            if (frame.bitmap) {
                GifFrame newFrame;
                newFrame.delay = frame.delay;
                newFrame.bitmap = frame.bitmap->Clone(0, 0,
                    frame.bitmap->GetWidth(),
                    frame.bitmap->GetHeight(),
                    PixelFormat32bppARGB);
                originalFrames.push_back(newFrame);
            }
        }
    }

    void DanceAnimatedKonata(const std::wstring& path, int targetWidth, int targetHeight) {

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

        UINT delay = 1000 / fps;

        for (UINT i = 0; i < frameCount; i++) {
            gifImage->SelectActiveFrame(&frameDimension, i);
            Bitmap* frameBitmap = new Bitmap(targetWidth, targetHeight, PixelFormat32bppARGB);
            Graphics graphics(frameBitmap);
            graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            graphics.Clear(Gdiplus::Color(0, 0, 0, 0));

            graphics.DrawImage(gifImage, 0, 0, targetWidth, targetHeight);

            GifFrame frame;
            frame.bitmap = frameBitmap;
            frame.delay = delay;
            frames.push_back(frame);
        }

        delete gifImage;
    }

    void reloadGif() {
        if (timerId) {
            KillTimer(hwnd, timerId);
            timerId = 0;
        }
        for (auto& frame : frames) {
            if (frame.bitmap) {
                delete frame.bitmap;
                frame.bitmap = nullptr;
            }
        }
        frames.clear();
        DanceAnimatedKonata(gifPath, frameWidth, frameHeight);

        if (frames.empty()) {
            return;
        }
        SaveOriginalFrames();

        windowWidth = frames[0].bitmap->GetWidth() * scale / 100;
        windowHeight = frames[0].bitmap->GetHeight() * scale / 100;
        SetWindowPos(hwnd, nullptr, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);
        //restart timer
        timerId = SetTimer(hwnd, 1, frames[0].delay, nullptr);
        InvalidateRect(hwnd, nullptr, TRUE);
    }

    void SaveConfig() {
        SimpleJSON config;
        RECT rect;
        GetWindowRect(hwnd, &rect);
        config.setInt("konata_x", rect.left);
        config.setInt("konata_y", rect.top);
        config.setInt("konata_scale", scale);
        config.setInt("konata_fps", fps);
        config.setFloat("konata_saturation", currentSaturation);
        config.setInt("frame_width", frameWidth);
        config.setInt("frame_height", frameHeight);

        config.setFloat("effect_temperature", m_temperature);
        config.setFloat("effect_sepia", m_sepia);
        config.setFloat("effect_hue_shift", m_hueShift);
        config.setFloat("effect_gamma", m_gamma);
        config.setFloat("effect_exposure", m_exposure);
        config.setBool("effect_inverted", m_inverted);
        config.setFloat("effect_old_film", m_oldFilm);
        config.setFloat("effect_terminator", m_termIntensity);
        config.setBool("effect_bw", m_blackWhite);
        config.setInt("effect_bw_tint_r", m_bwTint.rgbRed);
        config.setInt("effect_bw_tint_g", m_bwTint.rgbGreen);
        config.setInt("effect_bw_tint_b", m_bwTint.rgbBlue);

        std::ifstream readFile("notes.json");
        if (readFile.is_open()) {
            std::stringstream buffer;
            buffer << readFile.rdbuf();
            std::string jsonStr = buffer.str();
            readFile.close();
            SimpleJSON existingConfig;
            if (existingConfig.parse(jsonStr)) {
                std::string notes = existingConfig.getString("notes", "");
                if (!notes.empty()) config.setString("notes", notes);
                int notesX = existingConfig.getInt("notes_x", 200);
                int notesY = existingConfig.getInt("notes_y", 200);
                config.setInt("notes_x", notesX);
                config.setInt("notes_y", notesY);
            }
        }

        std::ofstream file("notes.json");
        if (file.is_open()) { file << config.serialize(); file.close(); }
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
            float savedSaturation = config.getFloat("konata_saturation", 1.0f);
            int savedWidth = config.getInt("frame_width", 320);
            int savedHeight = config.getInt("frame_height", 240);

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
                if (timerId) KillTimer(hwnd, timerId);
                if (!frames.empty()) {
                    for (auto& frame : frames) frame.delay = 1000 / fps;
                    timerId = SetTimer(hwnd, 1, frames[0].delay, nullptr);
                }
            }

            if (savedWidth != frameWidth || savedHeight != frameHeight) {
                frameWidth = savedWidth;
                frameHeight = savedHeight;
                reloadGif();
            }

            if (savedSaturation != 1.0f) {
                currentSaturation = savedSaturation;
                setSaturation(currentSaturation);
            }

            //effect loader
            m_temperature = config.getFloat("effect_temperature", 0.0f);
            m_sepia = config.getFloat("effect_sepia", 0.0f);
            m_hueShift = config.getFloat("effect_hue_shift", 0.0f);
            m_gamma = config.getFloat("effect_gamma", 1.0f);
            m_exposure = config.getFloat("effect_exposure", 0.0f);
            m_inverted = config.getBool("effect_inverted", false);
            m_oldFilm = config.getFloat("effect_old_film", 0.0f);
            m_termIntensity = config.getFloat("effect_terminator", 0.0f);
            m_blackWhite = config.getBool("effect_bw", false);
            m_bwTint.rgbRed = config.getInt("effect_bw_tint_r", 255);
            m_bwTint.rgbGreen = config.getInt("effect_bw_tint_g", 255);
            m_bwTint.rgbBlue = config.getInt("effect_bw_tint_b", 255);

            if (!(m_temperature == 0.0f && m_sepia == 0.0f && m_hueShift == 0.0f &&
                m_gamma == 1.0f && m_exposure == 0.0f && !m_inverted &&
                m_oldFilm == 0.0f && m_termIntensity == 0.0f && !m_blackWhite)) {
                applyAllEffectstoframes();
            }
        }
    }

    void applyAllEffectstoframes() {
        for (auto& frame : frames) {
            if (!frame.bitmap) continue;

            int width = frame.bitmap->GetWidth();
            int height = frame.bitmap->GetHeight();

            BitmapData bitmapData;
            Rect rect(0, 0, width, height);

            if (frame.bitmap->LockBits(&rect, ImageLockModeRead | ImageLockModeWrite,
                PixelFormat32bppARGB, &bitmapData) == Ok) {

                BYTE* pixels = (BYTE*)bitmapData.Scan0;
                int stride = bitmapData.Stride;

                for (int y = 0; y < height; y++) {
                    BYTE* row = pixels + y * stride;

                    for (int x = 0; x < width; x++) {
                        int idx = x * 4;

                        BYTE b = row[idx];
                        BYTE g = row[idx + 1];
                        BYTE r = row[idx + 2];
                        BYTE a = row[idx + 3];

                        RGBQUAD color = { b, g, r, 0 };

                        if (currentSaturation != 1.0f) {
                            color = ApplySaturationToColor(color, currentSaturation);
                        }

                        color = ApplyEffectsToColor(color);

                        row[idx] = color.rgbBlue;
                        row[idx + 1] = color.rgbGreen;
                        row[idx + 2] = color.rgbRed;
                        row[idx + 3] = a;
                    }
                }
                frame.bitmap->UnlockBits(&bitmapData);
            }
        }
    }

    RGBQUAD ApplyEffectsToColor(RGBQUAD color) {
        if (m_exposure != 0.0f) {
            color = Colors::exposure(color, powf(2.0f, m_exposure));
        }

        if (m_gamma != 1.0f) {
            color = Colors::gammaCorrect(color, m_gamma);
        }

        if (m_hueShift != 0.0f) {
            color = Colors::shiftHue(color, m_hueShift);
        }

        if (m_temperature != 0.0f) {
            if (m_temperature > 0) {
                for (int i = 0; i < (int)(m_temperature * 2); i++) {
                    color = Colors::warm(color);
                }
            }
            else {
                for (int i = 0; i < (int)(-m_temperature * 2); i++) {
                    color = Colors::cool(color);
                }
            }
        }

        if (m_oldFilm > 0.0f) {
            RGBQUAD sepiaColor = Colors::sepiaIntensity(color, m_oldFilm * 0.7f);
            color = Colors::blend(color, sepiaColor, m_oldFilm * 0.5f);
            color = Colors::saturate(color, 1.0f - m_oldFilm * 0.3f);
        }

        if (m_sepia > 0.0f) {
            color = Colors::sepiaIntensity(color, m_sepia);
        }

        if (m_blackWhite) {
            BYTE gray = Colors::ultraFastGrayscale(color);
            color.rgbRed = (BYTE)(gray * m_bwTint.rgbRed / 255);
            color.rgbGreen = (BYTE)(gray * m_bwTint.rgbGreen / 255);
            color.rgbBlue = (BYTE)(gray * m_bwTint.rgbBlue / 255);
        }

        if (m_termIntensity > 0.0f) {
            RGBQUAD termColor = color;
            termColor.rgbRed = min(255, (int)(termColor.rgbRed * (1.0f + m_termIntensity)));
            termColor.rgbGreen = (BYTE)(termColor.rgbGreen * (1.0f - m_termIntensity * 0.7f));
            termColor.rgbBlue = (BYTE)(termColor.rgbBlue * (1.0f - m_termIntensity * 0.7f));

            if (m_termIntensity > 0.5f) {
                termColor = Colors::invert(termColor);
            }

            color = Colors::blend(color, termColor, m_termIntensity);
        }

        if (m_inverted) {
            color = Colors::invert(color);
        }

        return color;
    }

    void framelogicshet(HDC hdc) {
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

        graphics.Clear(Gdiplus::Color(0, 0, 0, 0));

        graphics.DrawImage(currentBitmap, x, y, drawWidth, drawHeight);
    }

    void ProcessEffect() {
        EnterCriticalSection(&frameLock);
        if (frames.empty()) {
            LeaveCriticalSection(&frameLock);
            isApplyingEffect = false;
            return;
        }

        restoreFromOG();

        if (currentSaturation != 1.0f) {
            ApplySaturationToFrames(currentSaturation);
        }

        LeaveCriticalSection(&frameLock);

        if (hwnd && IsWindow(hwnd)) {
            PostMessage(hwnd, WM_USER + 100, 0, 0);
        }
        isApplyingEffect = false;
    }

RGBQUAD ApplySaturationToColor(RGBQUAD color, float saturation) {
    if (saturation == 1.0f) return color;

    int r = color.rgbRed;
    int g = color.rgbGreen;
    int b = color.rgbBlue;

    int minVal = r;
    int maxVal = r;
    if (g < minVal) minVal = g;
    if (g > maxVal) maxVal = g;
    if (b < minVal) minVal = b;
    if (b > maxVal) maxVal = b;

    int delta = maxVal - minVal;
    int lightness = (maxVal + minVal) >> 1;
    float l = lightness / 255.0f;

    float h, s;

    if (delta == 0) {
        s = 0.0f;
        h = 0.0f;
    }
    else {
        if (l < 0.5f) {
            s = (float)delta / (maxVal + minVal);
        }
        else {
            s = (float)delta / (510 - maxVal - minVal);
        }

        if (maxVal == r) {
            h = (float)(g - b) / delta;
            if (h < 0) h += 6;
        }
        else if (maxVal == g) {
            h = (float)(b - r) / delta + 2;
        }
        else {
            h = (float)(r - g) / delta + 4;
        }
        h /= 6.0f;
    }

    s = s * saturation;
    if (s > 1.0f) s = 1.0f;

    //HSL 2 RGB
    float rOut, gOut, bOut;

    if (s == 0) {
        rOut = gOut = bOut = l;
    }
    else {
        float v = (l <= 0.5f) ? (l * (1 + s)) : (l + s - l * s);
        float m = l + l - v;
        float sv = (v - m) / v;
        float hue6 = h * 6;
        int sextant = (int)hue6;
        float fract = hue6 - sextant;
        float vsf = v * sv * fract;
        float mid1 = m + vsf;
        float mid2 = v - vsf;

        switch (sextant) {
        case 0: rOut = v; gOut = mid1; bOut = m; break;
        case 1: rOut = mid2; gOut = v; bOut = m; break;
        case 2: rOut = m; gOut = v; bOut = mid1; break;
        case 3: rOut = m; gOut = mid2; bOut = v; break;
        case 4: rOut = mid1; gOut = m; bOut = v; break;
        case 5: rOut = v; gOut = m; bOut = mid2; break;
        default: rOut = v; gOut = m; bOut = m; break;
        }
    }

    color.rgbRed = (BYTE)(rOut * 255);
    color.rgbGreen = (BYTE)(gOut * 255);
    color.rgbBlue = (BYTE)(bOut * 255);

    return color;
}

void ApplySaturationToFrames(float saturation) {
    if (saturation <= 0.0f || saturation == 1.0f) return;

    //math sheet maybe
    float satMultiplier = saturation;

    for (auto& frame : frames) {
        if (!frame.bitmap) continue;

        int width = frame.bitmap->GetWidth();
        int height = frame.bitmap->GetHeight();

        BitmapData bitmapData;
        Rect rect(0, 0, width, height);

        if (frame.bitmap->LockBits(&rect, ImageLockModeRead | ImageLockModeWrite,
            PixelFormat32bppARGB, &bitmapData) == Ok) {

            BYTE* pixels = (BYTE*)bitmapData.Scan0;
            int stride = bitmapData.Stride;

            //parallel lines obrabotka
#pragma omp parallel for
            for (int y = 0; y < height; y++) {
                BYTE* row = pixels + y * stride;

                for (int x = 0; x < width; x++) {
                    int idx = x * 4;

                    BYTE b = row[idx];
                    BYTE g = row[idx + 1];
                    BYTE r = row[idx + 2];
                    BYTE a = row[idx + 3];

                    //fast RGB 2 HSL
                    int minVal = r;
                    int maxVal = r;
                    if (g < minVal) minVal = g;
                    if (g > maxVal) maxVal = g;
                    if (b < minVal) minVal = b;
                    if (b > maxVal) maxVal = b;

                    int delta = maxVal - minVal;
                    int lightness = (maxVal + minVal) >> 1;// /2

                    float h, s, l;
                    l = lightness / 255.0f;

                    if (delta == 0) {
                        s = 0.0f;
                        h = 0.0f;
                    }
                    else {
                        if (l < 0.5f) {
                            s = (float)delta / (maxVal + minVal);
                        }
                        else {
                            s = (float)delta / (510 - maxVal - minVal);
                        }

                        //HUE
                        if (maxVal == r) {
                            h = (float)(g - b) / delta;
                            if (h < 0) h += 6;
                        }
                        else if (maxVal == g) {
                            h = (float)(b - r) / delta + 2;
                        }
                        else {
                            h = (float)(r - g) / delta + 4;
                        }
                        h /= 6.0f;
                    }

                    //saturation
                    s = s * satMultiplier;
                    if (s > 1.0f) s = 1.0f;

                    //HSL 2 RGB numberss onleh
                    float rOut, gOut, bOut;

                    if (s == 0) {
                        rOut = gOut = bOut = l;
                    }
                    else {
                        float v = (l <= 0.5f) ? (l * (1 + s)) : (l + s - l * s);
                        float m = l + l - v;
                        float sv = (v - m) / v;
                        float hue6 = h * 6;
                        int sextant = (int)hue6;
                        float fract = hue6 - sextant;
                        float vsf = v * sv * fract;
                        float mid1 = m + vsf;
                        float mid2 = v - vsf;

                        switch (sextant) {
                        case 0: rOut = v; gOut = mid1; bOut = m; break;
                        case 1: rOut = mid2; gOut = v; bOut = m; break;
                        case 2: rOut = m; gOut = v; bOut = mid1; break;
                        case 3: rOut = m; gOut = mid2; bOut = v; break;
                        case 4: rOut = mid1; gOut = m; bOut = v; break;
                        case 5: rOut = v; gOut = m; bOut = mid2; break;
                        default: rOut = v; gOut = m; bOut = m; break;
                        }
                    }
                    row[idx] = (BYTE)(bOut * 255);
                    row[idx + 1] = (BYTE)(gOut * 255);
                    row[idx + 2] = (BYTE)(rOut * 255);
                    row[idx + 3] = a;
                }
            }

            frame.bitmap->UnlockBits(&bitmapData);
        }
    }
}

    void ProcessSaturationChange() {
        EnterCriticalSection(&frameLock);
        restoreFromOG();
        if (currentSaturation != 1.0f) {
            ApplySaturationToFrames(currentSaturation);
        }
        LeaveCriticalSection(&frameLock);
        InvalidateRect(hwnd, nullptr, TRUE);
        isApplyingEffect = false;
    }

    void ProcessEffectsChange() {
        EnterCriticalSection(&frameLock);

        restoreFromOG();

        if (currentSaturation != 1.0f) {
            ApplySaturationToFrames(currentSaturation);
        }
        if (!(m_temperature == 0.0f && m_sepia == 0.0f && m_hueShift == 0.0f &&
            m_gamma == 1.0f && m_exposure == 0.0f && !m_inverted &&
            m_oldFilm == 0.0f && m_termIntensity == 0.0f && !m_blackWhite)) {
            applyAllEffectstoframes();
        }
        LeaveCriticalSection(&frameLock);
        InvalidateRect(hwnd, nullptr, TRUE);
        SaveConfig();
    }

    static DWORD WINAPI SaturationThreadProc(LPVOID param) {
        KonataChan* pThis = (KonataChan*)param;
        pThis->ProcessSaturationChange();
        return 0;
    }

    void UpdateWindowSize() {
        if (!frames.empty()) {
            windowWidth = frames[0].bitmap->GetWidth() * scale / 100;
            windowHeight = frames[0].bitmap->GetHeight() * scale / 100;
            SetWindowPos(hwnd, nullptr, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);
            InvalidateRect(hwnd, nullptr, TRUE);
            SaveConfig();
        }
    }

    void SetFPS(int newFps) {
        fps = newFps;
        if (!frames.empty()) {
            for (auto& frame : frames) frame.delay = 1000 / fps;
            if (timerId) KillTimer(hwnd, timerId);
            timerId = SetTimer(hwnd, 1, frames[0].delay, nullptr);
        }
        SaveConfig();
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_USER + 100: InvalidateRect(hWnd, nullptr, TRUE); return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rect;
            GetClientRect(hWnd, &rect);
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
            framelogicshet(memDC);
            BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND: return 1;

        case WM_COMMAND:
        {
            int cmd = LOWORD(wParam);

            if (cmd == ID_TRAY_PRIZE) {
                ActivatePrize(hwnd);
                break;
            }
        }
        break;

        case WM_MOUSEMOVE: {
            if (isDragging) {
                POINT mousePos;
                GetCursorPos(&mousePos);
                SetWindowPos(hWnd, nullptr, mousePos.x - dragStart.x, mousePos.y - dragStart.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
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
            static KonataMenu* g_menu = nullptr;
            // Там где создается g_menu (в WM_RBUTTONDOWN):
            if (!g_menu) {
                g_menu = new KonataMenu();
                g_menu->SetCallbacks(
                    this,
                    KonataChan::OnScaleChangeStatic,
                    KonataChan::OnFpsChangeStatic,
                    KonataChan::OnSaturationChangeStatic,
                    KonataChan::OnFrameSizeChangeStatic,
                    KonataChan::OnEffectChangeStatic,
                    KonataChan::OnPrizeStatic,
                    KonataChan::OnExitStatic
                );
                g_menu->SetApplyEffectCallback([](void* target, int effectId) {
                    KonataChan* konata = (KonataChan*)target;
                    konata->ApplyEffectById(effectId);
                    });
            }

            g_menu->UpdateSaturation(currentSaturation);
            g_menu->UpdateHueShift(m_hueShift);
            g_menu->UpdateOldFilm(m_oldFilm);
            g_menu->UpdateTerminator(m_termIntensity);
            g_menu->UpdateInverted(m_inverted);

            POINT mousePos;
            GetCursorPos(&mousePos);
            g_menu->Show(hWnd, mousePos.x, mousePos.y);
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
            if (wParam == 1) {
                if (!frames.empty()) {
                    currentFrame = (currentFrame + 1) % frames.size();
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
            }
            return 0;
        }

        case WM_DESTROY: {
            if (timerId) {
                KillTimer(hWnd, timerId);
                timerId = 0;
            }
            SaveConfig();
            for (auto& frame : frames) {
                if (frame.bitmap) {
                    delete frame.bitmap;
                    frame.bitmap = nullptr;
                }
            }
            frames.clear();
            for (auto& frame : originalFrames) {
                if (frame.bitmap) {
                    delete frame.bitmap;
                    frame.bitmap = nullptr;
                }
            }
            originalFrames.clear();
            DontRespond(hWnd);
            return 0;
        }
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
public:
    KonataChan(const std::wstring& path)
        : hwnd(nullptr), currentFrame(0), isDragging(false), scale(100), fps(30),
        gifPath(path), windowWidth(250), windowHeight(250), timerId(0),
        currentSaturation(1.0f), isSaturating(false), frameWidth(320), frameHeight(240), m_initialized(false)
    {
    }

    ~KonataChan() {
        if (timerId) {
            KillTimer(hwnd, timerId);
            timerId = 0;
        }

        for (auto& frame : frames) {
            if (frame.bitmap) {
                delete frame.bitmap;
                frame.bitmap = nullptr;
            }
        }
        frames.clear();

        for (auto& frame : originalFrames) {
            if (frame.bitmap) {
                delete frame.bitmap;
                frame.bitmap = nullptr;
            }
        }
        originalFrames.clear();

        DeleteCriticalSection(&frameLock);

        if (hwnd && IsWindow(hwnd)) {
            DestroyWindow(hwnd);
            hwnd = nullptr;
        }
    }

    bool Init() {
        InitializeCriticalSection(&frameLock);
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wc.lpszClassName = L"KonataChan";

        if (!RegisterClassEx(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }

        hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            L"KonataChan", L"Konata Dancer", WS_POPUP,
            100, 100, windowWidth, windowHeight,
            nullptr, nullptr, GetModuleHandle(nullptr), this);

        if (!hwnd) {
            return false;
        }

        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

        DanceAnimatedKonata(gifPath, frameWidth, frameHeight);

        if (frames.empty()) {
            return false;
        }

        SaveOriginalFrames();

        LoadConfig();

        if (!frames.empty()) {
            timerId = SetTimer(hwnd, 1, frames[0].delay, nullptr);
            if (!timerId) {
            }
        }

        m_initialized = true;
        return true;
    }


    static void OnScaleChangeStatic(void* target, int scale);
    static void OnFpsChangeStatic(void* target, int fps);
    static void OnSaturationChangeStatic(void* target, float sat);
    static void OnFrameSizeChangeStatic(void* target, int w, int h);
    static void OnEffectChangeStatic(void* target);
    static void OnPrizeStatic(void* target);
    static void OnExitStatic(void* target);

    void ApplyEffectById(int effectId) {
        switch (effectId) {
        case 40: resetEffects(); break;
        case 41: SetWarm(); break;
        case 42: SetCool(); break;
        case 43: SetSepia(0.5f); break;
        case 44: SetSepia(1.0f); break;
        case 45: SetVintage(); break;
        case 46: SetHueShift(m_hueShift + 0.1f); break;
        case 47: SetHueShift(m_hueShift - 0.1f); break;
        case 48: SetHueShift(0.0f); break;
        case 49: SetGamma(0.8f); break;
        case 50: SetGamma(1.0f); break;
        case 51: SetGamma(1.2f); break;
        case 52: SetExposure(m_exposure + 1.0f); break;
        case 53: SetExposure(m_exposure - 1.0f); break;
        case 54: SetExposure(0.0f); break;
        case 55: SetOldFilm(1.0f); break;
        case 56: SetTerminator(1.0f); break;
        case 57: SetBlackAndWhite(); break;
        case 58: SetSepiaBW(); break;
        case 59: SetInverted(!m_inverted); break;
        }
    }

    void SetTemperature(float temp) {
        m_temperature = max(-1.0f, min(1.0f, temp));
        ProcessEffectsChange();
    }

    void SetWarm() { SetTemperature(0.5f); }
    void SetCool() { SetTemperature(-0.5f); }

    void SetSepia(float intensity) {
        m_sepia = max(0.0f, min(1.0f, intensity));
        ProcessEffectsChange();
    }

    void SetVintage() {
        m_sepia = 0.6f;
        m_gamma = 1.1f;
        ProcessEffectsChange();
    }

    void SetHueShift(float shift) {
        m_hueShift = fmod(shift, 1.0f);
        if (m_hueShift < 0) m_hueShift += 1.0f;
        ProcessEffectsChange();
    }

    void SetGamma(float gamma) {
        m_gamma = max(0.5f, min(2.0f, gamma));
        ProcessEffectsChange();
    }

    void SetExposure(float ev) {
        m_exposure = max(-2.0f, min(2.0f, ev));
        ProcessEffectsChange();
    }

    void SetOldFilm(float intensity = 1.0f) {
        m_oldFilm = max(0.0f, min(1.0f, intensity));
        ProcessEffectsChange();
    }

    void SetTerminator(float intensity = 1.0f) {
        m_termIntensity = max(0.0f, min(1.0f, intensity));
        ProcessEffectsChange();
    }

    void SetBlackAndWhite(RGBQUAD tint = { 255,255,255,0 }) {
        m_blackWhite = true;
        m_bwTint = tint;
        ProcessEffectsChange();
    }

    void SetSepiaBW() {
        m_blackWhite = true;
        m_bwTint = { 210, 180, 140, 0 };
        ProcessEffectsChange();
    }

    void DisableBlackAndWhite() {
        m_blackWhite = false;
        ProcessEffectsChange();
    }

    void SetInverted(bool invert) {
        m_inverted = invert;
        ProcessEffectsChange();
    }

    void resetEffects() {
        m_temperature = 0.0f;
        m_sepia = 0.0f;
        m_hueShift = 0.0f;
        m_gamma = 1.0f;
        m_exposure = 0.0f;
        m_inverted = false;
        m_oldFilm = 0.0f;
        m_termIntensity = 0.0f;
        m_blackWhite = false;
        ProcessEffectsChange();
    }

    void restoreFromOG() {
        if (originalFrames.empty()) return;

        for (auto& frame : frames) {
            if (frame.bitmap) {
                delete frame.bitmap;
                frame.bitmap = nullptr;
            }
        }
        frames.clear();

        for (auto& frame : originalFrames) {
            if (frame.bitmap) {
                GifFrame newFrame;
                newFrame.delay = frame.delay;
                newFrame.bitmap = frame.bitmap->Clone(0, 0,
                    frame.bitmap->GetWidth(),
                    frame.bitmap->GetHeight(),
                    PixelFormat32bppARGB);
                frames.push_back(newFrame);
            }
        }
    }

    void setSaturation(float saturation) {
        if (currentSaturation == saturation) return;
        if (isApplyingEffect) return;

        currentSaturation = saturation;
        isApplyingEffect = true;
        //effect magic
        HANDLE hThread = CreateThread(nullptr, 0, EffectThreadProc, this, 0, nullptr);
        SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);
        CloseHandle(hThread);
    }

    void SetFrameSize(int width, int height) {
        if (frameWidth == width && frameHeight == height) return;
        frameWidth = width;
        frameHeight = height;
        reloadGif();

        SaveOriginalFrames();

        SaveConfig();
    }

    void SetFrameSizeAndSaturation(int width, int height, float saturation) {
        frameWidth = width;
        frameHeight = height;
        currentSaturation = saturation;
        reloadGif();
        if (saturation != 1.0f) {
            setSaturation(saturation);
        }
    }

    void Show() { ShowWindow(hwnd, SW_SHOW); UpdateWindow(hwnd); }
    void Hide() { ShowWindow(hwnd, SW_HIDE); }
    HWND GetHWND() { return hwnd; }
    bool IsVisible() { return IsWindowVisible(hwnd) != FALSE; }
};

void KonataChan::OnScaleChangeStatic(void* target, int scale) {
    KonataChan* pThis = (KonataChan*)target;
    pThis->scale = scale;
    pThis->UpdateWindowSize();
}

void KonataChan::OnFpsChangeStatic(void* target, int fps) {
    KonataChan* pThis = (KonataChan*)target;
    pThis->SetFPS(fps);
}

void KonataChan::OnSaturationChangeStatic(void* target, float sat) {
    KonataChan* pThis = (KonataChan*)target;
    pThis->setSaturation(sat);
    pThis->SaveConfig();
}

void KonataChan::OnFrameSizeChangeStatic(void* target, int w, int h) {
    KonataChan* pThis = (KonataChan*)target;
    pThis->SetFrameSize(w, h);
}

void KonataChan::OnEffectChangeStatic(void* target) {
    KonataChan* pThis = (KonataChan*)target;
    pThis->SaveConfig();
}

void KonataChan::OnPrizeStatic(void* target) {
    KonataChan* pThis = (KonataChan*)target;
    ActivatePrize(pThis->hwnd);
}

void KonataChan::OnExitStatic(void* target) {
    KonataChan* pThis = (KonataChan*)target;
    DontRespond(pThis->hwnd);
}

class ToastNotification {
private:
    HWND hwnd;
    UINT_PTR timerId;
    std::wstring titleText;
    std::wstring messageText;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        ToastNotification* pThis = reinterpret_cast<ToastNotification*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (pThis) return pThis->HandleMessage(hwnd, msg, wParam, lParam);
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void DrawWhiteThing(HDC hdc, const RECT& rect) {
        HBRUSH hBrush = CreateSolidBrush(RGB(245, 245, 255));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);
    }
    void DrawXPStyleBorder(HDC hdc, const RECT& rect) {
        HPEN hPenLight = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        HPEN hPenDark = CreatePen(PS_SOLID, 1, RGB(100, 100, 120));

        //up and left white
        SelectObject(hdc, hPenLight);
        MoveToEx(hdc, 0, rect.bottom - 1, NULL);
        LineTo(hdc, 0, 0);
        LineTo(hdc, rect.right - 1, 0);

        //bottom and up rects? are dark
        SelectObject(hdc, hPenDark);
        LineTo(hdc, rect.right - 1, rect.bottom - 1);
        LineTo(hdc, 0, rect.bottom - 1);

        DeleteObject(hPenLight);
        DeleteObject(hPenDark);
    }

    void DrawXPIcon(HDC hdc, int x, int y) {
        //i ico
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 100, 200));
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 100));

        SelectObject(hdc, hBrush);
        SelectObject(hdc, hPen);
        Ellipse(hdc, x, y, x + 32, y + 32);

        //i shit
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        HFONT hFont = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Times New Roman");
        SelectObject(hdc, hFont);
        TextOutW(hdc, x + 12, y + 2, L"i", 1);

        DeleteObject(hFont);
        DeleteObject(hBrush);
        DeleteObject(hPen);
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rect;
            GetClientRect(hWnd, &rect);

            //boofer
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

            HBRUSH hBrushBg = CreateSolidBrush(RGB(224, 240, 255));
            FillRect(hdcMem, &rect, hBrushBg);
            DeleteObject(hBrushBg);

            DrawWhiteThing(hdc, rect);

            DrawXPStyleBorder(hdcMem, rect);

            DrawXPIcon(hdcMem, 8, 18);

            SetBkMode(hdcMem, TRANSPARENT);

            //title
            HFONT hFontTitle = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH, L"Tahoma");
            SelectObject(hdcMem, hFontTitle);

            SetTextColor(hdcMem, RGB(100, 100, 120));
            TextOutW(hdcMem, 47, 13, titleText.c_str(), titleText.length());
            SetTextColor(hdcMem, RGB(10, 36, 106));
            TextOutW(hdcMem, 46, 12, titleText.c_str(), titleText.length());

            //msg
            HFONT hFontMsg = CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH, L"Tahoma");
            SelectObject(hdcMem, hFontMsg);

            SetTextColor(hdcMem, RGB(100, 100, 120));
            TextOutW(hdcMem, 47, 33, messageText.c_str(), messageText.length());
            SetTextColor(hdcMem, RGB(0, 0, 0));
            TextOutW(hdcMem, 46, 32, messageText.c_str(), messageText.length());

            //X
            HPEN hPenClose = CreatePen(PS_SOLID, 1, RGB(100, 100, 120));
            SelectObject(hdcMem, hPenClose);

            MoveToEx(hdcMem, rect.right - 18, 8, NULL);
            LineTo(hdcMem, rect.right - 8, 18);
            MoveToEx(hdcMem, rect.right - 18, 18, NULL);
            LineTo(hdcMem, rect.right - 8, 8);
            Rectangle(hdcMem, rect.right - 20, 6, rect.right - 6, 20);

            BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
            DeleteObject(hFontTitle);
            DeleteObject(hFontMsg);
            DeleteObject(hPenClose);

            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_TIMER:
            if (wParam == timerId) {
                KillTimer(hWnd, timerId);
                //anim
                for (int i = 255; i > 0; i -= 25) {
                    SetLayeredWindowAttributes(hWnd, 0, i, LWA_ALPHA);
                    Sleep(15);
                }
                DestroyWindow(hWnd);
            }
            return 0;
        case WM_LBUTTONDOWN: {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            RECT rect;
            GetClientRect(hWnd, &rect);
            if (pt.x > rect.right - 20 && pt.x < rect.right - 6 &&
                pt.y > 6 && pt.y < 20) {
                DestroyWindow(hWnd);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            //highlight(mb doesn't work)
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hWnd;
            TrackMouseEvent(&tme);
            return 0;
        }
        case WM_MOUSELEAVE: {
            InvalidateRect(hWnd, NULL, TRUE);
            return 0;
        }
        case WM_DESTROY:
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

public:
    ToastNotification(const std::wstring& title, const std::wstring& message, int duration = 4000)
        : titleText(title), messageText(message) {

        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wc.lpszClassName = L"XPStyleToast";
        RegisterClassEx(&wc);

        int width = 320;
        int height = 75;
        int x = GetSystemMetrics(SM_CXSCREEN) - width - 15;
        int y = GetSystemMetrics(SM_CYSCREEN) - height - 30;

        hwnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            L"XPStyleToast", L"", WS_POPUP,
            x, y, width, height,
            NULL, NULL, GetModuleHandle(NULL), this);

        if (!hwnd) return;

        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        SetLayeredWindowAttributes(hwnd, 0, 245, LWA_ALPHA);

        //anim
        for (int i = 0; i <= 255; i += 15) {
            SetLayeredWindowAttributes(hwnd, 0, i, LWA_ALPHA);
            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);
            Sleep(5);
        }

        timerId = SetTimer(hwnd, 1, duration, NULL);
    }

    ~ToastNotification() {
        if (timerId) KillTimer(hwnd, timerId);
        if (hwnd) DestroyWindow(hwnd);
    }
};

//glob variables
KonataChan* g_konataWindow = nullptr;
HWND g_hTrayHwnd = nullptr;
const wchar_t* g_gifPath = L"Konata.gif";
bool g_bluehairEnabled = false;
HHOOK g_bluehairHook = NULL;
HHOOK g_bluehairMsgHook = NULL;
bool g_bShutdown = false;
ULONG_PTR g_gdiplusToken = 0;


//additional shit
void KonataShow() {
    if (g_konataWindow) g_konataWindow->Show();
}

void KonataHide() {
    if (g_konataWindow) g_konataWindow->Hide();
}

bool KonataIsVisible() {
    return g_konataWindow ? g_konataWindow->IsVisible() : false;
}

bool FileExists(const std::wstring& path) {
    std::ifstream file(path.c_str());
    return file.good();
}

void createwindows() {
    if (!g_konataWindow) {
        g_konataWindow = new KonataChan(g_gifPath);
    }
}

void MessageLoopadditional() {
    HWND hMessageOnly = CreateWindow(L"STATIC", L"MsgHandler",
        WS_POPUP, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    //DEP shit
    typedef BOOL(WINAPI* LPFN_SETDEP)(DWORD);
    LPFN_SETDEP setDep = (LPFN_SETDEP)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetProcessDEPPolicy");
    if (setDep) {
        setDep(PROCESS_DEP_ENABLE);
    }

    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Konatachan");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        XPMessageBox::Show(NULL,
            L"execution is already running dummy",
            L"Konataсhan",
            XPMessageBox::IconType::Warning,
            XPMessageBox::ButtonType::OK,
            XPMessageBox::DialogResult::OK,
            XPMessageBox::WindowMode::Dialog);
        CloseHandle(hMutex);
        return 0;
    }

    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;

    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }

    StartTrayAnimation();

    if (!FileExists(g_gifPath)) {
        XPMessageBox::Show(NULL,
            L"Konata.gif not found or fucking terminated",
            L"ERROR",
            XPMessageBox::IconType::Error,
            XPMessageBox::ButtonType::OK,
            XPMessageBox::DialogResult::OK,
            XPMessageBox::WindowMode::Dialog);

        GdiplusShutdown(gdiplusToken);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }
    g_konataWindow = new KonataChan(g_gifPath);
    if (!g_konataWindow) {
    }
    else if (!g_konataWindow->Init()) {
        delete g_konataWindow;
        g_konataWindow = nullptr;
    }
    else {
        g_konataWindow->Show();
        new ToastNotification(L"Konata-chan", L"well this app works fine i guess", 3000);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (g_konataWindow) {
        delete g_konataWindow;
        g_konataWindow = nullptr;
    }

    GdiplusShutdown(gdiplusToken);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    StopTrayAnimation();

    return 0;
}