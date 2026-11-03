#include "runtime/blue_screen.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
class Canvas {
public:
    HDC dc = CreateCompatibleDC(nullptr);
    HBITMAP bitmap = nullptr;
    HGDIOBJ previous = nullptr;
    uint32_t* pixels = nullptr;
    int width, height;
    Canvas(int w, int h) : width(w), height(h) {
        if (!dc) throw std::runtime_error("Cannot create test DC.");
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = w; info.bmiHeader.biHeight = -h;
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
        void* data = nullptr;
        bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &data, nullptr, 0);
        if (!bitmap) { DeleteDC(dc); throw std::runtime_error("Cannot create test DIB."); }
        previous = SelectObject(dc, bitmap);
        if (!previous || previous == HGDI_ERROR) {
            DeleteObject(bitmap); DeleteDC(dc);
            throw std::runtime_error("Cannot select test DIB.");
        }
        pixels = static_cast<uint32_t*>(data);
    }
    ~Canvas() { SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc); }
    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;
    size_t Count() const { return static_cast<size_t>(width) * height; }
    std::vector<uint32_t> Copy() const { GdiFlush(); return {pixels, pixels + Count()}; }
    void Save(const char* path) const {
        GdiFlush();
        BITMAPFILEHEADER file{};
        BITMAPINFOHEADER info{};
        file.bfType = 0x4D42;
        file.bfOffBits = sizeof(file) + sizeof(info);
        file.bfSize = file.bfOffBits + static_cast<DWORD>(Count() * 4);
        info.biSize = sizeof(info); info.biWidth = width; info.biHeight = -height;
        info.biPlanes = 1; info.biBitCount = 32;
        std::ofstream output(path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(&file), sizeof(file));
        output.write(reinterpret_cast<const char*>(&info), sizeof(info));
        output.write(reinterpret_cast<const char*>(pixels), Count() * 4);
        if (!output) throw std::runtime_error("Cannot save off-screen preview.");
    }
};
}

int main(int argc, char** argv) {
    using namespace Runtime;
    SetProcessDPIAware(); // Same physical-pixel metrics as the application.
    for (auto style : {BlueScreenStyle::XP, BlueScreenStyle::Vista, BlueScreenStyle::Windows7,
                       BlueScreenStyle::Windows10, BlueScreenStyle::Windows11}) {
        for (auto size : {std::pair<int, int>{640, 480}, {1440, 1080}, {2880, 2160}}) {
            std::cout << "Checking style " << static_cast<int>(style) << " at " << size.first << 'x' << size.second << std::endl;
            BlueScreenArtwork artwork(style, size.first, size.second);
            assert(artwork.Width() == size.first && artwork.Height() == size.second);
            RECT progress = artwork.ProgressRect();
            assert(progress.left >= 0 && progress.top >= 0);
            assert(progress.right <= size.first && progress.bottom <= size.second);
            assert(progress.right > progress.left && progress.bottom > progress.top);
            Canvas canvas(size.first, size.second);
            artwork.Draw(canvas.dc, 0, true);
            auto original = canvas.Copy();
            // Native-size classic fonts can use embedded bitmap strikes at 640x480;
            // larger presentations must contain smooth outline glyphs, not enlarged pixels.
            uint32_t background = original[0] & 0x00FFFFFFu;
            bool antialiased = std::any_of(original.begin(), original.end(), [=](uint32_t pixel) {
                pixel &= 0x00FFFFFFu;
                return pixel != background && pixel != 0x00FFFFFFu;
            });
            if (size.first >= 1440) assert(antialiased);
            for (int percent : {1, 9, 10, 99, 100, 0}) {
                auto before = canvas.Copy();
                artwork.Draw(canvas.dc, percent, false);
                auto partial = canvas.Copy();
                for (int y = 0; y < size.second; ++y) for (int x = 0; x < size.first; ++x) {
                    if (x >= progress.left && x < progress.right && y >= progress.top && y < progress.bottom) continue;
                    size_t i = static_cast<size_t>(y) * size.first + x;
                    assert(partial[i] == before[i]);
                }
                artwork.Draw(canvas.dc, percent, true);
                auto full = canvas.Copy();
                for (size_t i = 0; i < full.size(); ++i) {
                    int x = static_cast<int>(i % size.first), y = static_cast<int>(i / size.first);
                    if (x < progress.left || x >= progress.right || y < progress.top || y >= progress.bottom) continue;
                    if ((full[i] & 0x00FFFFFFu) != (partial[i] & 0x00FFFFFFu)) {
                        std::cerr << "Redraw mismatch at " << i % size.first << ',' << i / size.first
                                  << " progress " << percent << " full " << std::hex << full[i]
                                  << " partial " << partial[i] << std::dec << std::endl;
                        if (argc > 1) canvas.Save(argv[1]);
                        assert(false); // No clipped digits, ghosting or damage to support text.
                    }
                }
            }
        }
    }
    BlueScreenArtwork native(BlueScreenStyle::Automatic);
    int height = std::min(GetSystemMetrics(SM_CYSCREEN), GetSystemMetrics(SM_CXSCREEN) * 3 / 4);
    assert(native.Height() == height && native.Width() == height * 4 / 3);
    if (argc > 1) {
        BlueScreenArtwork artwork(BlueScreenStyle::Windows11, 1440, 1080);
        Canvas canvas(artwork.Width(), artwork.Height());
        artwork.Draw(canvas.dc, 42, true);
        canvas.Save(argv[1]); // Optional BMP snapshot; never opens a window or audio device.
    }
    if (argc > 2) {
        BlueScreenArtwork artwork(BlueScreenStyle::Windows7, 1440, 1080);
        Canvas canvas(artwork.Width(), artwork.Height());
        artwork.Draw(canvas.dc, 42, true);
        canvas.Save(argv[2]);
    }
    std::cout << "Blue-screen artwork tests passed (5 styles, 3 resolutions; no window or audio).\n";
}
