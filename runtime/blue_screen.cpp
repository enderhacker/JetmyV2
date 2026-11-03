#include "blue_screen.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <objidl.h> // PROPID/IStream declarations required by MinGW's GDI+ headers with lean Windows headers.
#include <gdiplus.h>
#include <stdexcept>

namespace Runtime {
namespace {
class FontRenderer {
    ULONG_PTR token_ = 0;
public:
    FontRenderer() {
        Gdiplus::GdiplusStartupInput startup;
        if (Gdiplus::GdiplusStartup(&token_, &startup, nullptr) != Gdiplus::Ok)
            throw std::runtime_error("Cannot initialize blue-screen font rendering.");
    }
    ~FontRenderer() { Gdiplus::GdiplusShutdown(token_); }
    FontRenderer(const FontRenderer&) = delete;
    FontRenderer& operator=(const FontRenderer&) = delete;
};
void Fill(HDC dc, RECT rect, COLORREF color) {
    GdiFlush();
    Gdiplus::Graphics graphics(dc);
    graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
    Gdiplus::SolidBrush brush(Gdiplus::Color(255, GetRValue(color), GetGValue(color), GetBValue(color)));
    graphics.FillRectangle(&brush, Gdiplus::Rect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top));
    graphics.Flush(Gdiplus::FlushIntentionSync);
}
uint8_t Multiply(uint8_t a, uint8_t b) {
    unsigned result = 0, value = a;
    for (int i = 0; i < 8; ++i) {
        if ((b >> i) & 1) result ^= value;
        value <<= 1;
        if (value & 0x100) value ^= 0x11D;
    }
    return static_cast<uint8_t>(result);
}
// A real version-3/L QR code for Microsoft's stop-code information page.
// Encoding is local and deterministic; the simulator never opens the URL.
class SupportQR {
    static constexpr int Size = 29;
    std::array<std::array<bool, Size>, Size> modules_{}, reserved_{};
    void Set(int x, int y, bool dark) {
        if (x < 0 || y < 0 || x >= Size || y >= Size) return;
        modules_[y][x] = dark; reserved_[y][x] = true;
    }
    void Finder(int cx, int cy) {
        for (int y = -4; y <= 4; ++y) for (int x = -4; x <= 4; ++x) {
            int distance = std::max(abs(x), abs(y));
            Set(cx + x, cy + y, distance != 2 && distance != 4);
        }
    }
public:
    SupportQR() {
        constexpr char url[] = "https://www.windows.com/stopcode";
        std::array<uint8_t, 70> bytes{};
        int bitCount = 0;
        auto append = [&](unsigned value, int count) {
            for (int i = count - 1; i >= 0; --i, ++bitCount)
                bytes[bitCount / 8] |= static_cast<uint8_t>(((value >> i) & 1u) << (7 - bitCount % 8));
        };
        append(4, 4); append(sizeof(url) - 1, 8);
        for (char c : url) if (c) append(static_cast<unsigned char>(c), 8);
        append(0, 4);
        while (bitCount % 8) append(0, 1);
        for (int i = bitCount / 8; i < 55; ++i) bytes[i] = (i - bitCount / 8) % 2 ? 0x11 : 0xEC;
        std::array<uint8_t, 15> divisor{}, remainder{};
        divisor.back() = 1;
        uint8_t root = 1;
        for (int i = 0; i < 15; ++i) {
            for (int j = 0; j < 15; ++j) {
                divisor[j] = Multiply(divisor[j], root);
                if (j < 14) divisor[j] ^= divisor[j + 1];
            }
            root = Multiply(root, 2);
        }
        for (int i = 0; i < 55; ++i) {
            uint8_t factor = bytes[i] ^ remainder[0];
            for (int j = 0; j < 14; ++j) remainder[j] = remainder[j + 1];
            remainder.back() = 0;
            for (int j = 0; j < 15; ++j) remainder[j] ^= Multiply(divisor[j], factor);
        }
        std::copy(remainder.begin(), remainder.end(), bytes.begin() + 55);
        Finder(3, 3); Finder(Size - 4, 3); Finder(3, Size - 4);
        for (int i = 8; i < Size - 8; ++i) { Set(6, i, i % 2 == 0); Set(i, 6, i % 2 == 0); }
        for (int y = -2; y <= 2; ++y) for (int x = -2; x <= 2; ++x)
            Set(22 + x, 22 + y, std::max(abs(x), abs(y)) != 1);
        constexpr unsigned format = 0x77C4; // Error correction L, mask 0, BCH protected.
        auto bit = [](int i) { return ((format >> i) & 1u) != 0; };
        for (int i = 0; i < 6; ++i) Set(8, i, bit(i));
        Set(8, 7, bit(6)); Set(8, 8, bit(7)); Set(7, 8, bit(8));
        for (int i = 9; i < 15; ++i) Set(14 - i, 8, bit(i));
        for (int i = 0; i < 8; ++i) Set(Size - 1 - i, 8, bit(i));
        for (int i = 8; i < 15; ++i) Set(8, Size - 15 + i, bit(i));
        Set(8, Size - 8, true);
        int index = 0;
        for (int right = Size - 1; right >= 1; right -= 2) {
            if (right == 6) --right;
            for (int row = 0; row < Size; ++row) {
                int y = ((right + 1) & 2) == 0 ? Size - 1 - row : row;
                for (int j = 0; j < 2; ++j) {
                    int x = right - j;
                    if (reserved_[y][x]) continue;
                    bool dark = index < 560 && ((bytes[index / 8] >> (7 - index % 8)) & 1);
                    modules_[y][x] = dark != ((x + y) % 2 == 0);
                    ++index;
                }
            }
        }
    }
    void Draw(HDC dc, int left, int top, int scale, COLORREF ink) const {
        constexpr int quiet = 4;
        const int extent = (Size + quiet * 2) * scale;
        Fill(dc, {left, top, left + extent, top + extent}, RGB(255, 255, 255));
        for (int y = 0; y < Size; ++y) for (int x = 0; x < Size; ++x) if (modules_[y][x]) {
            int px = left + (x + quiet) * scale, py = top + (y + quiet) * scale;
            Fill(dc, {px, py, px + scale, py + scale}, ink);
        }
    }
};
}

BlueScreenStyle DetectBlueScreenStyle() {
    OSVERSIONINFOW version{}; version.dwOSVersionInfoSize = sizeof(version);
    using Version = LONG (WINAPI*)(OSVERSIONINFOW*);
    HMODULE library = GetModuleHandleW(L"ntdll.dll");
    FARPROC address = library ? GetProcAddress(library, "RtlGetVersion") : nullptr;
    Version getVersion = nullptr;
    static_assert(sizeof(getVersion) == sizeof(address), "Windows pointer size mismatch");
    std::memcpy(&getVersion, &address, sizeof(getVersion));
    if (!getVersion || getVersion(&version) != 0) return BlueScreenStyle::Windows10;
    if (version.dwMajorVersion < 6) return BlueScreenStyle::XP;
    if (version.dwMajorVersion == 6 && version.dwMinorVersion == 0) return BlueScreenStyle::Vista;
    if (version.dwMajorVersion == 6 && version.dwMinorVersion == 1) return BlueScreenStyle::Windows7;
    if (version.dwMajorVersion >= 10 && version.dwBuildNumber >= 22000) return BlueScreenStyle::Windows11;
    return BlueScreenStyle::Windows10;
}
BlueScreenArtwork::BlueScreenArtwork(BlueScreenStyle requested, int pixelWidth, int pixelHeight)
    : style_(requested == BlueScreenStyle::Automatic ? DetectBlueScreenStyle() : requested) {
    if (pixelWidth < 0 || pixelHeight < 0 || (pixelWidth == 0) != (pixelHeight == 0))
        throw std::invalid_argument("Blue-screen dimensions must both be positive or both zero.");
    if (pixelWidth == 0) {
        height_ = std::max(1, std::min(GetSystemMetrics(SM_CYSCREEN), GetSystemMetrics(SM_CXSCREEN) * 3 / 4));
        width_ = std::max(1, height_ * 4 / 3);
    } else {
        width_ = pixelWidth; height_ = pixelHeight;
    }
    // Keep GDI+ alive across repeated menu previews and its internal font caches.
    static FontRenderer renderer;
    bool xp = style_ == BlueScreenStyle::XP;
    // Rasterize outlines at the final pixel size, never enlarge an already rendered font.
    // Grayscale antialiasing also avoids ClearType color fringes on the off-screen DIB.
    body_ = CreateFontW(-std::max(1, Y(Modern() ? 30 : xp ? 12 : 16)), Modern() ? 0 : std::max(1, X(xp ? 7 : 8)), 0, 0,
        Modern() ? FW_LIGHT : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        Modern() ? DEFAULT_PITCH : FIXED_PITCH, Modern() ? L"Segoe UI" : L"Lucida Console");
    if (Modern()) {
        detail_ = CreateFontW(-std::max(1, Y(19)), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        face_ = CreateFontW(-std::max(1, Y(164)), 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    }
    if (!body_ || (Modern() && (!detail_ || !face_))) {
        if (body_) DeleteObject(body_);
        if (detail_) DeleteObject(detail_);
        if (face_) DeleteObject(face_);
        throw std::runtime_error("Cannot create blue-screen typography.");
    }
}
BlueScreenArtwork::~BlueScreenArtwork() {
    if (body_) DeleteObject(body_);
    if (detail_) DeleteObject(detail_);
    if (face_) DeleteObject(face_);
}
bool BlueScreenArtwork::Modern() const { return style_ == BlueScreenStyle::Windows10 || style_ == BlueScreenStyle::Windows11; }
int BlueScreenArtwork::LayoutWidth() const { return Modern() ? 1280 : style_ == BlueScreenStyle::XP ? 640 : 800; }
int BlueScreenArtwork::X(int value) const { return MulDiv(value, width_, LayoutWidth()); }
int BlueScreenArtwork::Y(int value) const { return MulDiv(value, height_, LayoutWidth() * 3 / 4); }
int BlueScreenArtwork::Width() const { return width_; }
int BlueScreenArtwork::Height() const { return height_; }
RECT BlueScreenArtwork::ProgressRect() const {
    RECT logical = Modern() ? RECT{100, 445, 900, 500} : style_ == BlueScreenStyle::XP ?
        RECT{20, 438, 620, 458} : RECT{24, 546, 776, 576};
    return {X(logical.left), Y(logical.top), X(logical.right), Y(logical.bottom)};
}
void BlueScreenArtwork::Draw(HDC dc, int percent, bool initial) const {
    int saved = SaveDC(dc);
    if (!saved) throw std::runtime_error("Cannot save blue-screen artwork state.");
    COLORREF blue = Modern() ? RGB(0, 120, 215) : RGB(0, 0, 170);
    RECT entire{0, 0, Width(), Height()};
    Fill(dc, initial ? entire : ProgressRect(), blue);
    SelectObject(dc, body_); SetTextColor(dc, RGB(255, 255, 255)); SetBkMode(dc, TRANSPARENT);
    SetTextAlign(dc, TA_LEFT | TA_TOP | TA_NOUPDATECP);
    auto text = [&](int x, int y, const wchar_t* value) {
        GdiFlush(); // Synchronize earlier GDI work before GDI+ accesses the shared DIB.
        // Explicit grayscale rasterization works even when desktop font smoothing
        // is disabled. Never change the user's font-smoothing/ClearType settings.
        Gdiplus::Graphics graphics(dc);
        graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
        Gdiplus::Font font(dc, static_cast<HFONT>(GetCurrentObject(dc, OBJ_FONT)));
        Gdiplus::SolidBrush white(Gdiplus::Color(255, 255, 255));
        Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
        format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);
        Gdiplus::Status status = graphics.DrawString(value, -1, &font,
            Gdiplus::PointF(static_cast<float>(X(x)), static_cast<float>(Y(y))), &format, &white);
        graphics.Flush(Gdiplus::FlushIntentionSync);
        if (status != Gdiplus::Ok) {
            RestoreDC(dc, saved);
            throw std::runtime_error("Cannot render blue-screen text.");
        }
    };
    percent = std::clamp(percent, 0, 100);
    wchar_t progress[96]{};
    if (Modern()) {
        if (initial) {
            SelectObject(dc, face_); text(91, 110, L":(");
            SelectObject(dc, body_);
            text(100, 330, style_ == BlueScreenStyle::Windows11 ?
                L"Your device ran into a problem and needs to restart." : L"Your PC ran into a problem and needs to restart.");
            text(100, 374, L"We're just collecting some error info, and then we'll restart for you.");
            static const SupportQR qr;
            // QR modules remain whole physical pixels, with a four-module quiet zone.
            // Fit whole modules inside the QR slot and preserve the support-text gutter.
            const int module = std::max(1, std::min(X(111), Y(111)) / 37);
            qr.Draw(dc, X(100), Y(553), module, blue);
            SelectObject(dc, detail_);
            text(237, 554, L"For more information about this issue and possible fixes, visit");
            text(237, 582, L"https://www.windows.com/stopcode");
            text(237, 626, L"If you call a support person, give them this info:");
            text(237, 654, L"Stop code: PAGE FAULT IN NONPAGED AREA");
            text(237, 682, L"What failed: win32k.sys");
        }
        SelectObject(dc, body_);
        std::swprintf(progress, 96, L"%d%% complete", percent);
        text(100, 450, progress);
    } else {
        bool xp = style_ == BlueScreenStyle::XP;
        int x = xp ? 20 : 24, y = xp ? 22 : 26, step = xp ? 16 : 20;
        auto line = [&](int row, const wchar_t* value) { text(x, y + row * step, value); };
        if (initial) {
            line(0, L"A problem has been detected and Windows has been shut down to prevent");
            line(1, L"damage to your computer.");
            line(3, L"PAGE_FAULT_IN_NONPAGED_AREA");
            line(5, L"If this is the first time you've seen this Stop error screen,");
            line(6, L"restart your computer. If this screen appears again, follow");
            line(7, L"these steps:");
            line(9, L"Check to make sure any new hardware or software is properly installed.");
            line(10, L"If this is a new installation, ask your hardware or software manufacturer");
            line(11, L"for any Windows updates you might need.");
            line(13, L"If problems continue, disable or remove any newly installed hardware");
            line(14, L"or software. Disable BIOS memory options such as caching or shadowing.");
            line(15, L"If you need to use Safe Mode to remove or disable components, restart");
            line(16, L"your computer, press F8 to select Advanced Boot Options, and then");
            line(17, L"select Safe Mode.");
            line(19, L"Technical information:");
            line(21, xp ? L"*** STOP: 0x00000050 (0xE1A7D000, 0x00000000, 0xBF8013B2, 0x00000001)" :
                L"*** STOP: 0x00000050 (0xFFFFF880009A0000, 0x0, 0xFFFFF960001013B2, 0x0)");
            line(23, xp ? L"*** win32k.sys - Address BF8013B2 base at BF800000, DateStamp 3b7de698" :
                L"*** win32k.sys - Address FFFFF960001013B2 base at FFFFF96000000000");
            if (style_ == BlueScreenStyle::Windows7) {
                line(24, L"Collecting data for crash dump ...");
                line(25, L"Initializing disk for crash dump ...");
            } else line(25, L"Beginning dump of physical memory");
        }
        std::swprintf(progress, 96, xp ? L"Dumping physical memory to disk: %d" : L"Dumping physical memory to disk: %d%%", percent);
        line(26, progress);
    }
    RestoreDC(dc, saved); GdiFlush();
}
} // namespace Runtime
