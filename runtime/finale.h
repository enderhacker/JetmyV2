#pragma once
#include "app.h"
#include "blue_screen.h"
#include <functional>

namespace Runtime {
inline constexpr int FinaleSeconds = 10;
// Returns false when dismissed early or the application is shutting down.
bool RunBlueScreenFinale(HINSTANCE instance, const std::function<bool()>& shouldExit,
    BlueScreenStyle style = BlueScreenStyle::Automatic);
}
