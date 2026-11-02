#include "blue_screen.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <stdexcept>

namespace Runtime {
namespace {
void Text(HDC dc, int x, int y, const wchar_t* text) {
    TextOutW(dc, x, y, text, int(std::wcslen(text)));
}
void Fill(HDC dc, RECT rect, COLORREF color) {
    SetDCBrushColor(dc, color);
    FillRect(dc, &rect, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
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
    void Draw(HDC dc, int left, int top, COLORREF ink) const {
        constexpr int scale = 3, quiet = 4, extent = (Size + quiet * 2) * scale;
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
BlueScreenArtwork::BlueScreenArtwork(BlueScreenStyle requested)
    : style_(requested == BlueScreenStyle::Automatic ? DetectBlueScreenStyle() : requested) {
    bool xp = style_ == BlueScreenStyle::XP;
    body_ = CreateFontW(Modern() ? -30 : xp ? -12 : -16, Modern() ? 0 : xp ? 7 : 8, 0, 0,
        Modern() ? FW_LIGHT : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, Modern() ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY,
        Modern() ? DEFAULT_PITCH : FIXED_PITCH, Modern() ? L"Segoe UI" : L"Lucida Console");
    if (Modern()) {
        detail_ = CreateFontW(-19, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        face_ = CreateFontW(-164, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
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
int BlueScreenArtwork::Width() const { return Modern() ? 1280 : style_ == BlueScreenStyle::XP ? 640 : 800; }
int BlueScreenArtwork::Height() const { return Width() * 3 / 4; }
RECT BlueScreenArtwork::ProgressRect() const {
    if (Modern()) return {100, 445, 700, 493};
    if (style_ == BlueScreenStyle::XP) return {20, 438, 620, 454};
    return {24, 546, 776, 570};
}
void BlueScreenArtwork::Draw(HDC dc, int percent, bool initial) const {
    int saved = SaveDC(dc);
    if (!saved) throw std::runtime_error("Cannot save blue-screen artwork state.");
    COLORREF blue = Modern() ? RGB(0, 120, 215) : RGB(0, 0, 170);
    RECT entire{0, 0, Width(), Height()};
    Fill(dc, initial ? entire : ProgressRect(), blue);
    SelectObject(dc, body_); SetTextColor(dc, RGB(255, 255, 255)); SetBkMode(dc, TRANSPARENT);
    wchar_t progress[96]{};
    if (Modern()) {
        if (initial) {
            SelectObject(dc, face_); Text(dc, 91, 110, L":(");
            SelectObject(dc, body_);
            Text(dc, 100, 330, style_ == BlueScreenStyle::Windows11 ?
                L"Your device ran into a problem and needs to restart." : L"Your PC ran into a problem and needs to restart.");
            Text(dc, 100, 374, L"We're just collecting some error info, and then we'll restart for you.");
            static const SupportQR qr;
            qr.Draw(dc, 100, 553, blue);
            SelectObject(dc, detail_);
            Text(dc, 237, 554, L"For more information about this issue and possible fixes, visit");
            Text(dc, 237, 582, L"https://www.windows.com/stopcode");
            Text(dc, 237, 626, L"If you call a support person, give them this info:");
            Text(dc, 237, 654, L"Stop code: PAGE FAULT IN NONPAGED AREA");
            Text(dc, 237, 682, L"What failed: win32k.sys");
        }
        SelectObject(dc, body_);
        std::swprintf(progress, 96, L"%d%% complete", percent);
        Text(dc, 100, 450, progress);
    } else {
        bool xp = style_ == BlueScreenStyle::XP;
        int x = xp ? 20 : 24, y = xp ? 22 : 26, step = xp ? 16 : 20;
        auto line = [&](int row, const wchar_t* text) { Text(dc, x, y + row * step, text); };
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
