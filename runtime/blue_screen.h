#pragma once
#include "app.h"

namespace Runtime {
enum class BlueScreenStyle { Automatic, XP, Vista, Windows7, Windows10, Windows11 };
BlueScreenStyle DetectBlueScreenStyle();

// Off-screen artwork only; no input handling, sound, or desktop changes.
class BlueScreenArtwork {
public:
    // Zero dimensions choose the primary monitor's native, centered 4:3 area.
    explicit BlueScreenArtwork(BlueScreenStyle requested, int pixelWidth = 0, int pixelHeight = 0);
    ~BlueScreenArtwork();
    BlueScreenArtwork(const BlueScreenArtwork&) = delete;
    BlueScreenArtwork& operator=(const BlueScreenArtwork&) = delete;
    int Width() const;
    int Height() const;
    RECT ProgressRect() const;
    void Draw(HDC dc, int percent, bool initial) const;
private:
    bool Modern() const;
    int LayoutWidth() const;
    int X(int value) const;
    int Y(int value) const;
    BlueScreenStyle style_;
    int width_ = 0, height_ = 0;
    HFONT body_ = nullptr, detail_ = nullptr, face_ = nullptr;
};
} // namespace Runtime
