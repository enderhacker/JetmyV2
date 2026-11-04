#pragma once
#include "visual.h"

namespace VisualPayloads {
SIZE PopupDimensions(const wchar_t* message);
void DrawPopup(int x, int y, int width, int height, const wchar_t* title,
    const wchar_t* message, const wchar_t* button, int kind);
} // namespace VisualPayloads
