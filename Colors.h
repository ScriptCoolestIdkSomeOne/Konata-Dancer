#pragma once

#include <windows.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#include <numbers>
#include <array>
#include <mutex>
#include <chrono>
#include <span>

using RGBQUAD = ::RGBQUAD;

#define _USE_MATH_DEFINES

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923f
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

enum class BlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    Difference,
    Exclusion
};

enum class DeltaEMetric {
    CIE76,
    CIE94,
    CIEDE2000,
    CMC,
    LCH
};

//magic number shit
//TODO:replace all magic numvers to these shitty shits
namespace ColorConstants {
    //RGB to XYZ conversion matrix (sRGB)
    constexpr float XYZ_MATRIX_RX = 0.4124564f;
    constexpr float XYZ_MATRIX_RY = 0.3575761f;
    constexpr float XYZ_MATRIX_RZ = 0.1804375f;
    constexpr float XYZ_MATRIX_GX = 0.2126729f;
    constexpr float XYZ_MATRIX_GY = 0.7151522f;
    constexpr float XYZ_MATRIX_GZ = 0.0721750f;
    constexpr float XYZ_MATRIX_BX = 0.0193339f;
    constexpr float XYZ_MATRIX_BY = 0.1191920f;
    constexpr float XYZ_MATRIX_BZ = 0.9503041f;

    //XYZ to RGB conversion matrix
    constexpr float RGB_MATRIX_RX = 3.2404542f;
    constexpr float RGB_MATRIX_RY = -1.5371385f;
    constexpr float RGB_MATRIX_RZ = -0.4985314f;
    constexpr float RGB_MATRIX_GX = -0.9692660f;
    constexpr float RGB_MATRIX_GY = 1.8760108f;
    constexpr float RGB_MATRIX_GZ = 0.0415560f;
    constexpr float RGB_MATRIX_BX = 0.0556434f;
    constexpr float RGB_MATRIX_BY = -0.2040259f;
    constexpr float RGB_MATRIX_BZ = 1.0572252f;

    //CIE LAB reference white points (D65)
    constexpr float LAB_WHITE_X = 0.95047f;
    constexpr float LAB_WHITE_Y = 1.0f;
    constexpr float LAB_WHITE_Z = 1.08883f;

    //LAB conversion constants
    constexpr float LAB_EPSILON = 0.008856f;
    constexpr float LAB_KAPPA = 903.3f;
    constexpr float LAB_KAPPA_DIV_116 = 16.0f / 116.0f;
    constexpr float LAB_F_THRESHOLD = 7.787f;

    //YUV conversion constants (BT.601)
    constexpr float YUV_R_Y = 0.299f;
    constexpr float YUV_G_Y = 0.587f;
    constexpr float YUV_B_Y = 0.114f;
    constexpr float YUV_R_U = -0.14713f;
    constexpr float YUV_G_U = -0.28886f;
    constexpr float YUV_B_U = 0.436f;
    constexpr float YUV_R_V = 0.615f;
    constexpr float YUV_G_V = -0.51499f;
    constexpr float YUV_B_V = -0.10001f;
    constexpr float YUV_V_R = 1.13983f;
    constexpr float YUV_U_G = -0.39465f;
    constexpr float YUV_V_G = -0.58060f;
    constexpr float YUV_U_B = 2.03211f;

    //Color scheme angles in turns, where 1.0 = 360°
    constexpr float COMPLEMENTARY_TURN = 0.5f;      // 180°
    constexpr float SPLIT_COMP_1_TURN = 0.4167f;    // 150°
    constexpr float SPLIT_COMP_2_TURN = 0.5833f;    // 210°
    constexpr float TRIADIC_STEP = 0.3333f;         // 120°
    constexpr float TETRADIC_STEP = 0.25f;          // 90°
    constexpr float ANALOGOUS_STEP = 0.0833f;       // 30°

    //Gamma correction
    constexpr float GAMMA_SRGB_CUTOFF = 0.04045f;
    constexpr float GAMMA_SRGB_A = 0.055f;
    constexpr float GAMMA_SRGB_B = 12.92f;
    constexpr float GAMMA_LINEAR_CUTOFF = 0.0031308f;
    constexpr float GAMMA_LINEAR_A = 0.055f;
    constexpr float GAMMA_LINEAR_B = 12.92f;

    //Color temperature constants
    constexpr float TEMP_CUTOFF = 66.0f;
    constexpr float TEMP_DIVISOR = 100.0f;
    constexpr float TEMP_RED_R1 = 329.698727446f;
    constexpr float TEMP_RED_R2 = -0.1332047592f;
    constexpr float TEMP_GREEN_R1 = 99.4708025861f;
    constexpr float TEMP_GREEN_R2 = -161.1195681661f;
    constexpr float TEMP_GREEN_G1 = 288.1221695283f;
    constexpr float TEMP_GREEN_G2 = -0.0755148492f;
    constexpr float TEMP_BLUE_R1 = 138.5177312231f;
    constexpr float TEMP_BLUE_R2 = -305.0447927307f;
    constexpr float TEMP_BLUE_CUTOFF = 19.0f;

    //LUT size
    constexpr int LUT_SIZE = 256;

    //animation easing constants
    constexpr float EASE_ELASTIC_FREQ = 13.0f;
    constexpr float EASE_ELASTIC_MAG = 10.0f;
    constexpr float EASE_BOUNCE_D1 = 2.75f;
    constexpr float EASE_BOUNCE_D2 = 1.5f;
    constexpr float EASE_BOUNCE_D3 = 2.25f;
    constexpr float EASE_BOUNCE_D4 = 2.625f;
    constexpr float EASE_BOUNCE_C1 = 7.5625f;
    constexpr float EASE_BOUNCE_C2 = 0.75f;
    constexpr float EASE_BOUNCE_C3 = 0.9375f;
    constexpr float EASE_BOUNCE_C4 = 0.984375f;

    //Sepia filter constants
    constexpr float SEPIA_R1 = 0.393f;
    constexpr float SEPIA_R2 = 0.769f;
    constexpr float SEPIA_R3 = 0.189f;
    constexpr float SEPIA_G1 = 0.349f;
    constexpr float SEPIA_G2 = 0.686f;
    constexpr float SEPIA_G3 = 0.168f;
    constexpr float SEPIA_B1 = 0.272f;
    constexpr float SEPIA_B2 = 0.534f;
    constexpr float SEPIA_B3 = 0.131f;

    //luminance constants (Rec. 709)
    constexpr float LUM_R = 0.2126f;
    constexpr float LUM_G = 0.7152f;
    constexpr float LUM_B = 0.0722f;

    //WCAG contrast ratios
    constexpr float WCAG_AA_RATIO = 4.5f;
    constexpr float WCAG_AAA_RATIO = 7.0f;

    //temperature adjustment
    constexpr float TEMP_WARM_SHIFT = 0.1f;
    constexpr float TEMP_COOL_SHIFT = -0.1f;
    constexpr float TEMP_SAT_BOOST = 1.2f;
}

//color structures
struct IPT {
    float I;
    float P;
    float T;
};

struct YCoCg {
    float Y;
    float Co;
    float Cg;
};

struct ICtCp {
    float I;
    float Ct;
    float Cp;// Intensity, Blue-Yellow, Red-Green
};

struct Oklab {
    float L;
    float a;
    float b;
};

struct Oklch {
    float L;
    float C;
    float h;
};

struct xyY {
    float x, y, Y;  //x,y chromaticity, Y luminance
};

//HSL color Hue Saturation Lightness
struct HSL {
    float h; //Hue 0-1
    float s; //Saturation 0-1
    float l; //Lightness 0-1
};

//HSV color Hue Saturation Value
struct HSV {
    float h; //Hue 0-360
    float s; //Saturation 0-1
    float v; //Value 0-1
};

//CIE L*a*b* color space
struct LAB {
    float L;  //lightness 0-100
    float a;  //green(-) to Red(+)
    float b;  //blue(-) to Yellow(+)
};

//CIE XYZ color space
struct XYZ {
    float X, Y, Z;
};

//YUV color space video
struct YUV {
    float Y, U, V;
};

//CMYK color space print
struct CMYK {
    float C, M, Y, K;
};

//LCH color space Lightness Chroma Hue
struct LCH {
    float L, C, H;
};

//YCbCr color space digital video
struct YCbCr {
    float Y, Cb, Cr;
};

//RGBA with alpha channel
struct RGBA {
    BYTE r, g, b, a;

    RGBA() : r(0), g(0), b(0), a(255) {}
    RGBA(BYTE red, BYTE green, BYTE blue, BYTE alpha = 255) : r(red), g(green), b(blue), a(alpha) {}
    RGBA(RGBQUAD rgb) : r(rgb.rgbRed), g(rgb.rgbGreen), b(rgb.rgbBlue), a(255) {}
    RGBA(COLORREF color) : r(GetRValue(color)), g(GetGValue(color)), b(GetBValue(color)), a(255) {}

    operator RGBQUAD() const {
        RGBQUAD rgb = { r, g, b, 0 };
        return rgb;
    }

    operator COLORREF() const {
        return RGB(r, g, b);
    }
};

namespace Colors {
    //forward declarations
    inline RGBQUAD gammaCorrect(RGBQUAD rgb, float gamma);
    inline RGBQUAD levels(RGBQUAD rgb, float inputBlack, float inputWhite, float outputBlack, float outputWhite);
    inline RGBQUAD exposure(RGBQUAD rgb, float ev);
    inline RGBQUAD vibrance(RGBQUAD rgb, float amount);
    inline RGBQUAD adjustHSB(RGBQUAD rgb, float hueShift, float saturationFactor, float brightnessFactor);

    // New forward declarations
    inline LCH lab2lch(LAB lab);
    inline LAB lch2lab(LCH lch);
    inline YCbCr rgb2ycbcr(RGBQUAD rgb);
    inline RGBQUAD ycbcr2rgb(YCbCr ycbcr);
    inline xyY xyz2xyy(XYZ xyz);
    inline XYZ xyy2xyz(xyY xyy);
    inline RGBQUAD linear2rgb(float r, float g, float b);
    inline void rgb2linear(RGBQUAD rgb, float& r, float& g, float& b);
    inline ICtCp rgb2ictcp(RGBQUAD rgb);
    inline YCoCg rgb2ycocg(RGBQUAD rgb);
    inline RGBQUAD ycocg2rgb(YCoCg ycocg);
    inline Oklab rgb2oklab(RGBQUAD rgb);
    inline RGBQUAD oklab2rgb(Oklab lab);
    inline Oklch oklab2oklch(Oklab lab);
    inline Oklab oklch2oklab(Oklch lch);
    inline IPT rgb2ipt(RGBQUAD rgb);
    inline float colorDistanceCIE94(RGBQUAD c1, RGBQUAD c2);
    inline float colorDistanceCIEDE2000(RGBQUAD c1, RGBQUAD c2);
    inline float colorDistanceCMC(RGBQUAD c1, RGBQUAD c2, float l = 2.0f, float c = 1.0f);
    inline float colorDistanceLCH(LCH lch1, LCH lch2);
    inline float colorDistance(RGBQUAD c1, RGBQUAD c2, DeltaEMetric metric = DeltaEMetric::CIEDE2000);
    inline RGBA alphaCompose(RGBA src, RGBA dst, BlendMode mode = BlendMode::Normal);
    inline RGBA premultiplyAlpha(RGBA color);
    inline RGBA unpremultiplyAlpha(RGBA color);
    inline RGBQUAD gammaRec709(RGBQUAD rgb);
    inline RGBQUAD fastBlendNoExcept(RGBQUAD c1, RGBQUAD c2, uint32_t factor) noexcept;
    inline BYTE fastLuminanceNoExcept(RGBQUAD rgb) noexcept;

    inline HSL rgb2hsl(RGBQUAD rgb) {
        HSL hsl = { 0 };
        float r = rgb.rgbRed / 255.f;
        float g = rgb.rgbGreen / 255.f;
        float b = rgb.rgbBlue / 255.f;

        float rgbMin = (std::min)((std::min)(r, g), b);
        float rgbMax = (std::max)((std::max)(r, g), b);
        float fDelta = rgbMax - rgbMin;

        hsl.l = (rgbMax + rgbMin) / 2.f;

        if (fDelta != 0.f) {
            hsl.s = hsl.l < .5f ? fDelta / (rgbMax + rgbMin) : fDelta / (2.f - rgbMax - rgbMin);

            if (r == rgbMax)      hsl.h = (g - b) / fDelta + (g < b ? 6.f : 0.f);
            else if (g == rgbMax) hsl.h = (b - r) / fDelta + 2.f;
            else                  hsl.h = (r - g) / fDelta + 4.f;

            hsl.h /= 6.f;
            if (hsl.h < 0.f) hsl.h += 1.f;
            if (hsl.h > 1.f) hsl.h -= 1.f;
        }

        return hsl;
    }

    namespace ColorMagic {
        //fast gamma somewhere gamma 2.2
        constexpr uint32_t GAMMA_MAGIC = 0x3FAAAABA;

        //fast color mixing
        constexpr uint32_t BLEND_MAGIC = 0x9E3779B9;

        inline RGBQUAD fastBlend(RGBQUAD c1, RGBQUAD c2, uint32_t factor) {
            //factor 0-256 where 256 = 1.0
            uint32_t f = (factor * BLEND_MAGIC) >> 24;

            RGBQUAD result;
            result.rgbRed = (c1.rgbRed * (256 - f) + c2.rgbRed * f) >> 8;
            result.rgbGreen = (c1.rgbGreen * (256 - f) + c2.rgbGreen * f) >> 8;
            result.rgbBlue = (c1.rgbBlue * (256 - f) + c2.rgbBlue * f) >> 8;
            return result;
        }

        //fast contrast
        //for fast check luminance > 128
        constexpr uint32_t LUMINANCE_MAGIC = 0x4A6B;

        inline BYTE fastLuminance(RGBQUAD rgb) {
            //(R*77 + G*150 + B*29) >> 8 optimized siht
            return (rgb.rgbRed * 77 + rgb.rgbGreen * 150 + rgb.rgbBlue * 29) >> 8;
        }

        //fast temp
        constexpr uint32_t TEMP_MAGIC = 0x4B9F;

        inline RGBQUAD fastColorTemp(RGBQUAD rgb, int kelvin) {
            //close color temp correction
            float temp = kelvin / 100.0f;
            BYTE r = rgb.rgbRed;
            BYTE g = rgb.rgbGreen;
            BYTE b = rgb.rgbBlue;

            if (temp < 66) {
                //0x80 for fast closing
                r = (BYTE)((r * (int)(temp * TEMP_MAGIC)) >> 8);
                g = (BYTE)((g * (int)(temp * TEMP_MAGIC)) >> 8);
            }
            else {
                b = (BYTE)((b * (int)((100 - temp) * TEMP_MAGIC)) >> 8);
            }

            return { r, g, b, 0 };
        }

        inline RGBQUAD fastSaturateRGB(RGBQUAD rgb, int satInt) {
            if (satInt == 256) return rgb;
            if (satInt == 0) {
                BYTE gray = (BYTE)((rgb.rgbRed * 77 + rgb.rgbGreen * 150 + rgb.rgbBlue * 29) >> 8);
                return { gray, gray, gray, 0 };
            }

            int r = rgb.rgbRed;
            int g = rgb.rgbGreen;
            int b = rgb.rgbBlue;

            int maxVal = r;
            int minVal = r;
            if (g > maxVal) maxVal = g;
            if (g < minVal) minVal = g;
            if (b > maxVal) maxVal = b;
            if (b < minVal) minVal = b;

            int lightness = (maxVal + minVal) >> 1;
            int delta = maxVal - minVal;

            if (delta == 0) return rgb;

            int currentSat;
            if (lightness < 128) {
                currentSat = (delta * 256) / (maxVal + minVal);
            }
            else {
                currentSat = (delta * 256) / (510 - maxVal - minVal);
            }

            if (currentSat == 0) return rgb;

            int newSat = (currentSat * satInt) >> 8;
            if (newSat == currentSat) return rgb;

            int chroma;
            if (lightness <= 127) {
                chroma = (lightness * newSat) >> 7;
            }
            else {
                chroma = ((255 - lightness) * newSat) >> 7;
            }

            int hue;
            if (maxVal == r) {
                hue = (g - b) * 256 / delta;
                if (hue < 0) hue += 1536;
            }
            else if (maxVal == g) {
                hue = (b - r) * 256 / delta + 512;
            }
            else {
                hue = (r - g) * 256 / delta + 1024;
            }

            int m = lightness - (chroma >> 1);
            int x = (chroma * (256 - abs((hue % 512) - 256))) >> 8;

            switch (hue / 256) {
            case 0: return { (BYTE)(chroma + m), (BYTE)(x + m), (BYTE)m, 0 };
            case 1: return { (BYTE)(x + m), (BYTE)(chroma + m), (BYTE)m, 0 };
            case 2: return { (BYTE)m, (BYTE)(chroma + m), (BYTE)(x + m), 0 };
            case 3: return { (BYTE)m, (BYTE)(x + m), (BYTE)(chroma + m), 0 };
            case 4: return { (BYTE)(x + m), (BYTE)m, (BYTE)(chroma + m), 0 };
            default: return { (BYTE)(chroma + m), (BYTE)m, (BYTE)(x + m), 0 };
            }
        }
    }

    inline RGBQUAD hsl2rgb(HSL hsl) {
        RGBQUAD rgb = { 0 };
        float r = hsl.l, g = hsl.l, b = hsl.l;
        float v = (hsl.l <= .5f) ? (hsl.l * (1.f + hsl.s)) : (hsl.l + hsl.s - hsl.l * hsl.s);

        if (v > 0.f) {
            float m = hsl.l + hsl.l - v;
            float sv = (v - m) / v;
            float h = hsl.h * 6.f;
            INT sextant = (INT)h;
            float fract = h - sextant;
            float vsf = v * sv * fract;
            float mid1 = m + vsf;
            float mid2 = v - vsf;

            switch (sextant) {
            case 0: r = v; g = mid1; b = m; break;
            case 1: r = mid2; g = v; b = m; break;
            case 2: r = m; g = v; b = mid1; break;
            case 3: r = m; g = mid2; b = v; break;
            case 4: r = mid1; g = m; b = v; break;
            case 5: r = v; g = m; b = mid2; break;
            default: r = v; g = m; b = m; break;
            }
        }

        rgb.rgbRed = (BYTE)(r * 255.f);
        rgb.rgbGreen = (BYTE)(g * 255.f);
        rgb.rgbBlue = (BYTE)(b * 255.f);
        return rgb;
    }

    inline HSV rgb2hsv(RGBQUAD rgb) {
        HSV hsv = { 0 };
        float r = rgb.rgbRed / 255.f;
        float g = rgb.rgbGreen / 255.f;
        float b = rgb.rgbBlue / 255.f;

        float maxVal = (std::max)((std::max)(r, g), b);
        float minVal = (std::min)((std::min)(r, g), b);
        float delta = maxVal - minVal;

        hsv.v = maxVal;

        if (delta == 0) {
            hsv.h = 0;
            hsv.s = 0;
        }
        else {
            hsv.s = delta / maxVal;

            if (maxVal == r)
                hsv.h = 60 * (fmod((g - b) / delta, 6));
            else if (maxVal == g)
                hsv.h = 60 * ((b - r) / delta + 2);
            else
                hsv.h = 60 * ((r - g) / delta + 4);

            if (hsv.h < 0) hsv.h += 360;
        }

        return hsv;
    }

    inline RGBQUAD hsv2rgb(HSV hsv) {
        RGBQUAD rgb = { 0 };
        float c = hsv.v * hsv.s;
        float x = c * (1 - fabs(fmod(hsv.h / 60, 2) - 1));
        float m = hsv.v - c;

        if (hsv.h < 60) {
            rgb.rgbRed = (BYTE)((c + m) * 255);
            rgb.rgbGreen = (BYTE)((x + m) * 255);
            rgb.rgbBlue = (BYTE)(m * 255);
        }
        else if (hsv.h < 120) {
            rgb.rgbRed = (BYTE)((x + m) * 255);
            rgb.rgbGreen = (BYTE)((c + m) * 255);
            rgb.rgbBlue = (BYTE)(m * 255);
        }
        else if (hsv.h < 180) {
            rgb.rgbRed = (BYTE)(m * 255);
            rgb.rgbGreen = (BYTE)((c + m) * 255);
            rgb.rgbBlue = (BYTE)((x + m) * 255);
        }
        else if (hsv.h < 240) {
            rgb.rgbRed = (BYTE)(m * 255);
            rgb.rgbGreen = (BYTE)((x + m) * 255);
            rgb.rgbBlue = (BYTE)((c + m) * 255);
        }
        else if (hsv.h < 300) {
            rgb.rgbRed = (BYTE)((x + m) * 255);
            rgb.rgbGreen = (BYTE)(m * 255);
            rgb.rgbBlue = (BYTE)((c + m) * 255);
        }
        else {
            rgb.rgbRed = (BYTE)((c + m) * 255);
            rgb.rgbGreen = (BYTE)(m * 255);
            rgb.rgbBlue = (BYTE)((x + m) * 255);
        }

        return rgb;
    }

    //RGB to LAB/XYZ
    inline XYZ rgb2xyz(RGBQUAD rgb) {
        float r = rgb.rgbRed / 255.0f;
        float g = rgb.rgbGreen / 255.0f;
        float b = rgb.rgbBlue / 255.0f;

        //sRGB to linear
        r = (r > 0.04045f) ? powf((r + 0.055f) / 1.055f, 2.4f) : r / 12.92f;
        g = (g > 0.04045f) ? powf((g + 0.055f) / 1.055f, 2.4f) : g / 12.92f;
        b = (b > 0.04045f) ? powf((b + 0.055f) / 1.055f, 2.4f) : b / 12.92f;

        XYZ xyz;
        xyz.X = r * 0.4124564f + g * 0.3575761f + b * 0.1804375f;
        xyz.Y = r * 0.2126729f + g * 0.7151522f + b * 0.0721750f;
        xyz.Z = r * 0.0193339f + g * 0.1191920f + b * 0.9503041f;

        return xyz;
    }

    inline RGBQUAD xyz2rgb(XYZ xyz) {
        float r = xyz.X * 3.2404542f + xyz.Y * -1.5371385f + xyz.Z * -0.4985314f;
        float g = xyz.X * -0.9692660f + xyz.Y * 1.8760108f + xyz.Z * 0.0415560f;
        float b = xyz.X * 0.0556434f + xyz.Y * -0.2040259f + xyz.Z * 1.0572252f;

        //linear to sRGB
        r = (r > 0.0031308f) ? 1.055f * powf(r, 1.0f / 2.4f) - 0.055f : 12.92f * r;
        g = (g > 0.0031308f) ? 1.055f * powf(g, 1.0f / 2.4f) - 0.055f : 12.92f * g;
        b = (b > 0.0031308f) ? 1.055f * powf(b, 1.0f / 2.4f) - 0.055f : 12.92f * b;

        RGBQUAD rgb;
        rgb.rgbRed = (BYTE)((std::max)(0.0f, (std::min)(1.0f, r)) * 255);
        rgb.rgbGreen = (BYTE)((std::max)(0.0f, (std::min)(1.0f, g)) * 255);
        rgb.rgbBlue = (BYTE)((std::max)(0.0f, (std::min)(1.0f, b)) * 255);
        return rgb;
    }

    inline LAB xyz2lab(XYZ xyz) {
        float x = xyz.X / 0.95047f;
        float y = xyz.Y / 1.0f;
        float z = xyz.Z / 1.08883f;

        auto f = [](float t) {
            return (t > 0.008856f) ? powf(t, 1.0f / 3.0f) : (7.787f * t + 16.0f / 116.0f);
            };

        LAB lab;
        lab.L = (y > 0.008856f) ? 116.0f * powf(y, 1.0f / 3.0f) - 16.0f : 903.3f * y;
        lab.a = 500.0f * (f(x) - f(y));
        lab.b = 200.0f * (f(y) - f(z));

        return lab;
    }

    inline XYZ lab2xyz(LAB lab) {
        float y = (lab.L + 16.0f) / 116.0f;
        float x = lab.a / 500.0f + y;
        float z = y - lab.b / 200.0f;

        auto fInv = [](float t) {
            float t3 = t * t * t;
            return (t3 > 0.008856f) ? t3 : (t - 16.0f / 116.0f) / 7.787f;
            };

        XYZ xyz;
        xyz.X = fInv(x) * 0.95047f;
        xyz.Y = fInv(y);
        xyz.Z = fInv(z) * 1.08883f;
        return xyz;
    }

    inline LAB rgb2lab(RGBQUAD rgb) {
        return xyz2lab(rgb2xyz(rgb));
    }

    inline RGBQUAD lab2rgb(LAB lab) {
        return xyz2rgb(lab2xyz(lab));
    }

    inline LAB lch2lab(LCH lch) {
        LAB lab;
        float radH = lch.H * 3.14159265358979323846f / 180.0f;
        lab.L = lch.L;
        lab.a = lch.C * cosf(radH);
        lab.b = lch.C * sinf(radH);
        return lab;
    }

    inline LCH lab2lch(LAB lab) {
        LCH lch;
        lch.L = lab.L;
        lch.C = sqrtf(lab.a * lab.a + lab.b * lab.b);
        lch.H = atan2f(lab.b, lab.a) * 180.0f / 3.14159265358979323846f;
        if (lch.H < 0) lch.H += 360;
        return lch;
    }

    inline YCbCr rgb2ycbcr(RGBQUAD rgb) {
        YCbCr ycbcr;
        ycbcr.Y = 0.299f * rgb.rgbRed + 0.587f * rgb.rgbGreen + 0.114f * rgb.rgbBlue;
        ycbcr.Cb = -0.168736f * rgb.rgbRed - 0.331264f * rgb.rgbGreen + 0.5f * rgb.rgbBlue + 128.0f;
        ycbcr.Cr = 0.5f * rgb.rgbRed - 0.418688f * rgb.rgbGreen - 0.081312f * rgb.rgbBlue + 128.0f;
        return ycbcr;
    }

    inline RGBQUAD ycbcr2rgb(YCbCr ycbcr) {
        RGBQUAD rgb;
        float Y = ycbcr.Y;
        float Cb = ycbcr.Cb - 128;
        float Cr = ycbcr.Cr - 128;

        rgb.rgbRed = (BYTE)(std::min)(255.0f, (std::max)(0.0f, Y + 1.402f * Cr));
        rgb.rgbGreen = (BYTE)(std::min)(255.0f, (std::max)(0.0f, Y - 0.344136f * Cb - 0.714136f * Cr));
        rgb.rgbBlue = (BYTE)(std::min)(255.0f, (std::max)(0.0f, Y + 1.772f * Cb));
        return rgb;
    }

    inline xyY xyz2xyy(XYZ xyz) {
        xyY result;
        float sum = xyz.X + xyz.Y + xyz.Z;
        if (sum == 0) {
            result.x = 0;
            result.y = 0;
        }
        else {
            result.x = xyz.X / sum;
            result.y = xyz.Y / sum;
        }
        result.Y = xyz.Y;
        return result;
    }

    inline XYZ xyy2xyz(xyY xyy) {
        XYZ result;
        if (xyy.y == 0) {
            result.X = 0;
            result.Y = xyy.Y;
            result.Z = 0;
        }
        else {
            result.X = (xyy.x * xyy.Y) / xyy.y;
            result.Y = xyy.Y;
            result.Z = ((1 - xyy.x - xyy.y) * xyy.Y) / xyy.y;
        }
        return result;
    }

    inline RGBQUAD linear2rgb(float r, float g, float b) {
        RGBQUAD rgb;
        auto toSRGB = [](float x) -> BYTE {
            x = (std::max)(0.0f, (std::min)(1.0f, x));
            if (x <= 0.0031308f)
                x = 12.92f * x;
            else
                x = 1.055f * powf(x, 1.0f / 2.4f) - 0.055f;
            return (BYTE)(x * 255);
            };
        rgb.rgbRed = toSRGB(r);
        rgb.rgbGreen = toSRGB(g);
        rgb.rgbBlue = toSRGB(b);
        return rgb;
    }

    inline void rgb2linear(RGBQUAD rgb, float& r, float& g, float& b) {
        auto toLinear = [](BYTE c) -> float {
            float x = c / 255.0f;
            if (x <= 0.04045f)
                return x / 12.92f;
            else
                return powf((x + 0.055f) / 1.055f, 2.4f);
            };
        r = toLinear(rgb.rgbRed);
        g = toLinear(rgb.rgbGreen);
        b = toLinear(rgb.rgbBlue);
    }

    inline ICtCp rgb2ictcp(RGBQUAD rgb) {
        float r, g, b;
        rgb2linear(rgb, r, g, b);

        auto pq = [](float L) {
            const float m1 = 0.1593017578125f;
            const float m2 = 78.84375f;
            const float c1 = 0.8359375f;
            const float c2 = 18.8515625f;
            const float c3 = 18.6875f;
            float Lp = powf(L, m1);
            float num = c1 + c2 * Lp;
            float den = 1.0f + c3 * Lp;
            return powf(num / den, m2);
            };

        r = pq(r);
        g = pq(g);
        b = pq(b);

        float l = 0.359283f * r + 0.697642f * g + -0.035891f * b;
        float m = -0.192080f * r + 1.100477f * g + 0.075374f * b;
        float s = 0.007079f * r + 0.074839f * g + 0.843326f * b;

        l = powf(l, 0.5f);
        m = powf(m, 0.5f);
        s = powf(s, 0.5f);

        ICtCp result;
        result.I = 0.5f * l + 0.5f * m;
        result.Ct = (6610.0f / 4096.0f) * l - (6610.0f / 4096.0f) * m;
        result.Cp = (1793.0f / 4096.0f) * l + (1793.0f / 4096.0f) * m - (3586.0f / 4096.0f) * s;
        return result;
    }

    inline YCoCg rgb2ycocg(RGBQUAD rgb) {
        YCoCg result;
        result.Y = (rgb.rgbRed + (rgb.rgbGreen << 1) + rgb.rgbBlue) >> 2;
        result.Co = (rgb.rgbRed - rgb.rgbBlue) >> 1;
        result.Cg = ((-rgb.rgbRed) + (rgb.rgbGreen << 1) - rgb.rgbBlue) >> 2;
        return result;
    }

    inline RGBQUAD ycocg2rgb(YCoCg ycocg) {
        RGBQUAD rgb;
        int t = (int)ycocg.Y - ((int)ycocg.Cg >> 1);
        int g = (int)ycocg.Cg + t;
        int b = t - ((int)ycocg.Co >> 1);
        int r = b + (int)ycocg.Co;

        rgb.rgbGreen = (BYTE)(std::min)(255, (std::max)(0, g));
        rgb.rgbBlue = (BYTE)(std::min)(255, (std::max)(0, b));
        rgb.rgbRed = (BYTE)(std::min)(255, (std::max)(0, r));
        return rgb;
    }

    inline Oklab rgb2oklab(RGBQUAD rgb) {
        float r, g, b;
        rgb2linear(rgb, r, g, b);

        float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
        float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
        float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

        l = cbrtf(l);
        m = cbrtf(m);
        s = cbrtf(s);

        Oklab lab;
        lab.L = 0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s;
        lab.a = 1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s;
        lab.b = 0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s;
        return lab;
    }

    inline RGBQUAD oklab2rgb(Oklab lab) {
        float l = lab.L + 0.3963377774f * lab.a + 0.2158037573f * lab.b;
        float m = lab.L - 0.1055613458f * lab.a - 0.0638541728f * lab.b;
        float s = lab.L - 0.0894841775f * lab.a - 1.2914855480f * lab.b;

        l = l * l * l;
        m = m * m * m;
        s = s * s * s;

        float r = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
        float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
        float b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;

        return linear2rgb(r, g, b);
    }

    inline Oklch oklab2oklch(Oklab lab) {
        Oklch lch;
        lch.L = lab.L;
        lch.C = sqrtf(lab.a * lab.a + lab.b * lab.b);
        lch.h = atan2f(lab.b, lab.a) * 180.0f / 3.14159265358979323846f;
        if (lch.h < 0) lch.h += 360;
        return lch;
    }

    inline Oklab oklch2oklab(Oklch lch) {
        Oklab lab;
        float radH = lch.h * 3.14159265358979323846f / 180.0f;
        lab.L = lch.L;
        lab.a = lch.C * cosf(radH);
        lab.b = lch.C * sinf(radH);
        return lab;
    }

    inline YUV rgb2yuv(RGBQUAD rgb) {
        YUV yuv;
        yuv.Y = 0.299f * rgb.rgbRed + 0.587f * rgb.rgbGreen + 0.114f * rgb.rgbBlue;
        yuv.U = -0.14713f * rgb.rgbRed - 0.28886f * rgb.rgbGreen + 0.436f * rgb.rgbBlue;
        yuv.V = 0.615f * rgb.rgbRed - 0.51499f * rgb.rgbGreen - 0.10001f * rgb.rgbBlue;
        return yuv;
    }

    inline RGBQUAD yuv2rgb(YUV yuv) {
        RGBQUAD rgb;
        rgb.rgbRed = (BYTE)(std::min)(255.0f, (std::max)(0.0f, yuv.Y + 1.13983f * yuv.V));
        rgb.rgbGreen = (BYTE)(std::min)(255.0f, (std::max)(0.0f, yuv.Y - 0.39465f * yuv.U - 0.58060f * yuv.V));
        rgb.rgbBlue = (BYTE)(std::min)(255.0f, (std::max)(0.0f, yuv.Y + 2.03211f * yuv.U));
        return rgb;
    }

    inline CMYK rgb2cmyk(RGBQUAD rgb) {
        CMYK cmyk;
        float r = rgb.rgbRed / 255.0f;
        float g = rgb.rgbGreen / 255.0f;
        float b = rgb.rgbBlue / 255.0f;

        float k = 1.0f - (std::max)((std::max)(r, g), b);
        if (k < 1.0f) {
            cmyk.C = (1.0f - r - k) / (1.0f - k);
            cmyk.M = (1.0f - g - k) / (1.0f - k);
            cmyk.Y = (1.0f - b - k) / (1.0f - k);
        }
        else {
            cmyk.C = cmyk.M = cmyk.Y = 0.0f;
        }
        cmyk.K = k;
        return cmyk;
    }

    inline IPT rgb2ipt(RGBQUAD rgb) {
        XYZ xyz = rgb2xyz(rgb);

        float l = 0.4002f * xyz.X + 0.7075f * xyz.Y - 0.0807f * xyz.Z;
        float m = -0.2280f * xyz.X + 1.1500f * xyz.Y + 0.0612f * xyz.Z;
        float s = 0.0000f * xyz.X + 0.9184f * xyz.Y + 0.0812f * xyz.Z;

        l = (l > 0) ? cbrtf(l) : -cbrtf(-l);
        m = (m > 0) ? cbrtf(m) : -cbrtf(-m);
        s = (s > 0) ? cbrtf(s) : -cbrtf(-s);

        IPT result;
        result.I = 0.4000f * l + 0.4000f * m + 0.2000f * s;
        result.P = 4.4550f * l - 4.8510f * m + 0.3960f * s;
        result.T = 0.8056f * l + 0.3572f * m - 1.1628f * s;
        return result;
    }

    inline RGBQUAD cmyk2rgb(CMYK cmyk) {
        RGBQUAD rgb;
        rgb.rgbRed = (BYTE)((1.0f - cmyk.C) * (1.0f - cmyk.K) * 255);
        rgb.rgbGreen = (BYTE)((1.0f - cmyk.M) * (1.0f - cmyk.K) * 255);
        rgb.rgbBlue = (BYTE)((1.0f - cmyk.Y) * (1.0f - cmyk.K) * 255);
        return rgb;
    }

    //rgba operations
    inline RGBA blendAlpha(RGBA foreground, RGBA background) {
        RGBA result;
        float alpha = foreground.a / 255.0f;
        float invAlpha = 1.0f - alpha;

        result.r = (BYTE)(foreground.r * alpha + background.r * invAlpha);
        result.g = (BYTE)(foreground.g * alpha + background.g * invAlpha);
        result.b = (BYTE)(foreground.b * alpha + background.b * invAlpha);
        result.a = 255;

        return result;
    }

    inline BYTE ultraFastGrayscale(RGBQUAD rgb) {
        return (rgb.rgbRed * 77 + rgb.rgbGreen * 150 + rgb.rgbBlue * 29) >> 8;
    }

    inline RGBA alphaCompose(RGBA src, RGBA dst, BlendMode mode) {
        RGBA result;
        float alpha = src.a / 255.0f;
        float invAlpha = 1.0f - alpha;

        auto blendFunc = [&](float s, float d) -> float {
            switch (mode) {
            case BlendMode::Normal: return s;
            case BlendMode::Multiply: return s * d;
            case BlendMode::Screen: return 1 - (1 - s) * (1 - d);
            case BlendMode::Overlay: return (d < 0.5f) ? 2 * s * d : 1 - 2 * (1 - s) * (1 - d);
            case BlendMode::Darken: return (std::min)(s, d);
            case BlendMode::Lighten: return (std::max)(s, d);
            case BlendMode::Difference: return fabsf(s - d);
            case BlendMode::Exclusion: return s + d - 2 * s * d;
            default: return s;
            }
            };

        float sr = src.r / 255.0f;
        float sg = src.g / 255.0f;
        float sb = src.b / 255.0f;
        float dr = dst.r / 255.0f;
        float dg = dst.g / 255.0f;
        float db = dst.b / 255.0f;

        result.r = (BYTE)((blendFunc(sr, dr) * alpha + dr * invAlpha) * 255);
        result.g = (BYTE)((blendFunc(sg, dg) * alpha + dg * invAlpha) * 255);
        result.b = (BYTE)((blendFunc(sb, db) * alpha + db * invAlpha) * 255);
        result.a = 255;

        return result;
    }

    inline RGBA premultiplyAlpha(RGBA color) {
        float alpha = color.a / 255.0f;
        return RGBA((BYTE)(color.r * alpha), (BYTE)(color.g * alpha), (BYTE)(color.b * alpha), color.a);
    }

    inline RGBA unpremultiplyAlpha(RGBA color) {
        if (color.a == 0) return RGBA(0, 0, 0, 0);
        float alpha = color.a / 255.0f;
        return RGBA((BYTE)(color.r / alpha), (BYTE)(color.g / alpha), (BYTE)(color.b / alpha), color.a);
    }

    //basic color operations
    inline RGBQUAD toGrayscale(RGBQUAD rgb) {
        BYTE gray = (BYTE)(rgb.rgbRed * 0.299f + rgb.rgbGreen * 0.587f + rgb.rgbBlue * 0.114f);
        rgb.rgbRed = gray;
        rgb.rgbGreen = gray;
        rgb.rgbBlue = gray;
        return rgb;
    }

    inline RGBQUAD darken(RGBQUAD rgb, float amount) {
        rgb.rgbRed = (BYTE)(rgb.rgbRed * amount);
        rgb.rgbGreen = (BYTE)(rgb.rgbGreen * amount);
        rgb.rgbBlue = (BYTE)(rgb.rgbBlue * amount);
        return rgb;
    }

    inline RGBQUAD lighten(RGBQUAD rgb, float amount) {
        rgb.rgbRed = (BYTE)(std::min)(255, (int)(rgb.rgbRed + (255 - rgb.rgbRed) * amount));
        rgb.rgbGreen = (BYTE)(std::min)(255, (int)(rgb.rgbGreen + (255 - rgb.rgbGreen) * amount));
        rgb.rgbBlue = (BYTE)(std::min)(255, (int)(rgb.rgbBlue + (255 - rgb.rgbBlue) * amount));
        return rgb;
    }

    inline RGBQUAD invert(RGBQUAD rgb) {
        rgb.rgbRed = 255 - rgb.rgbRed;
        rgb.rgbGreen = 255 - rgb.rgbGreen;
        rgb.rgbBlue = 255 - rgb.rgbBlue;
        return rgb;
    }

    inline RGBQUAD blend(RGBQUAD rgb1, RGBQUAD rgb2, float factor) {
        RGBQUAD result;
        result.rgbRed = (BYTE)(rgb1.rgbRed * (1 - factor) + rgb2.rgbRed * factor);
        result.rgbGreen = (BYTE)(rgb1.rgbGreen * (1 - factor) + rgb2.rgbGreen * factor);
        result.rgbBlue = (BYTE)(rgb1.rgbBlue * (1 - factor) + rgb2.rgbBlue * factor);
        return result;
    }

    inline RGBQUAD saturate(RGBQUAD rgb, float amount) {
        HSL hsl = rgb2hsl(rgb);
        hsl.s = (std::min)(1.0f, hsl.s * amount);
        return hsl2rgb(hsl);
    }

    inline RGBQUAD shiftHue(RGBQUAD rgb, float shift) {
        HSL hsl = rgb2hsl(rgb);
        hsl.h = fmod(hsl.h + shift, 1.0f);
        return hsl2rgb(hsl);
    }

    inline RGBQUAD gradient(RGBQUAD start, RGBQUAD end, float t) {
        RGBQUAD result;
        result.rgbRed = (BYTE)(start.rgbRed * (1 - t) + end.rgbRed * t);
        result.rgbGreen = (BYTE)(start.rgbGreen * (1 - t) + end.rgbGreen * t);
        result.rgbBlue = (BYTE)(start.rgbBlue * (1 - t) + end.rgbBlue * t);
        return result;
    }

    inline RGBQUAD fastBlendNoExcept(RGBQUAD c1, RGBQUAD c2, uint32_t factor) noexcept {
        return ColorMagic::fastBlend(c1, c2, factor);
    }

    inline BYTE fastLuminanceNoExcept(RGBQUAD rgb) noexcept {
        return ColorMagic::fastLuminance(rgb);
    }

    inline RGBQUAD gammaRec709(RGBQUAD rgb) {
        auto gamma709 = [](float x) -> float {
            if (x < 0.018f)
                return 4.5f * x;
            else
                return 1.099f * powf(x, 0.45f) - 0.099f;
            };

        float r = rgb.rgbRed / 255.0f;
        float g = rgb.rgbGreen / 255.0f;
        float b = rgb.rgbBlue / 255.0f;

        RGBQUAD result;
        result.rgbRed = (BYTE)(gamma709(r) * 255);
        result.rgbGreen = (BYTE)(gamma709(g) * 255);
        result.rgbBlue = (BYTE)(gamma709(b) * 255);
        return result;
    }

    //color corrections
    inline RGBQUAD gammaCorrect(RGBQUAD rgb, float gamma) {
        float invGamma = 1.0f / gamma;
        rgb.rgbRed = (BYTE)(pow(rgb.rgbRed / 255.0f, invGamma) * 255);
        rgb.rgbGreen = (BYTE)(pow(rgb.rgbGreen / 255.0f, invGamma) * 255);
        rgb.rgbBlue = (BYTE)(pow(rgb.rgbBlue / 255.0f, invGamma) * 255);
        return rgb;
    }

    inline RGBQUAD levels(RGBQUAD rgb, float inputBlack, float inputWhite, float outputBlack, float outputWhite) {
        auto adjust = [&](float channel) -> BYTE {
            float value = channel / 255.0f;
            value = (std::max)(0.0f, (std::min)(1.0f, (value - inputBlack) / (inputWhite - inputBlack)));
            value = outputBlack + value * (outputWhite - outputBlack);
            return (BYTE)(value * 255);
            };

        rgb.rgbRed = adjust(rgb.rgbRed);
        rgb.rgbGreen = adjust(rgb.rgbGreen);
        rgb.rgbBlue = adjust(rgb.rgbBlue);
        return rgb;
    }

    inline RGBQUAD exposure(RGBQUAD rgb, float ev) {
        float factor = powf(2.0f, ev);
        rgb.rgbRed = (BYTE)(std::min)(255, (int)(rgb.rgbRed * factor));
        rgb.rgbGreen = (BYTE)(std::min)(255, (int)(rgb.rgbGreen * factor));
        rgb.rgbBlue = (BYTE)(std::min)(255, (int)(rgb.rgbBlue * factor));
        return rgb;
    }

    inline RGBQUAD vibrance(RGBQUAD rgb, float amount) {
        HSL hsl = rgb2hsl(rgb);
        float saturationFactor = 1.0f + amount * (1.0f - hsl.s);
        hsl.s = (std::min)(1.0f, hsl.s * saturationFactor);
        return hsl2rgb(hsl);
    }

    inline RGBQUAD adjustHSB(RGBQUAD rgb, float hueShift, float saturationFactor, float brightnessFactor) {
        HSL hsl = rgb2hsl(rgb);
        hsl.h = fmod(hsl.h + hueShift, 1.0f);
        hsl.s = (std::min)(1.0f, hsl.s * saturationFactor);
        hsl.l = (std::min)(1.0f, hsl.l * brightnessFactor);
        return hsl2rgb(hsl);
    }

    inline RGBQUAD colorTemperature(int kelvin) {
        RGBQUAD rgb = { 0 };
        float temp = kelvin / 100.0f;

        if (temp <= 66) {
            rgb.rgbRed = 255;
            rgb.rgbGreen = (BYTE)(99.4708025861f * log(temp) - 161.1195681661f);
        }
        else {
            rgb.rgbRed = (BYTE)(329.698727446f * pow(temp - 60, -0.1332047592f));
            rgb.rgbGreen = (BYTE)(288.1221695283f * pow(temp - 60, -0.0755148492f));
        }

        if (temp >= 66)
            rgb.rgbBlue = 255;
        else if (temp <= 19)
            rgb.rgbBlue = 0;
        else
            rgb.rgbBlue = (BYTE)(138.5177312231f * log(temp - 10) - 305.0447927307f);

        return rgb;
    }

    //color schemes
    inline std::vector<RGBQUAD> getComplementary(RGBQUAD baseColor) {
        std::vector<RGBQUAD> scheme;
        HSL hsl = rgb2hsl(baseColor);
        HSL complementary = hsl;
        complementary.h = fmod(hsl.h + 0.5f, 1.0f);
        scheme.push_back(hsl2rgb(complementary));
        return scheme;
    }

    inline std::vector<RGBQUAD> getSplitComplementary(RGBQUAD baseColor) {
        std::vector<RGBQUAD> scheme;
        HSL hsl = rgb2hsl(baseColor);
        HSL comp1 = hsl;
        HSL comp2 = hsl;
        comp1.h = fmod(hsl.h + 0.4167f, 1.0f);
        comp2.h = fmod(hsl.h + 0.5833f, 1.0f);
        scheme.push_back(hsl2rgb(comp1));
        scheme.push_back(hsl2rgb(comp2));
        return scheme;
    }

    inline std::vector<RGBQUAD> getTriadic(RGBQUAD baseColor) {
        std::vector<RGBQUAD> scheme;
        HSL hsl = rgb2hsl(baseColor);
        for (int i = 1; i <= 2; i++) {
            HSL triad = hsl;
            triad.h = fmod(hsl.h + i * 0.3333f, 1.0f);
            scheme.push_back(hsl2rgb(triad));
        }
        return scheme;
    }

    inline std::vector<RGBQUAD> getTetradic(RGBQUAD baseColor) {
        std::vector<RGBQUAD> scheme;
        HSL hsl = rgb2hsl(baseColor);
        HSL comp1 = hsl;
        HSL comp2 = hsl;
        HSL comp3 = hsl;
        comp1.h = fmod(hsl.h + 0.25f, 1.0f);
        comp2.h = fmod(hsl.h + 0.5f, 1.0f);
        comp3.h = fmod(hsl.h + 0.75f, 1.0f);
        scheme.push_back(hsl2rgb(comp1));
        scheme.push_back(hsl2rgb(comp2));
        scheme.push_back(hsl2rgb(comp3));
        return scheme;
    }

    inline std::vector<RGBQUAD> getAnalogous(RGBQUAD baseColor, float step = 0.0833f) {
        std::vector<RGBQUAD> scheme;
        HSL hsl = rgb2hsl(baseColor);
        for (int i = -2; i <= 2; i++) {
            HSL analog = hsl;
            analog.h = fmod(hsl.h + i * step + 1.0f, 1.0f);
            scheme.push_back(hsl2rgb(analog));
        }
        return scheme;
    }

    inline std::vector<RGBQUAD> getMonochromatic(RGBQUAD baseColor, int count = 5) {
        std::vector<RGBQUAD> scheme;
        HSL hsl = rgb2hsl(baseColor);
        for (int i = 0; i < count; i++) {
            HSL mono = hsl;
            mono.l = (std::min)(1.0f, (std::max)(0.0f, i / (float)(count - 1)));
            scheme.push_back(hsl2rgb(mono));
        }
        return scheme;
    }

    //color distance functions
    inline float colorDistanceCIE76(RGBQUAD c1, RGBQUAD c2) {
        LAB lab1 = rgb2lab(c1);
        LAB lab2 = rgb2lab(c2);
        float dL = lab1.L - lab2.L;
        float da = lab1.a - lab2.a;
        float db = lab1.b - lab2.b;
        return sqrtf(dL * dL + da * da + db * db);
    }

    inline float colorDistanceCIE94(RGBQUAD c1, RGBQUAD c2) {
        LAB lab1 = rgb2lab(c1);
        LAB lab2 = rgb2lab(c2);

        float dL = lab1.L - lab2.L;
        float da = lab1.a - lab2.a;
        float db = lab1.b - lab2.b;

        float C1 = sqrtf(lab1.a * lab1.a + lab1.b * lab1.b);
        float C2 = sqrtf(lab2.a * lab2.a + lab2.b * lab2.b);
        float dC = C1 - C2;

        float dH2 = da * da + db * db - dC * dC;
        float dH = (dH2 > 0) ? sqrtf(dH2) : 0;

        float sL = 1.0f;
        float sC = 1.0f + 0.045f * C1;
        float sH = 1.0f + 0.015f * C1;

        return sqrtf((dL / sL) * (dL / sL) + (dC / sC) * (dC / sC) + (dH / sH) * (dH / sH));
    }

    inline float colorDistanceCIEDE2000(RGBQUAD c1, RGBQUAD c2) {
        LAB lab1 = rgb2lab(c1);
        LAB lab2 = rgb2lab(c2);

        float L1 = lab1.L, a1 = lab1.a, b1 = lab1.b;
        float L2 = lab2.L, a2 = lab2.a, b2 = lab2.b;

        float C1 = sqrtf(a1 * a1 + b1 * b1);
        float C2 = sqrtf(a2 * a2 + b2 * b2);
        float C_avg = (C1 + C2) / 2.0f;

        float G = 0.5f * (1.0f - sqrtf(powf(C_avg, 7) / (powf(C_avg, 7) + powf(25, 7))));

        float a1p = (1.0f + G) * a1;
        float a2p = (1.0f + G) * a2;

        float C1p = sqrtf(a1p * a1p + b1 * b1);
        float C2p = sqrtf(a2p * a2p + b2 * b2);

        float h1p = atan2f(b1, a1p) * 180.0f / 3.14159265358979323846f;
        float h2p = atan2f(b2, a2p) * 180.0f / 3.14159265358979323846f;
        if (h1p < 0) h1p += 360;
        if (h2p < 0) h2p += 360;

        float dLp = L2 - L1;
        float dCp = C2p - C1p;

        float dh_p = 0;
        if (C1p * C2p != 0) {
            float diff = h2p - h1p;
            if (diff > 180) diff -= 360;
            else if (diff < -180) diff += 360;
            dh_p = 2.0f * sqrtf(C1p * C2p) * sinf(diff * 3.14159265358979323846f / 360.0f);
        }

        float L_avg = (L1 + L2) / 2.0f;
        float C_avgp = (C1p + C2p) / 2.0f;

        float h_avgp = h1p + h2p;
        if (C1p * C2p != 0) {
            if (fabsf(h1p - h2p) <= 180) h_avgp /= 2.0f;
            else {
                if (h1p + h2p < 360) h_avgp = (h1p + h2p + 360) / 2.0f;
                else h_avgp = (h1p + h2p - 360) / 2.0f;
            }
        }

        float T = 1.0f - 0.17f * cosf((h_avgp - 30) * 3.14159265358979323846f / 180.0f)
            + 0.24f * cosf((2 * h_avgp) * 3.14159265358979323846f / 180.0f)
            + 0.32f * cosf((3 * h_avgp + 6) * 3.14159265358979323846f / 180.0f)
            - 0.20f * cosf((4 * h_avgp - 63) * 3.14159265358979323846f / 180.0f);

        float delta_theta = 30.0f * expf(-powf((h_avgp - 275) / 25, 2));
        float Rc = 2.0f * sqrtf(powf(C_avgp, 7) / (powf(C_avgp, 7) + powf(25, 7)));
        float Rt = -Rc * sinf(2.0f * delta_theta * 3.14159265358979323846f / 180.0f);

        float sL = 1.0f + (0.015f * (L_avg - 50) * (L_avg - 50)) / sqrtf(20 + (L_avg - 50) * (L_avg - 50));
        float sC = 1.0f + 0.045f * C_avgp;
        float sH = 1.0f + 0.015f * C_avgp * T;

        float dLp_sL = dLp / sL;
        float dCp_sC = dCp / sC;
        float dh_p_sH = dh_p / sH;

        return sqrtf(dLp_sL * dLp_sL + dCp_sC * dCp_sC + dh_p_sH * dh_p_sH + Rt * dCp_sC * dh_p_sH);
    }

    inline float colorDistanceCMC(RGBQUAD c1, RGBQUAD c2, float l, float c) {
        LAB lab1 = rgb2lab(c1);
        LAB lab2 = rgb2lab(c2);

        float dL = lab1.L - lab2.L;
        float da = lab1.a - lab2.a;
        float db = lab1.b - lab2.b;

        float C1 = sqrtf(lab1.a * lab1.a + lab1.b * lab1.b);
        float C2 = sqrtf(lab2.a * lab2.a + lab2.b * lab2.b);
        float dC = C1 - C2;

        float dH2 = da * da + db * db - dC * dC;
        float dH = (dH2 > 0) ? sqrtf(dH2) : 0;

        float sL = (lab1.L < 16) ? 0.511f : (0.040975f * lab1.L) / (1.0f + 0.01765f * lab1.L);
        float sC = (0.0638f * C1) / (1.0f + 0.0131f * C1) + 0.638f;

        float H1 = atan2f(lab1.b, lab1.a) * 180.0f / 3.14159265358979323846f;
        if (H1 < 0) H1 += 360;

        float F = sqrtf(powf(C1, 4) / (powf(C1, 4) + 1900));
        float T = (H1 >= 164 && H1 <= 345) ?
            0.56f + fabsf(0.2f * cosf((H1 + 168) * 3.14159265358979323846f / 180.0f)) :
            0.36f + fabsf(0.4f * cosf((H1 + 35) * 3.14159265358979323846f / 180.0f));

        float sH = sC * (F * T + 1 - F);

        return sqrtf((dL / (l * sL)) * (dL / (l * sL)) + (dC / (c * sC)) * (dC / (c * sC)) + (dH / sH) * (dH / sH));
    }

    inline float colorDistanceLCH(LCH lch1, LCH lch2) {
        float dL = lch1.L - lch2.L;
        float dC = lch1.C - lch2.C;
        float dH = 2.0f * sqrtf(lch1.C * lch2.C) * sinf((lch1.H - lch2.H) * 3.14159265358979323846f / 360.0f);
        return sqrtf(dL * dL + dC * dC + dH * dH);
    }

    inline float colorDistance(RGBQUAD c1, RGBQUAD c2, DeltaEMetric metric) {
        switch (metric) {
        case DeltaEMetric::CIE76:
            return colorDistanceCIE76(c1, c2);
        case DeltaEMetric::CIE94:
            return colorDistanceCIE94(c1, c2);
        case DeltaEMetric::CIEDE2000:
            return colorDistanceCIEDE2000(c1, c2);
        case DeltaEMetric::CMC:
            return colorDistanceCMC(c1, c2);
        default:
            return colorDistanceCIE76(c1, c2);
        }
    }

    inline float colorDistanceEuclidean(RGBQUAD c1, RGBQUAD c2) {
        int dr = c1.rgbRed - c2.rgbRed;
        int dg = c1.rgbGreen - c2.rgbGreen;
        int db = c1.rgbBlue - c2.rgbBlue;
        return sqrtf((float)(dr * dr + dg * dg + db * db));
    }

    //accessibility
    inline float getLuminance(RGBQUAD rgb) {
        return 0.2126f * rgb.rgbRed + 0.7152f * rgb.rgbGreen + 0.0722f * rgb.rgbBlue;
    }

    inline float contrastRatio(RGBQUAD color1, RGBQUAD color2) {
        float l1 = getLuminance(color1);
        float l2 = getLuminance(color2);
        float lighter = (std::max)(l1, l2);
        float darker = (std::min)(l1, l2);
        return (lighter + 0.05f) / (darker + 0.05f);
    }

    inline bool isWCAGCompliant(RGBQUAD foreground, RGBQUAD background, int level) {
        float ratio = contrastRatio(foreground, background);
        if (level == 1)
            return ratio >= 4.5f;
        else
            return ratio >= 7.0f;
    }

    inline RGBQUAD getContrastColor(RGBQUAD backgroundColor) {
        float luminance = getLuminance(backgroundColor);
        if (luminance > 128) {
            RGBQUAD black = { 0, 0, 0, 0 };
            return black;
        }
        else {
            RGBQUAD white = { 255, 255, 255, 0 };
            return white;
        }
    }

    //validation functions
    inline bool isValid(RGBQUAD rgb) {
        return true;
    }

    inline bool isValid(HSL hsl) {
        return hsl.h >= 0 && hsl.h <= 1 && hsl.s >= 0 && hsl.s <= 1 && hsl.l >= 0 && hsl.l <= 1;
    }

    inline bool isValid(HSV hsv) {
        return hsv.h >= 0 && hsv.h <= 360 && hsv.s >= 0 && hsv.s <= 1 && hsv.v >= 0 && hsv.v <= 1;
    }

    inline bool isValid(LAB lab) {
        return lab.L >= 0 && lab.L <= 100 && lab.a >= -128 && lab.a <= 128 && lab.b >= -128 && lab.b <= 128;
    }

    inline bool isValid(LCH lch) {
        return lch.L >= 0 && lch.L <= 100 && lch.C >= 0 && lch.H >= 0 && lch.H <= 360;
    }

    inline bool isValid(XYZ xyz) {
        return xyz.X >= 0 && xyz.Y >= 0 && xyz.Z >= 0;
    }

    inline bool roundTripTest(RGBQUAD original, float tolerance) {
        HSL hsl = rgb2hsl(original);
        RGBQUAD restored = hsl2rgb(hsl);
        return colorDistanceCIE76(original, restored) <= tolerance;
    }

    inline bool isInGamut(RGBQUAD rgb) {
        return rgb.rgbRed >= 0 && rgb.rgbRed <= 255 &&
            rgb.rgbGreen >= 0 && rgb.rgbGreen <= 255 &&
            rgb.rgbBlue >= 0 && rgb.rgbBlue <= 255;
    }

    inline bool isInGamut(LAB lab) {
        return lab.L >= 0 && lab.L <= 100 &&
            lab.a >= -128 && lab.a <= 128 &&
            lab.b >= -128 && lab.b <= 128;
    }

    //utility
    inline std::string toHex(RGBQUAD rgb) {
        char hex[8];
        sprintf_s(hex, "#%02X%02X%02X", rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);
        return std::string(hex);
    }

    inline RGBQUAD fromHex(const std::string& hex) {
        RGBQUAD rgb = { 0 };
        if (hex[0] == '#') {
            unsigned int r, g, b;
            sscanf_s(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);
            rgb.rgbRed = (BYTE)r;
            rgb.rgbGreen = (BYTE)g;
            rgb.rgbBlue = (BYTE)b;
        }
        return rgb;
    }

    //thread-safe LUT
    template<typename Key, typename Value, size_t Size = 256>
    class ThreadSafeLUT {
    private:
        std::array<Value, Size> cache;
        std::array<bool, Size> filled;
        mutable std::mutex mutex;

    public:
        ThreadSafeLUT() {
            filled.fill(false);
        }

        Value get(Key key, std::function<Value(Key)> generator) {
            size_t idx = static_cast<size_t>(key);
            if (idx >= Size) return generator(key);

            if (!filled[idx]) {
                std::lock_guard<std::mutex> lock(mutex);
                if (!filled[idx]) {
                    cache[idx] = generator(key);
                    filled[idx] = true;
                }
            }
            return cache[idx];
        }

        void invalidate() {
            std::lock_guard<std::mutex> lock(mutex);
            filled.fill(false);
        }
    };

    //mass processing
    template<typename Container>
    inline void applyToAll(Container& pixels, std::function<RGBQUAD(RGBQUAD)> func) {
        for (auto& pixel : pixels) {
            pixel = func(pixel);
        }
    }

    template<typename Container>
    inline void parallelApply(Container& pixels, std::function<RGBQUAD(RGBQUAD)> func) {
#pragma omp parallel for
        for (int i = 0; i < (int)pixels.size(); i++) {
            pixels[i] = func(pixels[i]);
        }
    }

    template<typename Container>
    inline std::vector<RGBQUAD> map(const Container& pixels, std::function<RGBQUAD(RGBQUAD)> func) {
        std::vector<RGBQUAD> result;
        result.reserve(pixels.size());
        for (const auto& pixel : pixels) {
            result.push_back(func(pixel));
        }
        return result;
    }

    //Color class
    class Color {
    private:
        RGBQUAD data;
        uint32_t magic;

    public:
        Color() : data{ 0, 0, 0, 0 }, magic(ColorMagic::BLEND_MAGIC) {}
        Color(RGBQUAD rgb) : data(rgb), magic(ColorMagic::BLEND_MAGIC) {}
        Color(BYTE r, BYTE g, BYTE b) : data{ r, g, b, 0 }, magic(ColorMagic::BLEND_MAGIC) {}
        Color(COLORREF color) : data{ GetRValue(color), GetGValue(color), GetBValue(color), 0 },
            magic(ColorMagic::BLEND_MAGIC) {
        }

        bool isValid() const { return magic == ColorMagic::BLEND_MAGIC; }

        Color& lighten(float amount) {
            data = Colors::lighten(data, amount);
            return *this;
        }

        Color& darken(float amount) {
            data = Colors::darken(data, amount);
            return *this;
        }

        Color& saturate(float amount) {
            data = Colors::saturate(data, amount);
            return *this;
        }

        Color& desaturate(float amount) {
            data = Colors::saturate(data, 1.0f - amount);
            return *this;
        }

        Color& shiftHue(float shift) {
            data = Colors::shiftHue(data, shift);
            return *this;
        }

        Color& invert() {
            data = Colors::invert(data);
            return *this;
        }

        Color& toGrayscale() {
            data = Colors::toGrayscale(data);
            return *this;
        }

        Color& blend(const Color& other, float factor) {
            data = Colors::blend(data, other.data, factor);
            return *this;
        }

        Color& gammaCorrect(float gamma) {
            data = Colors::gammaCorrect(data, gamma);
            return *this;
        }

        Color& fastBlend(const Color& other, float factor) {
            data = ColorMagic::fastBlend(data, other.data, (uint32_t)(factor * 256));
            return *this;
        }

        BYTE fastLuminance() const {
            return ColorMagic::fastLuminance(data);
        }

        RGBQUAD getRGB() const { return data; }
        HSL getHSL() const { return Colors::rgb2hsl(data); }
        HSV getHSV() const { return Colors::rgb2hsv(data); }
        LAB getLAB() const { return Colors::rgb2lab(data); }

        BYTE getRed() const { return data.rgbRed; }
        BYTE getGreen() const { return data.rgbGreen; }
        BYTE getBlue() const { return data.rgbBlue; }

        std::string toHex() const { return Colors::toHex(data); }

        operator RGBQUAD() const { return data; }
        operator COLORREF() const { return RGB(data.rgbRed, data.rgbGreen, data.rgbBlue); }

        bool operator==(const Color& other) const {
            return data.rgbRed == other.data.rgbRed &&
                data.rgbGreen == other.data.rgbGreen &&
                data.rgbBlue == other.data.rgbBlue;
        }

        bool operator!=(const Color& other) const { return !(*this == other); }
    };

    //ColorLUT class
    class ColorLUT {
    private:
        static const int LUT_SIZE = 256;
        static RGBQUAD lightenLUT[256][256][256];
        static RGBQUAD darkenLUT[256][256][256];
        static RGBQUAD invertLUT[256][256][256];
        static bool initialized;

        static void initialize() {
            if (initialized) return;

            for (int r = 0; r < LUT_SIZE; r++) {
                for (int g = 0; g < LUT_SIZE; g++) {
                    for (int b = 0; b < LUT_SIZE; b++) {
                        RGBQUAD color = { (BYTE)r, (BYTE)g, (BYTE)b, 0 };
                        lightenLUT[r][g][b] = Colors::lighten(color, 0.3f);
                        darkenLUT[r][g][b] = Colors::darken(color, 0.7f);
                        invertLUT[r][g][b] = Colors::invert(color);
                    }
                }
            }
            initialized = true;
        }

    public:
        static RGBQUAD fastLighten(RGBQUAD color) {
            if (!initialized) initialize();
            return lightenLUT[color.rgbRed][color.rgbGreen][color.rgbBlue];
        }

        static RGBQUAD fastDarken(RGBQUAD color) {
            if (!initialized) initialize();
            return darkenLUT[color.rgbRed][color.rgbGreen][color.rgbBlue];
        }

        static RGBQUAD fastInvert(RGBQUAD color) {
            if (!initialized) initialize();
            return invertLUT[color.rgbRed][color.rgbGreen][color.rgbBlue];
        }
    };

    //GammaLUT class
    class GammaLUT {
    private:
        static const int LUT_SIZE = 256;
        static BYTE sRGBtoLinearLUT[LUT_SIZE];
        static BYTE linearToSRGBLUT[LUT_SIZE];
        static bool initialized;

        static void init() {
            if (initialized) return;
            for (int i = 0; i < LUT_SIZE; i++) {
                float x = i / 255.0f;
                if (x <= 0.04045f)
                    x = x / 12.92f;
                else
                    x = powf((x + 0.055f) / 1.055f, 2.4f);
                sRGBtoLinearLUT[i] = (BYTE)(x * 255);

                x = i / 255.0f;
                if (x <= 0.0031308f)
                    x = 12.92f * x;
                else
                    x = 1.055f * powf(x, 1.0f / 2.4f) - 0.055f;
                linearToSRGBLUT[i] = (BYTE)(x * 255);
            }
            initialized = true;
        }

    public:
        static BYTE sRGBtoLinear(BYTE value) {
            if (!initialized) init();
            return sRGBtoLinearLUT[value];
        }

        static BYTE linearToSRGB(BYTE value) {
            if (!initialized) init();
            return linearToSRGBLUT[value];
        }

        static RGBQUAD sRGBtoLinear(RGBQUAD rgb) {
            if (!initialized) init();
            RGBQUAD result;
            result.rgbRed = sRGBtoLinearLUT[rgb.rgbRed];
            result.rgbGreen = sRGBtoLinearLUT[rgb.rgbGreen];
            result.rgbBlue = sRGBtoLinearLUT[rgb.rgbBlue];
            return result;
        }

        static RGBQUAD linearToSRGB(RGBQUAD rgb) {
            if (!initialized) init();
            RGBQUAD result;
            result.rgbRed = linearToSRGBLUT[rgb.rgbRed];
            result.rgbGreen = linearToSRGBLUT[rgb.rgbGreen];
            result.rgbBlue = linearToSRGBLUT[rgb.rgbBlue];
            return result;
        }
    };

    //SaturationCache class
    class SaturationCache {
    private:
        static const int CACHE_BITS = 5;
        static const int CACHE_SIZE = 32;
        static RGBQUAD cache[32][32][32][2];
        static bool initialized;

        static void init() {
            if (initialized) return;

            for (int r = 0; r < CACHE_SIZE; r++) {
                for (int g = 0; g < CACHE_SIZE; g++) {
                    for (int b = 0; b < CACHE_SIZE; b++) {
                        RGBQUAD rgb = {
                            (BYTE)(b * 8 + 4),
                            (BYTE)(g * 8 + 4),
                            (BYTE)(r * 8 + 4),
                            0
                        };
                        cache[r][g][b][0] = rgb;

                        HSL hsl = rgb2hsl(rgb);
                        hsl.s = 1.0f;
                        cache[r][g][b][1] = hsl2rgb(hsl);
                    }
                }
            }
            initialized = true;
        }

    public:
        static RGBQUAD fastSaturate(RGBQUAD rgb, float factor) {
            if (!initialized) init();

            int rIdx = rgb.rgbRed >> 3;
            int gIdx = rgb.rgbGreen >> 3;
            int bIdx = rgb.rgbBlue >> 3;

            RGBQUAD orig = cache[rIdx][gIdx][bIdx][0];
            RGBQUAD sat = cache[rIdx][gIdx][bIdx][1];

            if (factor >= 1.0f) return sat;
            if (factor <= 0.0f) return orig;

            int f = (int)(factor * 256);
            RGBQUAD result;
            result.rgbRed = (orig.rgbRed * (256 - f) + sat.rgbRed * f) >> 8;
            result.rgbGreen = (orig.rgbGreen * (256 - f) + sat.rgbGreen * f) >> 8;
            result.rgbBlue = (orig.rgbBlue * (256 - f) + sat.rgbBlue * f) >> 8;
            return result;
        }
    };

    //ColorAdjustment struct
    struct ColorAdjustment {
        float brightness = 0.0f;
        float contrast = 1.0f;
        float saturation = 1.0f;
        float hueShift = 0.0f;
        float gamma = 1.0f;

        RGBQUAD apply(RGBQUAD color) const {
            HSL hsl = Colors::rgb2hsl(color);

            hsl.h = fmod(hsl.h + hueShift + 1.0f, 1.0f);
            hsl.s = (std::max)(0.0f, (std::min)(1.0f, hsl.s * saturation));
            hsl.l = (std::max)(0.0f, (std::min)(1.0f, hsl.l + brightness));

            if (contrast != 1.0f) {
                hsl.l = 0.5f + (hsl.l - 0.5f) * contrast;
                hsl.l = (std::max)(0.0f, (std::min)(1.0f, hsl.l));
            }

            RGBQUAD result = Colors::hsl2rgb(hsl);

            if (gamma != 1.0f) {
                result = Colors::gammaCorrect(result, gamma);
            }

            return result;
        }

        Color apply(const Color& color) const {
            return Color(apply(color.getRGB()));
        }

        template<typename Container>
        void applyToAll(Container& pixels) const {
            for (auto& pixel : pixels) {
                pixel = apply(pixel);
            }
        }
    };

    //ColorBalance struct
    struct ColorBalance {
        float shadows[3] = { 1.0f, 1.0f, 1.0f };
        float midtones[3] = { 1.0f, 1.0f, 1.0f };
        float highlights[3] = { 1.0f, 1.0f, 1.0f };

        RGBQUAD apply(RGBQUAD rgb) const {
            float r = rgb.rgbRed / 255.0f;
            float g = rgb.rgbGreen / 255.0f;
            float b = rgb.rgbBlue / 255.0f;

            float luminance = 0.299f * r + 0.587f * g + 0.114f * b;

            auto adjust = [&](float channel, int idx) -> float {
                float shadowWeight = (std::max)(0.0f, 1.0f - luminance * 2.0f);
                float highlightWeight = (std::max)(0.0f, luminance * 2.0f - 1.0f);
                float midWeight = 1.0f - shadowWeight - highlightWeight;

                return channel * (shadows[idx] * shadowWeight + midtones[idx] * midWeight + highlights[idx] * highlightWeight);
                };

            r = adjust(r, 0);
            g = adjust(g, 1);
            b = adjust(b, 2);

            RGBQUAD result;
            result.rgbRed = (BYTE)((std::min)(1.0f, (std::max)(0.0f, r)) * 255);
            result.rgbGreen = (BYTE)((std::min)(1.0f, (std::max)(0.0f, g)) * 255);
            result.rgbBlue = (BYTE)((std::min)(1.0f, (std::max)(0.0f, b)) * 255);
            return result;
        }
    };

    //ColorTemperature struct
    struct ColorTemperature {
        float cct;
        float duv;

        static ColorTemperature fromRGB(RGBQUAD rgb) {
            XYZ xyz = rgb2xyz(rgb);
            xyY xyy = xyz2xyy(xyz);

            float n = (xyy.x - 0.3320f) / (0.1858f - xyy.y);
            float cct = 449.0f * n * n * n + 3525.0f * n * n + 6823.3f * n + 5520.33f;

            ColorTemperature result;
            result.cct = cct;
            result.duv = 0;
            return result;
        }

        static RGBQUAD fromCCT(float kelvin) {
            return Colors::colorTemperature((int)kelvin);
        }
    };

    //ColorHistogram struct
    struct ColorHistogram {
        std::array<int, 256> red;
        std::array<int, 256> green;
        std::array<int, 256> blue;

        ColorHistogram() {
            red.fill(0);
            green.fill(0);
            blue.fill(0);
        }

        void add(RGBQUAD rgb) {
            red[rgb.rgbRed]++;
            green[rgb.rgbGreen]++;
            blue[rgb.rgbBlue]++;
        }

        float getMean(int channel) const {
            long long sum = 0;
            long long count = 0;
            const auto& hist = (channel == 0) ? red : (channel == 1) ? green : blue;
            for (int i = 0; i < 256; i++) {
                sum += (long long)i * hist[i];
                count += hist[i];
            }
            return (count > 0) ? (float)sum / count : 0;
        }

        float getVariance(int channel) const {
            float mean = getMean(channel);
            long long sumSq = 0;
            long long count = 0;
            const auto& hist = (channel == 0) ? red : (channel == 1) ? green : blue;
            for (int i = 0; i < 256; i++) {
                float diff = i - mean;
                sumSq += (long long)(diff * diff * hist[i]);
                count += hist[i];
            }
            return (count > 0) ? (float)sumSq / count : 0;
        }
    };

    //ColorMoments struct
    struct ColorMoments {
        float mean[3];
        float stddev[3];
        float skewness[3];

        void calculate(const std::vector<RGBQUAD>& pixels) {
            for (int c = 0; c < 3; c++) {
                double sum = 0, sum2 = 0, sum3 = 0;
                int count = (int)pixels.size();
                for (const auto& p : pixels) {
                    float val = (c == 0) ? p.rgbRed : (c == 1) ? p.rgbGreen : p.rgbBlue;
                    sum += val;
                    sum2 += val * val;
                    sum3 += val * val * val;
                }
                mean[c] = (float)(sum / count);
                stddev[c] = (float)sqrt(sum2 / count - mean[c] * mean[c]);
                skewness[c] = (float)((sum3 / count - 3 * mean[c] * sum2 / count + 2 * mean[c] * mean[c] * mean[c]) /
                    (stddev[c] * stddev[c] * stddev[c]));
            }
        }
    };

    //Benchmark class
    class Benchmark {
    private:
        std::chrono::high_resolution_clock::time_point startTime;
        std::string name;

    public:
        Benchmark(const std::string& testName) : name(testName) {
            startTime = std::chrono::high_resolution_clock::now();
        }

        ~Benchmark() {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        }

        double elapsed() const {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(now - startTime).count() / 1000.0;
        }
    };

    //animation easing functions
    inline float easeLinear(float t) { return t; }
    inline float easeInQuad(float t) { return t * t; }
    inline float easeOutQuad(float t) { return t * (2 - t); }
    inline float easeInOutQuad(float t) { return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t; }
    inline float easeInCubic(float t) { return t * t * t; }
    inline float easeOutCubic(float t) { return (--t) * t * t + 1; }
    inline float easeInOutCubic(float t) { return t < 0.5f ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1; }
    inline float easeInElastic(float t) { return sin(13 * M_PI_2 * t) * pow(2, 10 * (t - 1)); }
    inline float easeOutElastic(float t) { return sin(-13 * M_PI_2 * (t + 1)) * pow(2, -10 * t) + 1; }
    inline float easeBounce(float t) {
        if (t < (1 / 2.75f)) return 7.5625f * t * t;
        if (t < (2 / 2.75f)) return 7.5625f * (t -= (1.5f / 2.75f)) * t + 0.75f;
        if (t < (2.5f / 2.75f)) return 7.5625f * (t -= (2.25f / 2.75f)) * t + 0.9375f;
        return 7.5625f * (t -= (2.625f / 2.75f)) * t + 0.984375f;
    }

    //ColorAnimation class
    class ColorAnimation {
    private:
        RGBQUAD startColor;
        RGBQUAD endColor;
        float duration;
        float elapsed;
        std::function<float(float)> easing;

    public:
        ColorAnimation(RGBQUAD start, RGBQUAD end, float durationSec,
            std::function<float(float)> easingFunc = easeLinear)
            : startColor(start), endColor(end), duration(durationSec),
            elapsed(0), easing(easingFunc) {
        }

        RGBQUAD update(float deltaTime) {
            elapsed += deltaTime;
            float t = (std::min)(1.0f, elapsed / duration);
            float eased = easing(t);
            return Colors::gradient(startColor, endColor, eased);
        }

        bool isFinished() const {
            return elapsed >= duration;
        }

        void reset() {
            elapsed = 0;
        }

        void reset(RGBQUAD newStart, RGBQUAD newEnd) {
            startColor = newStart;
            endColor = newEnd;
            elapsed = 0;
        }
    };

    //simple filters
    inline RGBQUAD sepia(RGBQUAD rgb) {
        BYTE r = rgb.rgbRed;
        BYTE g = rgb.rgbGreen;
        BYTE b = rgb.rgbBlue;

        rgb.rgbRed = (BYTE)(std::min)(255, (int)(r * 0.393f + g * 0.769f + b * 0.189f));
        rgb.rgbGreen = (BYTE)(std::min)(255, (int)(r * 0.349f + g * 0.686f + b * 0.168f));
        rgb.rgbBlue = (BYTE)(std::min)(255, (int)(r * 0.272f + g * 0.534f + b * 0.131f));
        return rgb;
    }

    inline RGBQUAD sepiaIntensity(RGBQUAD rgb, float intensity) {
        BYTE r = rgb.rgbRed;
        BYTE g = rgb.rgbGreen;
        BYTE b = rgb.rgbBlue;

        RGBQUAD sepiaColor;
        sepiaColor.rgbRed = (BYTE)(std::min)(255, (int)(r * 0.393f + g * 0.769f + b * 0.189f));
        sepiaColor.rgbGreen = (BYTE)(std::min)(255, (int)(r * 0.349f + g * 0.686f + b * 0.168f));
        sepiaColor.rgbBlue = (BYTE)(std::min)(255, (int)(r * 0.272f + g * 0.534f + b * 0.131f));

        return Colors::blend(rgb, sepiaColor, intensity);
    }

    inline RGBQUAD cool(RGBQUAD rgb) {
        HSL hsl = rgb2hsl(rgb);
        hsl.h = fmod(hsl.h + 0.1f, 1.0f);
        hsl.s = (std::min)(1.0f, hsl.s * 1.2f);
        return hsl2rgb(hsl);
    }

    inline RGBQUAD warm(RGBQUAD rgb) {
        HSL hsl = rgb2hsl(rgb);
        hsl.h = fmod(hsl.h - 0.1f + 1.0f, 1.0f);
        hsl.s = (std::min)(1.0f, hsl.s * 1.2f);
        return hsl2rgb(hsl);
    }

    inline RGBQUAD coolWarm(RGBQUAD rgb, float temperature) {
        HSL hsl = rgb2hsl(rgb);
        hsl.h = fmod(hsl.h + temperature * 0.1f + 1.0f, 1.0f);
        hsl.s = (std::min)(1.0f, hsl.s * (1.0f + temperature * 0.2f));
        return hsl2rgb(hsl);
    }

    inline RGBQUAD vintage(RGBQUAD rgb) {
        RGBQUAD result = sepiaIntensity(rgb, 0.5f);
        result = Colors::saturate(result, 0.8f);
        result = Colors::gammaCorrect(result, 1.1f);
        return result;
    }

    inline std::vector<RGBQUAD> generatePalette(RGBQUAD base, int count, float saturationVariation, float lightnessVariation) {
        std::vector<RGBQUAD> palette;
        HSL baseHsl = rgb2hsl(base);

        for (int i = 0; i < count; i++) {
            HSL variation = baseHsl;
            float t = (float)i / (count - 1);

            variation.s = (std::max)(0.0f, (std::min)(1.0f, baseHsl.s + (t - 0.5f) * saturationVariation * 2));
            variation.l = (std::max)(0.0f, (std::min)(1.0f, baseHsl.l + (t - 0.5f) * lightnessVariation * 2));

            palette.push_back(hsl2rgb(variation));
        }

        return palette;
    }

    //FastMath namespace
    namespace FastMath {
        inline float fastInvSqrt(float x) {
            float xhalf = 0.5f * x;
            int i = *(int*)&x;
            i = 0x5f3759df - (i >> 1);
            x = *(float*)&i;
            x = x * (1.5f - xhalf * x * x);
            return x;
        }

        constexpr int HSV_MAGIC = 0x2B1B0B;

        inline RGBQUAD fastHSV2RGB(HSV hsv) {
            int h = (int)(hsv.h * 6);
            float f = hsv.h * 6 - h;

            BYTE p = (BYTE)(hsv.v * (1 - hsv.s) * 255);
            BYTE q = (BYTE)(hsv.v * (1 - hsv.s * f) * 255);
            BYTE t = (BYTE)(hsv.v * (1 - hsv.s * (1 - f)) * 255);
            BYTE v = (BYTE)(hsv.v * 255);

            switch (h) {
            case 0: return { v, t, p, 0 };
            case 1: return { q, v, p, 0 };
            case 2: return { p, v, t, 0 };
            case 3: return { p, q, v, 0 };
            case 4: return { t, p, v, 0 };
            default: return { v, p, q, 0 };
            }
        }
    }

    //CompileTime namespace
    namespace CompileTime {
        constexpr float srgbToLinearConstexpr(float x) {
            if (x <= 0.04045f)
                return x / 12.92f;
            else
                return (x + 0.055f) / 1.055f;
        }

        constexpr RGBQUAD blendConstexpr(RGBQUAD c1, RGBQUAD c2, float t) {
            RGBQUAD result;
            result.rgbRed = (BYTE)(c1.rgbRed * (1 - t) + c2.rgbRed * t);
            result.rgbGreen = (BYTE)(c1.rgbGreen * (1 - t) + c2.rgbGreen * t);
            result.rgbBlue = (BYTE)(c1.rgbBlue * (1 - t) + c2.rgbBlue * t);
            return result;
        }

        constexpr bool isValidConstexpr(RGBQUAD rgb) {
            return rgb.rgbRed <= 255 && rgb.rgbGreen <= 255 && rgb.rgbBlue <= 255;
        }
    }
}

//using aliases
using Color = Colors::Color;
using ColorF = struct { float r, g, b; };
using PixelBuffer = std::span<RGBQUAD>;
using ColorTransform = std::function<RGBQUAD(RGBQUAD)>;
using ColorPredicate = std::function<bool(RGBQUAD)>;

//static member definitions
RGBQUAD Colors::ColorLUT::lightenLUT[256][256][256];
RGBQUAD Colors::ColorLUT::darkenLUT[256][256][256];
RGBQUAD Colors::ColorLUT::invertLUT[256][256][256];
bool Colors::ColorLUT::initialized = false;

BYTE Colors::GammaLUT::sRGBtoLinearLUT[256];
BYTE Colors::GammaLUT::linearToSRGBLUT[256];
bool Colors::GammaLUT::initialized = false;

RGBQUAD Colors::SaturationCache::cache[32][32][32][2];
bool Colors::SaturationCache::initialized = false;

//Presets namespace
namespace Colors {
    namespace Presets {
        const RGBQUAD WHITE = { 255, 255, 255, 0 };
        const RGBQUAD BLACK = { 0, 0, 0, 0 };
        const RGBQUAD RED = { 255, 0, 0, 0 };
        const RGBQUAD GREEN = { 0, 255, 0, 0 };
        const RGBQUAD BLUE = { 0, 0, 255, 0 };
        const RGBQUAD YELLOW = { 255, 255, 0, 0 };
        const RGBQUAD CYAN = { 0, 255, 255, 0 };
        const RGBQUAD MAGENTA = { 255, 0, 255, 0 };
        const RGBQUAD ORANGE = { 255, 165, 0, 0 };
        const RGBQUAD PURPLE = { 128, 0, 128, 0 };
        const RGBQUAD PINK = { 255, 192, 203, 0 };
        const RGBQUAD BROWN = { 165, 42, 42, 0 };

        const RGBQUAD DARK_GRAY = { 64, 64, 64, 0 };
        const RGBQUAD GRAY = { 128, 128, 128, 0 };
        const RGBQUAD LIGHT_GRAY = { 192, 192, 192, 0 };

        const RGBQUAD WINDOWS_BLUE = { 0, 120, 215, 0 };
        const RGBQUAD SUCCESS_GREEN = { 70, 180, 70, 0 };
        const RGBQUAD WARNING_YELLOW = { 255, 200, 0, 0 };
        const RGBQUAD ERROR_RED = { 200, 50, 50, 0 };
        const RGBQUAD INFO_BLUE = { 50, 150, 200, 0 };

        class Gradient {
        private:
            struct Stop {
                float position;
                RGBQUAD color;
            };
            std::vector<Stop> stops;

        public:
            Gradient& addStop(float position, RGBQUAD color) {
                position = (std::max)(0.0f, (std::min)(1.0f, position));
                stops.push_back({ position, color });
                std::sort(stops.begin(), stops.end(),
                    [](const Stop& a, const Stop& b) { return a.position < b.position; });
                return *this;
            }

            Gradient& addStop(float position, HSL color) {
                return addStop(position, Colors::hsl2rgb(color));
            }

            RGBQUAD sample(float t) const {
                t = (std::max)(0.0f, (std::min)(1.0f, t));

                if (stops.empty()) return Presets::BLACK;
                if (stops.size() == 1) return stops[0].color;
                if (t <= stops[0].position) return stops[0].color;
                if (t >= stops.back().position) return stops.back().color;

                for (size_t i = 0; i < stops.size() - 1; i++) {
                    if (t >= stops[i].position && t <= stops[i + 1].position) {
                        float segmentT = (t - stops[i].position) /
                            (stops[i + 1].position - stops[i].position);
                        return Colors::gradient(stops[i].color, stops[i + 1].color, segmentT);
                    }
                }

                return stops.back().color;
            }

            std::vector<RGBQUAD> generate(int steps) const {
                std::vector<RGBQUAD> result;
                result.reserve(steps);
                for (int i = 0; i <= steps; i++) {
                    float t = (float)i / steps;
                    result.push_back(sample(t));
                }
                return result;
            }

            static Gradient rainbow() {
                Gradient grad;
                grad.addStop(0.0f, HSL{ 0.0f, 1.0f, 0.5f });
                grad.addStop(0.166f, HSL{ 0.166f, 1.0f, 0.5f });
                grad.addStop(0.333f, HSL{ 0.333f, 1.0f, 0.5f });
                grad.addStop(0.5f, HSL{ 0.5f, 1.0f, 0.5f });
                grad.addStop(0.666f, HSL{ 0.666f, 1.0f, 0.5f });
                grad.addStop(0.833f, HSL{ 0.833f, 1.0f, 0.5f });
                grad.addStop(1.0f, HSL{ 1.0f, 1.0f, 0.5f });
                return grad;
            }

            static Gradient sunset() {
                Gradient grad;
                grad.addStop(0.0f, RGBQUAD{ 255, 0, 0, 0 });
                grad.addStop(0.33f, RGBQUAD{ 255, 165, 0, 0 });
                grad.addStop(0.66f, RGBQUAD{ 255, 255, 0, 0 });
                grad.addStop(1.0f, RGBQUAD{ 255, 0, 255, 0 });
                return grad;
            }

            static Gradient ocean() {
                Gradient grad;
                grad.addStop(0.0f, RGBQUAD{ 0, 0, 128, 0 });
                grad.addStop(0.33f, RGBQUAD{ 0, 128, 255, 0 });
                grad.addStop(0.66f, RGBQUAD{ 0, 255, 255, 0 });
                grad.addStop(1.0f, RGBQUAD{ 255, 255, 255, 0 });
                return grad;
            }

            static Gradient fire() {
                Gradient grad;
                grad.addStop(0.0f, RGBQUAD{ 255, 0, 0, 0 });
                grad.addStop(0.5f, RGBQUAD{ 255, 165, 0, 0 });
                grad.addStop(1.0f, RGBQUAD{ 255, 255, 0, 0 });
                return grad;
            }
        };
    }
}