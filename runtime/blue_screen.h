#pragma once
#include "app.h"

namespace Runtime {
enum class BlueScreenStyle { Automatic, XP, Vista, Windows7, Windows10, Windows11 };
BlueScreenStyle DetectBlueScreenStyle();

// Off-screen artwork only; no input handling, sound, or desktop changes.
class BlueScreenArtwork {
public:
    explicit BlueScreenArtwork(BlueScreenStyle requested);
    ~BlueScreenArtwork();
    BlueScreenArtwork(const BlueScreenArtwork&) = delete;
    BlueScreenArtwork& operator=(const BlueScreenArtwork&) = delete;
    int Width() const;
    int Height() const;
    RECT ProgressRect() const;
    void Draw(HDC dc, int percent, bool initial) const;
private:
    bool Modern() const;
    BlueScreenStyle style_;
    HFONT body_ = nullptr, detail_ = nullptr, face_ = nullptr;
};
} // namespace Runtime
